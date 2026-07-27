#ifndef COMMAND_HPP
#define COMMAND_HPP

// Forward declaration
class Player;

/**
 * @brief Command Pattern base interface for encapsulating player actions.
 */
class Command {
public:
    virtual ~Command() = default;
    virtual void execute(Player& player) = 0;
};

class MoveLeftCommand : public Command {
public:
    void execute(Player& player) override;
};

class MoveRightCommand : public Command {
public:
    void execute(Player& player) override;
};

class JumpCommand : public Command {
public:
    void execute(Player& player) override;
};

class DuckCommand : public Command {
public:
    void execute(Player& player) override;
};

class ShootCommand : public Command {
public:
    void execute(Player& player) override;
};

class StopMoveCommand : public Command {
public:
    void execute(Player& player) override;
};

#endif // COMMAND_HPP
