#include "Entities/Star.hpp"
#include "Entities/Player.hpp"
#include "Graphics/SpriteRegistry.hpp"
#include "Observers/EventManager.hpp"
#include "Physics/PhysicsConstants.hpp"

Star::Star() {
    m_type   = ObjectType::Star;
    m_isGif  = true;  // Starman.gif has multiple sparkle frames
    m_moving = true;
    m_velocity.x = 100.0f;
    m_velocity.y = m_bounceVelocity;
    refreshSprite();
}

void Star::activate(Player& player) {
    player.setStarPower(10.0f); // 10 seconds of invincibility
    EventManager::getInstance().publish({EventType::PowerUpCollected, 1000, 0.0f, "Star"});
}

void Star::update(float dt) {
    if (!m_active) return;

    // Bouncing movement
    m_velocity.y += GRAVITY * dt;
    m_position += m_velocity * dt;

    m_animTimer += dt;
    static constexpr float FLICKER_SPEED = 0.1f;
    if (m_animTimer >= FLICKER_SPEED) {
        m_animTimer = 0.0f;
        m_animFrame = (m_animFrame + 1) % SpriteRegistry::starFrameCount();
        m_texturePath = SpriteRegistry::starPath(m_theme, m_animFrame);
    }
}

void Star::refreshSprite() {
    m_texturePath = SpriteRegistry::starPath(m_theme, m_animFrame);
}

void Star::onLanded() {
    m_velocity.y = m_bounceVelocity;
}
