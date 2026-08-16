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
    m_pressBindings[sf::Keyboard::F]     = std::make_unique<FireCommand>();

    m_jumpKeys = {sf::Keyboard::W, sf::Keyboard::Space};
    m_sprintKeys = {sf::Keyboard::LShift};

    m_heldKeys.clear();
    seedHeldKeys();
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

    m_heldKeys.clear();
    seedHeldKeys();
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
    m_pressBindings[sf::Keyboard::F]       = std::make_unique<FireCommand>();

    m_jumpKeys = {sf::Keyboard::W, sf::Keyboard::Space,
                  sf::Keyboard::Up, sf::Keyboard::Numpad0};
    m_sprintKeys = {sf::Keyboard::LShift, sf::Keyboard::RShift};

    m_heldKeys.clear();
    seedHeldKeys();
}

void InputHandler::bindKey(sf::Keyboard::Key key, std::unique_ptr<Command> command) {
    m_keyBindings[key] = std::move(command);
}

void InputHandler::seedHeldKeys() {
    for (auto& [key, command] : m_keyBindings) {
        if (sf::Keyboard::isKeyPressed(key)) {
            m_heldKeys.insert(key);
        }
    }
    for (auto k : m_jumpKeys) {
        if (sf::Keyboard::isKeyPressed(k)) {
            m_heldKeys.insert(k);
        }
    }
    for (auto k : m_sprintKeys) {
        if (sf::Keyboard::isKeyPressed(k)) {
            m_heldKeys.insert(k);
        }
    }
}

void InputHandler::clearHeldKeys() {
    m_heldKeys.clear();
}

std::vector<Command*> InputHandler::handleInput() {
    std::vector<Command*> commands;
    for (auto key : m_heldKeys) {
        auto it = m_keyBindings.find(key);
        if (it != m_keyBindings.end()) {
            commands.push_back(it->second.get());
        }
    }
    return commands;
}

bool InputHandler::isJumpHeld() const {
    for (auto k : m_jumpKeys) {
        if (m_heldKeys.count(k)) return true;
    }
    return false;
}

bool InputHandler::isSprintHeld() const {
    for (auto k : m_sprintKeys) {
        if (m_heldKeys.count(k)) return true;
    }
    return false;
}

Command* InputHandler::handleEvent(const sf::Event& event) {
    if (event.type == sf::Event::KeyPressed) {
        m_heldKeys.insert(event.key.code);
        auto it = m_pressBindings.find(event.key.code);
        if (it != m_pressBindings.end()) {
            return it->second.get();
        }
    } else if (event.type == sf::Event::KeyReleased) {
        m_heldKeys.erase(event.key.code);
    } else if (event.type == sf::Event::LostFocus) {
        m_heldKeys.clear();
    }
    return nullptr;
}
