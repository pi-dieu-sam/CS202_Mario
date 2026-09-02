#pragma once
#include "GameObject.hpp"
#include <SFML/Graphics.hpp>
#include <string>

/// Character — Abstract base class for all characters (player and enemies).
/// Adds health, movement physics, animation state, and facing direction.
class Character : public GameObject {
    friend class SnapshotAccess;
public:
    Character();
    virtual ~Character() = default;

    // ── Movement ──
    virtual void moveLeft(float dt);
    virtual void moveRight(float dt);
    virtual void jump();
    virtual void stopMoving();

    // ── Damage ──
    virtual void takeDamage(int amount = 1);
    virtual void die();
    bool         isDead() const;

    // ── Physics helpers ──
    void applyGravity(float dt);
    virtual void applyFriction();

    /// Horizontal speed moveLeft()/moveRight() should move at this frame.
    /// Base implementation is just m_speed; Player overrides it to fold in
    /// the sprint multiplier so sprinting only ever affects velocity while
    /// there's active directional input (see Player::getEffectiveSpeed).
    virtual float getEffectiveSpeed() const;

    // ── Ground state ──
    bool isGrounded() const;
    void setGrounded(bool grounded);

    // ── Direction ──
    bool isFacingRight() const;

    // ── Stats ──
    float getSpeed() const;
    void  setSpeed(float speed);
    float getJumpForce() const;
    void  setJumpForce(float force);
    int   getHealth() const;
    void  setHealth(int hp);

protected:
    /// Advance the animation frame timer (call every update()).
    void updateSprite(float dt);

    /// Change the current animation's frame count, resetting frame/timer so
    /// a stale frame index from the previous animation state can't produce
    /// a modulo artifact. Call whenever the logical animation state changes
    /// (idle→walk, walk→jump, etc.), not every frame.
    void setAnimFrameCount(int frames, float speed = -1.0f);

    /// Draw m_sprite textured from the file at path, fit to box (bottom-center
    /// pivot, auto-scaled, flipped horizontally when facing left), tinted
    /// by tint (default: untinted).
    void drawSprite(sf::RenderWindow &window, const std::string &path,
                     const sf::FloatRect &box,
                     sf::Color tint = sf::Color::White);

    float m_speed       = 200.0f;
    float m_jumpForce   = -420.0f;
    int   m_health      = 1;
    bool  m_grounded    = false;
    bool  m_facingRight = true;
    bool  m_dead        = false;
    bool  m_skidding    = false; // was moving one way, told to move the other

    // Sprite & animation
    sf::Sprite   m_sprite;
    float        m_animTimer  = 0.0f;
    int          m_animFrame  = 0;
    int          m_animFrames = 1;
    float        m_animSpeed  = 0.1f;
};
