#pragma once
// Koopa.hpp — shelled enemy with walking, shell, and sliding states
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

    KoopaState getKoopaState() const noexcept;
    bool isSliding() const noexcept;

    /// Kick the shell in the given direction (-1 = left, +1 = right).
    void kick(float direction);
    /// Bounce a sliding shell away from a horizontal obstacle.  The rebound
    /// slows down over roughly four tiles before the shell comes to rest.
    void bounce(float incomingVelocity);
    /// Stop the sliding shell (for example, when the player stomps it).
    void stopSliding();

private:
    KoopaState m_koopaState = KoopaState::Walking;
    bool       m_sliding    = false;
    bool       m_brakingAfterWall = false;
    float      m_dieTimer   = 0.0f;
    float      m_shellSpeed = 300.0f;
};
