#pragma once
#include "GameState.hpp"
#include <SFML/Graphics.hpp>

/// PauseState — overlay drawn on top of PlayingState.
/// Shows "Resume" and "Quit to Menu" options.
class PauseState : public GameState {
public:
    PauseState();

    void onEnter() override;
    void onExit() override;
    void handleEvent(const sf::Event& event) override;
    void update(float dt) override;
    void render(sf::RenderWindow& window) override;

private:
    sf::RectangleShape m_overlay;  // semi-transparent dark overlay
    sf::Text           m_title;
    sf::Text           m_options[2]; // Resume, Quit
    int                m_selected = 0;
};
