#include "Entities/Character.hpp"
#include "Graphics/SpriteRegistry.hpp"
#include "Physics/PhysicsConstants.hpp"
#include <cmath>

Character::Character() {
    m_type = ObjectType::None;
}

void Character::moveLeft(float dt) {
    m_skidding = (m_facingRight && m_velocity.x > 10.0f);
    m_velocity.x = -getEffectiveSpeed();
    m_facingRight = false;
}

void Character::moveRight(float dt) {
    m_skidding = (!m_facingRight && m_velocity.x < -10.0f);
    m_velocity.x = getEffectiveSpeed();
    m_facingRight = true;
}

float Character::getEffectiveSpeed() const { return m_speed; }

void Character::jump() {
    if (m_grounded) {
        m_velocity.y = m_jumpForce;
        m_grounded = false;
    }
}

void Character::stopMoving() {
    m_velocity.x = 0.0f;
}

void Character::takeDamage(int amount) {
    m_health -= amount;
    if (m_health <= 0) {
        die();
    }
}

void Character::die() {
    m_dead = true;
    m_velocity = {0.0f, 0.0f};
}

bool Character::isDead() const { return m_dead; }

void Character::applyGravity(float dt) {
    m_velocity.y += GRAVITY * dt;
    if (m_velocity.y > MAX_FALL_SPEED) {
        m_velocity.y = MAX_FALL_SPEED;
    }
}

void Character::applyFriction() {
    m_velocity.x *= GROUND_FRICTION;
    if (std::abs(m_velocity.x) < 1.0f) {
        m_velocity.x = 0.0f;
    }
}

bool Character::isGrounded() const { return m_grounded; }
void Character::setGrounded(bool grounded) { m_grounded = grounded; }
bool Character::isFacingRight() const { return m_facingRight; }

float Character::getSpeed() const { return m_speed; }
void  Character::setSpeed(float speed) { m_speed = speed; }
float Character::getJumpForce() const { return m_jumpForce; }
void  Character::setJumpForce(float force) { m_jumpForce = force; }
int   Character::getHealth() const { return m_health; }
void  Character::setHealth(int hp) { m_health = hp; }

void Character::updateSprite(float dt) {
    m_animTimer += dt;
    if (m_animTimer >= m_animSpeed) {
        m_animTimer = 0.0f;
        m_animFrame = (m_animFrame + 1) % m_animFrames;
    }
}

void Character::setAnimFrameCount(int frames, float speed) {
    if (m_animFrames != frames) {
        m_animFrames = frames;
        m_animFrame = 0;
        m_animTimer = 0.0f;
    }
    if (speed > 0.0f) {
        m_animSpeed = speed;
    }
}

void Character::drawSprite(sf::RenderWindow &window, const std::string &path,
                            const sf::FloatRect &box, sf::Color tint) {
    SpriteRegistry::applyFrame(m_sprite, path, box, !m_facingRight);
    m_sprite.setColor(tint);
    window.draw(m_sprite);
}
