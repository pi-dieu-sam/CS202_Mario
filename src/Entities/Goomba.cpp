#include "Entities/Goomba.hpp"
#include "Graphics/SpriteRegistry.hpp"
#include "Physics/PhysicsConstants.hpp"

Goomba::Goomba() {
  m_speed = 60.0f;
  m_scoreValue = 100;
  m_velocity.x = -m_speed; // Start walking left

  setAnimFrameCount(16, 0.08f);
}

void Goomba::onStomped() {
  if (m_squished || m_dead)
    return;

  // Mark the Goomba dead immediately so collision loops stop treating its
  // bounds as a solid enemy. Keep it active only as a render-only corpse.
  die();
  m_squished = true;
  m_deathTimer = 0.5f;
}

void Goomba::update(float dt) {
  if (m_squished) {
    m_deathTimer -= dt;
    if (m_deathTimer <= 0.0f) {
      m_active = false;
    }
    return;
  }

  Enemy::update(dt);
}

void Goomba::draw(sf::RenderWindow &window) {
  if (!m_active)
    return;

  if (m_squished) {
    drawSprite(window, SpriteRegistry::goombaSquishPath(m_theme), getBounds());
    return;
  }

  SpriteRegistry::applyGoombaFrame(m_sprite, m_animFrame, getBounds(),
                                   m_facingRight);
  window.draw(m_sprite);
}

bool Goomba::isVulnerable() const {
  return !m_squished && !m_dead;
}
