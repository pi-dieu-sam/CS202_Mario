#include "Entities/LavaFireball.hpp"

#include "Graphics/SpriteRegistry.hpp"
#include <algorithm>
#include <cmath>

namespace {
constexpr int FRAME_COUNT = 11;
constexpr int FRAME_WIDTH = 15;
constexpr float SOURCE_FRAME_HEIGHT = 25.0f;
constexpr float VISUAL_SCALE = 2.0f;
constexpr float VISUAL_WIDTH = FRAME_WIDTH * VISUAL_SCALE;
constexpr float VISUAL_HEIGHT = SOURCE_FRAME_HEIGHT * VISUAL_SCALE;
constexpr float COOLDOWN_SECONDS = 3.0f;
}

LavaFireball::LavaFireball(float tileX, float tileY) {
  m_type = ObjectType::LavaFireball;

  // The bottom of the sprite stays anchored to the bottom of the i marker's
  // map cell, which lets the tail disappear naturally into a lava tile.
  m_launchPosition = {
      tileX + (TILE_SIZE - VISUAL_WIDTH) * 0.5f,
      tileY + TILE_SIZE - VISUAL_HEIGHT};
  m_position = m_launchPosition;

  // v² = 2gh: launch slowly loses speed on the way up, then accelerates
  // downward under the same gravity used by the rest of the game.
  m_launchSpeed = std::sqrt(2.0f * GRAVITY * LAUNCH_HEIGHT);
  m_totalFlightTime = 2.0f * m_launchSpeed / GRAVITY;
  launch();
}

void LavaFireball::launch() {
  m_position = m_launchPosition;
  m_velocity = {0.0f, -m_launchSpeed};
  m_flightTimer = 0.0f;
  m_animationFrame = 0;
  m_visible = true;
}

void LavaFireball::update(float dt) {
  if (!m_visible) {
    m_cooldownTimer += dt;
    if (m_cooldownTimer >= COOLDOWN_SECONDS) {
      m_cooldownTimer = 0.0f;
      launch();
    }
    return;
  }

  m_velocity.y += GRAVITY * dt;
  m_position += m_velocity * dt;
  m_flightTimer += dt;

  // Walk through the supplied strip once across the complete up/down arc.
  const float progress = std::min(m_flightTimer / m_totalFlightTime, 1.0f);
  m_animationFrame = std::min(
      FRAME_COUNT - 1, static_cast<int>(progress * FRAME_COUNT));

  // Only finish after descending to the launch height; this avoids ending at
  // the apex or on a one-frame numerical overshoot.
  if (m_velocity.y > 0.0f && m_position.y >= m_launchPosition.y) {
    m_position = m_launchPosition;
    m_velocity = {0.0f, 0.0f};
    m_visible = false;
    m_cooldownTimer = 0.0f;
  }
}

void LavaFireball::draw(sf::RenderWindow &window) {
  if (!m_visible) return;

  SpriteRegistry::applySheetFrame(
      m_sprite, SpriteRegistry::lavaFireballPath(), m_animationFrame,
      FRAME_WIDTH, 0, getBounds());
  window.draw(m_sprite);
}

sf::FloatRect LavaFireball::getBounds() const {
  return {m_position.x, m_position.y, VISUAL_WIDTH, VISUAL_HEIGHT};
}

bool LavaFireball::isVisible() const { return m_visible; }
