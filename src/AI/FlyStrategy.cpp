#include "AI/FlyStrategy.hpp"
#include "Entities/Enemy.hpp"
#include <cmath>

FlyStrategy::FlyStrategy(float amplitude, float frequency)
    : m_amplitude(amplitude), m_frequency(frequency) {}

void FlyStrategy::execute(Enemy& enemy, float dt) {
    m_time += dt;

    sf::Vector2f vel = enemy.getVelocity();
    // Horizontal patrol
    if (vel.x == 0.0f) {
        vel.x = -enemy.getSpeed();
    }

    // Sinusoidal vertical movement (override gravity)
    vel.y = m_amplitude * std::cos(m_time * m_frequency * 2.0f * 3.14159f);

    enemy.setVelocity(vel);
    enemy.setGrounded(false); // Flying enemies are never grounded
}