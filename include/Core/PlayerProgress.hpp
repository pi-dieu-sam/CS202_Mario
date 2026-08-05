#pragma once
#include <string>

/// PlayerProgress — holds player-progression data (score, lives, coins,
/// current level, selected character) and the business rules around them.
/// Deliberately has no SFML/window/singleton dependency so it can be
/// constructed and unit-tested in isolation.
class PlayerProgress {
public:
    PlayerProgress() = default;

    int  getScore() const;
    void addScore(int points);
    void setScore(int score);

    int  getLives() const;
    void setLives(int lives);
    void loseLife();

    int  getCurrentLevel() const;
    void setCurrentLevel(int level);

    int  getCoins() const;
    void addCoin();
    void setCoins(int coins);

    const std::string& getSelectedCharacter() const;
    void                setSelectedCharacter(const std::string& name);

    /// Reset all progress data to defaults.
    void resetGameData();

private:
    int         m_score         = 0;
    int         m_lives         = 3;
    int         m_coins         = 0;
    int         m_currentLevel  = 1;
    std::string m_selectedChar  = "Mario";
};
