#include "Core/Command.hpp"
#include "Entities/Player.hpp"

void MoveLeftCommand::execute(Player& player, float dt) {
    player.moveLeft(dt);
}

void MoveRightCommand::execute(Player& player, float dt) {
    player.moveRight(dt);
}

void JumpCommand::execute(Player& player, float dt) {
    player.jump();
}

void SprintCommand::execute(Player& player, float dt) {
    player.setSprinting(true);
}

void FireCommand::execute(Player& player, float dt) {
    player.shoot();
}
