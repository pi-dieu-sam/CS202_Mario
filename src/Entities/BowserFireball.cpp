#include "Entities/BowserFireball.hpp"
#include "Graphics/SpriteRegistry.hpp"

namespace {
constexpr float FIREBALL_SPEED = 160.0f;
constexpr float FIREBALL_WIDTH = 30.0f;
constexpr float FIREBALL_HEIGHT = 20.0f;
constexpr float FRAME_DURATION = 0.10f;
}

BowserFireball::BowserFireball(float x, float y, int direction) {
  m_type = ObjectType::Fireball;
  m_position = {x, y};
  m_velocity = {direction < 0 ? -FIREBALL_SPEED : FIREBALL_SPEED, 0.0f};
}

void BowserFireball::update(float dt) {
  if (!m_active)
    return;

  m_position += m_velocity * dt;
  m_animTimer += dt;
  while (m_animTimer >= FRAME_DURATION) {
    m_animTimer -= FRAME_DURATION;
    m_animFrame = (m_animFrame + 1) % SpriteRegistry::bowserFireFrameCount();
  }

  m_lifetime -= dt;
  if (m_lifetime <= 0.0f || m_position.x + FIREBALL_WIDTH < -50.0f ||
      m_position.x > 10000.0f) {
    m_active = false;
  }
}

void BowserFireball::draw(sf::RenderWindow &window) {
  if (!m_active)
    return;
  SpriteRegistry::applySheetFrame(m_sprite, SpriteRegistry::bowserFirePath(),
                                  m_animFrame, 24, 0, getBounds(), true);
  window.draw(m_sprite);
}

sf::FloatRect BowserFireball::getBounds() const {
  return sf::FloatRect(m_position.x, m_position.y, FIREBALL_WIDTH,
                       FIREBALL_HEIGHT);
}
