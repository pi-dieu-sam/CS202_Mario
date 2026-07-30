#include "AI/PatrolStrategy.hpp"
#include "Entities/Enemy.hpp"

void PatrolStrategy::execute(Enemy& enemy, float dt) {
    // Simple patrol: walk at constant speed, direction set by velocity
    sf::Vector2f vel = enemy.getVelocity();

    if (vel.x == 0.0f) {
        // Start moving left by default
        enemy.setVelocity(-enemy.getSpeed(), vel.y);
    }
    // Direction reversal happens on collision with tiles (handled by CollisionDetector)
}