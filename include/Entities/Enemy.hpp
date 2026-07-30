#ifndef ENEMY_HPP
#define ENEMY_HPP

#include <SFML/Graphics.hpp>
#include <string>

/**
 * @brief Minimal base enemy class for factory-created enemies.
 */
class Enemy {
public:
	Enemy(const std::string& enemyType = "Goomba", float x = 0.0f, float y = 0.0f)
		: m_enemyType(enemyType), m_position({x, y}), m_velocity({-80.0f, 0.0f}) {
		m_shape.setSize({28.0f, 28.0f});
		m_shape.setPosition(m_position);
		m_shape.setFillColor(enemyType == "Koopa" ? sf::Color::Green : sf::Color(150, 75, 0));
	}

	virtual ~Enemy() = default;

	virtual void update(float dt) {
		m_position += m_velocity * dt;
		m_shape.setPosition(m_position);
	}

	virtual void render(sf::RenderWindow& window) {
		window.draw(m_shape);
	}

	const std::string& getType() const { return m_enemyType; }
	sf::Vector2f getPosition() const { return m_position; }
	void setPosition(const sf::Vector2f& position) { m_position = position; m_shape.setPosition(position); }

protected:
	std::string m_enemyType;
	sf::Vector2f m_position;
	sf::Vector2f m_velocity;
	sf::RectangleShape m_shape;
};

#endif // ENEMY_HPP