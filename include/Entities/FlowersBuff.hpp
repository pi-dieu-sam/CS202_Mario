#pragma once
#include "Item.hpp"

/// FlowersBuff — grants the player a temporary size/speed/jump buff.
/// Picking one up grows the player to 1.5x size (gradually over 0.7s) and
/// adds +0.2 to both the speed and jump multipliers for 40 seconds.
class FlowersBuff : public Item {
public:
    FlowersBuff();
    void activate(Player& player) override;
    void update(float dt) override;

protected:
    void refreshSprite() override;
};
