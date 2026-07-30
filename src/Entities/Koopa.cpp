#include "Entities/Koopa.hpp"
#include "AI/PatrolStrategy.hpp"
#include "Graphics/SpriteRegistry.hpp"
#include "Physics/PhysicsConstants.hpp"

Koopa::Koopa() {
  m_speed = 60.0f;
  m_scoreValue = 200;
  m_velocity.x = -m_speed;
  setStrategy(std::make_unique<PatrolStrategy>());

  setAnimFrameCount(2, 0.3f);
}

void Koopa::onStomped() {
  switch (m_koopaState) {
  case KoopaState::Walking:
    m_koopaState = KoopaState::Shell;
    m_velocity = {0.0f, 0.0f};
    m_shellTimer = 8.0f;  // Recovers after 8 seconds
    m_strategy = nullptr; // No AI in shell state
    break;

  case KoopaState::Shell:
    // Kick the shell in the direction the player was facing
    kickShell(1); // Default kick right
    break;

  case KoopaState::Sliding:
    // Stop the shell
    m_koopaState = KoopaState::Shell;
    m_velocity = {0.0f, 0.0f};
    m_shellTimer = 8.0f;
    break;
  }
}

void Koopa::kickShell(int direction) {
  m_koopaState = KoopaState::Sliding;
  m_velocity.x = direction * 300.0f; // Fast shell slide
}

void Koopa::update(float dt) {
  if (m_dead)
    return;

  if (m_koopaState == KoopaState::Shell) {
    // Timer to recover from shell
    m_shellTimer -= dt;
    if (m_shellTimer <= 0.0f) {
      m_koopaState = KoopaState::Walking;
      m_velocity.x = -m_speed;
      setStrategy(std::make_unique<PatrolStrategy>());
    }
    applyGravity(dt);
    m_position += m_velocity * dt;
    return;
  }

  if (m_koopaState == KoopaState::Sliding) {
    // Shell slides — no AI, just move
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
    drawSprite(window, SpriteRegistry::koopaWalkPath(m_theme, m_animFrame), box);
  } else {
    sf::FloatRect box(m_position.x + 1, m_position.y + 3, TILE_SIZE - 2,
                       TILE_SIZE - 4);
    bool spinning = (m_koopaState == KoopaState::Sliding);
    drawSprite(window, SpriteRegistry::koopaShellPath(m_theme, spinning), box);
  }
}

KoopaState Koopa::getKoopaState() const { return m_koopaState; }
