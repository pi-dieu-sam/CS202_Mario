#include "Entities/Escalater.hpp"
#include "Core/AssetManager.hpp"
#include "Graphics/SpriteRegistry.hpp"

Escalater::Escalater() {
  m_type = ObjectType::Tile;
}

Escalater::Escalater(float x, float y, MovementAxis axis) : m_axis(axis) {
  m_type = ObjectType::Tile;
  m_position = {x, y};
  m_centerX = x;
  m_centerY = y;
  if (m_axis == MovementAxis::Horizontal) {
    m_range = 5.0f * TILE_SIZE;
    m_direction = 1.0f;
  }
}

void Escalater::update(float dt) {
  if (m_axis == MovementAxis::Horizontal) {
    m_position.x += m_speed * m_direction * dt;

    if (m_position.x < m_centerX - m_range) {
      m_position.x = m_centerX - m_range;
      m_direction = 1.0f;
    } else if (m_position.x > m_centerX + m_range) {
      m_position.x = m_centerX + m_range;
      m_direction = -1.0f;
    }

    if (m_mapRight > m_mapLeft) {
      if (m_position.x < m_mapLeft) {
        m_position.x = m_mapLeft;
        m_direction = 1.0f;
      } else if (m_position.x + m_size.x > m_mapRight) {
        m_position.x = m_mapRight - m_size.x;
        m_direction = -1.0f;
      }
    }

    m_velocity = {m_speed * m_direction, 0.0f};
    return;
  }

  m_position.y += m_speed * m_direction * dt;

  // Check range-based limits
  if (m_position.y < m_centerY - m_range) {
    m_position.y = m_centerY - m_range;
    m_direction = 1.0f;
  } else if (m_position.y > m_centerY + m_range) {
    m_position.y = m_centerY + m_range;
    m_direction = -1.0f;
  }

  // Check map boundaries
  if (m_mapBottom > m_mapTop) {
    if (m_position.y < m_mapTop) {
      m_position.y = m_mapTop;
      m_direction = 1.0f;
    } else if (m_position.y + m_size.y > m_mapBottom) {
      m_position.y = m_mapBottom - m_size.y;
      m_direction = -1.0f;
    }
  }

  m_velocity.y = m_speed * m_direction;
}

void Escalater::draw(sf::RenderWindow &window) {
  if (!m_active) return;
  SpriteRegistry::applyFrame(m_sprite, SpriteRegistry::escalaterPath(),
                              sf::FloatRect(m_position.x, m_position.y,
                                            m_renderSize.x, m_renderSize.y));
  window.draw(m_sprite);
}

sf::FloatRect Escalater::getBounds() const {
  return sf::FloatRect(m_position.x, m_position.y, m_size.x, m_size.y);
}

void Escalater::setRange(float range) { m_range = range; }
void Escalater::setSpeed(float speed) { m_speed = speed; }

void Escalater::reverseDirection() {
  m_direction = -m_direction;
  if (m_axis == MovementAxis::Horizontal) {
    m_velocity = {m_speed * m_direction, 0.0f};
  } else {
    m_velocity.y = m_speed * m_direction;
  }
}

void Escalater::setMapBounds(float left, float right, float top, float bottom) {
  m_mapLeft = left;
  m_mapRight = right;
  m_mapTop = top;
  m_mapBottom = bottom;
}

bool Escalater::movesHorizontally() const {
  return m_axis == MovementAxis::Horizontal;
}
