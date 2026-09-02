#pragma once

#include "Core/PlayerProgress.hpp"

#include <SFML/Graphics.hpp>
#include <string>

/// HUD — pixel status strip for gameplay. Single-player emphasizes the chosen
/// hero, while co-op/PvP adds a mirrored P2 identity without inventing a new
/// per-player score or life system.
class HUD {
public:
    HUD() = default;

    void init(GameMode mode = GameMode::SinglePlayer);
    void update(float dt);
    void render(sf::RenderWindow& window);

    void setScore(int score);
    void setCoins(int coins);
    void setLives(int lives);
    void setLevel(int level);
    void setTime(float time);
    void setCharacterName(const std::string& name);
    void setPlayer2Name(const std::string& name);
    void setGameMode(GameMode mode);

private:
    void refreshLayout();
    static std::string uppercase(std::string value);

    GameMode m_mode = GameMode::SinglePlayer;
    std::string m_characterName = "MARIO";
    std::string m_player2Name = "LUIGI";
    sf::RectangleShape m_background;
    sf::Text m_characterText;
    sf::Text m_player2Text;
    sf::Text m_scoreLabel;
    sf::Text m_scoreText;
    sf::Text m_coinLabel;
    sf::Text m_coinText;
    sf::Text m_levelLabel;
    sf::Text m_levelText;
    sf::Text m_livesLabel;
    sf::Text m_livesText;
    sf::Text m_timeLabel;
    sf::Text m_timeText;
};
