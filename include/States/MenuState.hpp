#pragma once
#include "GameState.hpp"
#include <SFML/Graphics.hpp>

/// MenuState — the main title screen.
/// Shows game title, "New Game", "Load Game", and "Exit" options.
class MenuState : public GameState {
public:
    MenuState();

    void onEnter() override;
    void onExit() override;
    void handleEvent(const sf::Event& event) override;
    void update(float dt) override;
    void render(sf::RenderWindow& window) override;

private:
    void updateOptionVisuals();

    sf::Text  m_title;
    sf::Text  m_options[3]; // New Game, Load Game, Exit
    int       m_selectedOption = 0;

    // Background
    sf::RectangleShape m_background;

    // Animation
    float m_titleBounce = 0.0f;
};
