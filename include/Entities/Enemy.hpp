#pragma once
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

  /// Set the AI strategy (Strategy pattern).
  void setStrategy(std::unique_ptr<AIStrategy> strategy);

  /// Forward the player's current position to this enemy's AI strategy.
  /// No-op if there's no strategy, or if it doesn't use the position
  /// (e.g. PatrolStrategy).
  void updatePlayerPosition(const sf::Vector2f& playerPos);

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
