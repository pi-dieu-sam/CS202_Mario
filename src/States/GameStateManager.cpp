#include "States/GameStateManager.hpp"
#include <iostream>

GameStateManager::GameStateManager(Engine& engine) : m_engine(engine) {}

void GameStateManager::pushState(std::unique_ptr<GameState> state) {
    if (!m_states.empty()) {
        m_states.back()->pause();
    }
    m_states.push_back(std::move(state));
    m_states.back()->init();
}

void GameStateManager::popState() {
    if (!m_states.empty()) {
        m_states.pop_back();
    }
    if (!m_states.empty()) {
        m_states.back()->resume();
    }
}

void GameStateManager::changeState(std::unique_ptr<GameState> state) {
    clearStates();
    pushState(std::move(state));
}

void GameStateManager::clearStates() {
    m_states.clear();
}

void GameStateManager::handleInput(const sf::Event& event) {
    if (!m_states.empty()) {
        m_states.back()->handleInput(event);
    }
}

void GameStateManager::update(float dt) {
    if (!m_states.empty()) {
        m_states.back()->update(dt);
    }
}

void GameStateManager::render(sf::RenderWindow& window) {
    if (!m_states.empty()) {
        m_states.back()->render(window);
    }
}
