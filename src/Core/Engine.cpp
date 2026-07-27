#include "Core/Engine.hpp"
#include "States/GameStateManager.hpp"
#include "States/MainMenuState.hpp"
#include <iostream>

Engine::Engine(unsigned int width, unsigned int height, const std::string& title)
    : m_window(sf::VideoMode({width, height}), title),
      m_stateManager(std::make_unique<GameStateManager>(*this)),
      m_isRunning(true) {
    
    m_window.setFramerateLimit(60);
    std::cout << "[Engine] Initialized SFML 3 Window (" << width << "x" << height << ")" << std::endl;

    // Push initial state (MainMenuState)
    m_stateManager->pushState(std::make_unique<MainMenuState>(*m_stateManager));
}

Engine::~Engine() = default;

void Engine::run() {
    sf::Clock clock;
    const float timePerFrame = 1.0f / 60.0f;
    float timeSinceLastUpdate = 0.0f;

    while (m_window.isOpen() && m_isRunning && !m_stateManager->isEmpty()) {
        float dt = clock.restart().asSeconds();
        timeSinceLastUpdate += dt;

        processEvents();

        while (timeSinceLastUpdate >= timePerFrame) {
            timeSinceLastUpdate -= timePerFrame;
            update(timePerFrame);
        }

        render();
    }
}

void Engine::processEvents() {
    while (const std::optional event = m_window.pollEvent()) {
        if (event->is<sf::Event::Closed>()) {
            m_window.close();
            m_isRunning = false;
        } else {
            m_stateManager->handleInput(*event);
        }
    }
}

void Engine::update(float dt) {
    m_stateManager->update(dt);
}

void Engine::render() {
    m_window.clear(sf::Color(107, 140, 255)); // Super Mario sky blue background
    m_stateManager->render(m_window);
    m_window.display();
}
