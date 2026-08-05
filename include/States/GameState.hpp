#pragma once
#include <SFML/Graphics.hpp>

/// GameState — Abstract base class for the State pattern.
/// Each state represents a game screen (menu, playing, pause, etc.).
class GameState {
public:
    virtual ~GameState() = default;

    /// Called when this state becomes active.
    virtual void onEnter() = 0;

    /// Called when this state is removed.
    virtual void onExit() = 0;

    /// Called when a stacked state on top of this state is popped.
    virtual void onResume() {}

    /// Handle SFML events (key presses, window close, etc.).
    virtual void handleEvent(const sf::Event& event) = 0;

    /// Update game logic with fixed timestep.
    virtual void update(float dt) = 0;

    /// Render to the window.
    virtual void render(sf::RenderWindow& window) = 0;
};
