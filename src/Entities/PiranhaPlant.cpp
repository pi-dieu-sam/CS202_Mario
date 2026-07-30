#include "Entities/PiranhaPlant.hpp"
#include "Core/AssetManager.hpp"
#include "Graphics/SpriteRegistry.hpp"
#include "Physics/PhysicsConstants.hpp"
#include <cmath>

PiranhaPlant::PiranhaPlant() {
    m_speed      = 0.0f; // Stationary
    m_scoreValue = 100;
    m_strategy   = nullptr; // No AI strategy needed

    setAnimFrameCount(2, 0.4f);
}

void PiranhaPlant::update(float dt) {
    if (m_dead) return;

    // EntityFactory sets m_position via the base GameObject::setPosition,
    // which isn't virtual, so this can't be captured by overriding it --
    // grab the spawn position the first time update() runs instead.
    if (!m_baseCaptured) {
        m_basePosition = m_position;
        m_baseCaptured = true;
    }

    m_emergeTimer += dt;
    float t = std::fmod(m_emergeTimer, m_emergeCycle) / m_emergeCycle;

    // Sinusoidal emerge/retract
    m_emergeOffset = m_maxEmerge * std::sin(t * 3.14159f);
    m_position.y = m_basePosition.y - m_emergeOffset;

    updateSprite(dt);
}

void PiranhaPlant::draw(sf::RenderWindow& window) {
    if (!m_active || m_emergeOffset < 2.0f) return; // Don't draw when fully retracted

    // The plant slides up out of the pipe, so only the top portion of its
    // sprite (head/mouth) should be visible until it's fully emerged. Crop
    // the texture's height to match, and let applyFrame's bottom-anchor
    // keep the crop flush with the pipe top (m_basePosition.y).
    sf::Texture &texture = AssetManager::getInstance().getTexture(
        SpriteRegistry::piranhaPlantPath(m_animFrame));
    sf::Vector2u size = texture.getSize();
    float scale = m_maxEmerge / static_cast<float>(size.y);
    int visibleSourcePx = static_cast<int>(m_emergeOffset / scale);
    if (visibleSourcePx > static_cast<int>(size.y)) visibleSourcePx = size.y;
    sf::IntRect croppedRect(0, 0, static_cast<int>(size.x), visibleSourcePx);

    sf::FloatRect box(m_position.x + 2, m_basePosition.y - m_emergeOffset,
                       TILE_SIZE - 4, m_emergeOffset);
    SpriteRegistry::applyFrame(m_sprite, texture, croppedRect, box);
    window.draw(m_sprite);
}

void PiranhaPlant::onStomped() {
    // Piranha plants cannot be stomped — they only die to fireballs or star power
}
