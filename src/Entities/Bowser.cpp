#include "Entities/Bowser.hpp"
#include "Graphics/SpriteRegistry.hpp"
#include "Physics/PhysicsConstants.hpp"
#include <algorithm>

namespace {
constexpr float BOWSER_SIZE = TILE_SIZE * 2.0f;
constexpr float IDLE_DURATION = 2.0f;
constexpr float BREATH_DURATION = 3.0f;
constexpr float FIRE_RANGE = TILE_SIZE * 30.0f;
constexpr int FIREBALLS_PER_IDLE = 3;
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
    // Bowser is fixed facing left, so only a player within 30 tiles to his
    // left can trigger these three evenly-spaced shots during the idle phase.
    const float fireInterval = IDLE_DURATION / FIREBALLS_PER_IDLE;
    while (m_nextFireTime < IDLE_DURATION &&
           m_stateTimer >= m_nextFireTime) {
      const float distanceLeft = m_position.x - m_playerPosition.x;
      if (m_hasPlayerPosition && distanceLeft > 0.0f &&
          distanceLeft <= FIRE_RANGE) {
        ++m_pendingFireballs;
      }
      m_nextFireTime += fireInterval;
    }

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
      m_nextFireTime = 0.0f;
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

void Bowser::updatePlayerPosition(const sf::Vector2f &playerPos) {
  m_playerPosition = playerPos;
  m_hasPlayerPosition = true;
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

int Bowser::takePendingFireballs() {
  const int count = m_pendingFireballs;
  m_pendingFireballs = 0;
  return count;
}
