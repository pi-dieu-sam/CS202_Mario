#ifndef PLAYER_HPP
#define PLAYER_HPP

#include <SFML/Graphics.hpp>
#include <string>

/**
 * @brief Base Player character class.
 */
class Player {
public:
    Player(const std::string& name = "Mario", float x = 100.0f, float y = 400.0f)
        : m_name(name), m_position({x, y}), m_velocity({0.0f, 0.0f}), m_isGrounded(true) {
        m_shape.setSize({30.0f, 48.0f});
        m_shape.setPosition(m_position);
        m_shape.setFillColor(name == "Luigi" ? sf::Color::Green : sf::Color::Red);
    }

    virtual ~Player() = default;

    virtual void moveLeft() { m_velocity.x = -200.0f; }
    virtual void moveRight() { m_velocity.x = 200.0f; }
    virtual void jump() { if (m_isGrounded) { m_velocity.y = -450.0f; m_isGrounded = false; } }
    virtual void duck() {}
    virtual void shoot() {}
    virtual void stopMoving() { m_velocity.x = 0.0f; }

    virtual void update(float dt) {
        // Gravity
        m_velocity.y += 980.0f * dt;
        m_position += m_velocity * dt;

        // Ground floor check
        if (m_position.y >= 450.0f) {
            m_position.y = 450.0f;
            m_velocity.y = 0.0f;
            m_isGrounded = true;
        }

        m_shape.setPosition(m_position);
    }

    virtual void render(sf::RenderWindow& window) {
        window.draw(m_shape);
    }

    sf::Vector2f getPosition() const { return m_position; }
    void setPosition(const sf::Vector2f& pos) { m_position = pos; m_shape.setPosition(pos); }
    const std::string& getName() const { return m_name; }

protected:
    std::string m_name;
    sf::Vector2f m_position;
    sf::Vector2f m_velocity;
    bool m_isGrounded;
    sf::RectangleShape m_shape;
};

#endif // PLAYER_HPP
