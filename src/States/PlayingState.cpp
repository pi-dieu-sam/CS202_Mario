#include "States/PlayingState.hpp"
#include "States/PauseState.hpp"
#include "States/GameOverState.hpp"
#include "States/MenuState.hpp"
#include "Core/Game.hpp"
#include "Core/Command.hpp"
#include "Core/SoundManager.hpp"
#include "Level/Level.hpp"
#include "Entities/Player.hpp"
#include "Entities/Fireball.hpp"
#include "States/StateManager.hpp"
#include "Physics/PhysicsConstants.hpp"
#include "Observers/EventManager.hpp"
#include "UI/HUD.hpp"
#include <iostream>
#include <sstream>

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
}

void PlayingState::onExit() {
    m_coinSub.reset();
    m_enemyDefeatedSub.reset();
    m_playerDiedSub.reset();
    m_powerUpSub.reset();
    SoundManager::getInstance().stopMusic();
}

void PlayingState::onResume() {
    SoundManager::getInstance().resumeMusic();
}

void PlayingState::handleEvent(const sf::Event& event) {
    if (event.type == sf::Event::KeyPressed) {
        if (event.key.code == sf::Keyboard::Escape) {
            SoundManager::getInstance().playSound(SoundID::Pause);
            SoundManager::getInstance().pauseMusic();
            Game::getInstance().getStateManager().pushState(
                std::make_unique<PauseState>());
            return;
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
    if (!m_level || !m_player || m_levelComplete) return;

    // Reset sprint each frame (only active while key held)
    m_player->setSprinting(false);

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
}

void PlayingState::loadLevel(int levelNumber) {
    m_level = std::make_unique<Level>();

    std::string filename = "assets/levels/level" + std::to_string(levelNumber) + ".txt";
    std::string charName = Game::getInstance().getProgress().getSelectedCharacter();

    LevelTheme theme = LevelTheme::Castle;
    if (levelNumber == 1) theme = LevelTheme::Overworld;
    else if (levelNumber == 2) theme = LevelTheme::Underground;

    if (!m_level->loadFromFile(filename, charName, theme)) {
        std::cerr << "[PlayingState] Failed to load level: " << filename << std::endl;
        return;
    }

    m_player = m_level->getPlayer();
    m_camera.setLevelBounds(m_level->getWidth(), m_level->getHeight());
}

void PlayingState::checkLevelComplete() {
    if (m_level && m_level->isComplete()) {
        m_levelComplete = true;
        Game& game = Game::getInstance();
        int nextLevel = game.getProgress().getCurrentLevel() + 1;

        SoundManager::getInstance().stopMusic();
        SoundManager::getInstance().playSound(SoundID::LevelComplete);

        if (nextLevel > TOTAL_LEVELS) {
            // Game won! Go to game over with win message
            SoundManager::getInstance().playSound(SoundID::GameOver);
            game.getStateManager().changeState(std::make_unique<GameOverState>());
        } else {
            game.getProgress().setCurrentLevel(nextLevel);
            game.getStateManager().changeState(std::make_unique<PlayingState>());
        }
    }
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
