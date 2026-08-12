#include "States/PlayingState.hpp"
#include "States/PauseState.hpp"
#include "States/GameOverState.hpp"
#include "States/MenuState.hpp"
#include "Core/Game.hpp"
#include "Core/AssetManager.hpp"
#include "Core/LevelCompletion.hpp"
#include "Core/Command.hpp"
#include "Core/SoundManager.hpp"
#include "Level/Level.hpp"
#include "Entities/Player.hpp"
#include "Entities/Fireball.hpp"
#include "States/StateManager.hpp"
#include "Physics/PhysicsConstants.hpp"
#include "Observers/EventManager.hpp"
#include "UI/HUD.hpp"
#include <algorithm>
#include <iostream>
#include <sstream>

namespace {
constexpr float TIME_BONUS_TICK_INTERVAL = 0.03f;
}

PlayingState::PlayingState() : m_hud(std::make_unique<HUD>()) {}
PlayingState::~PlayingState() {}

void PlayingState::onEnter() {
    PlayerProgress& progress = Game::getInstance().getProgress();
    loadLevel(progress.getCurrentLevel());
    m_levelTimer = LEVEL_TIME;
    m_levelComplete = false;

    m_hud->init();
    m_hud->setCharacterName(progress.getSelectedCharacter());
    m_hud->setLevel(progress.getCurrentLevel());
    m_hud->setLives(progress.getLives());
    m_hud->setScore(progress.getScore());
    m_hud->setCoins(progress.getCoins());

    m_completionBreakdownText.setFont(
        AssetManager::getInstance().getFont("assets/fonts/mario_font.ttf"));
    m_completionBreakdownText.setCharacterSize(20);
    m_completionBreakdownText.setFillColor(sf::Color::White);
    m_completionBreakdownText.setOutlineColor(sf::Color::Black);
    m_completionBreakdownText.setOutlineThickness(1.0f);
    m_completionBreakdownText.setPosition(WINDOW_WIDTH / 2.0f - 130.0f, 250.0f);
    m_completionBreakdownText.setString("");

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
    });

    m_enemyDefeatedSub = ScopedEventSubscription(EventType::EnemyDefeated, [this](const GameEvent& e) {
        SoundManager::getInstance().playSound(SoundID::Stomp);
        PlayerProgress& progress = Game::getInstance().getProgress();
        progress.addScore(e.intData);
        m_hud->setScore(progress.getScore());
    });

    m_playerDiedSub = ScopedEventSubscription(EventType::PlayerDied, [this](const GameEvent& e) {
        SoundManager::getInstance().stopMusic();
        SoundManager::getInstance().playSound(SoundID::PlayerDeath);
        onPlayerDeath();
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
    m_playerDiedSub.reset();
    m_powerUpSub.reset();
    m_blockHitSub.reset();
    m_playerDamagedSub.reset();
    m_levelCompletedSub.reset();
    SoundManager::getInstance().stopMusic();
}

void PlayingState::onResume() {
    SoundManager::getInstance().resumeMusic();
}

void PlayingState::handleEvent(const sf::Event& event) {
    if (m_transitionStage != LevelTransitionStage::Inactive) {
        return;
    }

    if (event.type == sf::Event::KeyPressed) {
        if (event.key.code == sf::Keyboard::Escape) {
            SoundManager::getInstance().playSound(SoundID::Pause);
            SoundManager::getInstance().pauseMusic();
            Game::getInstance().getStateManager().pushState(
                std::make_unique<PauseState>());
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
            }
        }

        // Handle one-shot commands (jump, fire)
        if (m_player && !m_player->isDead()) {
            Command* cmd = m_input.handleEvent(event);
            if (cmd) {
                cmd->execute(*m_player, FIXED_DT);
            }
        }
    }
}

void PlayingState::update(float dt) {
    if (!m_level || !m_player) return;
    if (m_levelComplete) return;

    if (m_transitionStage != LevelTransitionStage::Inactive) {
        if (m_transitionStage == LevelTransitionStage::FlagSlide ||
            m_transitionStage == LevelTransitionStage::CastleEntry ||
            m_transitionStage == LevelTransitionStage::FlagpoleScoreCount ||
            m_transitionStage == LevelTransitionStage::TimeBonusCount) {
            m_level->update(dt);
        }
        updateLevelTransition(dt);
        return;
    }

    // Set sprint before processing movement. handleInput() stores bindings in
    // an unordered map, so relying on a SprintCommand's execution order made
    // sprint speed inconsistent when sprint and direction were held together.
    m_player->setSprinting(m_input.isSprintHeld());
    m_player->setJumpHeld(m_input.isJumpHeld());

    // Handle held-key commands
    auto commands = m_input.handleInput();
    for (auto* cmd : commands) {
        cmd->execute(*m_player, dt);
    }

    // Handle fireball spawning
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

    // Update level (entities + collisions)
    m_level->update(dt);

    // In secret rooms, touching the exit pipe auto-returns to the main map.
    if (m_inSecretRoom && m_transitionStage == LevelTransitionStage::Inactive) {
        if (m_level->getTouchedPipeBounds(*m_player)) {
            startPipeTransition(false);
            return;
        }
    }

    // Update camera
    m_camera.update(m_player->getPosition());

    // Timer
    m_levelTimer -= dt;
    m_hud->setTime(m_levelTimer);
    if (m_levelTimer <= 0.0f) {
        m_player->die();
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
        m_level->render(window, m_camera.getView().getCenter().x);
    }

    // Reset view for HUD (screen-space)
    window.setView(window.getDefaultView());
    m_hud->render(window);
    if (m_transitionStage == LevelTransitionStage::FlagpoleScoreCount ||
        m_transitionStage == LevelTransitionStage::TimeBonusCount) {
        window.draw(m_completionBreakdownText);
    }
}

void PlayingState::loadLevel(int levelNumber) {
    m_level = std::make_unique<Level>();
    m_mainLevelNumber = levelNumber;
    m_inSecretRoom = false;

    std::string filename = getLevelPath(levelNumber, false);
    std::string charName = Game::getInstance().getProgress().getSelectedCharacter();

    LevelTheme theme = getLevelTheme(levelNumber, false);

    if (!m_level->loadFromFile(filename, charName, theme)) {
        std::cerr << "[PlayingState] Failed to load level: " << filename << std::endl;
        return;
    }

    m_player = m_level->getPlayer();
    m_camera.setLevelBounds(m_level->getWidth(), m_level->getHeight());
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
    m_transitionDisplayScore = 0;
    m_hud->setTime(static_cast<float>(m_transitionRemainingSeconds));
    updateCompletionBreakdown();

    if (m_player) {
        m_player->setGrounded(false);
        m_player->setVelocity(0.0f, 0.0f);
    }

    SoundManager::getInstance().stopMusic();
    SoundManager::getInstance().playSound(SoundID::LevelComplete);
}

void PlayingState::updateCompletionBreakdown() {
    std::ostringstream text;
    text << "FLAG BONUS: " << m_transitionFlagpoleBonus << "\n"
         << "TIME: " << m_transitionRemainingSeconds << " x "
         << TIME_BONUS_PER_SECOND << "\n"
         << "BONUS: " << m_transitionDisplayScore;
    m_completionBreakdownText.setString(text.str());
}

std::string PlayingState::getLevelPath(int levelNumber, bool secretRoom) const {
    if (secretRoom) {
        return "assets/levels/level" + std::to_string(levelNumber) + "_secret.txt";
    }
    return "assets/levels/level" + std::to_string(levelNumber) + ".txt";
}

LevelTheme PlayingState::getLevelTheme(int levelNumber, bool secretRoom) const {
    if (secretRoom) {
        return LevelTheme::Underground;
    }
    if (levelNumber == 1) return LevelTheme::Overworld;
    if (levelNumber == 2) return LevelTheme::Underground;
    return LevelTheme::Castle;
}

bool PlayingState::tryEnterPipe() {
    if (!m_level || !m_player || !m_player->isGrounded() || m_inSecretRoom)
        return false;

    auto pipeBounds = m_level->getEnterablePipeBounds(*m_player);
    if (!pipeBounds)
        return false;

    m_pipeReturnPosition = m_player->getPosition();
    m_pipeReturnPowerUp = m_player->getPowerUpState();
    startPipeTransition(true);
    return true;
}

bool PlayingState::tryExitPipe() {
    if (!m_level || !m_player || !m_player->isGrounded() || !m_inSecretRoom)
        return false;

    auto pipeBounds = m_level->getEnterablePipeBounds(*m_player);
    if (!pipeBounds)
        return false;

    startPipeTransition(false);
    return true;
}

void PlayingState::startPipeTransition(bool enteringSecret) {
    m_transitionStage = enteringSecret ? LevelTransitionStage::PipeEnter
                                       : LevelTransitionStage::PipeReturn;
    m_transitionTimer = 0.0f;

    if (m_player) {
        m_player->setGrounded(false);
        m_player->setVelocity(0.0f, 0.0f);
    }
}

void PlayingState::updatePipeTransition(float dt) {
    if (!m_level || !m_player) {
        return;
    }

    const float pipeTravelSpeed = 120.0f;
    sf::Vector2f pos = m_player->getPosition();

    if (m_transitionStage == LevelTransitionStage::PipeEnter) {
        pos.y += pipeTravelSpeed * dt;
    } else if (m_transitionStage == LevelTransitionStage::PipeReturn) {
        pos.y -= pipeTravelSpeed * dt;
    }

    m_player->setPosition(pos);
    m_player->setVelocity(0.0f, 0.0f);

    m_transitionTimer += dt;
    if (m_transitionTimer < 0.45f) {
        return;
    }

    const bool enteringSecret = m_transitionStage == LevelTransitionStage::PipeEnter;
    const int levelNumber = m_mainLevelNumber;
    const bool secretRoom = enteringSecret;
    const std::string filename = getLevelPath(levelNumber, secretRoom);
    const std::string charName = Game::getInstance().getProgress().getSelectedCharacter();
    const LevelTheme theme = getLevelTheme(levelNumber, secretRoom);

    m_level = std::make_unique<Level>();
    if (!m_level->loadFromFile(filename, charName, theme, !secretRoom)) {
        std::cerr << "[PlayingState] Failed to load pipe level: " << filename << std::endl;
        m_transitionStage = LevelTransitionStage::Inactive;
        return;
    }

    m_player = m_level->getPlayer();
    m_camera.setLevelBounds(m_level->getWidth(), m_level->getHeight());

    if (m_player) {
        m_player->applyPowerUp(m_pipeReturnPowerUp);
        if (!enteringSecret) {
            m_player->setPosition(m_pipeReturnPosition);
            m_player->setVelocity(0.0f, 0.0f);
            m_player->setGrounded(false);
        }
    }

    m_inSecretRoom = enteringSecret;
    m_transitionStage = LevelTransitionStage::Inactive;
    m_transitionTimer = 0.0f;
}

void PlayingState::updateLevelTransition(float dt) {
    if (!m_level || !m_player) {
        return;
    }

    if (m_transitionStage == LevelTransitionStage::PipeEnter ||
        m_transitionStage == LevelTransitionStage::PipeReturn) {
        updatePipeTransition(dt);
        return;
    }

    switch (m_transitionStage) {
    case LevelTransitionStage::FlagSlide: {
        sf::Vector2f pos = m_player->getPosition();
        pos.y += 220.0f * dt;
        m_player->setPosition(pos);
        m_player->setVelocity(0.0f, 0.0f);

        m_transitionTimer += dt;
        if (m_transitionTimer >= 1.0f) {
            m_transitionStage = LevelTransitionStage::CastleEntry;
            m_transitionTimer = 0.0f;
        }
        break;
    }
    case LevelTransitionStage::CastleEntry: {
        sf::Vector2f pos = m_player->getPosition();
        pos.x += 120.0f * dt;
        pos.y -= 70.0f * dt;
        m_player->setPosition(pos);
        m_player->setVelocity(0.0f, 0.0f);

        m_transitionTimer += dt;
        if (m_transitionTimer >= 0.75f) {
            m_player->setActive(false);
            m_transitionStage = LevelTransitionStage::FlagpoleScoreCount;
            m_transitionTimer = 0.0f;
        }
        break;
    }
    case LevelTransitionStage::FlagpoleScoreCount: {
        m_transitionScoreTimer += dt;
        if (m_transitionScoreTimer >= 0.08f) {
            m_transitionScoreTimer = 0.0f;
            if (m_transitionFlagpoleBonus > m_transitionDisplayScore) {
                const int increment = std::max(100, m_transitionFlagpoleBonus / 10);
                const int add = std::min(
                    increment, m_transitionFlagpoleBonus - m_transitionDisplayScore);
                m_transitionDisplayScore += add;

                m_hud->setScore(m_transitionStartScore + m_transitionDisplayScore);
                updateCompletionBreakdown();
                SoundManager::getInstance().playSound(SoundID::Coin);
            } else {
                m_transitionScoreTimer = 0.0f;
                m_transitionStage = LevelTransitionStage::TimeBonusCount;
            }
        }
        break;
    }
    case LevelTransitionStage::TimeBonusCount: {
        m_transitionScoreTimer += dt;
        if (m_transitionScoreTimer >= TIME_BONUS_TICK_INTERVAL) {
            m_transitionScoreTimer -= TIME_BONUS_TICK_INTERVAL;
            if (LevelCompletion::convertNextSecond(
                    m_transitionRemainingSeconds, m_transitionConvertedTimeScore)) {
                m_transitionDisplayScore =
                    m_transitionFlagpoleBonus + m_transitionConvertedTimeScore;
                m_hud->setTime(static_cast<float>(m_transitionRemainingSeconds));
                m_hud->setScore(m_transitionStartScore + m_transitionDisplayScore);
                updateCompletionBreakdown();
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

void PlayingState::onPlayerDeath() {
    Game& game = Game::getInstance();
    game.getProgress().loseLife();
    m_hud->setLives(game.getProgress().getLives());

    if (game.getProgress().getLives() <= 0) {
        game.getStateManager().changeState(std::make_unique<GameOverState>());
    } else {
        // Restart current level
        game.getStateManager().changeState(std::make_unique<PlayingState>());
    }
}
