#pragma once

#include "Enemy.hpp"

/// Bowser is a stationary, two-by-two-tile boss. It idles for two seconds,
/// then plays one three-second, six-frame breathing animation before looping.
class Bowser final : public Enemy {
public:
  enum class State { Idle, Breathing };

  Bowser();

  void update(float dt) override;
  void draw(sf::RenderWindow &window) override;
  sf::FloatRect getBounds() const override;
  void onStomped() override;
  bool hitByFireball() override;
  bool canBeStomped() const override;
  bool usesTerrainCollisions() const override;

  State getState() const;
  int getBreathFrame() const;
  int getFireballHits() const;

private:
  State m_state = State::Idle;
  float m_stateTimer = 0.0f;
  int m_breathFrame = 0;
  int m_fireballHits = 0;
};
