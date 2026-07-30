#pragma once
#include "Item.hpp"

/// Coin — gives +100 score when collected.
class Coin : public Item {
public:
    Coin();
    void activate(Player& player) override;
    void update(float dt) override;

protected:
    void refreshSprite() override;
};
