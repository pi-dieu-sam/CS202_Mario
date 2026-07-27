#include "States/CharacterSelectState.hpp"
#include "States/GameStateManager.hpp"
#include "Core/SoundManager.hpp"
#include <iostream>

std::string CharacterSelectState::s_selectedCharacter = "Mario";

CharacterSelectState::CharacterSelectState(GameStateManager& stateManager)
    : GameState(stateManager), m_selectedIndex(0) {}

void CharacterSelectState::init() {
    if (!m_font.openFromFile("assets/fonts/arial.ttf")) {
        std::cerr << "[CharacterSelectState] Warning: font asset missing." << std::endl;
    }

    m_titleText.emplace(m_font, "CHOOSE YOUR HERO", 36);
    m_titleText->setFillColor(sf::Color::Yellow);
    m_titleText->setPosition({240.0f, 40.0f});

    // Mario Card setup
    m_marioCard.setSize({260.0f, 320.0f});
    m_marioCard.setPosition({110.0f, 130.0f});
    m_marioCard.setFillColor(sf::Color(180, 40, 40));
    m_marioCard.setOutlineThickness(4.0f);

    m_marioText.emplace(m_font, "MARIO", 30);
    m_marioText->setFillColor(sf::Color::White);
    m_marioText->setPosition({180.0f, 150.0f});

    m_marioStatsText.emplace(m_font, "Speed: *****\nJump:  ****\nTraction: High\n\nBalanced hero!", 18);
    m_marioStatsText->setFillColor(sf::Color::White);
    m_marioStatsText->setPosition({130.0f, 230.0f});

    // Luigi Card setup
    m_luigiCard.setSize({260.0f, 320.0f});
    m_luigiCard.setPosition({430.0f, 130.0f});
    m_luigiCard.setFillColor(sf::Color(40, 150, 40));
    m_luigiCard.setOutlineThickness(4.0f);

    m_luigiText.emplace(m_font, "LUIGI", 30);
    m_luigiText->setFillColor(sf::Color::White);
    m_luigiText->setPosition({510.0f, 150.0f});

    m_luigiStatsText.emplace(m_font, "Speed: ****\nJump:  *****\nTraction: Low\n\nHigh jump master!", 18);
    m_luigiStatsText->setFillColor(sf::Color::White);
    m_luigiStatsText->setPosition({450.0f, 230.0f});

    m_confirmText.emplace(m_font, "Press ENTER to Select | ESC to Back", 20);
    m_confirmText->setFillColor(sf::Color::White);
    m_confirmText->setPosition({220.0f, 500.0f});

    updateCardHighlight();
}

void CharacterSelectState::handleInput(const sf::Event& event) {
    if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
        if (keyPressed->code == sf::Keyboard::Key::Left || keyPressed->code == sf::Keyboard::Key::A) {
            m_selectedIndex = 0;
            updateCardHighlight();
            SoundManager::getInstance().playSound("select");
        }
        else if (keyPressed->code == sf::Keyboard::Key::Right || keyPressed->code == sf::Keyboard::Key::D) {
            m_selectedIndex = 1;
            updateCardHighlight();
            SoundManager::getInstance().playSound("select");
        }
        else if (keyPressed->code == sf::Keyboard::Key::Enter || keyPressed->code == sf::Keyboard::Key::Space) {
            s_selectedCharacter = (m_selectedIndex == 0) ? "Mario" : "Luigi";
            std::cout << "[CharacterSelectState] Selected Character: " << s_selectedCharacter << std::endl;
            m_stateManager.popState();
        }
        else if (keyPressed->code == sf::Keyboard::Key::Escape) {
            m_stateManager.popState();
        }
    }
}

void CharacterSelectState::updateCardHighlight() {
    if (m_selectedIndex == 0) {
        m_marioCard.setOutlineColor(sf::Color::Yellow);
        m_luigiCard.setOutlineColor(sf::Color::Transparent);
    } else {
        m_marioCard.setOutlineColor(sf::Color::Transparent);
        m_luigiCard.setOutlineColor(sf::Color::Yellow);
    }
}

void CharacterSelectState::update(float dt) {
    (void)dt;
}

void CharacterSelectState::render(sf::RenderWindow& window) {
    if (m_titleText) window.draw(*m_titleText);
    window.draw(m_marioCard);
    if (m_marioText) window.draw(*m_marioText);
    if (m_marioStatsText) window.draw(*m_marioStatsText);

    window.draw(m_luigiCard);
    if (m_luigiText) window.draw(*m_luigiText);
    if (m_luigiStatsText) window.draw(*m_luigiStatsText);

    if (m_confirmText) window.draw(*m_confirmText);
}
