#pragma once

#include "GameState.hpp"

#include <SFML/Graphics.hpp>

/// CharacterSelectState — an animated Mario/Luigi showcase for single-player
/// games. Each card loops through idle, run, jump, and fireball poses before
/// the player explicitly confirms a hero.
class CharacterSelectState : public GameState {
public:
    CharacterSelectState() = default;

    void onEnter() override;
    void onExit() override;
    void handleEvent(const sf::Event& event) override;
    void update(float dt) override;
    void render(sf::RenderWindow& window) override;

private:
    void confirmSelection();
    void updateVisuals();
    void updateShowcase(float dt);

    sf::RectangleShape m_background;
    sf::RectangleShape m_characterCards[2];
    sf::RectangleShape m_stageFloors[2];
    sf::RectangleShape m_confirmButton;
    sf::Sprite m_charSprites[2];
    sf::Sprite m_fireballSprites[2];
    sf::Text m_title;
    sf::Text m_subtitle;
    sf::Text m_charNames[2];
    sf::Text m_charStats[2];
    sf::Text m_actionText;
    sf::Text m_helpText;
    int m_selected = 0;
    float m_showcaseTime = 0.0f;
};
