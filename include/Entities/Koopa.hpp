#pragma once
#include "Enemy.hpp"

/// Koopa states.
enum class KoopaState {
    Walking,  // Normal patrol
    Shell     // Shell on ground — sitting or sliding after being kicked
};

/// Koopa — walks back and forth; becomes a shell when stomped.
/// The shell can be kicked by the player and slides at high speed,
/// killing Goombas and turning other Koopas into shells on contact.
class Koopa : public Enemy {
public:
    Koopa();

    void onStomped() override;
    void kill() override;
    void update(float dt) override;
    void draw(sf::RenderWindow& window) override;
    bool isVulnerable() const override;

    KoopaState getKoopaState() const;
    bool isSliding() const;

    /// Kick the shell in the given direction (-1 = left, +1 = right).
    void kick(float direction);
    /// Stop the sliding shell (e.g. when it hits a wall).
    void stopSliding();

private:
    KoopaState m_koopaState = KoopaState::Walking;
    bool       m_sliding    = false;
    float      m_dieTimer   = 0.0f;
    float      m_shellSpeed = 300.0f;
};
