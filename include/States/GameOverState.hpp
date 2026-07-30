#pragma once
#include "GameState.hpp"
#include <SFML/Graphics.hpp>

/// GameOverState — shown when the player runs out of lives.
class GameOverState : public GameState {
public:
    GameOverState();

    void onEnter() override;
    void onExit() override;
    void handleEvent(const sf::Event& event) override;
    void update(float dt) override;
    void render(sf::RenderWindow& window) override;

private:
    sf::RectangleShape m_background;
    sf::Text           m_title;
    sf::Text           m_scoreText;
    sf::Text           m_options[2]; // Retry, Main Menu
    int                m_selected = 0;
};
