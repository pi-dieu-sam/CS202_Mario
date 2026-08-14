#pragma once
#include "GameState.hpp"
#include <SFML/Graphics.hpp>

enum class GameResult {
    Lost,
    Won,
    P1Won,
    P2Won
};

/// GameOverState — shown when the player runs out of lives or wins.
class GameOverState : public GameState {
public:
    explicit GameOverState(GameResult result = GameResult::Lost,
                           const std::string& winnerName = "");

    void onEnter() override;
    void onExit() override;
    void handleEvent(const sf::Event& event) override;
    void update(float dt) override;
    void render(sf::RenderWindow& window) override;

private:
    void activateSelectedOption();

    GameResult          m_result;
    std::string         m_winnerName; ///< e.g. "MARIO" or "LUIGI" for PvP
    sf::RectangleShape  m_background;
    sf::Text            m_title;
    sf::Text            m_scoreText;
    sf::Text            m_options[2]; // Primary action, Main Menu
    int                 m_selected = 0;
};
