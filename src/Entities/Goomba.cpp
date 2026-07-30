#include "Entities/Goomba.hpp"
#include "AI/PatrolStrategy.hpp"
#include "Graphics/SpriteRegistry.hpp"
#include "Physics/PhysicsConstants.hpp"

Goomba::Goomba() {
  m_speed = 60.0f;
  m_scoreValue = 100;
  m_velocity.x = -m_speed; // Start walking left
  setStrategy(std::make_unique<PatrolStrategy>());

  setAnimFrameCount(2, 0.3f);
}

void Goomba::onStomped() {
  m_squished = true;
  m_deathTimer = 0.5f; // Show squished sprite for 0.5s
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

  std::string path = m_squished
                          ? SpriteRegistry::goombaSquishPath(m_theme)
                          : SpriteRegistry::goombaPath(m_theme, m_animFrame);
  drawSprite(window, path, getBounds());
}
