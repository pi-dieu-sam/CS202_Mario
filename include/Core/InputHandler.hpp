#pragma once
#include <SFML/Graphics.hpp>
#include <memory>
#include <set>
#include <vector>
#include <unordered_map>

// Forward declarations
class Player;
class Command;

/// InputHandler — Command pattern.
/// Maps keyboard keys to Command objects and returns a list
/// of commands to execute each frame.
class InputHandler {
public:
    InputHandler();

    /// Set bindings for Player 1 (WASD)
    void setPlayer1Bindings();

    /// Set bindings for Player 2 (Arrows)
    void setPlayer2Bindings();

    /// Set combined bindings (WASD + Arrows) for single-player control.
    void setSinglePlayerBindings();
    void bindKey(sf::Keyboard::Key key, std::unique_ptr<Command> command);

    /// Seed the held-key set from the physical keyboard state. Call once
    /// after (re)binding so keys already held still register.
    void seedHeldKeys();

    /// Clear all tracked held keys (e.g. when the window loses focus).
    void clearHeldKeys();

    /// Return commands to execute this frame for all currently held keys.
    std::vector<Command*> handleInput();

    /// Check whether any jump key is currently held.
    bool isJumpHeld() const;

    /// Check whether sprint is held. Read separately so movement always sees
    /// the sprint state before directional commands execute.
    bool isSprintHeld() const;

    /// Handle a single SFML event. Tracks KeyPressed/KeyReleased/LostFocus
    /// to maintain the held-key set, and returns a one-shot command for
    /// press-only actions like jump or fire.
    Command* handleEvent(const sf::Event& event);

private:
    std::unordered_map<sf::Keyboard::Key, std::unique_ptr<Command>> m_keyBindings;
    std::unordered_map<sf::Keyboard::Key, std::unique_ptr<Command>> m_pressBindings;
    std::vector<sf::Keyboard::Key> m_jumpKeys;
    std::vector<sf::Keyboard::Key> m_sprintKeys;
    std::set<sf::Keyboard::Key> m_heldKeys;
};
