#include "States/CharacterSelectState.hpp"
#include "States/GameStateManager.hpp"
#include "Core/SoundManager.hpp"
#include <iostream>

std::string CharacterSelectState::s_selectedCharacter = "Mario";

CharacterSelectState::CharacterSelectState(GameStateManager& stateManager)
    : GameState(stateManager),
      m_marioCard("MARIO", "Speed: *****\nJump:  ****\nTraction: High\n\nBalanced hero!", {110.0f, 130.0f}, sf::Color(180, 40, 40)),
      m_luigiCard("LUIGI", "Speed: ****\nJump:  *****\nTraction: Low\n\nHigh jump master!", {430.0f, 130.0f}, sf::Color(40, 150, 40)),
      m_selectedIndex(0) {}

void CharacterSelectState::init() {
    if (!m_font.openFromFile("assets/fonts/arial.ttf")) {
        std::cerr << "[CharacterSelectState] Warning: font asset missing." << std::endl;
    }

    m_titleText.emplace(m_font, "CHOOSE YOUR HERO", 36);
    m_titleText->setFillColor(sf::Color::Yellow);
    m_titleText->setPosition({240.0f, 40.0f});

    m_marioCard.init(m_font);
    m_luigiCard.init(m_font);

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
    m_marioCard.setSelected(m_selectedIndex == 0);
    m_luigiCard.setSelected(m_selectedIndex == 1);
}

void CharacterSelectState::update(float dt) {
    (void)dt;
}

void CharacterSelectState::render(sf::RenderWindow& window) {
    if (m_titleText) window.draw(*m_titleText);
    
    m_marioCard.render(window);
    m_luigiCard.render(window);

    if (m_confirmText) window.draw(*m_confirmText);
}
