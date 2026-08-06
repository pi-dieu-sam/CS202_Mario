#include "AI/PatrolStrategy.hpp"
#include "Entities/Enemy.hpp"

void PatrolStrategy::execute(Enemy& enemy, float dt) {
    // Simple patrol: walk at constant speed, direction set by velocity
    sf::Vector2f vel = enemy.getVelocity();

    if (vel.x == 0.0f) {
        // Fallback for an enemy that spawns with vel.x == 0; not relied upon
        // for wall-bounce reversal. Start moving left by default.
        enemy.setVelocity(-enemy.getSpeed(), vel.y);
    }
    // Direction reversal on wall hits happens entirely in
    // Level::handleCollisions() via CollisionDetector::reflectHorizontalVelocity()
    // (see issue #23) — this strategy no longer participates in it.
}