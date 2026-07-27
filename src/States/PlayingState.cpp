#include "States/PlayingState.hpp"
#include "States/GameStateManager.hpp"
#include "States/PauseState.hpp"
#include "States/GameOverState.hpp"
#include "Core/InputHandler.hpp"
#include "Core/SoundManager.hpp"
#include "Core/SaveSystem.hpp"
#include <iostream>

PlayingState::PlayingState(GameStateManager& stateManager, int levelIndex, const std::string& characterName)
    : GameState(stateManager),
      m_levelIndex(levelIndex),
      m_characterName(characterName),
      m_score(0),
      m_coins(0),
      m_lives(3),
      m_levelTime(400.0f),
      m_player(std::make_unique<Player>(characterName)),
      m_level(std::make_unique<Level>(levelIndex)),
      m_camera(std::make_unique<Camera>()),
      m_inputHandler(std::make_unique<InputHandler>()) {}

PlayingState::~PlayingState() = default;

void PlayingState::init() {
    std::cout << "[PlayingState] Started Level " << m_levelIndex << " with Character: " << m_characterName << std::endl;
    m_hud.init("assets/fonts/arial.ttf");
    m_hud.update(m_characterName, m_levelIndex, m_score, m_coins, m_lives, m_levelTime);
}

void PlayingState::handleInput(const sf::Event& event) {
    if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
        if (keyPressed->code == sf::Keyboard::Key::P || keyPressed->code == sf::Keyboard::Key::Escape) {
            m_stateManager.pushState(std::make_unique<PauseState>(m_stateManager));
        }

        Command* cmd = m_inputHandler->handleEventInput(keyPressed->code);
        if (cmd && m_player) {
            cmd->execute(*m_player);
        }
    }
}

void PlayingState::update(float dt) {
    m_levelTime -= dt;
    if (m_levelTime <= 0.0f) {
        playerDied();
        return;
    }

    Command* continuousCmd = m_inputHandler->handleRealtimeInput();
    if (continuousCmd && m_player) {
        continuousCmd->execute(*m_player);
    }

    if (m_player) {
        m_player->update(dt);
        if (m_camera) {
            m_camera->update(m_player->getPosition(), m_level ? m_level->getWidth() : 3200.0f);
        }
    }

    if (m_level) {
        m_level->update(dt);
    }

    m_hud.update(m_characterName, m_levelIndex, m_score, m_coins, m_lives, m_levelTime);
}

void PlayingState::playerDied() {
    m_lives--;
    SoundManager::getInstance().playSound("die");

    if (m_lives <= 0) {
        m_stateManager.changeState(std::make_unique<GameOverState>(m_stateManager, false, m_score));
    } else {
        restartLevel();
    }
}

void PlayingState::restartLevel() {
    m_levelTime = 400.0f;
    if (m_player) {
        m_player->setPosition({100.0f, 400.0f});
    }
    std::cout << "[PlayingState] Restarting Level " << m_levelIndex << " (Remaining Lives: " << m_lives << ")" << std::endl;
}

void PlayingState::loadNextLevel() {
    m_levelIndex++;
    if (m_levelIndex > 3) {
        m_stateManager.changeState(std::make_unique<GameOverState>(m_stateManager, true, m_score));
    } else {
        m_levelTime = 400.0f;
        std::cout << "[PlayingState] Loaded Next Level " << m_levelIndex << std::endl;
    }
}

void PlayingState::saveCurrentProgress() {
    SaveData save;
    save.currentLevel = m_levelIndex;
    save.characterName = m_characterName;
    save.score = m_score;
    save.coins = m_coins;
    save.lives = m_lives;

    SaveSystem::getInstance().saveGame(save);
}

void PlayingState::render(sf::RenderWindow& window) {
    if (m_camera) {
        window.setView(m_camera->getView());
    }

    if (m_level) {
        m_level->render(window);
    }

    if (m_player) {
        m_player->render(window);
    }

    // Render HUD component
    m_hud.render(window);
}
