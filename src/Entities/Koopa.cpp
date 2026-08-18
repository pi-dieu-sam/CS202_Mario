#include "Entities/Koopa.hpp"
#include "AI/PatrolStrategy.hpp"
#include "Graphics/SpriteRegistry.hpp"
#include "Physics/PhysicsConstants.hpp"

Koopa::Koopa() {
  m_speed = 60.0f;
  m_scoreValue = 200;
  m_velocity.x = -m_speed;

  setAnimFrameCount(20, 0.08f);
}

void Koopa::onStomped() {
  if (m_koopaState != KoopaState::Walking)
    return;

  m_koopaState = KoopaState::Dying;
  m_dieTimer = 5.0f;
  m_velocity = {0.0f, 0.0f};
  m_strategy = nullptr;
}

void Koopa::kill() {
  if (m_koopaState != KoopaState::Walking)
    return;

  m_koopaState = KoopaState::Dying;
  m_dieTimer = 5.0f;
  m_velocity = {0.0f, 0.0f};
  m_strategy = nullptr;
  m_dead = false;
}

void Koopa::update(float dt) {
  if (m_koopaState == KoopaState::Dying) {
    m_dieTimer -= dt;
    if (m_dieTimer <= 0.0f) {
      m_koopaState = KoopaState::Walking;
      m_velocity.x = -m_speed;
      m_facingRight = false;
      setStrategy(std::make_unique<PatrolStrategy>());
    }
    applyGravity(dt);
    m_position += m_velocity * dt;
    return;
  }

  // Walking state — normal enemy behavior
  Enemy::update(dt);
}

void Koopa::draw(sf::RenderWindow &window) {
  if (!m_active)
    return;

  if (m_koopaState == KoopaState::Walking) {
    sf::FloatRect box(m_position.x + 1, m_position.y - TILE_SIZE * 0.5f + 1,
                       TILE_SIZE - 2, TILE_SIZE * 1.5f - 2);
    SpriteRegistry::applyKoopaFrame(m_sprite, m_animFrame, box, m_facingRight);
    window.draw(m_sprite);
  } else {
    sf::FloatRect box(m_position.x + 1, m_position.y + 3, TILE_SIZE - 2,
                       TILE_SIZE - 4);
    drawSprite(window, SpriteRegistry::koopaDiePath(), box);
  }
}

bool Koopa::isVulnerable() const {
  return m_koopaState == KoopaState::Walking;
}

KoopaState Koopa::getKoopaState() const { return m_koopaState; }
