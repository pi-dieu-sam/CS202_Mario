#pragma once

// Forward declaration
class Player;

/// Command — Abstract base for the Command pattern.
/// Each concrete command encapsulates a player action.
class Command {
public:
    virtual ~Command() = default;
    virtual void execute(Player& player, float dt) = 0;
};

// ── Concrete Commands ──

class MoveLeftCommand : public Command {
public:
    void execute(Player& player, float dt) override;
};

class MoveRightCommand : public Command {
public:
    void execute(Player& player, float dt) override;
};

class JumpCommand : public Command {
public:
    void execute(Player& player, float dt) override;
};

class SprintCommand : public Command {
public:
    void execute(Player& player, float dt) override;
};

class FireCommand : public Command {
public:
    void execute(Player& player, float dt) override;
};
