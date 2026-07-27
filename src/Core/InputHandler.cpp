#include "Core/InputHandler.hpp"

InputHandler::InputHandler() 
    : m_btnLeft(std::make_unique<MoveLeftCommand>()),
      m_btnRight(std::make_unique<MoveRightCommand>()),
      m_btnJump(std::make_unique<JumpCommand>()),
      m_btnDuck(std::make_unique<DuckCommand>()),
      m_btnShoot(std::make_unique<ShootCommand>()),
      m_btnStop(std::make_unique<StopMoveCommand>()) {}

Command* InputHandler::handleRealtimeInput() {
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A) || 
        sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left)) {
        return m_btnLeft.get();
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D) || 
        sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) {
        return m_btnRight.get();
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S) || 
        sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down)) {
        return m_btnDuck.get();
    }
    return m_btnStop.get();
}

Command* InputHandler::handleEventInput(sf::Keyboard::Key key) {
    if (key == sf::Keyboard::Key::Space || key == sf::Keyboard::Key::W || key == sf::Keyboard::Key::Up || key == sf::Keyboard::Key::Z) {
        return m_btnJump.get();
    }
    if (key == sf::Keyboard::Key::LShift || key == sf::Keyboard::Key::RShift || key == sf::Keyboard::Key::X) {
        return m_btnShoot.get();
    }
    return nullptr;
}
