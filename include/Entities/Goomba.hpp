#pragma once
// Goomba.hpp — basic walking enemy that dies on stomp
#include "Enemy.hpp"

/// Goomba — basic walking enemy. Dies on stomp.
class Goomba : public Enemy {
    friend class SnapshotAccess;
public:
    Goomba();
    virtual ~Goomba() = default;

    void onStomped() override;
    void update(float dt) override;
    void draw(sf::RenderWindow& window) override;
    bool isVulnerable() const override;

private:
    // Keep the squished sprite drawable for a short time while the Goomba is
    // already dead and excluded from every collision path.
    float m_deathTimer = 0.0f;
    bool  m_squished = false;
};
