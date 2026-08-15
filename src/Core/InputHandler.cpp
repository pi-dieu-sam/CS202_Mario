#include "Core/InputHandler.hpp"
#include "Core/Command.hpp"
#include "Entities/Player.hpp"

InputHandler::InputHandler() {
    setPlayer1Bindings(); // default
}

void InputHandler::setPlayer1Bindings() {
    m_keyBindings.clear();
    m_pressBindings.clear();
    
    // Held keys
    m_keyBindings[sf::Keyboard::A] = std::make_unique<MoveLeftCommand>();
    m_keyBindings[sf::Keyboard::D] = std::make_unique<MoveRightCommand>();

    // Press keys
    m_pressBindings[sf::Keyboard::W]     = std::make_unique<JumpCommand>();
    m_pressBindings[sf::Keyboard::Space] = std::make_unique<JumpCommand>();
    m_pressBindings[sf::Keyboard::LShift]= std::make_unique<FireCommand>();

    m_jumpKeys = {sf::Keyboard::W, sf::Keyboard::Space};
    m_sprintKeys = {sf::Keyboard::LShift};
}

void InputHandler::setPlayer2Bindings() {
    m_keyBindings.clear();
    m_pressBindings.clear();
    
    // Held keys
    m_keyBindings[sf::Keyboard::Left]  = std::make_unique<MoveLeftCommand>();
    m_keyBindings[sf::Keyboard::Right] = std::make_unique<MoveRightCommand>();

    // Press keys
    m_pressBindings[sf::Keyboard::Up]       = std::make_unique<JumpCommand>();
    m_pressBindings[sf::Keyboard::Numpad0]  = std::make_unique<JumpCommand>();
    m_pressBindings[sf::Keyboard::RShift]   = std::make_unique<FireCommand>();

    m_jumpKeys = {sf::Keyboard::Up, sf::Keyboard::Numpad0};
    m_sprintKeys = {sf::Keyboard::RShift};
}

void InputHandler::setSinglePlayerBindings() {
    m_keyBindings.clear();
    m_pressBindings.clear();

    // Held keys — WASD + arrows
    m_keyBindings[sf::Keyboard::A]     = std::make_unique<MoveLeftCommand>();
    m_keyBindings[sf::Keyboard::Left]  = std::make_unique<MoveLeftCommand>();
    m_keyBindings[sf::Keyboard::D]     = std::make_unique<MoveRightCommand>();
    m_keyBindings[sf::Keyboard::Right] = std::make_unique<MoveRightCommand>();

    // Press keys
    m_pressBindings[sf::Keyboard::W]       = std::make_unique<JumpCommand>();
    m_pressBindings[sf::Keyboard::Space]   = std::make_unique<JumpCommand>();
    m_pressBindings[sf::Keyboard::Up]      = std::make_unique<JumpCommand>();
    m_pressBindings[sf::Keyboard::Numpad0] = std::make_unique<JumpCommand>();
    m_pressBindings[sf::Keyboard::LShift]  = std::make_unique<FireCommand>();
    m_pressBindings[sf::Keyboard::RShift]  = std::make_unique<FireCommand>();

    m_jumpKeys = {sf::Keyboard::W, sf::Keyboard::Space,
                  sf::Keyboard::Up, sf::Keyboard::Numpad0};
    m_sprintKeys = {sf::Keyboard::LShift, sf::Keyboard::RShift};
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
    for (auto k : m_jumpKeys) {
        if (sf::Keyboard::isKeyPressed(k)) return true;
    }
    return false;
}

bool InputHandler::isSprintHeld() const {
    for (auto k : m_sprintKeys) {
        if (sf::Keyboard::isKeyPressed(k)) return true;
    }
    return false;
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
