#pragma once
#include <string>

/// GameMode — describes how many players and what rules apply.
enum class GameMode {
    SinglePlayer, ///< Classic 1-player game
    Coop,         ///< 2 players cooperate; no friendly fire
    PvP           ///< 2 players compete; stomping and fireballs deal damage
};

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

    /// Advance to the next level. Returns false when the current level is
    /// already the final level, so callers can show the victory flow.
    bool advanceToNextLevel(int totalLevels);

    /// Restart the current level after a game over without discarding the
    /// player's score, coins, selected character, or level selection.
    void retryCurrentLevel();

    int  getCoins() const;
    void addCoin();
    void setCoins(int coins);

    const std::string& getSelectedCharacter() const;
    void               setSelectedCharacter(const std::string& name);

    /// Reset all progress data to defaults.
    void resetGameData();

    GameMode getGameMode() const { return m_gameMode; }
    void     setGameMode(GameMode mode) { m_gameMode = mode; }

    /// Convenience helpers
    bool isCoop() const { return m_gameMode == GameMode::Coop; }
    bool isPvP()  const { return m_gameMode == GameMode::PvP;  }
    bool isMultiplayer() const { return m_gameMode != GameMode::SinglePlayer; }

private:
    int         m_score        = 0;
    int         m_lives        = 3;
    int         m_coins        = 0;
    int         m_currentLevel = 1;
    std::string m_selectedChar = "Mario";
    GameMode    m_gameMode     = GameMode::SinglePlayer;
};
