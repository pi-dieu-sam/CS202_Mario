#include "Entities/Luigi.hpp"
#include "Physics/PhysicsConstants.hpp"
#include <cmath>

namespace {
// Closer to 1.0 than GROUND_FRICTION (0.85f) -- decelerates more slowly
// each frame, giving Luigi his signature slide.
constexpr float LUIGI_FRICTION = 0.93f;
}

Luigi::Luigi() {
  m_speed = PLAYER_SPEED * 0.9f;   // Slightly slower than Mario
  m_jumpForce = PLAYER_JUMP * 1.2f; // Higher jump (more negative = higher)
  m_health = 1;
  m_characterId = CharacterId::Luigi;
}

void Luigi::applyFriction() {
  m_velocity.x *= LUIGI_FRICTION;
  if (std::abs(m_velocity.x) < 1.0f) {
    m_velocity.x = 0.0f;
  }
}
