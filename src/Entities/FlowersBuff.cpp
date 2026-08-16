#include "Entities/FlowersBuff.hpp"
#include "Entities/Player.hpp"
#include "Graphics/SpriteRegistry.hpp"
#include "Observers/EventManager.hpp"

namespace {
constexpr float SPIN_SPEED = 0.12f;
}

FlowersBuff::FlowersBuff() {
    m_type            = ObjectType::FlowersBuff;
    m_sheetFrameWidth = 16; // FlowersBuff.png cells are 16x16, 2px apart
    m_sheetGap        = 2;
    refreshSprite();
}

void FlowersBuff::activate(Player& player) {
    player.applySizeBuff();
    EventManager::getInstance().publish(
        {EventType::PowerUpCollected, 1000, 0.0f, "FlowersBuff"});
}

void FlowersBuff::update(float dt) {
    Item::update(dt);

    if (m_animTimer >= SPIN_SPEED) {
        m_animTimer = 0.0f;
        m_animFrame = (m_animFrame + 1) % SpriteRegistry::flowersBuffFrameCount();
    }
}

void FlowersBuff::refreshSprite() {
    m_texturePath = SpriteRegistry::flowersBuffPath();
}
