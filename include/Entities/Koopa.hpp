#pragma once
#include "Enemy.hpp"

/// Koopa states.
enum class KoopaState {
    Walking,  // Normal patrol
    Dying     // Shown Koopa_Die.png, respawns after timer
};

/// Koopa — walks back and forth; respawns after being stomped or killed by fireball.
class Koopa : public Enemy {
public:
    Koopa();

    void onStomped() override;
    void kill() override;
    void update(float dt) override;
    void draw(sf::RenderWindow& window) override;
    bool isVulnerable() const override;

    KoopaState getKoopaState() const;

private:
    KoopaState m_koopaState = KoopaState::Walking;
    float      m_dieTimer   = 0.0f;
};
