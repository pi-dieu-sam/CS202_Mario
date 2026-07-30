#pragma once
#include <SFML/System/Vector2.hpp>

// Forward declaration
class Enemy;

/// AIStrategy — Abstract base for the Strategy pattern.
/// Each concrete strategy defines a different enemy movement behavior.
class AIStrategy {
public:
    virtual ~AIStrategy() = default;
    virtual void execute(Enemy& enemy, float dt) = 0;

    /// Fed the player's current position once per frame before execute()
    /// runs. No-op by default -- only strategies that care (ChaseStrategy)
    /// need to override it.
    virtual void setPlayerPosition(const sf::Vector2f&) {}
};