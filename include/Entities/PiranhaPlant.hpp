#pragma once
#include "Enemy.hpp"

/// PiranhaPlant — stationary enemy that emerges from pipes periodically.
class PiranhaPlant : public Enemy {
public:
    PiranhaPlant();

    void update(float dt) override;
    void draw(sf::RenderWindow& window) override;
    void onStomped() override; // Cannot be stomped normally

private:
    float m_emergeTimer  = 0.0f;
    float m_emergeCycle  = 3.0f;   // seconds for full emerge/retract cycle
    float m_emergeOffset = 0.0f;   // current vertical offset
    float m_maxEmerge    = 32.0f;  // how far it comes out of the pipe
    bool  m_emerging     = true;
    sf::Vector2f m_basePosition;   // position of the pipe top
    bool  m_baseCaptured = false;  // true once m_basePosition is set from spawn
};
