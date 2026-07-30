#include "Entities/GameObject.hpp"

GameObject::GameObject() {}

void GameObject::onCollision(GameObject& other) {
    // Default: no reaction. Subclasses override as needed.
}

sf::Vector2f GameObject::getPosition() const { return m_position; }
void GameObject::setPosition(float x, float y) { m_position = {x, y}; }
void GameObject::setPosition(const sf::Vector2f& pos) { m_position = pos; }

sf::Vector2f GameObject::getVelocity() const { return m_velocity; }
void GameObject::setVelocity(float vx, float vy) { m_velocity = {vx, vy}; }
void GameObject::setVelocity(const sf::Vector2f& vel) { m_velocity = vel; }

bool GameObject::isActive() const { return m_active; }
void GameObject::setActive(bool active) { m_active = active; }

ObjectType GameObject::getType() const { return m_type; }
