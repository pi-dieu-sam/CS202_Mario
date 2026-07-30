#include "Entities/Mario.hpp"
#include "Physics/PhysicsConstants.hpp"

Mario::Mario() {
  m_speed = PLAYER_SPEED;
  m_jumpForce = PLAYER_JUMP;
  m_health = 1;
}
