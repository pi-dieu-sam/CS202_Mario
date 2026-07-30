#pragma once
#include "Item.hpp"

/// FireFlower — upgrades the player to Fire state, enabling fireball shooting.
class FireFlower : public Item {
public:
    FireFlower();
    void activate(Player& player) override;

protected:
    void refreshSprite() override;
};
