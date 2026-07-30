#include "Entities/Item.hpp"
#include "Entities/Player.hpp"
#include "Graphics/SpriteRegistry.hpp"
#include "Observers/EventManager.hpp"
#include "Physics/PhysicsConstants.hpp"

Item::Item() {
  m_type = ObjectType::Coin; // Default, overridden by subclasses
}

void Item::update(float dt) {
  if (!m_active)
    return;

  if (m_moving) {
    m_velocity.y += GRAVITY * dt;
    if (m_velocity.y > MAX_FALL_SPEED)
      m_velocity.y = MAX_FALL_SPEED;
    m_position += m_velocity * dt;
  }

  // Animate
  m_animTimer += dt;
}

void Item::draw(sf::RenderWindow &window) {
  if (!m_active)
    return;
  SpriteRegistry::applyFrame(m_sprite, m_texturePath, getBounds());
  window.draw(m_sprite);
}

sf::FloatRect Item::getBounds() const {
  return sf::FloatRect(m_position.x, m_position.y, TILE_SIZE, TILE_SIZE);
}

void Item::collect(Player &player) {
  if (m_collected)
    return;
  m_collected = true;
  m_active = false;
  activate(player);
}

bool Item::isMoving() const { return m_moving; }

void Item::setTheme(LevelTheme theme) {
  m_theme = theme;
  refreshSprite();
}
