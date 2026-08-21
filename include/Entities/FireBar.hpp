#pragma once
#include "GameObject.hpp"
#include "../Physics/PhysicsConstants.hpp"
#include <SFML/Graphics.hpp>

/// FireBar — a rotating fire hazard anchored to a block.
/// Maps to character 'O' in the level file. A block sits at the center and
/// fire segments rotate around it at a constant angular velocity. Contact
/// with any segment kills the player.
class FireBar : public GameObject {
public:
  FireBar();
  FireBar(float x, float y, int segmentCount = 8);

  void update(float dt) override;
  void draw(sf::RenderWindow &window) override;
  sf::FloatRect getBounds() const override;

  /// Get the world-space bounding box of a specific fire segment.
  sf::FloatRect getSegmentBounds(int index) const;

  /// The fixed source block, scaled to fill the map tile containing O.
  sf::FloatRect getAnchorBounds() const;

  int getSegmentCount() const;

private:
  sf::Sprite m_blockSprite;
  sf::Sprite m_fireSprite;
  float m_angle = 0.0f;
  float m_angularSpeed = 1.5f;
  float m_fireballRotationDegrees = 0.0f;
  float m_fireballSpinSpeed = 540.0f; // degrees per second, about 1.5 turns/s
  float m_animationTimer = 0.0f;
  int   m_animationFrame = 0;
  int   m_segmentCount = 8;
  // The 8x8 source fire frame is displayed at 16x16 to match the game's
  // 32px tile scale. This makes all eight fireballs one continuous bar.
  float m_segmentSpacing = 16.0f;
};
