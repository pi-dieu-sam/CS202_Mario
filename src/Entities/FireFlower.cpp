#include "Entities/FireFlower.hpp"
#include "Entities/Player.hpp"
#include "Graphics/SpriteRegistry.hpp"
#include "Observers/EventManager.hpp"

FireFlower::FireFlower() {
    m_type  = ObjectType::FireFlower;
    m_isGif = true;  // Fire_Flower_SMB.gif has multiple blossom frames
    refreshSprite();
}

void FireFlower::activate(Player& player) {
    player.enableFire();
    EventManager::getInstance().publish({EventType::PowerUpCollected, 1000, 0.0f, "FireFlower"});
}

void FireFlower::refreshSprite() {
    m_texturePath = SpriteRegistry::fireFlowerPath(m_theme);
}
