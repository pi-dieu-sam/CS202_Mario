#pragma once
#include "Item.hpp"

/// Star — grants temporary invincibility to the player.
class Star : public Item {
public:
    Star();
    void activate(Player& player) override;
    void update(float dt) override;

protected:
    void refreshSprite() override;

private:
    float m_bounceVelocity = -200.0f; // bounces when moving
};
