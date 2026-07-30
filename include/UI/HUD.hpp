#pragma once
#include <SFML/Graphics.hpp>

/// HUD — Heads-Up Display showing score, coins, lives, level, and timer.
/// Subscribes to EventManager to receive score/coin/life updates.
class HUD {
public:
    HUD();

    /// Initialize fonts and text layout.
    void init();

    /// Update timer and any animated elements.
    void update(float dt);

    /// Render the HUD (always drawn in screen space, not world space).
    void render(sf::RenderWindow& window);

    /// Setters for HUD data.
    void setScore(int score);
    void setCoins(int coins);
    void setLives(int lives);
    void setLevel(int level);
    void setTime(float time);
    void setCharacterName(const std::string& name);

private:
    sf::Text m_scoreText;
    sf::Text m_coinText;
    sf::Text m_livesText;
    sf::Text m_levelText;
    sf::Text m_timeText;
    sf::Text m_characterText;
};