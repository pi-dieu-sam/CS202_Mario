#include "States/PauseState.hpp"
#include "States/GameStateManager.hpp"
#include "States/PlayingState.hpp"
#include "Core/SoundManager.hpp"
#include <iostream>

PauseState::PauseState(GameStateManager& stateManager)
    : GameState(stateManager), m_selectedIndex(0) {
    m_options = {
        "RESUME GAME",
        "SAVE PROGRESS",
        "MAIN MENU"
    };
}

void PauseState::init() {
    if (!m_font.openFromFile("assets/fonts/arial.ttf")) {
        std::cerr << "[PauseState] Warning: font loading failed." << std::endl;
    }

    m_overlay.setSize({800.0f, 600.0f});
    m_overlay.setFillColor(sf::Color(0, 0, 0, 180));

    m_titleText.emplace(m_font, "GAME PAUSED", 38);
    m_titleText->setFillColor(sf::Color::Yellow);
    m_titleText->setPosition({260.0f, 150.0f});

    m_optionsText.clear();
    for (size_t i = 0; i < m_options.size(); ++i) {
        m_optionsText.emplace_back(m_font, m_options[i], 24);
        m_optionsText.back().setPosition({300.0f, 260.0f + static_cast<float>(i) * 50.0f});
        m_optionsText.back().setFillColor(i == 0 ? sf::Color::Yellow : sf::Color::White);
    }
}

void PauseState::handleInput(const sf::Event& event) {
    if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
        if (keyPressed->code == sf::Keyboard::Key::Up || keyPressed->code == sf::Keyboard::Key::W) {
            m_selectedIndex = (m_selectedIndex - 1 + static_cast<int>(m_options.size())) % m_options.size();
            updateSelection();
            SoundManager::getInstance().playSound("select");
        }
        else if (keyPressed->code == sf::Keyboard::Key::Down || keyPressed->code == sf::Keyboard::Key::S) {
            m_selectedIndex = (m_selectedIndex + 1) % m_options.size();
            updateSelection();
            SoundManager::getInstance().playSound("select");
        }
        else if (keyPressed->code == sf::Keyboard::Key::Enter || keyPressed->code == sf::Keyboard::Key::Space) {
            processSelection();
            SoundManager::getInstance().playSound("confirm");
        }
        else if (keyPressed->code == sf::Keyboard::Key::P || keyPressed->code == sf::Keyboard::Key::Escape) {
            m_stateManager.popState();
        }
    }
}

void PauseState::updateSelection() {
    for (size_t i = 0; i < m_optionsText.size(); ++i) {
        if (static_cast<int>(i) == m_selectedIndex) {
            m_optionsText[i].setFillColor(sf::Color::Yellow);
            m_optionsText[i].setStyle(sf::Text::Bold);
        } else {
            m_optionsText[i].setFillColor(sf::Color::White);
            m_optionsText[i].setStyle(sf::Text::Regular);
        }
    }
}

void PauseState::processSelection() {
    if (m_selectedIndex == 0) {
        m_stateManager.popState();
    } else if (m_selectedIndex == 1) {
        m_stateManager.popState();
    } else if (m_selectedIndex == 2) {
        m_stateManager.clearStates();
    }
}

void PauseState::update(float dt) {
    (void)dt;
}

void PauseState::render(sf::RenderWindow& window) {
    window.draw(m_overlay);
    if (m_titleText) window.draw(*m_titleText);
    for (const auto& text : m_optionsText) {
        window.draw(text);
    }
}
