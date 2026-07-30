#pragma once
#include "Enemy.hpp"

/// Goomba — basic walking enemy. Dies on stomp.
class Goomba : public Enemy {
public:
    Goomba();

    void onStomped() override;
    void update(float dt) override;
    void draw(sf::RenderWindow& window) override;

private:
    // Squish animation on death
    float m_deathTimer = 0.0f;
    bool  m_squished   = false;
};
