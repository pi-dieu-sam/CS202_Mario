#pragma once
#include "AIStrategy.hpp"
#include <SFML/Graphics.hpp>

/// ChaseStrategy — enemy moves toward the player when within detection radius.
/// Falls back to patrol behavior when the player is out of range.
class ChaseStrategy : public AIStrategy {
public:
    explicit ChaseStrategy(float detectionRadius = 200.0f);
    void execute(Enemy& enemy, float dt) override;

    /// Set the player position so the enemy knows where to chase.
    void setPlayerPosition(const sf::Vector2f& playerPos) override;

private:
    sf::Vector2f m_playerPos;
    float        m_detectionRadius;
};