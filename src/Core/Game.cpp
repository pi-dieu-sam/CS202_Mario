#include "Core/Game.hpp"
#include "States/StateManager.hpp"
#include "States/MenuState.hpp"
#include "Core/AssetManager.hpp"
#include "Core/SoundManager.hpp"
#include "Physics/PhysicsConstants.hpp"
#include <iostream>

Game& Game::getInstance() {
    static Game instance;
    return instance;
}

Game::Game()
    : m_window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Super Mario",
               sf::Style::Close | sf::Style::Titlebar)
    , m_stateManager(std::make_unique<StateManager>())
{
    m_window.setFramerateLimit(60);
    m_window.setKeyRepeatEnabled(false);
}

void Game::run() {
    // Push the initial state (main menu)
    m_stateManager->pushState(std::make_unique<MenuState>());

    sf::Clock clock;
    float accumulator = 0.0f;

    while (m_running && m_window.isOpen()) {
        float frameTime = clock.restart().asSeconds();
        // Clamp to avoid spiral of death
        if (frameTime > 0.25f) frameTime = 0.25f;
        accumulator += frameTime;

        // Process pending state changes
        m_stateManager->processPending();

        if (m_stateManager->isEmpty()) {
            m_running = false;
            break;
        }

        processEvents();

        // Fixed timestep updates
        while (accumulator >= FIXED_DT) {
            update(FIXED_DT);
            accumulator -= FIXED_DT;
        }

        render();
    }

    m_window.close();
}

void Game::processEvents() {
    sf::Event event;
    while (m_window.pollEvent(event)) {
        if (event.type == sf::Event::Closed) {
            m_running = false;
            return;
        }
        m_stateManager->handleEvent(event);
    }
}

void Game::update(float dt) {
    m_stateManager->update(dt);
}

void Game::render() {
    m_window.clear(sf::Color(107, 140, 255)); // Sky blue
    m_stateManager->render(m_window);
    m_window.display();
}

sf::RenderWindow& Game::getWindow() {
    return m_window;
}

StateManager& Game::getStateManager() {
    return *m_stateManager;
}

// ── Score ──
int Game::getScore() const { return m_score; }
void Game::addScore(int points) { m_score += points; }
void Game::setScore(int score) { m_score = score; }

// ── Lives ──
int Game::getLives() const { return m_lives; }
void Game::setLives(int lives) { m_lives = lives; }
void Game::loseLife() {
    m_lives--;
    if (m_lives < 0) m_lives = 0;
}

// ── Level ──
int Game::getCurrentLevel() const { return m_currentLevel; }
void Game::setCurrentLevel(int level) { m_currentLevel = level; }

// ── Coins ──
int Game::getCoins() const { return m_coins; }
void Game::addCoin() {
    m_coins++;
    addScore(COIN_SCORE);
    // Every 100 coins = extra life
    if (m_coins % 100 == 0) {
        m_lives++;
    }
}
void Game::setCoins(int coins) { m_coins = coins; }

// ── Character ──
const std::string& Game::getSelectedCharacter() const { return m_selectedChar; }
void Game::setSelectedCharacter(const std::string& name) { m_selectedChar = name; }

// ── Reset ──
void Game::resetGameData() {
    m_score        = 0;
    m_lives        = STARTING_LIVES;
    m_coins        = 0;
    m_currentLevel = 1;
}
