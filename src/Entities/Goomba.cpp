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
  m_squished = true;
  m_deathTimer = 1.0f; // Show death sprite for 1s
  m_velocity = {0.0f, 0.0f};
}

void Goomba::update(float dt) {
  if (m_squished) {
    m_deathTimer -= dt;
    if (m_deathTimer <= 0.0f) {
      m_active = false;
      m_dead = true;
    }
    return;
  }

  Enemy::update(dt);
}

void Goomba::draw(sf::RenderWindow &window) {
  if (!m_active)
    return;

  if (m_squished) {
    std::string path = SpriteRegistry::goombaSquishPath(m_theme);
    drawSprite(window, path, getBounds());
  } else {
    SpriteRegistry::applyGoombaFrame(m_sprite, m_animFrame, getBounds(),
                                     m_facingRight);
    window.draw(m_sprite);
  }
}
