#include "States/PlayingState.hpp"
#include "States/PauseState.hpp"
#include "States/GameOverState.hpp"
#include "States/MenuState.hpp"
#include "Core/Game.hpp"
#include "Core/LevelCompletion.hpp"
#include "Core/Command.hpp"
#include "Core/SoundManager.hpp"
#include "Level/Level.hpp"
#include "Entities/Player.hpp"
#include "Entities/Fireball.hpp"
#include "Entities/Flagpole.hpp"
#include "States/StateManager.hpp"
#include "Physics/PhysicsConstants.hpp"
#include "Observers/EventManager.hpp"
#include "UI/HUD.hpp"
#include <algorithm>
#include <iostream>
#include <sstream>

namespace {
constexpr float TIME_BONUS_TICK_INTERVAL = 0.03f;
constexpr int LEVEL1_SECRET_ENTRY_PIPE_COLUMN = 59; // A: enter the secret room
constexpr int LEVEL1_SECRET_RETURN_PIPE_COLUMN = 73; // B: emerge 14 tiles to the right
constexpr float PIPE_VERTICAL_TRAVEL_TIME = 0.45f;
constexpr float PIPE_HORIZONTAL_TRAVEL_TIME = 0.80f;
constexpr float PIPE_FADE_DURATION = 0.12f;
}

PlayingState::PlayingState() : m_hud(std::make_unique<HUD>()) {
    if (Game::getInstance().getProgress().isMultiplayer()) {
        m_inputP1.setPlayer1Bindings();
        m_inputP2.setPlayer2Bindings();
    } else {
        m_inputP1.setSinglePlayerBindings();
    }
}

PlayingState::PlayingState(SaveData::GameSnapshot snapshot)
    : m_hud(std::make_unique<HUD>()), m_pendingSnapshot(std::move(snapshot)) {
    // Snapshot slots are single-player only, even if a previous session left
    // PlayerProgress in a multiplayer mode before the user opened Load Game.
    m_inputP1.setSinglePlayerBindings();
}

PlayingState::~PlayingState() {}

void PlayingState::onEnter() {
    PlayerProgress& progress = Game::getInstance().getProgress();
    if (m_pendingSnapshot) {
        if (!restoreSnapshot(*m_pendingSnapshot)) {
            std::cerr << "[PlayingState] Failed to restore save snapshot\n";
            Game::getInstance().getStateManager().changeState(
                std::make_unique<MenuState>());
            return;
        }
        m_pendingSnapshot.reset();
    } else {
        loadLevel(progress.getCurrentLevel());
        m_levelTimer = LEVEL_TIME;
        m_levelComplete = false;
    }

    m_hud->init(progress.getGameMode());
    m_scorePopups.init();
    m_hud->setCharacterName(progress.getSelectedCharacter());
    if (progress.isMultiplayer()) {
        m_hud->setPlayer2Name(progress.getSelectedCharacter() == "Mario" ? "Luigi" : "Mario");
    }
    m_hud->setLevel(progress.getCurrentLevel());
    m_hud->setLives(progress.getLives());
    m_hud->setScore(progress.getScore());
    m_hud->setCoins(progress.getCoins());
    m_hud->setTime(m_levelTimer);

    // Start/continue selected background music track (does not restart if already playing)
    SoundManager& snd = SoundManager::getInstance();
    snd.selectTrack(snd.getCurrentTrackIndex());

    // Subscribe to events for HUD updates and sound effects
    m_coinSub = ScopedEventSubscription(EventType::CoinCollected, [this](const GameEvent& e) {
        SoundManager::getInstance().playSound(SoundID::Coin);
        PlayerProgress& progress = Game::getInstance().getProgress();
        progress.addCoin();
        m_hud->setCoins(progress.getCoins());
        m_hud->setScore(progress.getScore());
        m_scorePopups.add(e.intData, e.worldPosition);
    });

    m_enemyDefeatedSub = ScopedEventSubscription(EventType::EnemyDefeated, [this](const GameEvent& e) {
        SoundManager::getInstance().playSound(SoundID::Stomp);
        PlayerProgress& progress = Game::getInstance().getProgress();
        progress.addScore(e.intData);
        m_hud->setScore(progress.getScore());
        m_scorePopups.add(e.intData, e.worldPosition);
    });

    m_enemyFireballHitSub = ScopedEventSubscription(EventType::EnemyHitByFireball, [](const GameEvent&) {
        SoundManager::getInstance().playSound(SoundID::Fireball);
    });

    m_playerDiedSub = ScopedEventSubscription(EventType::PlayerDied, [this](const GameEvent& e) {
        SoundManager::getInstance().stopMusic();
        SoundManager::getInstance().playSound(SoundID::PlayerDeath);
        // In PvP, only trigger the death transition once (first player to die wins the sequence)
        if (Game::getInstance().getProgress().isPvP()) {
            // Mark who died; finishPlayerDeath() reads isDead() directly
            beginPlayerDeath();
        } else {
            beginPlayerDeath();
        }
    });

    m_powerUpSub = ScopedEventSubscription(EventType::PowerUpCollected, [this](const GameEvent& e) {
        SoundManager::getInstance().playSound(SoundID::PowerUp);
        PlayerProgress& progress = Game::getInstance().getProgress();
        progress.addScore(e.intData);
        m_hud->setScore(progress.getScore());
    });

    m_blockHitSub = ScopedEventSubscription(EventType::BlockHit, [this](const GameEvent& e) {
        SoundManager::getInstance().playSound(SoundID::BlockBump);
    });

    m_playerDamagedSub = ScopedEventSubscription(EventType::PlayerDamaged, [this](const GameEvent& e) {
        SoundManager::getInstance().playSound(SoundID::BlockBump);
    });

    m_levelCompletedSub = ScopedEventSubscription(EventType::LevelCompleted, [this](const GameEvent& e) {
        m_transitionFlagpoleBonus = e.intData;
    });
}

void PlayingState::onExit() {
    m_coinSub.reset();
    m_enemyDefeatedSub.reset();
    m_enemyFireballHitSub.reset();
    m_playerDiedSub.reset();
    m_powerUpSub.reset();
    m_blockHitSub.reset();
    m_playerDamagedSub.reset();
    m_levelCompletedSub.reset();
    m_scorePopups.clear();
    SoundManager::getInstance().stopMusic();
}

void PlayingState::onPause() {
    // PauseState is being pushed on top of us — matches the sound handling
    // onResume() undoes below.
    SoundManager::getInstance().pauseMusic();
}

void PlayingState::onResume() {
    SoundManager::getInstance().resumeMusic();
    // While paused the state receives no events, so re-sync the held-key
    // tracking with the physical keyboard state on the way back in.
    m_inputP1.clearHeldKeys();
    m_inputP1.seedHeldKeys();
    m_inputP2.clearHeldKeys();
    m_inputP2.seedHeldKeys();
}

void PlayingState::handleEvent(const sf::Event& event) {
    // Keep the held-key tracking in sync regardless of player state so a
    // released key never leaves the character moving on its own.
    if (event.type == sf::Event::KeyReleased ||
        event.type == sf::Event::LostFocus) {
        m_inputP1.handleEvent(event);
        m_inputP2.handleEvent(event);
        return;
    }

    if (m_transitionStage != LevelTransitionStage::Inactive) {
        return;
    }

    if (event.type == sf::Event::KeyPressed) {
        if (event.key.code == sf::Keyboard::Escape) {
            SoundManager::getInstance().playSound(SoundID::Pause);
            // pauseMusic() happens in onPause(), called by StateManager as
            // part of pushing PauseState on top of us.
            Game::getInstance().getStateManager().pushState(
                std::make_unique<PauseState>(captureSnapshot()));
            return;
        }

        if (m_level && m_player && !m_player->isDead()) {
            if (event.key.code == sf::Keyboard::Down) {
                if (m_inSecretRoom) {
                    if (tryExitPipe()) {
                        return;
                    }
                } else {
                    if (tryEnterPipe()) {
                        return;
                    }
                }
            } else if (event.key.code == sf::Keyboard::Right &&
                       m_inSecretRoom && m_mainLevelNumber == 1) {
                // The bonus-room B pipe is horizontal, so it is entered
                // from the left rather than with the Down key.
                if (tryExitPipe()) {
                    return;
                }
            }
        }

        // Handle one-shot commands (jump, fire) for P1
        if (m_player && !m_player->isDead()) {
            Command* cmd = m_inputP1.handleEvent(event);
            if (cmd) {
                cmd->execute(*m_player, FIXED_DT);
            }
        }
        
        // Handle one-shot commands for P2
        if (m_player2 && !m_player2->isDead()) {
            Command* cmd2 = m_inputP2.handleEvent(event);
            if (cmd2) {
                cmd2->execute(*m_player2, FIXED_DT);
            }
        }
    }
}

void PlayingState::update(float dt) {
    m_scorePopups.update(dt);

    if (!m_level || !m_player) return;
    if (m_levelComplete) return;

    if (m_transitionStage != LevelTransitionStage::Inactive) {
        if (m_transitionStage == LevelTransitionStage::DeathAnimation) {
            // The player owns the arc and pause timing. Keep the level alive
            // until that animation reports completion.
            m_level->update(dt);
            if (m_player2 && !m_player2->isDead()) {
                m_camera.update(m_player->getPosition(), m_player2->getPosition());
            } else if (!m_player->isDead()) {
                m_camera.update(m_player->getPosition());
            }
            // In PvP: wait for the dying player's animation to complete
            bool p1AnimDone = m_player->isDead() && m_player->isDeathAnimationComplete();
            bool p2AnimDone = m_player2 && m_player2->isDead() && m_player2->isDeathAnimationComplete();
            if (p1AnimDone || p2AnimDone) {
                finishPlayerDeath();
            }
            return;
        }

        if (m_transitionStage == LevelTransitionStage::FlagSlide ||
            m_transitionStage == LevelTransitionStage::CastleEntry ||
            m_transitionStage == LevelTransitionStage::TimeBonusCount) {
            m_level->updateCompletion(dt);
        }
        updateLevelTransition(dt);
        return;
    }

    // Handle P1 inputs
    if (!m_player->isDead()) {
        m_player->setSprinting(m_inputP1.isSprintHeld());
        m_player->setJumpHeld(m_inputP1.isJumpHeld());
        m_player->setVineHorizontalInput(m_inputP1.isHorizontalHeld());
        auto commands1 = m_inputP1.handleInput();
        for (auto* cmd : commands1) {
            cmd->execute(*m_player, dt);
        }
        if (m_player->wantsToShoot()) {
            m_player->clearShootFlag();
            int dir = m_player->isFacingRight() ? 1 : -1;
            auto fb = std::make_unique<Fireball>(
                m_player->getPosition().x + (dir > 0 ? 20.0f : -20.0f),
                m_player->getPosition().y + 8.0f,
                dir
            );
            m_level->addFireball(std::move(fb));
        }
    }

    // Handle P2 inputs
    if (m_player2 && !m_player2->isDead()) {
        m_player2->setSprinting(m_inputP2.isSprintHeld());
        m_player2->setJumpHeld(m_inputP2.isJumpHeld());
        m_player2->setVineHorizontalInput(m_inputP2.isHorizontalHeld());
        auto commands2 = m_inputP2.handleInput();
        for (auto* cmd : commands2) {
            cmd->execute(*m_player2, dt);
        }
        if (m_player2->wantsToShoot()) {
            m_player2->clearShootFlag();
            int dir = m_player2->isFacingRight() ? 1 : -1;
            auto fb = std::make_unique<Fireball>(
                m_player2->getPosition().x + (dir > 0 ? 20.0f : -20.0f),
                m_player2->getPosition().y + 8.0f,
                dir
            );
            m_level->addFireball(std::move(fb));
        }
    }

    // Update level (entities + collisions)
    m_level->update(dt);

    if (m_transitionStage == LevelTransitionStage::DeathAnimation) {
        if (m_player2 && !m_player2->isDead()) {
            m_camera.update(m_player->getPosition(), m_player2->getPosition());
        } else if (!m_player->isDead()) {
            m_camera.update(m_player->getPosition());
        }
        bool p1AnimDone = m_player->isDead() && m_player->isDeathAnimationComplete();
        bool p2AnimDone = m_player2 && m_player2->isDead() && m_player2->isDeathAnimationComplete();
        if (p1AnimDone || p2AnimDone) {
            finishPlayerDeath();
        }
        return;
    }

    // Levels 2 and 3 retain their legacy touch-to-exit pipes. Level 1 uses
    // the B horizontal pipe, which must be entered intentionally from its
    // left edge with Right.
    if (m_inSecretRoom && m_mainLevelNumber != 1 &&
        m_transitionStage == LevelTransitionStage::Inactive) {
        if (m_level->getTouchedPipeBounds(*m_player)) {
            startPipeTransition(false);
            return;
        }
    }

    // Update camera
    if (m_player2 && !m_player2->isDead() && !m_player->isDead()) {
        m_camera.update(m_player->getPosition(), m_player2->getPosition());
    } else {
        m_camera.update(m_player->getPosition());
    }

    // Timer
    m_levelTimer -= dt;
    m_hud->setTime(m_levelTimer);
    if (!m_player->isDead() && m_levelTimer <= 0.0f) {
        m_player->die();
    }

    if (m_transitionStage == LevelTransitionStage::DeathAnimation) {
        if (m_player2 && !m_player->isDead()) {
            m_camera.update(m_player->getPosition(), m_player2->getPosition());
        } else {
            m_camera.update(m_player->getPosition());
        }
        return;
    }

    // Check level completion
    checkLevelComplete();

    // Update HUD
    PlayerProgress& progress = Game::getInstance().getProgress();
    m_hud->setScore(progress.getScore());
    m_hud->setLives(progress.getLives());
    m_hud->update(dt);
}

void PlayingState::render(sf::RenderWindow& window) {
    // Apply camera for world rendering
    m_camera.applyTo(window);

    if (m_level) {
        const bool pipeTransition =
            m_transitionStage == LevelTransitionStage::PipeEnter ||
            m_transitionStage == LevelTransitionStage::PipeReturn ||
            m_transitionStage == LevelTransitionStage::PipeExit ||
            m_transitionStage == LevelTransitionStage::PipeFadeOut ||
            m_transitionStage == LevelTransitionStage::PipeFadeIn;
        m_level->render(window, m_camera.getView().getCenter().x, pipeTransition);
        m_scorePopups.render(window);
    }

    // Reset view for HUD (screen-space)
    window.setView(Game::getInstance().getUiView());
    m_hud->render(window);

    // Hide the costly map replacement and camera relocation behind a short
    // fade. It keeps the secret-room round trip continuous instead of showing
    // a one-frame jump to the newly loaded map.
    float fade = 0.0f;
    if (m_transitionStage == LevelTransitionStage::PipeFadeOut) {
        fade = std::min(m_transitionTimer / PIPE_FADE_DURATION, 1.0f);
    } else if (m_transitionStage == LevelTransitionStage::PipeFadeIn) {
        fade = 1.0f - std::min(m_transitionTimer / PIPE_FADE_DURATION, 1.0f);
    }
    if (fade > 0.0f) {
        sf::RectangleShape overlay(
            sf::Vector2f(static_cast<float>(WINDOW_WIDTH),
                         static_cast<float>(WINDOW_HEIGHT)));
        overlay.setFillColor(sf::Color(0, 0, 0,
            static_cast<sf::Uint8>(fade * 255.0f)));
        window.draw(overlay);
    }
}

void PlayingState::loadLevel(int levelNumber) {
    m_scorePopups.clear();
    m_level = std::make_unique<Level>();
    m_mainLevelNumber = levelNumber;
    m_inSecretRoom = false;

    PlayerProgress& progress = Game::getInstance().getProgress();
    std::string charName = progress.getSelectedCharacter();

    std::string filename;
    LevelTheme theme;

    if (progress.isPvP()) {
        // PvP always loads the dedicated arena — no level progression
        filename = "assets/levels/pvp_arena.txt";
        theme    = LevelTheme::Overworld;
    } else {
        filename = getLevelPath(levelNumber, false);
        theme    = getLevelTheme(levelNumber, false);
    }

    if (!m_level->loadFromFile(filename, charName, theme, /*autoPlaceFlagpole=*/!progress.isPvP())) {
        std::cerr << "[PlayingState] Failed to load level: " << filename << std::endl;
        return;
    }

    m_player = m_level->getPlayer();
    m_player2 = m_level->getPlayer2();
    m_camera.setLevelBounds(m_level->getWidth(), m_level->getHeight());
}

std::optional<SaveData::GameSnapshot> PlayingState::captureSnapshot() const {
    const PlayerProgress& progress = Game::getInstance().getProgress();
    if (progress.getGameMode() != GameMode::SinglePlayer || !m_level || !m_player ||
        m_player2 || m_levelComplete ||
        m_transitionStage != LevelTransitionStage::Inactive || m_player->isDead()) {
        return std::nullopt;
    }

    SaveData::GameSnapshot snapshot;
    snapshot.gameMode = static_cast<int>(GameMode::SinglePlayer);
    snapshot.progress.level = progress.getCurrentLevel();
    snapshot.progress.score = progress.getScore();
    snapshot.progress.lives = progress.getLives();
    snapshot.progress.coins = progress.getCoins();
    snapshot.progress.character = progress.getSelectedCharacter();
    snapshot.levelTimer = m_levelTimer;
    snapshot.mainLevelNumber = m_mainLevelNumber;
    snapshot.inSecretRoom = m_inSecretRoom;
    snapshot.pipeReturnPosition = {m_pipeReturnPosition.x, m_pipeReturnPosition.y};
    snapshot.pipeReturnPowerUp = static_cast<int>(m_pipeReturnPowerUp);
    snapshot.level = m_level->captureSnapshot();
    return snapshot;
}

bool PlayingState::restoreSnapshot(const SaveData::GameSnapshot& snapshot) {
    if (snapshot.gameMode != static_cast<int>(GameMode::SinglePlayer) ||
        snapshot.mainLevelNumber < 1 || snapshot.mainLevelNumber > TOTAL_LEVELS ||
        snapshot.progress.level < 1 || snapshot.progress.level > TOTAL_LEVELS) {
        return false;
    }

    PlayerProgress& progress = Game::getInstance().getProgress();
    progress.setGameMode(GameMode::SinglePlayer);
    progress.setCurrentLevel(snapshot.progress.level);
    progress.setScore(snapshot.progress.score);
    progress.setLives(snapshot.progress.lives);
    progress.setCoins(snapshot.progress.coins);
    progress.setSelectedCharacter(snapshot.progress.character);

    m_scorePopups.clear();
    m_mainLevelNumber = snapshot.mainLevelNumber;
    m_inSecretRoom = snapshot.inSecretRoom;
    m_pipeReturnPosition = {snapshot.pipeReturnPosition.x, snapshot.pipeReturnPosition.y};
    m_pipeReturnPowerUp = static_cast<PowerUpState>(snapshot.pipeReturnPowerUp);
    m_pipeReturnPowerUp2 = PowerUpState::Small;
    m_pipeTransitionEnteringSecret = false;
    m_transitionStage = LevelTransitionStage::Inactive;
    m_transitionTimer = 0.0f;
    m_transitionScoreTimer = 0.0f;
    m_transitionStartScore = 0;
    m_transitionFlagpoleBonus = 0;
    m_transitionTimeBonus = 0;
    m_transitionRemainingSeconds = 0;
    m_transitionConvertedTimeScore = 0;
    m_transitionConvertedFlagpoleScore = 0;
    m_transitionDisplayScore = 0;
    m_levelComplete = false;

    const std::string filename = getLevelPath(m_mainLevelNumber, m_inSecretRoom);
    const LevelTheme theme = getLevelTheme(m_mainLevelNumber, m_inSecretRoom);
    m_level = std::make_unique<Level>();
    if (!m_level->loadFromFile(filename, progress.getSelectedCharacter(), theme,
                               !m_inSecretRoom) ||
        !m_level->restoreSnapshot(snapshot.level)) {
        m_level.reset();
        m_player = nullptr;
        m_player2 = nullptr;
        return false;
    }

    m_player = m_level->getPlayer();
    m_player2 = m_level->getPlayer2();
    if (!m_player || m_player2) return false;
    m_camera.setLevelBounds(m_level->getWidth(), m_level->getHeight());
    m_camera.update(m_player->getPosition());
    m_levelTimer = snapshot.levelTimer;
    return true;
}

void PlayingState::checkLevelComplete() {
    if (m_level && m_level->isComplete() &&
        m_transitionStage == LevelTransitionStage::Inactive) {
        startLevelTransition();
    }
}

void PlayingState::startLevelTransition() {
    m_transitionStage = LevelTransitionStage::FlagSlide;
    m_transitionTimer = 0.0f;
    m_transitionScoreTimer = 0.0f;
    m_transitionStartScore = Game::getInstance().getProgress().getScore();
    m_transitionRemainingSeconds = LevelCompletion::displayedSeconds(m_levelTimer);
    m_transitionTimeBonus =
        LevelCompletion::timeBonusForSeconds(m_transitionRemainingSeconds);
    m_transitionConvertedTimeScore = 0;
    m_transitionConvertedFlagpoleScore = 0;
    m_transitionDisplayScore = 0;
    m_hud->setTime(static_cast<float>(m_transitionRemainingSeconds));

    if (m_player) {
        if (Flagpole* flagpole = m_level ? m_level->getFlagpole() : nullptr) {
            m_player->beginFlagpoleSlide(flagpole->getSlideAnchorX(),
                                         flagpole->getSlideEndY());
        } else {
            // isComplete() is normally driven by a flagpole, but malformed
            // custom levels must still be able to finish instead of waiting
            // forever for a slide target that does not exist.
            beginCastleEntry();
        }
    }

    SoundManager::getInstance().stopMusic();
    SoundManager::getInstance().playSound(SoundID::LevelComplete);
}

std::string PlayingState::getLevelPath(int levelNumber, bool secretRoom) const {
    if (secretRoom) {
        return "assets/levels/level" + std::to_string(levelNumber) + "_secret.txt";
    }
    return "assets/levels/level" + std::to_string(levelNumber) + ".txt";
}

LevelTheme PlayingState::getLevelTheme(int levelNumber, bool secretRoom) const {
    // Level 1's secret room is entered from the overworld map, so keep its
    // terrain palette consistent with level1 (green pipes and overworld X
    // ground) instead of switching those tiles to the underground palette.
    if (levelNumber == 1) return LevelTheme::Overworld;
    if (secretRoom) {
        return LevelTheme::Underground;
    }
    if (levelNumber == 2) return LevelTheme::Underground;
    return LevelTheme::Castle;
}

bool PlayingState::tryEnterPipe() {
    if (!m_level || !m_player || !m_player->isGrounded() || m_inSecretRoom)
        return false;

    auto pipeBounds = m_level->getEnterablePipeBounds(*m_player);
    if (!pipeBounds) {
        if (m_player2 && m_player2->isGrounded()) {
            pipeBounds = m_level->getEnterablePipeBounds(*m_player2);
        }
    }
    if (!pipeBounds)
        return false;

    // World 1-1 has one designated bonus-pipe entrance (A). Other decorative
    // pipes remain ordinary scenery instead of accidentally opening the room.
    if (m_mainLevelNumber == 1 &&
        static_cast<int>(pipeBounds->left / TILE_SIZE) !=
            LEVEL1_SECRET_ENTRY_PIPE_COLUMN) {
        return false;
    }

    m_pipeReturnPosition = m_player->getPosition(); // We'll just spawn both here on return
    m_pipeReturnPowerUp = m_player->getPowerUpState();
    if (m_player2) m_pipeReturnPowerUp2 = m_player2->getPowerUpState();
    startPipeTransition(true);
    return true;
}

bool PlayingState::tryExitPipe() {
    if (!m_level || !m_inSecretRoom)
        return false;

    if (m_mainLevelNumber == 1) {
        if (!m_player || !m_level->getHorizontalPipeEntranceBounds(*m_player)) {
            return false;
        }
        startPipeTransition(false);
        return true;
    }

    bool canExit = (m_player && m_player->isGrounded() && m_level->getEnterablePipeBounds(*m_player));
    if (!canExit && m_player2 && m_player2->isGrounded()) {
        canExit = m_level->getEnterablePipeBounds(*m_player2).has_value();
    }
    
    if (!canExit) return false;

    startPipeTransition(false);
    return true;
}

void PlayingState::startPipeTransition(bool enteringSecret) {
    m_transitionStage = enteringSecret ? LevelTransitionStage::PipeEnter
                                       : LevelTransitionStage::PipeReturn;
    m_transitionTimer = 0.0f;
    m_pipeTransitionEnteringSecret = enteringSecret;

    if (m_player) {
        m_player->setGrounded(false);
        m_player->setVelocity(0.0f, 0.0f);
    }
    if (m_player2) {
        m_player2->setGrounded(false);
        m_player2->setVelocity(0.0f, 0.0f);
    }
}

void PlayingState::updatePipeTransition(float dt) {
    if (!m_level || !m_player) {
        return;
    }

    if (m_transitionStage == LevelTransitionStage::PipeFadeOut) {
        m_transitionTimer += dt;
        if (m_transitionTimer >= PIPE_FADE_DURATION) {
            swapPipeMap();
        }
        return;
    }

    if (m_transitionStage == LevelTransitionStage::PipeFadeIn) {
        m_transitionTimer += dt;
        if (m_transitionTimer >= PIPE_FADE_DURATION) {
            m_transitionStage = !m_pipeTransitionEnteringSecret &&
                                        m_mainLevelNumber == 1
                                    ? LevelTransitionStage::PipeExit
                                    : LevelTransitionStage::Inactive;
            m_transitionTimer = 0.0f;
        }
        return;
    }

    const float pipeTravelSpeed = 120.0f;
    sf::Vector2f pos = m_player->getPosition();
    sf::Vector2f pos2 = m_player2 ? m_player2->getPosition() : sf::Vector2f();

    if (m_transitionStage == LevelTransitionStage::PipeEnter) {
        pos.y += pipeTravelSpeed * dt;
        pos2.y += pipeTravelSpeed * dt;
    } else if (m_transitionStage == LevelTransitionStage::PipeReturn) {
        if (m_mainLevelNumber == 1 && m_inSecretRoom) {
            pos.x += pipeTravelSpeed * dt;
            pos2.x += pipeTravelSpeed * dt;
        } else {
            pos.y -= pipeTravelSpeed * dt;
            pos2.y -= pipeTravelSpeed * dt;
        }
    } else if (m_transitionStage == LevelTransitionStage::PipeExit) {
        pos.y -= pipeTravelSpeed * dt;
        pos2.y -= pipeTravelSpeed * dt;
    }

    m_player->setPosition(pos);
    m_player->setVelocity(0.0f, 0.0f);
    if (m_player2) {
        m_player2->setPosition(pos2);
        m_player2->setVelocity(0.0f, 0.0f);
    }
    if (m_player2) {
        m_camera.update(m_player->getPosition(), m_player2->getPosition());
    } else {
        m_camera.update(m_player->getPosition());
    }

    m_transitionTimer += dt;
    const float travelTime =
        ((m_transitionStage == LevelTransitionStage::PipeReturn &&
          m_mainLevelNumber == 1 && m_inSecretRoom) ||
         m_transitionStage == LevelTransitionStage::PipeExit)
            ? PIPE_HORIZONTAL_TRAVEL_TIME
            : PIPE_VERTICAL_TRAVEL_TIME;
    if (m_transitionTimer < travelTime) {
        return;
    }

    if (m_transitionStage == LevelTransitionStage::PipeExit) {
        m_transitionStage = LevelTransitionStage::Inactive;
        m_transitionTimer = 0.0f;
        return;
    }

    m_transitionStage = LevelTransitionStage::PipeFadeOut;
    m_transitionTimer = 0.0f;
}

void PlayingState::beginCastleEntry() {
    if (!m_player) {
        return;
    }

    m_player->beginFlagpoleCastleWalk();
    const auto door = m_level ? m_level->getCastleDoorEntryPosition()
                               : std::nullopt;
    // Custom maps without a `4`/`5` door keep the original short walk as a
    // safe fallback. Official level 1 resolves to the real castle doorway.
    m_castleDoorTargetX = door ? door->x : m_player->getPosition().x + 90.0f;
    if (door) {
        m_player->setPosition(m_player->getPosition().x, door->y);
    }
    m_transitionStage = LevelTransitionStage::CastleEntry;
    m_transitionTimer = 0.0f;
}

void PlayingState::swapPipeMap() {
    const bool enteringSecret = m_pipeTransitionEnteringSecret;
    const int levelNumber = m_mainLevelNumber;
    const bool secretRoom = enteringSecret;
    const std::string filename = getLevelPath(levelNumber, secretRoom);
    const std::string charName = Game::getInstance().getProgress().getSelectedCharacter();
    const LevelTheme theme = getLevelTheme(levelNumber, secretRoom);

    m_scorePopups.clear();
    m_level = std::make_unique<Level>();
    if (!m_level->loadFromFile(filename, charName, theme, !secretRoom)) {
        std::cerr << "[PlayingState] Failed to load pipe level: " << filename << std::endl;
        m_transitionStage = LevelTransitionStage::Inactive;
        return;
    }

    m_player = m_level->getPlayer();
    m_player2 = m_level->getPlayer2();
    m_camera.setLevelBounds(m_level->getWidth(), m_level->getHeight());

    if (m_player) {
        m_player->applyPowerUp(m_pipeReturnPowerUp);
        if (!enteringSecret) {
            if (m_mainLevelNumber == 1) {
                const auto exitPipe =
                    m_level->getPipeBoundsAtColumn(LEVEL1_SECRET_RETURN_PIPE_COLUMN);
                if (!exitPipe) {
                    std::cerr << "[PlayingState] Missing level 1 B return pipe\n";
                    m_player->setPosition(m_pipeReturnPosition);
                } else {
                    // Start inside B, then visibly rise out of the vertical
                    // pipe before returning player control.
                    m_player->setPosition(exitPipe->left + TILE_SIZE * 0.5f,
                                          exitPipe->top + TILE_SIZE);
                }
            } else {
                m_player->setPosition(m_pipeReturnPosition);
            }
            m_player->setVelocity(0.0f, 0.0f);
            m_player->setGrounded(false);
        }
    }
    if (m_player2) {
        m_player2->applyPowerUp(m_pipeReturnPowerUp2);
        if (!enteringSecret) {
            // Spawn P2 slightly off to the side so they don't exactly overlap
            if (m_mainLevelNumber == 1) {
                const auto exitPipe =
                    m_level->getPipeBoundsAtColumn(LEVEL1_SECRET_RETURN_PIPE_COLUMN);
                m_player2->setPosition(
                    exitPipe ? sf::Vector2f(exitPipe->left + TILE_SIZE * 1.5f,
                                             exitPipe->top + TILE_SIZE)
                             : m_pipeReturnPosition + sf::Vector2f(32.0f, 0.0f));
            } else {
                m_player2->setPosition(m_pipeReturnPosition + sf::Vector2f(32.0f, 0.0f));
            }
            m_player2->setVelocity(0.0f, 0.0f);
            m_player2->setGrounded(false);
        }
    }

    m_inSecretRoom = enteringSecret;
    if (m_player2) {
        m_camera.update(m_player->getPosition(), m_player2->getPosition());
    } else if (m_player) {
        m_camera.update(m_player->getPosition());
    }
    m_transitionStage = LevelTransitionStage::PipeFadeIn;
    m_transitionTimer = 0.0f;
}

void PlayingState::updateLevelTransition(float dt) {
    if (!m_level || !m_player) {
        return;
    }

    if (m_transitionStage == LevelTransitionStage::PipeEnter ||
        m_transitionStage == LevelTransitionStage::PipeReturn ||
        m_transitionStage == LevelTransitionStage::PipeExit ||
        m_transitionStage == LevelTransitionStage::PipeFadeOut ||
        m_transitionStage == LevelTransitionStage::PipeFadeIn) {
        updatePipeTransition(dt);
        return;
    }

    switch (m_transitionStage) {
    case LevelTransitionStage::DeathAnimation:
        // DeathAnimation is updated directly in update() so the player keeps
        // advancing its own rise/fall/pause timeline.
        break;
    case LevelTransitionStage::FlagSlide: {
        const Flagpole* flagpole = m_level->getFlagpole();
        // Mario waits at the pole base while the flag finishes dropping.
        // Player::SlideComplete already holds the character still for us.
        if (m_player->isFlagpoleSlideComplete() &&
            (!flagpole || flagpole->isFlagDropComplete())) {
            beginCastleEntry();
        }
        break;
    }
    case LevelTransitionStage::CastleEntry: {
        sf::Vector2f pos = m_player->getPosition();
        const float remainingX = m_castleDoorTargetX - pos.x;
        const float step = 120.0f * dt;
        if (remainingX <= step) {
            pos.x = m_castleDoorTargetX;
        } else {
            pos.x += step;
        }
        m_player->setPosition(pos);
        m_player->setVelocity(0.0f, 0.0f);

        if (pos.x >= m_castleDoorTargetX) {
            m_player->setActive(false);
            m_transitionStage = LevelTransitionStage::TimeBonusCount;
            m_transitionTimer = 0.0f;
        }
        break;
    }
    case LevelTransitionStage::TimeBonusCount: {
        m_transitionScoreTimer += dt;
        if (m_transitionScoreTimer >= TIME_BONUS_TICK_INTERVAL) {
            m_transitionScoreTimer -= TIME_BONUS_TICK_INTERVAL;
            const int flagpoleIncrement =
                LevelCompletion::flagpoleBonusForNextTick(
                    m_transitionFlagpoleBonus, m_transitionConvertedFlagpoleScore,
                    m_transitionRemainingSeconds);
            if (LevelCompletion::convertNextSecond(
                    m_transitionRemainingSeconds, m_transitionConvertedTimeScore)) {
                m_transitionConvertedFlagpoleScore += flagpoleIncrement;
                m_transitionDisplayScore =
                    m_transitionConvertedFlagpoleScore + m_transitionConvertedTimeScore;
                m_hud->setTime(static_cast<float>(m_transitionRemainingSeconds));
                m_hud->setScore(m_transitionStartScore + m_transitionDisplayScore);
                SoundManager::getInstance().playSound(SoundID::Coin);
            } else {
                PlayerProgress& progress = Game::getInstance().getProgress();
                progress.addScore(m_transitionFlagpoleBonus + m_transitionTimeBonus);
                m_hud->setScore(progress.getScore());
                m_transitionStage = LevelTransitionStage::Finished;
            }
        }
        break;
    }
    case LevelTransitionStage::Finished:
        finishLevelTransition();
        break;
    case LevelTransitionStage::Inactive:
        break;
    }
}

void PlayingState::finishLevelTransition() {
    Game& game = Game::getInstance();

    if (game.getProgress().advanceToNextLevel(TOTAL_LEVELS)) {
        game.getStateManager().changeState(std::make_unique<PlayingState>());
    } else {
        game.getStateManager().changeState(
            std::make_unique<GameOverState>(GameResult::Won));
    }

    m_levelComplete = true;
}

void PlayingState::beginPlayerDeath() {
    if (m_transitionStage != LevelTransitionStage::Inactive || !m_player) {
        return;
    }

    m_transitionStage = LevelTransitionStage::DeathAnimation;
}

void PlayingState::finishPlayerDeath() {
    Game& game = Game::getInstance();

    // PvP: whoever is still alive wins — no lives system
    if (game.getProgress().isPvP()) {
        bool p1Dead = !m_player || m_player->isDead();
        bool p2Dead = !m_player2 || m_player2->isDead();

        if (p1Dead && !p2Dead) {
            // P2 wins
            std::string winner = (game.getProgress().getSelectedCharacter() == "Mario") ? "LUIGI" : "MARIO";
            game.getStateManager().changeState(
                std::make_unique<GameOverState>(GameResult::P2Won, winner));
        } else if (p2Dead && !p1Dead) {
            // P1 wins
            std::string winner = game.getProgress().getSelectedCharacter();
            std::transform(winner.begin(), winner.end(), winner.begin(), ::toupper);
            game.getStateManager().changeState(
                std::make_unique<GameOverState>(GameResult::P1Won, winner));
        } else {
            // Both dead simultaneously = draw
            game.getStateManager().changeState(
                std::make_unique<GameOverState>(GameResult::Lost));
        }
        return;
    }

    // Normal / Co-op: classic lives system
    game.getProgress().loseLife();
    m_hud->setLives(game.getProgress().getLives());

    if (game.getProgress().getLives() <= 0) {
        game.getStateManager().changeState(std::make_unique<GameOverState>());
    } else {
        // Restart current level
        game.getStateManager().changeState(std::make_unique<PlayingState>());
    }
}
