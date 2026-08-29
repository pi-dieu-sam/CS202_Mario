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
  /// Compact, feet-anchored body used only for interactive block collisions.
  /// This keeps low question blocks usable while a power-up enlarges the
  /// player's visual/terrain body.
  sf::FloatRect getBlockInteractionBounds() const;
  float getEffectiveSpeed() const override;
  void moveLeft(float dt) override;
  void moveRight(float dt) override;
  void jump() override;

  // -- Vine climbing --
  /// Called by Level after tile collision detection to enter/leave the vine.
  /// `vineX` is the tile's left edge; entering centres the player on it.
  /// A horizontal detach cannot reattach until the player has left that tile.
  void updateVineContact(bool touchingVine, float vineX);
  void climbUp(float dt);
  void climbDown(float dt);
  bool isClimbing() const;
  /// Horizontal input held while first touching a vine must be released
  /// before a new left/right press can detach the player.
  void setVineHorizontalInput(bool held);

  // ── Power-ups ──
  void applyPowerUp(PowerUpState state);
  PowerUpState getPowerUpState() const;
  void growBig();
  void shrink();
  void enableFire();

  // ── Size buff (FlowersBuff) ──
  /// Start the temporary 1.5x size / +0.2 speed / +0.2 jump buff.
  void applySizeBuff();
  /// Current visual/collision size multiplier (ramps 1.0 -> 1.5 while growing).
  float getSizeScale() const;
  /// True while the buff is active (growing or already at full size).
  bool hasSizeBuff() const;

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

  // ── Player ID (1 = P1, 2 = P2) for event routing ──
  int  getPlayerId() const { return m_playerId; }
  void setPlayerId(int id) { m_playerId = id; }

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
  bool m_climbing = false;
  bool m_climbMoving = false;
  bool m_vineReattachLocked = false;
  bool m_vineHorizontalReleaseRequired = false;
  bool m_hasVineAnchor = false;
  sf::Vector2f m_lastVineAnchor = {0.0f, 0.0f};
  int  m_playerId = 1; ///< 1 = P1, 2 = P2

  // Invincibility after damage
  float m_invincibleTimer = 0.0f;
  bool m_invincible = false;

  // Star power
  float m_starTimer = 0.0f;
  bool m_starPower = false;

  // FlowersBuff temporary buff
  float m_sizeScale = 1.0f; ///< size multiplier, ramps 1.0 -> 1.5 over 0.7s
  bool  m_growing   = false; ///< currently ramping the size up
  float m_growTimer = 0.0f;  ///< elapsed time in the 0.7s growth animation
  float m_buffTimer = 0.0f;  ///< remaining buff seconds once fully grown

  // Blinking effect during invincibility
  float m_blinkTimer = 0.0f;
  bool m_visible = true;

  // ── Sprite selection ──
  CharacterId m_characterId = CharacterId::Mario;
  SpriteRegistry::PlayerAnim m_currentAnim = SpriteRegistry::PlayerAnim::Idle;

  // Brief post-shot window during which the Fire (shoot) animation plays
  // before reverting to the normal state. Only used by Mario.
  float m_shootAnimTimer = 0.0f;

  void leaveVine(bool lockReattach);

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
