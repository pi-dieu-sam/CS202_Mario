#include "Entities/Koopa.hpp"
#include "AI/PatrolStrategy.hpp"
#include "Graphics/SpriteRegistry.hpp"
#include "Physics/PhysicsConstants.hpp"
#include <cmath>

namespace {
constexpr float SHELL_RESPAWN_TIME = 5.0f;
constexpr float WALL_BOUNCE_SPEED = 120.0f;
constexpr float WALL_BOUNCE_DISTANCE = TILE_SIZE * 4.0f;
// v^2 = u^2 + 2as: exactly brings a WALL_BOUNCE_SPEED rebound to rest
// after WALL_BOUNCE_DISTANCE, independent of the fixed update rate.
constexpr float WALL_BRAKE_DECELERATION =
    (WALL_BOUNCE_SPEED * WALL_BOUNCE_SPEED) / (2.0f * WALL_BOUNCE_DISTANCE);
}

Koopa::Koopa() {
  m_speed = 60.0f;
  m_scoreValue = 200;
  m_velocity.x = -m_speed;

  setAnimFrameCount(20, 0.08f);
}

void Koopa::onStomped() {
  switch (m_koopaState) {
  case KoopaState::Walking:
    m_koopaState = KoopaState::Shell;
    m_sliding = false;
    m_brakingAfterWall = false;
    m_dieTimer = SHELL_RESPAWN_TIME;
    m_velocity = {0.0f, 0.0f};
    m_strategy = nullptr;
    break;

  case KoopaState::Shell:
    if (m_sliding) {
      // Stomp stops a sliding shell
      m_sliding = false;
      m_brakingAfterWall = false;
      m_velocity.x = 0.0f;
      m_dieTimer = SHELL_RESPAWN_TIME;
    } else {
      // Second stomp on a sitting shell — resume walking
      m_koopaState = KoopaState::Walking;
      m_velocity.x = -m_speed;
      m_facingRight = false;
      setStrategy(std::make_unique<PatrolStrategy>());
    }
    break;
  }
}

void Koopa::kill() {
  if (m_koopaState == KoopaState::Walking) {
    m_koopaState = KoopaState::Shell;
    m_sliding = false;
    m_brakingAfterWall = false;
    m_dieTimer = SHELL_RESPAWN_TIME;
    m_velocity = {0.0f, 0.0f};
    m_strategy = nullptr;
    m_dead = false;
  }
}

void Koopa::update(float dt) {
  if (m_koopaState == KoopaState::Shell) {
    // The shell's respawn clock continues even when it is moving.  Every
    // kick refreshes this clock, so a kicked shell cannot slide forever.
    m_dieTimer -= dt;
    if (m_dieTimer <= 0.0f) {
      m_koopaState = KoopaState::Walking;
      m_sliding = false;
      m_brakingAfterWall = false;
      m_velocity.x = -m_speed;
      m_facingRight = false;
      setStrategy(std::make_unique<PatrolStrategy>());
      return;
    }

    if (m_sliding && m_brakingAfterWall) {
      const float speed = std::abs(m_velocity.x);
      const float speedLoss = WALL_BRAKE_DECELERATION * dt;
      if (speed <= speedLoss) {
        m_velocity.x = 0.0f;
        m_sliding = false;
        m_brakingAfterWall = false;
      } else {
        m_velocity.x -= std::copysign(speedLoss, m_velocity.x);
      }
    }
    applyGravity(dt);
    m_position += m_velocity * dt;
    return;
  }

  // Walking state — normal enemy behavior
  Enemy::update(dt);
}

void Koopa::draw(sf::RenderWindow &window) {
  if (!m_active)
    return;

  if (m_koopaState == KoopaState::Walking) {
    sf::FloatRect box(m_position.x + 1, m_position.y - TILE_SIZE * 0.5f + 1,
                       TILE_SIZE - 2, TILE_SIZE * 1.5f - 2);
    SpriteRegistry::applyKoopaFrame(m_sprite, m_animFrame, box, m_facingRight);
    window.draw(m_sprite);
  } else {
    // Shell (sitting or sliding)
    sf::FloatRect box(m_position.x + 1, m_position.y + 3, TILE_SIZE - 2,
                       TILE_SIZE - 4);
    drawSprite(window, SpriteRegistry::koopaDiePath(), box);
  }
}

bool Koopa::isVulnerable() const {
  return m_koopaState == KoopaState::Walking;
}

KoopaState Koopa::getKoopaState() const noexcept { return m_koopaState; }

bool Koopa::isSliding() const noexcept { return m_sliding; }

void Koopa::kick(float direction) {
  // Only a stationary shell can be kicked; ignore if already sliding
  if (m_koopaState != KoopaState::Shell || m_sliding)
    return;
  m_sliding = true;
  m_brakingAfterWall = false;
  m_velocity.x = direction * m_shellSpeed;
  m_facingRight = direction > 0.0f;
  m_dieTimer = SHELL_RESPAWN_TIME;
}

void Koopa::bounce(float incomingVelocity) {
  if (m_koopaState != KoopaState::Shell || !m_sliding)
    return;

  // A wall impact is deliberately gentler than a kick.  update() applies a
  // constant brake so this rebound travels about four tiles, then stops.
  m_velocity.x = incomingVelocity >= 0.0f ? -WALL_BOUNCE_SPEED
                                           : WALL_BOUNCE_SPEED;
  m_brakingAfterWall = true;
  m_facingRight = m_velocity.x > 0.0f;
}

void Koopa::stopSliding() {
  // Only applies to a currently sliding shell
  if (m_koopaState != KoopaState::Shell || !m_sliding)
    return;
  m_sliding = false;
  m_brakingAfterWall = false;
  m_velocity.x = 0.0f;
  m_dieTimer = SHELL_RESPAWN_TIME;
}
