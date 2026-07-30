#include "Entities/Mushroom.hpp"
#include "Entities/Player.hpp"
#include "Graphics/SpriteRegistry.hpp"
#include "Observers/EventManager.hpp"
#include "Physics/PhysicsConstants.hpp"

Mushroom::Mushroom() {
    m_type   = ObjectType::Mushroom;
    m_moving = true;
    m_velocity.x = 80.0f; // Moves right after spawning
    refreshSprite();
}

void Mushroom::activate(Player& player) {
    player.growBig();
    EventManager::getInstance().publish({EventType::PowerUpCollected, 1000, 0.0f, "Mushroom"});
}

void Mushroom::update(float dt) {
    Item::update(dt);
}

void Mushroom::refreshSprite() {
    m_texturePath = SpriteRegistry::mushroomPath(m_theme);
}
