#ifndef INPUT_HANDLER_HPP
#define INPUT_HANDLER_HPP

#include "Command.hpp"
#include <SFML/Window/Keyboard.hpp>
#include <memory>

/**
 * @brief Translates user keyboard and controller inputs into executable Commands.
 */
class InputHandler {
public:
    InputHandler();
    ~InputHandler() = default;

    /**
     * @brief Poll continuous key states and return command to execute.
     */
    Command* handleRealtimeInput();

    /**
     * @brief Handle discrete key press events (e.g., single jump trigger or shoot).
     */
    Command* handleEventInput(sf::Keyboard::Key key);

private:
    std::unique_ptr<Command> m_btnLeft;
    std::unique_ptr<Command> m_btnRight;
    std::unique_ptr<Command> m_btnJump;
    std::unique_ptr<Command> m_btnDuck;
    std::unique_ptr<Command> m_btnShoot;
    std::unique_ptr<Command> m_btnStop;
};

#endif // INPUT_HANDLER_HPP
