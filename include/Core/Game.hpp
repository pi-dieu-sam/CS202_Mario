#pragma once
#include <SFML/Graphics.hpp>
#include <memory>
#include <string>
#include "PlayerProgress.hpp"

// Forward declarations
class StateManager;
class AssetManager;
class SoundManager;

/// The main Game class — Singleton pattern.
/// Owns the window, runs the fixed-timestep game loop, and owns the
/// StateManager. Player-progression data lives in PlayerProgress
/// (see getProgress()), kept separate so it can be used/tested without
/// a window or state manager.
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

    /// Access player-progression data (score, lives, coins, level, character).
    PlayerProgress& getProgress();

private:
    Game();

    void processEvents();
    void update(float dt);
    void render();

    sf::RenderWindow                m_window;
    std::unique_ptr<StateManager>   m_stateManager;
    PlayerProgress                  m_progress;

    bool m_running = true;
};
