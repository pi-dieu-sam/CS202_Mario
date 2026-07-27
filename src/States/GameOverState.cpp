#include "States/GameOverState.hpp"
#include "States/GameStateManager.hpp"
#include "States/PlayingState.hpp"
#include "States/MainMenuState.hpp"
#include "Core/SoundManager.hpp"
#include <iostream>

GameOverState::GameOverState(GameStateManager& stateManager, bool isVictory, int finalScore)
    : GameState(stateManager), m_isVictory(isVictory), m_finalScore(finalScore), m_selectedIndex(0) {
    m_options = {
        "RETRY LEVEL",
        "MAIN MENU"
    };
}

void GameOverState::init() {
    if (!m_font.openFromFile("assets/fonts/arial.ttf")) {
        std::cerr << "[GameOverState] Warning: font missing." << std::endl;
    }

    std::string titleStr = m_isVictory ? "STAGE CLEAR! VICTORY!" : "GAME OVER";
    sf::Color titleColor = m_isVictory ? sf::Color::Green : sf::Color::Red;

    m_titleText.emplace(m_font, titleStr, 42);
    m_titleText->setFillColor(titleColor);
    m_titleText->setPosition({220.0f, 150.0f});

    m_scoreText.emplace(m_font, "FINAL SCORE: " + std::to_string(m_finalScore), 26);
    m_scoreText->setFillColor(sf::Color::White);
    m_scoreText->setPosition({290.0f, 230.0f});

    m_buttons.clear();
    for (size_t i = 0; i < m_options.size(); ++i) {
        m_buttons.emplace_back(m_options[i], sf::Vector2f{310.0f, 330.0f + static_cast<float>(i) * 50.0f}, 24);
        m_buttons.back().init(m_font);
        m_buttons.back().setSelected(i == 0);
    }
}

void GameOverState::handleInput(const sf::Event& event) {
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
    }
}

void GameOverState::updateSelection() {
    for (size_t i = 0; i < m_buttons.size(); ++i) {
        m_buttons[i].setSelected(static_cast<int>(i) == m_selectedIndex);
    }
}

void GameOverState::processSelection() {
    if (m_selectedIndex == 0) {
        m_stateManager.changeState(std::make_unique<PlayingState>(m_stateManager, 1, "Mario"));
    } else {
        m_stateManager.changeState(std::make_unique<MainMenuState>(m_stateManager));
    }
}

void GameOverState::update(float dt) {
    (void)dt;
}

void GameOverState::render(sf::RenderWindow& window) {
    if (m_titleText) window.draw(*m_titleText);
    if (m_scoreText) window.draw(*m_scoreText);
    for (auto& btn : m_buttons) {
        btn.render(window);
    }
}
