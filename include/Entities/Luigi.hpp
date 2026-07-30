#pragma once
#include "Player.hpp"

/// Luigi — higher jump, but lower traction (slides more).
class Luigi : public Player {
public:
    Luigi();
    void applyFriction() override;
};
