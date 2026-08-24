#include "Entities/PiranhaPlant.hpp"
#include "Core/AssetManager.hpp"
#include "Graphics/SpriteRegistry.hpp"
#include "Physics/PhysicsConstants.hpp"
#include <algorithm>

namespace {
constexpr float PLANT_HEIGHT     = 66.0f;   // sprite frame height
constexpr float PIPE_MOUTH_HEIGHT = 16.0f;  // pipe mouth hides bottom of sprite
constexpr float MOVE_SPEED       = 80.0f;   // pixels/sec vertical movement
constexpr float HIDE_DURATION    = 2.0f;    // seconds hidden
constexpr float WAIT_DURATION    = 2.5f;    // seconds waiting at top
constexpr float FRAME_SPEED      = 0.04f;   // seconds per animation frame
} // namespace

PiranhaPlant::PiranhaPlant() {
    m_speed      = 0.0f;
    m_scoreValue = 100;
    m_strategy   = nullptr;
}

void PiranhaPlant::update(float dt) {
    if (m_dead) return;

    if (!m_baseCaptured) {
        m_basePosition = m_position;
        m_basePosition.y += TILE_SIZE + PIPE_MOUTH_HEIGHT;
        m_position.y = m_basePosition.y;
        m_baseCaptured = true;
    }

    // Sprite animation runs independently of vertical movement
    m_frameTimer += dt;
    while (m_frameTimer >= FRAME_SPEED) {
        m_frameTimer -= FRAME_SPEED;
        m_currentFrame = (m_currentFrame + 1) %
                         SpriteRegistry::piranhaFrameCount();
    }

    float hiddenY  = m_basePosition.y;
    float visibleY = m_basePosition.y - PLANT_HEIGHT;

    switch (m_state) {
    case State::HIDDEN:
        m_hideTimer += dt;
        if (m_hideTimer >= HIDE_DURATION) {
            m_state = State::EMERGING;
            m_hideTimer = 0.0f;
        }
        break;

    case State::EMERGING:
        m_position.y -= MOVE_SPEED * dt;
        if (m_position.y <= visibleY) {
            m_position.y = visibleY;
            m_state = State::WAITING;
            m_waitTimer = 0.0f;
        }
        break;

    case State::WAITING:
        m_waitTimer += dt;
        if (m_waitTimer >= WAIT_DURATION) {
            m_state = State::RETRACTING;
        }
        break;

    case State::RETRACTING:
        m_position.y += MOVE_SPEED * dt;
        if (m_position.y >= hiddenY) {
            m_position.y = hiddenY;
            m_state = State::HIDDEN;
            m_hideTimer = 0.0f;
        }
        break;
    }
}

void PiranhaPlant::draw(sf::RenderWindow& window) {
    if (!m_active || m_state == State::HIDDEN) return;

    sf::IntRect cell = SpriteRegistry::piranhaFrameRect(m_currentFrame);

    float pipeCenterX = m_position.x + TILE_SIZE;
    float w = static_cast<float>(cell.width);
    float h = static_cast<float>(cell.height);

    // Only show portion above the pipe mouth bottom.
    // m_position.y is the top of the sprite; it starts at basePosition
    // (pipe mouth bottom) and travels upward by one sprite height.
    const float visibleH = std::clamp(m_basePosition.y - m_position.y,
                                      0.0f, h);

    const int cropH = static_cast<int>(visibleH);
    if (cropH <= 0) return;

    // Crop from the top of the texture — shows head first as it rises
    sf::IntRect crop(cell.left, cell.top, cell.width, cropH);

    // Bottom of visible portion sits at pipe mouth bottom
    sf::FloatRect box(pipeCenterX - w / 2.0f,
                       m_basePosition.y - visibleH,
                       w, visibleH);

    sf::Texture &texture = AssetManager::getInstance().getTexture(
        SpriteRegistry::piranhaPlantPath(0));
    SpriteRegistry::applyFrame(m_sprite, texture, crop, box);
    window.draw(m_sprite);
}

sf::FloatRect PiranhaPlant::getBounds() const {
    if (m_state == State::HIDDEN) return sf::FloatRect();

    sf::IntRect cell = SpriteRegistry::piranhaFrameRect(m_currentFrame);
    float pipeCenterX = m_position.x + TILE_SIZE;
    float w = static_cast<float>(cell.width);
    float h = static_cast<float>(cell.height);

    float plantTop = m_position.y;
    if (plantTop >= m_basePosition.y) return sf::FloatRect();

    float visibleH = m_basePosition.y - plantTop;
    if (visibleH > h) visibleH = h;

    return sf::FloatRect(pipeCenterX - w / 2.0f + 2,
                          m_basePosition.y - visibleH,
                          w - 4, visibleH);
}

void PiranhaPlant::onStomped() {
    die();
    m_active = false;
}

void PiranhaPlant::kill() {
    die();
    m_active = false;
}

bool PiranhaPlant::canBeStomped() const { return false; }
