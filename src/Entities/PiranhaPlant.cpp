#include "Entities/PiranhaPlant.hpp"
#include "Core/AssetManager.hpp"
#include "Graphics/SpriteRegistry.hpp"
#include "Physics/PhysicsConstants.hpp"
#include <algorithm>

namespace {
// The source GIF is 16x24 NES pixels. Render it at the same 2x scale as the
// 16px environment art, producing a stable 32x48 world-space plant.
constexpr float PLANT_WIDTH      = TILE_SIZE;
constexpr float PLANT_HEIGHT     = TILE_SIZE * 1.5f;
constexpr float PIPE_MOUTH_HEIGHT = 16.0f;  // pipe mouth hides bottom of sprite
constexpr float MOVE_SPEED       = 80.0f;   // pixels/sec vertical movement
constexpr float HIDE_DURATION    = 2.0f;    // seconds hidden
constexpr float WAIT_DURATION    = 2.5f;    // seconds waiting at top
constexpr float FRAME_SPEED      = 0.20f;   // seconds per mouth pose
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

    // Animate only while visible. Resetting on each emergence prevents time
    // spent hidden from choosing an arbitrary first mouth pose.
    if (m_state != State::HIDDEN) {
        m_frameTimer += dt;
        while (m_frameTimer >= FRAME_SPEED) {
            m_frameTimer -= FRAME_SPEED;
            m_currentFrame = (m_currentFrame + 1) %
                             SpriteRegistry::piranhaFrameCount();
        }
    }

    float hiddenY  = m_basePosition.y;
    float visibleY = m_basePosition.y - PLANT_HEIGHT;

    switch (m_state) {
    case State::HIDDEN:
        m_hideTimer += dt;
        if (m_hideTimer >= HIDE_DURATION) {
            m_state = State::EMERGING;
            m_hideTimer = 0.0f;
            m_currentFrame = 0;
            m_frameTimer = 0.0f;
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

    if (cell.width <= 0 || cell.height <= 0) return;

    const auto &frames = AssetManager::getInstance().getGifFrames(
        SpriteRegistry::piranhaPlantPath(m_currentFrame));
    if (frames.empty()) return;

    const sf::Texture &selectedFrame =
        frames[static_cast<size_t>(m_currentFrame) % frames.size()];
    sf::Texture &texture = const_cast<sf::Texture &>(selectedFrame);

    // Horizontal placement belongs to the pipe anchor, not the mutable
    // animation position. This keeps the plant centered over the two tiles.
    float pipeCenterX = m_basePosition.x + TILE_SIZE;
    const float sourceScale = PLANT_WIDTH / static_cast<float>(cell.width);

    // Only show portion above the pipe mouth bottom.
    // m_position.y is the top of the sprite; it starts at basePosition
    // (pipe mouth bottom) and travels upward by one sprite height.
    const float visibleH = std::clamp(m_basePosition.y - m_position.y,
                                      0.0f, PLANT_HEIGHT);

    // Keep the NES 2x scale constant while revealing whole source-pixel rows.
    const int cropH = std::clamp(
        static_cast<int>(visibleH / sourceScale + 0.001f), 0, cell.height);
    if (cropH <= 0) return;
    const float drawnH = static_cast<float>(cropH) * sourceScale;

    // Crop from the top of the texture — shows head first as it rises
    sf::IntRect crop(cell.left, cell.top, cell.width, cropH);

    // Bottom of visible portion sits at pipe mouth bottom
    sf::FloatRect box(pipeCenterX - PLANT_WIDTH / 2.0f,
                       m_basePosition.y - drawnH,
                       PLANT_WIDTH, drawnH);

    SpriteRegistry::applyFrame(m_sprite, texture, crop, box);
    window.draw(m_sprite);
}

sf::FloatRect PiranhaPlant::getBounds() const {
    if (m_state == State::HIDDEN) return sf::FloatRect();

    float pipeCenterX = m_basePosition.x + TILE_SIZE;

    float plantTop = m_position.y;
    if (plantTop >= m_basePosition.y) return sf::FloatRect();

    float visibleH = m_basePosition.y - plantTop;
    if (visibleH > PLANT_HEIGHT) visibleH = PLANT_HEIGHT;

    return sf::FloatRect(pipeCenterX - PLANT_WIDTH / 2.0f + 2,
                          m_basePosition.y - visibleH,
                          PLANT_WIDTH - 4, visibleH);
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

bool PiranhaPlant::usesTerrainCollisions() const { return false; }
