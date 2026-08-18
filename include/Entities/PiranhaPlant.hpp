#pragma once
#include "Enemy.hpp"

/// PiranhaPlant — stationary enemy that emerges from pipes periodically.
/// Two independent systems:
/// - Sprite animation: 95-frame cycle (head movement), runs continuously
/// - Vertical movement: HIDDEN → EMERGING → WAITING → RETRACTING → HIDDEN
/// Clipping hides the portion below the pipe mouth.
/// Cannot be stomped; only fireballs and star power can kill it.
class PiranhaPlant : public Enemy {
public:
    enum class State { HIDDEN, EMERGING, WAITING, RETRACTING };

    PiranhaPlant();

    void update(float dt) override;
    void draw(sf::RenderWindow& window) override;
    sf::FloatRect getBounds() const override;

    void onStomped() override;
    void kill() override;
    bool canBeStomped() const override;

private:
    State m_state = State::HIDDEN;
    int   m_currentFrame = 0;
    float m_frameTimer   = 0.0f;
    float m_hideTimer    = 0.0f;
    float m_waitTimer    = 0.0f;

    sf::Vector2f m_basePosition;
    bool  m_baseCaptured = false;
};
