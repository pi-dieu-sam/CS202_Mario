#include "AI/ChaseStrategy.hpp"
#include "Entities/Enemy.hpp"
#include <cmath>

ChaseStrategy::ChaseStrategy(float detectionRadius)
    : m_detectionRadius(detectionRadius) {}

void ChaseStrategy::execute(Enemy& enemy, float dt) {
    sf::Vector2f enemyPos = enemy.getPosition();
    float dx = m_playerPos.x - enemyPos.x;
    float distance = std::abs(dx);

    if (distance < m_detectionRadius) {
        // Chase toward player
        float dir = (dx > 0) ? 1.0f : -1.0f;
        enemy.setVelocity(dir * enemy.getSpeed() * 1.3f, enemy.getVelocity().y);
    } else {
        // Patrol when player is far
        sf::Vector2f vel = enemy.getVelocity();
        if (vel.x == 0.0f) {
            enemy.setVelocity(-enemy.getSpeed(), vel.y);
        }
    }
}

void ChaseStrategy::setPlayerPosition(const sf::Vector2f& playerPos) {
    m_playerPos = playerPos;
}