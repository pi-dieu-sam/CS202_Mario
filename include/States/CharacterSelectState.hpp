#pragma once
#include "GameState.hpp"
#include <SFML/Graphics.hpp>

/// CharacterSelectState — lets the player choose between Mario and Luigi.
class CharacterSelectState : public GameState {
public:
    CharacterSelectState();

    void onEnter() override;
    void onExit() override;
    void handleEvent(const sf::Event& event) override;
    void update(float dt) override;
    void render(sf::RenderWindow& window) override;

private:
    sf::Text m_title;
    sf::Text m_charNames[2]; // "Mario", "Luigi"
    sf::Text m_charStats[2]; // Stat descriptions
    sf::RectangleShape m_charBoxes[2];
    sf::Sprite m_charSprites[2];
    sf::RectangleShape m_background;
    int  m_selected = 0;
};
