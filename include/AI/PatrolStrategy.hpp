#pragma once
#include "AIStrategy.hpp"

/// PatrolStrategy — enemy walks back and forth,
/// reversing direction when hitting a wall or edge.
class PatrolStrategy : public AIStrategy {
public:
    void execute(Enemy& enemy, float dt) override;
};