#ifndef ITEM_HPP
#define ITEM_HPP

#include <SFML/Graphics.hpp>
#include <string>

/**
 * @brief Minimal base item class for factory-created items.
 */
class Item {
public:
	Item(const std::string& itemType = "Coin", float x = 0.0f, float y = 0.0f)
		: m_itemType(itemType), m_position({x, y}), m_collected(false) {
		m_shape.setSize({24.0f, 24.0f});
		m_shape.setPosition(m_position);
		m_shape.setFillColor(itemType == "Mushroom" ? sf::Color::Red : sf::Color::Yellow);
	}

	virtual ~Item() = default;

	virtual void update(float dt) {
		(void)dt;
	}

	virtual void render(sf::RenderWindow& window) {
		if (!m_collected) {
			window.draw(m_shape);
		}
	}

	const std::string& getType() const { return m_itemType; }
	bool isCollected() const { return m_collected; }
	void collect() { m_collected = true; }
	sf::Vector2f getPosition() const { return m_position; }
	void setPosition(const sf::Vector2f& position) { m_position = position; m_shape.setPosition(position); }

protected:
	std::string m_itemType;
	sf::Vector2f m_position;
	bool m_collected;
	sf::RectangleShape m_shape;
};

#endif // ITEM_HPP