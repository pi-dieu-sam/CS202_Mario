#pragma once
#include "GameState.hpp"
#include <memory>
#include <vector>

/// StateManager — manages a stack of GameState objects.
/// The topmost state receives all update/render/event calls.
class StateManager {
public:
    StateManager() = default;

    /// Push a new state on top of the stack.
    void pushState(std::unique_ptr<GameState> state);

    /// Pop the topmost state.
    void popState();

    /// Replace the topmost state (pop + push).
    void changeState(std::unique_ptr<GameState> state);

    /// Clear all states.
    void clearStates();

    /// Process pending state changes (call at start of frame).
    void processPending();

    /// Delegate to topmost state.
    void handleEvent(const sf::Event& event);
    void update(float dt);
    void render(sf::RenderWindow& window);

    /// Check if there are any states.
    bool isEmpty() const;

    /// Get current (topmost) state.
    GameState* currentState() const;

private:
    std::vector<std::unique_ptr<GameState>> m_states;

    // Pending operations (processed at start of frame to avoid mid-iteration issues)
    enum class Action { Push, Pop, Change, Clear };
    struct PendingChange {
        Action                      action;
        std::unique_ptr<GameState>  state;
    };
    std::vector<PendingChange> m_pending;
};
