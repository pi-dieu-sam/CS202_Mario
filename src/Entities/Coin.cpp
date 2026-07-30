#include "Entities/Coin.hpp"
#include "Entities/Player.hpp"
#include "Graphics/SpriteRegistry.hpp"
#include "Observers/EventManager.hpp"
#include "Physics/PhysicsConstants.hpp"
#include <cmath>

Coin::Coin() {
    m_type = ObjectType::Coin;
    refreshSprite();
}

void Coin::activate(Player& player) {
    EventManager::getInstance().publish({EventType::CoinCollected, COIN_SCORE});
}

void Coin::update(float dt) {
    Item::update(dt);

    static constexpr float SPIN_SPEED = 0.15f;
    if (m_animTimer >= SPIN_SPEED) {
        m_animTimer = 0.0f;
        m_animFrame = (m_animFrame + 1) % SpriteRegistry::coinFrameCount();
        m_texturePath = SpriteRegistry::coinPath(m_theme, m_animFrame);
    }
}

void Coin::refreshSprite() {
    m_texturePath = SpriteRegistry::coinPath(m_theme, m_animFrame);
}
