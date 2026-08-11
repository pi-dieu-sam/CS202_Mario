#pragma once
#include <SFML/Graphics.hpp>
#include <memory>
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

    /// Set default key bindings.
    void setDefaultBindings();

    /// Rebind a key to a command.
    void bindKey(sf::Keyboard::Key key, std::unique_ptr<Command> command);

    /// Poll held keys and return commands to execute this frame.
    std::vector<Command*> handleInput();

    /// Check whether any jump key is currently held.
    bool isJumpHeld() const;

    /// Check whether sprint is held. Read separately so movement always sees
    /// the sprint state before directional commands execute.
    bool isSprintHeld() const;

    /// Handle a single SFML key-press event (for one-shot actions like jump).
    Command* handleEvent(const sf::Event& event);

private:
    std::unordered_map<sf::Keyboard::Key, std::unique_ptr<Command>> m_keyBindings;
    std::unordered_map<sf::Keyboard::Key, std::unique_ptr<Command>> m_pressBindings;
};
