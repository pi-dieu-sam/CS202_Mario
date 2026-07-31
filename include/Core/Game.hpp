#pragma once
#include <SFML/Graphics.hpp>
#include <memory>
#include <string>

// Forward declarations
class StateManager;
class AssetManager;
class SoundManager;

/// The main Game class — Singleton pattern.
/// Owns the window, runs the fixed-timestep game loop,
/// and holds global state (score, lives, selected character).
class Game {
public:
    static Game& getInstance();

    // Delete copy/move for singleton
    Game(const Game&)            = delete;
    Game& operator=(const Game&) = delete;
    Game(Game&&)                 = delete;
    Game& operator=(Game&&)      = delete;

    /// Start the game loop. Blocks until window closes.
    void run();

    /// Access the render window.
    sf::RenderWindow& getWindow();

    /// Access the state manager.
    StateManager& getStateManager();

    // ── Global game data ──
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
    void               setSelectedCharacter(const std::string& name);

    /// Reset all game data to defaults.
    void resetGameData();

private:
    Game();

    void processEvents();
    void update(float dt);
    void render();

    sf::RenderWindow                m_window;
    std::unique_ptr<StateManager>   m_stateManager;

    // Global game data
    int         m_score         = 0;
    int         m_lives         = 3;
    int         m_coins         = 0;
    int         m_currentLevel  = 1;
    std::string m_selectedChar  = "Mario";

    bool m_running = true;
};
