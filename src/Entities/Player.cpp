#include "Entities/Player.hpp"
#include "Core/Game.hpp"
#include "Graphics/SpriteRegistry.hpp"
#include "Observers/EventManager.hpp"
#include "Physics/PhysicsConstants.hpp"
#include <cmath>

namespace {
constexpr float DEATH_INITIAL_VELOCITY = -300.0f;
constexpr float DEATH_OFFSCREEN_MARGIN = TILE_SIZE;
constexpr float DEATH_PAUSE_DURATION = 0.35f;
}

Player::Player() {
  m_type = ObjectType::Player;
  m_lives = STARTING_LIVES;
}

void Player::update(float dt) {
  if (m_dead) {
    updateDeathAnimation(dt);
    return;
  }

  // Apply gravity
  applyGravity(dt);

  // Apply friction when no horizontal input this frame
  applyFriction();

  // Move
  m_position += m_velocity * dt;

  // Invincibility timer (post-damage blink)
  if (m_invincible) {
    m_invincibleTimer -= dt;
    m_blinkTimer += dt;
    if (m_blinkTimer >= 0.08f) {
      m_visible = !m_visible;
      m_blinkTimer = 0.0f;
    }
    if (m_invincibleTimer <= 0.0f) {
      m_invincible = false;
      m_visible = true;
    }
  }

  // Star power timer
  if (m_starPower) {
    m_starTimer -= dt;
    if (m_starTimer <= 0.0f) {
      m_starPower = false;
    }
  }

  // Reset sprint flag (re-set by SprintCommand each frame if held)
  m_sprinting = false;

  // ── Decide animation state ──
  SpriteRegistry::PlayerAnim anim;
  if (!m_grounded) {
    anim = SpriteRegistry::PlayerAnim::Jump;
  } else if (m_skidding) {
    anim = SpriteRegistry::PlayerAnim::Skid;
  } else if (std::abs(m_velocity.x) > 5.0f) {
    anim = SpriteRegistry::PlayerAnim::Walk;
  } else {
    anim = SpriteRegistry::PlayerAnim::Idle;
  }
  m_currentAnim = anim;
  setAnimFrameCount(
      SpriteRegistry::playerFrameCount(m_characterId, m_powerUp, anim));

  updateSprite(dt);
}

void Player::draw(sf::RenderWindow &window) {
  if (!m_active)
    return;

  if (m_dead) {
    // Death artwork is a front-facing pose, so it must never inherit the
    // gameplay left/right flip. The animation itself moves the player.
    drawSprite(window, SpriteRegistry::playerDeathPath(m_characterId),
               getDeathBounds());
    return;
  }

  if (!m_visible)
    return;

  sf::Color tint = sf::Color::White;
  if (m_starPower) {
    // Rainbow flash effect
    static float hue = 0;
    hue += 5.0f;
    if (hue > 360)
      hue -= 360;
    int r = static_cast<int>(128 + 127 * std::sin(hue * 3.14159f / 180.0f));
    int g =
        static_cast<int>(128 + 127 * std::sin((hue + 120) * 3.14159f / 180.0f));
    int b =
        static_cast<int>(128 + 127 * std::sin((hue + 240) * 3.14159f / 180.0f));
    tint = sf::Color(r, g, b);
  }

  std::string path = SpriteRegistry::playerPath(m_characterId, m_powerUp,
                                                 m_currentAnim, m_animFrame);
  drawSprite(window, path, getBounds(), tint);
}

sf::FloatRect Player::getBounds() const {
  float height = (m_powerUp == PowerUpState::Small) ? TILE_SIZE : TILE_SIZE * 2;
  return sf::FloatRect(m_position.x + 2,
                       m_position.y + (TILE_SIZE * 2 - height), TILE_SIZE - 4,
                       height - 2);
}

float Player::getEffectiveSpeed() const {
  return m_sprinting ? m_speed * PLAYER_SPRINT : m_speed;
}

// ── Power-ups ──
void Player::applyPowerUp(PowerUpState state) { m_powerUp = state; }

PowerUpState Player::getPowerUpState() const { return m_powerUp; }

void Player::growBig() {
  if (m_powerUp == PowerUpState::Small) {
    m_powerUp = PowerUpState::Big;
  }
}

void Player::shrink() {
  if (m_powerUp != PowerUpState::Small) {
    m_powerUp = PowerUpState::Small;
    setInvincibleTimer(INVINCIBILITY_DUR);
  }
}

void Player::enableFire() { m_powerUp = PowerUpState::Fire; }

// ── Shooting ──
void Player::shoot() {
  if (m_powerUp == PowerUpState::Fire) {
    m_wantsToShoot = true;
  }
}

bool Player::wantsToShoot() const { return m_wantsToShoot; }
void Player::clearShootFlag() { m_wantsToShoot = false; }

// ── Sprint ──
void Player::setSprinting(bool sprinting) { m_sprinting = sprinting; }
bool Player::isSprinting() const { return m_sprinting; }

// ── Jump input state ──
void Player::setJumpHeld(bool held) { m_jumpHeld = held; }
bool Player::isJumpHeld() const { return m_jumpHeld; }

// ── Damage override ──
void Player::takeDamage(int amount) {
  if (m_invincible || m_starPower)
    return;

  if (m_powerUp == PowerUpState::Fire) {
    shrink(); // Fire → Small
  } else if (m_powerUp == PowerUpState::Big) {
    shrink(); // Big → Small
  } else {
    die();
  }

  EventManager::getInstance().publish({EventType::PlayerDamaged});
}

void Player::die() {
  if (m_dead)
    return;

  m_dead = true;
  m_grounded = false;
  m_sprinting = false;
  m_visible = true;
  m_deathAnimationPhase = DeathAnimationPhase::Rising;
  m_deathPauseTimer = 0.0f;
  // The camera is vertically fixed, so this world-space target puts the
  // entire death pose below the viewport before the pause begins.
  m_deathPauseY = static_cast<float>(WINDOW_HEIGHT) + DEATH_OFFSCREEN_MARGIN;
  m_velocity = {0.0f, DEATH_INITIAL_VELOCITY};
  EventManager::getInstance().publish({EventType::PlayerDied});
}

bool Player::isDeathAnimationComplete() const {
  return m_deathAnimationPhase == DeathAnimationPhase::Complete;
}

void Player::updateDeathAnimation(float dt) {
  switch (m_deathAnimationPhase) {
  case DeathAnimationPhase::Rising:
  case DeathAnimationPhase::Falling:
    applyGravity(dt);
    m_position += m_velocity * dt;

    if (m_deathAnimationPhase == DeathAnimationPhase::Rising &&
        m_velocity.y >= 0.0f) {
      m_deathAnimationPhase = DeathAnimationPhase::Falling;
    }

    if (m_deathAnimationPhase == DeathAnimationPhase::Falling &&
        m_position.y >= m_deathPauseY) {
      m_position.y = m_deathPauseY;
      m_velocity = {0.0f, 0.0f};
      m_deathPauseTimer = DEATH_PAUSE_DURATION;
      m_deathAnimationPhase = DeathAnimationPhase::Paused;
    }
    break;

  case DeathAnimationPhase::Paused:
    m_deathPauseTimer -= dt;
    if (m_deathPauseTimer <= 0.0f) {
      m_deathAnimationPhase = DeathAnimationPhase::Complete;
    }
    break;

  case DeathAnimationPhase::None:
  case DeathAnimationPhase::Complete:
    break;
  }
}

sf::FloatRect Player::getDeathBounds() const {
  // Death is represented by the small, front-facing pose even if the player
  // was Big/Fire. Keep the feet aligned with the normal player anchor.
  return sf::FloatRect(m_position.x + 2.0f, m_position.y + TILE_SIZE,
                       TILE_SIZE - 4.0f, TILE_SIZE - 2.0f);
}

// ── Lives ──
int Player::getLives() const { return m_lives; }
void Player::setLives(int lives) { m_lives = lives; }
void Player::loseLife() { m_lives--; }

// ── Invincibility ──
bool Player::isInvincible() const { return m_invincible; }
void Player::setInvincibleTimer(float seconds) {
  m_invincible = true;
  m_invincibleTimer = seconds;
}

// ── Star ──
bool Player::hasStarPower() const { return m_starPower; }
void Player::setStarPower(float duration) {
  m_starPower = true;
  m_starTimer = duration;
}
