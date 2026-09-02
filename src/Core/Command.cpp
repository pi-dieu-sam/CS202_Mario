#include "Core/Command.hpp"
#include "Core/SoundManager.hpp"
#include "Entities/Player.hpp"

void MoveLeftCommand::execute(Player& player, float dt) {
    player.moveLeft(dt);
}

void MoveRightCommand::execute(Player& player, float dt) {
    player.moveRight(dt);
}

void ClimbUpCommand::execute(Player& player, float dt) {
    player.climbUp(dt);
}

void ClimbDownCommand::execute(Player& player, float dt) {
    player.climbDown(dt);
}

void JumpCommand::execute(Player& player, float dt) {
    if (player.isGrounded()) {
        SoundManager::getInstance().playSound(SoundID::Jump);
    }
    player.jump();
}

void SprintCommand::execute(Player& player, float dt) {
    player.setSprinting(true);
}

void FireCommand::execute(Player& player, float dt) {
    // The sound now plays where the fireball is actually spawned
    // (PlayingState::update), not here where a shot is merely requested.
    player.shoot();
}
