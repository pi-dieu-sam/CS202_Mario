#pragma once
#include "GameObject.hpp"
#include "../Physics/PhysicsConstants.hpp"
#include <SFML/Graphics.hpp>

/// Escalater — a moving platform that oscillates vertically.
/// Maps to character 'E' in the level file. The player can stand on it
/// and is carried along with its vertical movement.
class Escalater : public GameObject {
  friend class SnapshotAccess;
public:
  enum class MovementAxis { Vertical, Horizontal };

  Escalater();
  Escalater(float x, float y, MovementAxis axis = MovementAxis::Vertical);

  void update(float dt) override;
  void draw(sf::RenderWindow &window) override;
  sf::FloatRect getBounds() const override;

  /// Movement range in pixels (default: 3 tiles = 96px each direction).
  void setRange(float range);
  /// Movement speed in pixels/s (default: 60).
  void setSpeed(float speed);
  /// Reverse the current movement direction.
  void reverseDirection();
  /// Set map bounds used to keep the platform inside the level.
  void setMapBounds(float left, float right, float top, float bottom);
  bool movesHorizontally() const;

private:
  MovementAxis m_axis = MovementAxis::Vertical;
  sf::Vector2f m_size = {TILE_SIZE, TILE_SIZE};
  sf::Vector2f m_renderSize = {TILE_SIZE, TILE_SIZE / 2.0f};
  sf::Sprite m_sprite;

  float m_centerX   = 0.0f;
  float m_centerY   = 0.0f;
  float m_range     = 3.0f * TILE_SIZE;  // 96px up and down
  float m_speed     = 60.0f;             // pixels/s
  float m_direction = -1.0f;             // -1 = up, +1 = down
  float m_mapLeft   = 0.0f;
  float m_mapRight  = 0.0f;
  float m_mapTop    = 0.0f;             // top boundary of the map
  float m_mapBottom = 0.0f;             // bottom boundary of the map
};
