#include "Entities/FireBar.hpp"
#include "Graphics/SpriteRegistry.hpp"
#include <cmath>

namespace {
constexpr float FIRE_SEGMENT_SIZE = 16.0f;
constexpr int FIRE_ANIMATION_FRAMES = 8;
constexpr float FIRE_FRAME_TIME = 0.075f;
constexpr float TWO_PI = 6.283185307f;
}

FireBar::FireBar() { m_type = ObjectType::FireBar; }

FireBar::FireBar(float x, float y, int segmentCount)
    : m_segmentCount(segmentCount) {
  m_type = ObjectType::FireBar;
  m_position = {x, y};
}

void FireBar::update(float dt) {
  m_angle += m_angularSpeed * dt;
  // Keep angle in [0, 2π)
  while (m_angle >= TWO_PI) m_angle -= TWO_PI;

  // Second rotation layer: while the complete chain orbits the anchor, each
  // individual fireball also spins around its own centre like a rolling ball.
  m_fireballRotationDegrees += m_fireballSpinSpeed * dt;
  while (m_fireballRotationDegrees >= 360.0f) {
    m_fireballRotationDegrees -= 360.0f;
  }

  // The source strip has eight 8x8 fireball poses. This animation is kept
  // independent from the arm rotation, so the fireballs flicker as the whole
  // bar sweeps around its centre block.
  m_animationTimer += dt;
  while (m_animationTimer >= FIRE_FRAME_TIME) {
    m_animationTimer -= FIRE_FRAME_TIME;
    m_animationFrame = (m_animationFrame + 1) % FIRE_ANIMATION_FRAMES;
  }
}

void FireBar::draw(sf::RenderWindow &window) {
  if (!m_active) return;

  // The 16x16 source block is scaled to one 32x32 map tile.
  SpriteRegistry::applyFrame(m_blockSprite, SpriteRegistry::fireBarBlockPath(),
                              getAnchorBounds());
  window.draw(m_blockSprite);

  // Draw each fire segment along the rotated line
  const float centerX = m_position.x + TILE_SIZE / 2.0f;
  const float centerY = m_position.y + TILE_SIZE / 2.0f;
  for (int i = 0; i < m_segmentCount; ++i) {
    // The first fireball is the fixed pivot at the centre block. The other
    // seven form the rotating arm around it.
    float sx = centerX;
    float sy = centerY;
    if (i > 0) {
      float dist = m_segmentSpacing * static_cast<float>(i);
      sx += std::cos(m_angle) * dist;
      sy += std::sin(m_angle) * dist;
    }

    // FireBarWithoutBlock is a strip of eight contiguous 8x8 frames.
    SpriteRegistry::applySheetFrame(
        m_fireSprite, SpriteRegistry::fireBarSegmentPath(), m_animationFrame,
        8, 0, sf::FloatRect(sx - FIRE_SEGMENT_SIZE / 2.0f,
                             sy - FIRE_SEGMENT_SIZE / 2.0f,
                             FIRE_SEGMENT_SIZE, FIRE_SEGMENT_SIZE));
    // applySheetFrame uses a bottom-centre pivot by default. A self-rotating
    // fireball must instead pivot at its centre. Each ball has a fixed phase
    // offset so the eight rotations are visually distinct.
    m_fireSprite.setOrigin(4.0f, 4.0f); // source frame centre (8x8)
    m_fireSprite.setPosition(sx, sy);
    m_fireSprite.setRotation(m_fireballRotationDegrees + i * 45.0f);
    window.draw(m_fireSprite);
  }
}

sf::FloatRect FireBar::getBounds() const {
  // This is only the visual anchor; the fire bar has no map collision.
  return getAnchorBounds();
}

sf::FloatRect FireBar::getSegmentBounds(int index) const {
  const float centerX = m_position.x + TILE_SIZE / 2.0f;
  const float centerY = m_position.y + TILE_SIZE / 2.0f;
  float sx = centerX - FIRE_SEGMENT_SIZE / 2.0f;
  float sy = centerY - FIRE_SEGMENT_SIZE / 2.0f;
  if (index > 0) {
    float dist = m_segmentSpacing * static_cast<float>(index);
    sx += std::cos(m_angle) * dist;
    sy += std::sin(m_angle) * dist;
  }

  return sf::FloatRect(sx, sy, FIRE_SEGMENT_SIZE, FIRE_SEGMENT_SIZE);
}

sf::FloatRect FireBar::getAnchorBounds() const {
  return sf::FloatRect(m_position.x, m_position.y, TILE_SIZE, TILE_SIZE);
}

int FireBar::getSegmentCount() const { return m_segmentCount; }
