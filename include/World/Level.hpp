#ifndef LEVEL_HPP
#define LEVEL_HPP

#include <SFML/Graphics.hpp>
#include <string>
#include <vector>

/**
 * @brief Level container holding tilemaps, background elements, and entity lists.
 */
class Level {
public:
    explicit Level(int levelIndex = 1) : m_levelIndex(levelIndex) {}
    ~Level() = default;

    bool loadFromFile(const std::string& filename) {
        (void)filename;
        return true;
    }

    void update(float dt) {
        (void)dt;
    }

    void render(sf::RenderWindow& window) {
        // Draw basic ground line
        sf::RectangleShape floor({3200.0f, 100.0f});
        floor.setPosition({0.0f, 498.0f});
        floor.setFillColor(sf::Color(139, 69, 19));
        window.draw(floor);
    }

    float getWidth() const { return 3200.0f; }

private:
    int m_levelIndex;
};

#endif // LEVEL_HPP
