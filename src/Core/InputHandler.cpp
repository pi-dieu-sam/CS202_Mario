#include "Core/InputHandler.hpp"
#include "Core/Command.hpp"
#include "Entities/Player.hpp"

InputHandler::InputHandler() {
    setDefaultBindings();
}

void InputHandler::setDefaultBindings() {
    // Held keys — polled every frame
    m_keyBindings[sf::Keyboard::Left]   = std::make_unique<MoveLeftCommand>();
    m_keyBindings[sf::Keyboard::Right]  = std::make_unique<MoveRightCommand>();
    m_keyBindings[sf::Keyboard::A]      = std::make_unique<MoveLeftCommand>();
    m_keyBindings[sf::Keyboard::D]      = std::make_unique<MoveRightCommand>();
    m_keyBindings[sf::Keyboard::LShift] = std::make_unique<SprintCommand>();

    // Press keys — triggered once on key down
    m_pressBindings[sf::Keyboard::Space] = std::make_unique<JumpCommand>();
    m_pressBindings[sf::Keyboard::Up]    = std::make_unique<JumpCommand>();
    m_pressBindings[sf::Keyboard::W]     = std::make_unique<JumpCommand>();
    m_pressBindings[sf::Keyboard::X]     = std::make_unique<FireCommand>();
}

void InputHandler::bindKey(sf::Keyboard::Key key, std::unique_ptr<Command> command) {
    m_keyBindings[key] = std::move(command);
}

std::vector<Command*> InputHandler::handleInput() {
    std::vector<Command*> commands;
    for (auto& [key, command] : m_keyBindings) {
        if (sf::Keyboard::isKeyPressed(key)) {
            commands.push_back(command.get());
        }
    }
    return commands;
}

bool InputHandler::isJumpHeld() const {
    return sf::Keyboard::isKeyPressed(sf::Keyboard::Space) ||
           sf::Keyboard::isKeyPressed(sf::Keyboard::Up) ||
           sf::Keyboard::isKeyPressed(sf::Keyboard::W);
}

Command* InputHandler::handleEvent(const sf::Event& event) {
    if (event.type == sf::Event::KeyPressed) {
        auto it = m_pressBindings.find(event.key.code);
        if (it != m_pressBindings.end()) {
            return it->second.get();
        }
    }
    return nullptr;
}
