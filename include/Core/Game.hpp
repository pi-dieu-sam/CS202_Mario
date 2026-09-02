#pragma once
#include <SFML/Graphics.hpp>
#include <memory>
#include <string>
#include <vector>
#include "GameSettings.hpp"
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

    /// The only intended way to end the game loop from outside a window-close
    /// event (e.g. the menu's Exit option). Popping/clearing the state stack
    /// down to empty is treated as an error, not a way to quit — see run().
    void requestExit();

    /// Access the render window.
    sf::RenderWindow& getWindow();

    /// The fixed 800x608 logical canvas used by every menu and HUD. It is
    /// letterboxed inside the physical window when a larger resolution is
    /// selected, keeping pixel art and mouse hitboxes aligned.
    const sf::View& getUiView() const;
    sf::Vector2f mapPixelToUiCoords(const sf::Vector2i& pixel) const;

    /// Persisted display/audio preferences.
    GameSettings& getSettings();
    const GameSettings& getSettings() const;

    /// Recreate the SFML window for normalized display preferences. Returns
    /// false if an unsupported fullscreen mode required a safe fallback.
    bool applyGraphicsSettings(GraphicsSettings settings);
    void applyAudioSettings();
    std::vector<sf::VideoMode> getFullscreenModes() const;

    /// Access the state manager.
    StateManager& getStateManager();

    /// Access player-progression data (score, lives, coins, level, character).
    PlayerProgress& getProgress();

private:
    Game();

    void processEvents();
    void update(float dt);
    void render();
    void updateUiView();

    GameSettings                    m_settings;
    sf::RenderWindow                m_window;
    sf::View                        m_uiView;
    std::unique_ptr<StateManager>   m_stateManager;
    PlayerProgress                  m_progress;

    bool m_running = true;
};
