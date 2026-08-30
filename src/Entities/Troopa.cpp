#include "Entities/Troopa.hpp"
#include "Graphics/SpriteRegistry.hpp"
#include "Physics/PhysicsConstants.hpp"

Troopa::Troopa() {
  m_speed = 65.0f;
  m_scoreValue = 400;
  m_velocity.x = -m_speed;
  setAnimFrameCount(SpriteRegistry::troopaFrameCount(), 0.12f);
}

void Troopa::update(float dt) {
  if (m_dead)
    return;

  // This enemy is always airborne: do not call Enemy::update(), because it
  // applies gravity after running an AI strategy.
  m_velocity.y = 0.0f;
  m_position.x += m_velocity.x * dt;

  if (m_velocity.x > 0.0f)
    m_facingRight = true;
  else if (m_velocity.x < 0.0f)
    m_facingRight = false;

  updateSprite(dt);
}

void Troopa::draw(sf::RenderWindow &window) {
  if (!m_active)
    return;

  const sf::FloatRect box(m_position.x + 1.0f, m_position.y + 1.0f,
                          TILE_SIZE - 2.0f, TILE_SIZE - 2.0f);
  SpriteRegistry::applyTroopaFrame(m_sprite, m_animFrame, box,
                                   m_facingRight);
  window.draw(m_sprite);
}

void Troopa::onStomped() { kill(); }

void Troopa::kill() {
  m_dead = true;
  m_active = false;
  m_velocity = {0.0f, 0.0f};
}
