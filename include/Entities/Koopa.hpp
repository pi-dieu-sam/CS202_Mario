#pragma once
#include "Enemy.hpp"

/// Koopa states after being stomped.
enum class KoopaState {
    Walking,  // Normal patrol
    Shell,    // In shell, stationary
    Sliding   // Shell sliding after kicked
};

/// Koopa — retreats into shell on stomp; shell can slide and defeat other enemies.
class Koopa : public Enemy {
public:
    Koopa();

    void onStomped() override;
    void update(float dt) override;
    void draw(sf::RenderWindow& window) override;

    KoopaState getKoopaState() const;

    /// Kick the shell in the given direction (1 = right, -1 = left).
    void kickShell(int direction);

private:
    KoopaState m_koopaState = KoopaState::Walking;
    float      m_shellTimer = 0.0f;  // Time before shell Koopa recovers
};
