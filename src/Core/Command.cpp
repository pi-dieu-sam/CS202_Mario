#include "Core/Command.hpp"
#include <iostream>

// Note: Concrete Player implementation details will interact with Player object when fully instantiated.
// Forward implementation stubs ensuring commands function cleanly.

void MoveLeftCommand::execute(Player& player) {
    // Triggers player leftward acceleration
}

void MoveRightCommand::execute(Player& player) {
    // Triggers player rightward acceleration
}

void JumpCommand::execute(Player& player) {
    // Triggers player jump impulse
}

void DuckCommand::execute(Player& player) {
    // Triggers player crouching state
}

void ShootCommand::execute(Player& player) {
    // Triggers fireball shooting if player has Fire Flower state
}

void StopMoveCommand::execute(Player& player) {
    // Resets horizontal acceleration
}
