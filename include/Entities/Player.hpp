#pragma once
#include "Character.hpp"
#include "../Graphics/SpriteRegistry.hpp"

/// Power-up state for the player character.
enum class PowerUpState {
  Small, // Default — dies on hit
  Big,   // Mushroom — can break bricks, shrinks on hit
  Fire   // FireFlower — can shoot fireballs
};

/// Which character's sprite art to use (Mario/Luigi occupy separate
/// regions of the shared "Playable Characters" spritesheet).
enum class CharacterId {
  Mario,
  Luigi
};

/// Player — base class for player-controlled characters (Mario, Luigi).
/// Adds lives, score, power-up system, and invincibility after damage.
class Player : public Character {
public:
  Player();
  virtual ~Player() = default;

  void update(float dt) override;
  void draw(sf::RenderWindow &window) override;
  sf::FloatRect getBounds() const override;
  float getEffectiveSpeed() const override;

  // ── Power-ups ──
  void applyPowerUp(PowerUpState state);
  PowerUpState getPowerUpState() const;
  void growBig();
  void shrink();
  void enableFire();

  // ── Shooting ──
  void shoot();
  bool wantsToShoot() const;
  void clearShootFlag();

  // ── Sprinting ──
  void setSprinting(bool sprinting);
  bool isSprinting() const;

  // ── Jump input state ──
  void setJumpHeld(bool held);
  bool isJumpHeld() const;

  // ── Goal cutscene ──
  /// Lock the player to the pole and descend to `landingY` using the
  /// character-specific climbing animation.
  void beginFlagpoleSlide(float poleCenterX, float landingY);
  /// Switch from the pole pose to a cutscene-only walk animation used while
  /// the player heads toward the castle.
  void beginFlagpoleCastleWalk();
  bool isFlagpoleSlideComplete() const;
  bool isFlagpoleCutsceneActive() const;

  // ── Override damage to handle power-up states ──
  void takeDamage(int amount = 1) override;
  void die() override;
  bool isDeathAnimationComplete() const;

  // ── Lives ──
  int getLives() const;
  void setLives(int lives);
  void loseLife();

  // ── Invincibility (star or post-damage) ──
  bool isInvincible() const;
  void setInvincibleTimer(float seconds);

  // ── Star power ──
  bool hasStarPower() const;
  void setStarPower(float duration);

protected:
  PowerUpState m_powerUp = PowerUpState::Small;
  int m_lives = 3;
  bool m_sprinting = false;
  bool m_wantsToShoot = false;
  bool m_jumpHeld = false;

  // Invincibility after damage
  float m_invincibleTimer = 0.0f;
  bool m_invincible = false;

  // Star power
  float m_starTimer = 0.0f;
  bool m_starPower = false;

  // Blinking effect during invincibility
  float m_blinkTimer = 0.0f;
  bool m_visible = true;

  // ── Sprite selection ──
  CharacterId m_characterId = CharacterId::Mario;
  SpriteRegistry::PlayerAnim m_currentAnim = SpriteRegistry::PlayerAnim::Idle;

private:
  enum class DeathAnimationPhase { None, Rising, Falling, Paused, Complete };
  enum class GoalAnimationPhase { None, Sliding, SlideComplete, CastleWalk };

  void updateDeathAnimation(float dt);
  void updateFlagpoleSlide(float dt);
  void updateFlagpoleCastleWalk(float dt);
  sf::FloatRect getDeathBounds() const;

  DeathAnimationPhase m_deathAnimationPhase = DeathAnimationPhase::None;
  float m_deathPauseTimer = 0.0f;
  float m_deathPauseY = 0.0f;

  GoalAnimationPhase m_goalAnimationPhase = GoalAnimationPhase::None;
  float m_flagpoleSlideX = 0.0f;
  float m_flagpoleSlideTargetY = 0.0f;
};
