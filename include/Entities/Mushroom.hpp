#pragma once
#include "Item.hpp"

/// Mushroom — grows the player from Small to Big.
/// Moves sideways after spawning from a question block.
class Mushroom : public Item {
public:
    Mushroom();
    void activate(Player& player) override;
    void update(float dt) override;

protected:
    void refreshSprite() override;
};
