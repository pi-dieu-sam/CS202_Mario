#pragma once

#include "GameObject.hpp"
#include "../Physics/PhysicsConstants.hpp"
#include <SFML/Graphics.hpp>

/// LavaFireball — a repeating hazard launched vertically from lava.
/// It is created by the 'i' marker in a level map. The marker is only the
/// launch point: this entity passes through map geometry and kills players on
/// contact while it is visible.
class LavaFireball : public GameObject {
public:
  /// Maximum height above the lava launch point. Adjust this value to tune
  /// every lava fireball in the game.
  static constexpr float LAUNCH_HEIGHT = TILE_SIZE * 10.0f;

  LavaFireball(float tileX, float tileY);

  void update(float dt) override;
  void draw(sf::RenderWindow &window) override;
  sf::FloatRect getBounds() const override;

  /// False for the three-second pause after the ball returns to lava.
  bool isVisible() const;

private:
  void launch();

  sf::Sprite m_sprite;
  sf::Vector2f m_launchPosition;
  float m_launchSpeed = 0.0f;
  float m_totalFlightTime = 0.0f;
  float m_flightTimer = 0.0f;
  float m_cooldownTimer = 0.0f;
  int m_animationFrame = 0;
  bool m_visible = true;
};
