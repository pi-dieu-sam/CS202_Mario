#pragma once
// Enemy.hpp — abstract base for all hostile NPCs
#include "../AI/AIStrategy.hpp"
#include "../Level/LevelTheme.hpp"
#include "Character.hpp"
#include <memory>

/// Enemy — Abstract base class for all enemy characters.
/// Uses the Strategy pattern for AI behavior.
class Enemy : public Character {
public:
  Enemy();
  virtual ~Enemy() = default;

  void update(float dt) override;
  void draw(sf::RenderWindow &window) override;
  sf::FloatRect getBounds() const override;

  /// Called when the player stomps on this enemy from above.
  virtual void onStomped();

  /// Instantly defeat this enemy regardless of type (used by fireballs).
  /// Unlike onStomped(), which subclasses may override to only change state
  /// (Koopa -> shell) or do nothing (Bowser), kill() always removes the enemy.
  virtual void kill();

  /// Apply a fireball hit and return whether it defeated the enemy. Most
  /// enemies die immediately; bosses can require multiple fireballs without
  /// changing other defeat types.
  virtual bool hitByFireball();

  /// Returns false when the enemy is in a non-interactive state (e.g. dying
  /// animation) and should not deal or receive damage.
  virtual bool isVulnerable() const;

  /// Whether this enemy can be defeated by stomping. PiranhaPlant returns false
  /// so stomping it always hurts the player instead.
  virtual bool canBeStomped() const;

  /// Whether Level should apply walking-enemy tile/block collision response.
  /// Stationary hazards anchored inside terrain, such as PiranhaPlant, opt out
  /// so the collision resolver cannot displace their scripted position.
  virtual bool usesTerrainCollisions() const;

  /// Set the AI strategy (Strategy pattern).
  void setStrategy(std::unique_ptr<AIStrategy> strategy);

  /// Forward the player's current position to this enemy's AI strategy.
  /// No-op if there's no strategy, or if it doesn't use the position
  /// (e.g. PatrolStrategy).
  virtual void updatePlayerPosition(const sf::Vector2f& playerPos);

  /// Points awarded when this enemy is defeated.
  int getScoreValue() const;

  /// Which environment palette this enemy's sprite should use, set by
  /// EntityFactory right after construction (level-load time).
  void setTheme(LevelTheme theme);

protected:
  std::unique_ptr<AIStrategy> m_strategy;
  int m_scoreValue = 200;
  LevelTheme m_theme = LevelTheme::Overworld;
};
