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
constexpr float FLAGPOLE_SLIDE_SPEED = 180.0f;
constexpr float FLAGPOLE_ANIM_SPEED = 0.12f;

// Shoot (Fire) animation: plays once for this long after a fireball is fired,
// then reverts to the normal Idle/Walk/Jump state.
constexpr float SHOOT_ANIM_DURATION = 0.35f;
// Per-animation frame rates (seconds per frame) for the new Mario sheets.
constexpr float WALK_ANIM_SPEED = 0.08f;
constexpr float JUMP_ANIM_SPEED = 0.15f;
constexpr float IDLE_ANIM_SPEED = 0.16f;
constexpr float FIRE_ANIM_SPEED = 0.12f;

// FlowersBuff parameters
constexpr float BUFF_DURATION = 40.0f;      // seconds the buff lasts
constexpr float BUFF_GROW_DURATION = 0.7f;  // seconds to ramp size 1.0 -> 1.5
constexpr float BUFF_SCALE_MAX = 1.5f;      // final size multiplier
constexpr float BUFF_SCALE_STEP = 0.1f;     // 1.1, 1.2, 1.3, 1.4, 1.5
constexpr float BUFF_STEP_INTERVAL =
    BUFF_GROW_DURATION / ((BUFF_SCALE_MAX - 1.0f) / BUFF_SCALE_STEP);
constexpr float BUFF_SPEED_BONUS = 0.2f;    // +0.2 to the speed multiplier
constexpr float BUFF_JUMP_BONUS = 0.2f;     // +0.2 to the jump multiplier
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

  if (m_goalAnimationPhase == GoalAnimationPhase::Sliding) {
    updateFlagpoleSlide(dt);
    return;
  }

  if (m_goalAnimationPhase == GoalAnimationPhase::SlideComplete) {
    // Hold the player at the pole base until PlayingState starts the castle
    // walk. This is still a cutscene, so normal gravity must not resume.
    m_position.x = m_flagpoleSlideX;
    m_position.y = m_flagpoleSlideTargetY;
    m_velocity = {0.0f, 0.0f};
    m_grounded = false;
    return;
  }

  if (m_goalAnimationPhase == GoalAnimationPhase::CastleWalk) {
    updateFlagpoleCastleWalk(dt);
    return;
  }

  // FlowersBuff: ramp size 1.0 -> 1.5 in 0.1 steps over 0.7s, then count
  // down the 40s duration and revert to normal size when it expires.
  if (m_growing) {
    m_growTimer += dt;
    int step = static_cast<int>(m_growTimer / BUFF_STEP_INTERVAL);
    if (step >= static_cast<int>((BUFF_SCALE_MAX - 1.0f) / BUFF_SCALE_STEP)) {
      m_growing = false;
      m_sizeScale = BUFF_SCALE_MAX;
      m_buffTimer = BUFF_DURATION;
    } else {
      m_sizeScale = 1.0f + BUFF_SCALE_STEP * step;
    }
  } else if (m_buffTimer > 0.0f) {
    m_buffTimer -= dt;
    if (m_buffTimer <= 0.0f) {
      m_buffTimer = 0.0f;
      m_sizeScale = 1.0f;
    }
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
  float animSpeed = -1.0f;
  if (m_shootAnimTimer > 0.0f) {
    // Mario's Fire sheet plays briefly after each shot, even mid-air.
    m_shootAnimTimer -= dt;
    anim = SpriteRegistry::PlayerAnim::Fire;
    animSpeed = FIRE_ANIM_SPEED;
  } else if (!m_grounded) {
    anim = SpriteRegistry::PlayerAnim::Jump;
    animSpeed = JUMP_ANIM_SPEED;
  } else if (m_skidding) {
    anim = SpriteRegistry::PlayerAnim::Skid;
    animSpeed = IDLE_ANIM_SPEED;
  } else if (std::abs(m_velocity.x) > 5.0f) {
    anim = SpriteRegistry::PlayerAnim::Walk;
    animSpeed = WALK_ANIM_SPEED;
  } else {
    anim = SpriteRegistry::PlayerAnim::Idle;
    animSpeed = IDLE_ANIM_SPEED;
  }
  m_currentAnim = anim;
  setAnimFrameCount(
      SpriteRegistry::playerFrameCount(m_characterId, m_powerUp, anim),
      animSpeed);

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

  if (m_goalAnimationPhase == GoalAnimationPhase::Sliding ||
      m_goalAnimationPhase == GoalAnimationPhase::SlideComplete) {
    SpriteRegistry::applyPlayerFlagpoleSlideFrame(
        m_sprite, m_characterId, m_powerUp, m_animFrame, getBounds(), false);
    window.draw(m_sprite);
    return;
  }

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

  SpriteRegistry::applyPlayerFrame(m_sprite, m_characterId, m_powerUp,
                                   m_currentAnim, m_animFrame, getBounds(),
                                   !m_facingRight);
  m_sprite.setColor(tint);
  window.draw(m_sprite);
}

sf::FloatRect Player::getBounds() const {
  float baseHeight = (m_powerUp == PowerUpState::Small) ? TILE_SIZE : TILE_SIZE * 2;
  float baseWidth = TILE_SIZE - 4;
  float h = baseHeight * m_sizeScale;
  float w = baseWidth * m_sizeScale;
  // Keep the feet anchored where they were pre-buff (m_position.y + 2*TILE - 2)
  // and center the growth horizontally so the box scales "literally".
  float top = m_position.y + TILE_SIZE * 2 - h;
  float left = m_position.x + 2 - (w - baseWidth) / 2.0f;
  return sf::FloatRect(left, top, w, h - 2);
}

float Player::getEffectiveSpeed() const {
  float base = m_sprinting ? m_speed * PLAYER_SPRINT : m_speed;
  if (hasSizeBuff()) {
    base += PLAYER_SPEED * BUFF_SPEED_BONUS; // +0.2 speed multiplier
  }
  return base;
}

void Player::jump() {
  if (m_grounded) {
    m_velocity.y = m_jumpForce;
    if (hasSizeBuff()) {
      m_velocity.y += PLAYER_JUMP * BUFF_JUMP_BONUS; // +0.2 jump multiplier
    }
    m_grounded = false;
  }
}

// ── Size buff (FlowersBuff) ──
void Player::applySizeBuff() {
  if (m_growing)
    return; // already ramping up
  if (m_buffTimer > 0.0f) {
    m_buffTimer = BUFF_DURATION; // refresh the remaining duration
    return;
  }
  m_growing = true;
  m_growTimer = 0.0f;
}

float Player::getSizeScale() const { return m_sizeScale; }

bool Player::hasSizeBuff() const { return m_growing || m_buffTimer > 0.0f; }

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
  // Fireballs are always available, no Fire power-up required.
  m_wantsToShoot = true;
  // Only Mario has a dedicated shoot pose (Mario_Fire.png sheet).
  if (m_characterId == CharacterId::Mario) {
    m_shootAnimTimer = SHOOT_ANIM_DURATION;
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

// ── Goal cutscene ──
void Player::beginFlagpoleSlide(float poleCenterX, float landingY) {
  // Player::getBounds() is 28px wide. Place its right edge at the centre of
  // the 6px pole so the character's outstretched hands meet it while facing
  // right, just as the player approaches it during regular gameplay.
  m_flagpoleSlideX = poleCenterX - (TILE_SIZE - 2.0f);
  m_flagpoleSlideTargetY = landingY;
  m_position.x = m_flagpoleSlideX;
  m_velocity = {0.0f, 0.0f};
  m_grounded = false;
  m_sprinting = false;
  m_wantsToShoot = false;
  m_facingRight = true;
  m_goalAnimationPhase = GoalAnimationPhase::Sliding;
  m_currentAnim = SpriteRegistry::PlayerAnim::FlagpoleSlide;
  m_animFrames = SpriteRegistry::playerFlagpoleSlideFrameCount(
      m_characterId, m_powerUp);
  m_animFrame = 0;
  m_animTimer = 0.0f;
  m_animSpeed = FLAGPOLE_ANIM_SPEED;
}

void Player::beginFlagpoleCastleWalk() {
  m_goalAnimationPhase = GoalAnimationPhase::CastleWalk;
  m_velocity = {0.0f, 0.0f};
  m_grounded = false;
  m_facingRight = true;
  m_currentAnim = SpriteRegistry::PlayerAnim::Walk;
  m_animFrames = SpriteRegistry::playerFrameCount(
      m_characterId, m_powerUp, m_currentAnim);
  m_animFrame = 0;
  m_animTimer = 0.0f;
  m_animSpeed = FLAGPOLE_ANIM_SPEED;
}

bool Player::isFlagpoleSlideComplete() const {
  return m_goalAnimationPhase == GoalAnimationPhase::SlideComplete;
}

bool Player::isFlagpoleCutsceneActive() const {
  return m_goalAnimationPhase != GoalAnimationPhase::None;
}

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

  m_goalAnimationPhase = GoalAnimationPhase::None;
  m_dead = true;
  m_grounded = false;
  m_sprinting = false;
  m_visible = true;
  // FlowersBuff does not survive death/respawn.
  m_growing = false;
  m_growTimer = 0.0f;
  m_buffTimer = 0.0f;
  m_sizeScale = 1.0f;
  m_deathAnimationPhase = DeathAnimationPhase::Rising;
  m_deathPauseTimer = 0.0f;
  // The camera is vertically fixed, so this world-space target puts the
  // entire death pose below the viewport before the pause begins.
  m_deathPauseY = static_cast<float>(WINDOW_HEIGHT) + DEATH_OFFSCREEN_MARGIN;
  m_velocity = {0.0f, DEATH_INITIAL_VELOCITY};
  GameEvent evt;
  evt.type     = EventType::PlayerDied;
  evt.playerId = m_playerId;
  EventManager::getInstance().publish(evt);
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

void Player::updateFlagpoleSlide(float dt) {
  m_position.x = m_flagpoleSlideX;
  m_velocity = {0.0f, 0.0f};
  m_grounded = false;
  m_sprinting = false;
  m_wantsToShoot = false;
  m_facingRight = true;
  m_currentAnim = SpriteRegistry::PlayerAnim::FlagpoleSlide;
  setAnimFrameCount(
      SpriteRegistry::playerFlagpoleSlideFrameCount(m_characterId, m_powerUp),
      FLAGPOLE_ANIM_SPEED);
  updateSprite(dt);

  const float remainingY = m_flagpoleSlideTargetY - m_position.y;
  const float movement = FLAGPOLE_SLIDE_SPEED * dt;
  if (std::abs(remainingY) <= movement) {
    m_position.y = m_flagpoleSlideTargetY;
    m_goalAnimationPhase = GoalAnimationPhase::SlideComplete;
  } else {
    m_position.y += remainingY > 0.0f ? movement : -movement;
  }
}

void Player::updateFlagpoleCastleWalk(float dt) {
  m_velocity = {0.0f, 0.0f};
  m_grounded = false;
  m_sprinting = false;
  m_wantsToShoot = false;
  m_facingRight = true;
  m_currentAnim = SpriteRegistry::PlayerAnim::Walk;
  setAnimFrameCount(
      SpriteRegistry::playerFrameCount(m_characterId, m_powerUp, m_currentAnim),
      FLAGPOLE_ANIM_SPEED);
  updateSprite(dt);
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
