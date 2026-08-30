#include "Entities/Bowser.hpp"
#include "Graphics/SpriteRegistry.hpp"
#include "Physics/PhysicsConstants.hpp"
#include <algorithm>

namespace {
constexpr float BOWSER_SIZE = TILE_SIZE * 2.0f;
constexpr float IDLE_DURATION = 2.0f;
constexpr float BREATH_DURATION = 3.0f;
}

Bowser::Bowser() {
  m_speed = 0.0f;
  m_scoreValue = 1000;
  m_strategy = nullptr;
  // The supplied sprites face right. Bowser is intentionally rendered
  // mirrored, so he always faces left.
  m_facingRight = false;
}

void Bowser::update(float dt) {
  if (m_dead)
    return;

  m_velocity = {0.0f, 0.0f};
  m_stateTimer += dt;

  if (m_state == State::Idle) {
    if (m_stateTimer >= IDLE_DURATION) {
      m_state = State::Breathing;
      m_stateTimer -= IDLE_DURATION;
      m_breathFrame = 0;
    }
  }

  if (m_state == State::Breathing) {
    const float frameDuration =
        BREATH_DURATION / static_cast<float>(SpriteRegistry::bowserBreathFrameCount());
    m_breathFrame = std::min(
        static_cast<int>(m_stateTimer / frameDuration),
        SpriteRegistry::bowserBreathFrameCount() - 1);

    if (m_stateTimer >= BREATH_DURATION) {
      m_state = State::Idle;
      m_stateTimer -= BREATH_DURATION;
      m_breathFrame = 0;
    }
  }
}

void Bowser::draw(sf::RenderWindow &window) {
  if (!m_active)
    return;

  const sf::FloatRect box(m_position.x, m_position.y, BOWSER_SIZE,
                          BOWSER_SIZE);
  if (m_state == State::Idle) {
    SpriteRegistry::applyFrame(m_sprite, SpriteRegistry::bowserPath(), box,
                               true);
  } else {
    SpriteRegistry::applyBowserBreathFrame(m_sprite, m_breathFrame, box,
                                            true);
  }
  window.draw(m_sprite);
}

sf::FloatRect Bowser::getBounds() const {
  return sf::FloatRect(m_position.x, m_position.y, BOWSER_SIZE, BOWSER_SIZE);
}

void Bowser::onStomped() {
  // Every normal player contact with Bowser is lethal; Level therefore never
  // treats a landing on his head as a stomp kill.
}

bool Bowser::hitByFireball() {
  // Bowser survives the first four fireballs and is removed by the fifth.
  ++m_fireballHits;
  if (m_fireballHits >= 5) {
    Enemy::kill();
    return true;
  }
  return false;
}

bool Bowser::canBeStomped() const { return false; }

bool Bowser::usesTerrainCollisions() const { return false; }

Bowser::State Bowser::getState() const { return m_state; }

int Bowser::getBreathFrame() const { return m_breathFrame; }

int Bowser::getFireballHits() const { return m_fireballHits; }
