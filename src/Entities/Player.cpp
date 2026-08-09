#include "Entities/Player.hpp"
#include "Core/Game.hpp"
#include "Graphics/SpriteRegistry.hpp"
#include "Observers/EventManager.hpp"
#include "Physics/PhysicsConstants.hpp"
#include <cmath>

Player::Player() {
  m_type = ObjectType::Player;
  m_lives = STARTING_LIVES;
}

void Player::update(float dt) {
  if (m_dead)
    return;

  // Apply gravity
  applyGravity(dt);

  // Apply friction when no horizontal input this frame
  applyFriction();

  // Sprint multiplier
  if (m_sprinting) {
    m_velocity.x *= PLAYER_SPRINT;
  }

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
  if (!m_active || !m_visible)
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
  m_dead = true;
  m_velocity = {0.0f, -300.0f}; // Jump-up death animation
  EventManager::getInstance().publish({EventType::PlayerDied});
}

// ── Score & lives ──
int Player::getScore() const { return m_score; }
void Player::addScore(int points) { m_score += points; }
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
