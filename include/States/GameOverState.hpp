#pragma once
#include "GameState.hpp"
#include "States/ScreenFlow.hpp"
#include <SFML/Graphics.hpp>

/// GameResult lives in ScreenFlow.hpp (not here) so the post-game-over
/// navigation decision can be unit-tested without pulling in SFML.
using GameResult = ScreenFlow::GameResult;

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
    sf::RectangleShape  m_resultPanel;
    sf::Sprite          m_heroSprite;
    sf::Text            m_title;
    sf::Text            m_subtitleText; ///< "PLAYER 1 / PLAYER 2 IS THE WINNER!" for PvP
    sf::Text            m_detailText;
    sf::Text            m_scoreText;
    sf::Text            m_options[2]; // Primary action, Main Menu
    int                 m_selected = 0;
    float               m_animTime = 0.0f; ///< For pulsing title animation
};
