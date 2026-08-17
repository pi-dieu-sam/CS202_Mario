#include "Entities/Enemy.hpp"
#include "AI/AIStrategy.hpp"
#include "Physics/PhysicsConstants.hpp"

Enemy::Enemy() {
    m_type   = ObjectType::Enemy;
    m_speed  = 80.0f;
    m_health = 1;
}

void Enemy::update(float dt) {
    if (m_dead) return;

    // Delegate movement to AI strategy
    if (m_strategy) {
        m_strategy->execute(*this, dt);
    }

    applyGravity(dt);
    m_position += m_velocity * dt;

    // Fall off screen
    if (m_position.y > 800.0f) {
        m_active = false;
    }

    // Sync facing direction with movement
    if (m_velocity.x > 0.0f) m_facingRight = true;
    else if (m_velocity.x < 0.0f) m_facingRight = false;

    updateSprite(dt);
}

void Enemy::draw(sf::RenderWindow& window) {
    if (!m_active) return;

    // Placeholder rectangle
    sf::RectangleShape shape;
    shape.setSize(sf::Vector2f(TILE_SIZE - 2, TILE_SIZE - 2));
    shape.setPosition(m_position.x + 1, m_position.y + 1);
    shape.setFillColor(sf::Color(139, 69, 19)); // Brown
    window.draw(shape);
}

sf::FloatRect Enemy::getBounds() const {
    return sf::FloatRect(m_position.x + 1, m_position.y + 1,
                         TILE_SIZE - 2, TILE_SIZE - 2);
}

void Enemy::onStomped() {
    die();
    m_active = false;
}

void Enemy::kill() {
    die();
    m_active = false;
}

bool Enemy::isVulnerable() const { return true; }

void Enemy::setStrategy(std::unique_ptr<AIStrategy> strategy) {
    m_strategy = std::move(strategy);
}

void Enemy::updatePlayerPosition(const sf::Vector2f& playerPos) {
    if (m_strategy) {
        m_strategy->setPlayerPosition(playerPos);
    }
}

int Enemy::getScoreValue() const { return m_scoreValue; }

void Enemy::setTheme(LevelTheme theme) { m_theme = theme; }
