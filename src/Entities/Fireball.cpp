#include "Entities/Fireball.hpp"
#include "Graphics/SpriteRegistry.hpp"
#include "Physics/PhysicsConstants.hpp"
#include <cmath>

Fireball::Fireball(float x, float y, int direction)
    : m_direction(direction)
{
    m_type = ObjectType::Fireball;
    m_position = {x, y};
    m_velocity = {direction * 350.0f, 0.0f};
}

void Fireball::update(float dt) {
    if (!m_active) return;

    // Apply gravity (bouncing)
    m_velocity.y += GRAVITY * 0.5f * dt;
    m_position += m_velocity * dt;

    // Spin animation
    m_animTimer += dt;
    if (m_animTimer >= 0.08f) {
        m_animTimer = 0.0f;
        m_animFrame = (m_animFrame + 1) % SpriteRegistry::fireballFrameCount();
    }

    // Lifetime
    m_lifetime -= dt;
    if (m_lifetime <= 0.0f) {
        m_active = false;
    }

    // Off-screen check
    if (m_position.y > 800.0f || m_position.x < -50.0f || m_position.x > 10000.0f) {
        m_active = false;
    }
}

void Fireball::draw(sf::RenderWindow& window) {
    if (!m_active) return;
    SpriteRegistry::applySheetFrame(m_sprite, SpriteRegistry::fireballPath(),
                                    m_animFrame, 16, 0, getBounds());
    window.draw(m_sprite);
}

sf::FloatRect Fireball::getBounds() const {
    return sf::FloatRect(m_position.x, m_position.y, 10.0f, 10.0f);
}

void Fireball::onCollision(GameObject& other) {
    if (other.getType() == ObjectType::Tile || other.getType() == ObjectType::Block) {
        // Bounce off ground
        m_velocity.y = -200.0f;
    }
    if (other.getType() == ObjectType::Enemy) {
        m_active = false; // Destroy on hitting enemy
    }
}
