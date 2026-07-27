#ifndef HUD_HPP
#define HUD_HPP

#include <SFML/Graphics.hpp>
#include <string>
#include <optional>

namespace UI {

/**
 * @brief Class managing all head-up display elements during gameplay.
 */
class HUD {
public:
    HUD();
    ~HUD() = default;

    bool init(const std::string& fontPath = "assets/fonts/arial.ttf");
    void update(const std::string& characterName, int levelIndex, int score, int coins, int lives, float levelTime);
    void render(sf::RenderWindow& window);

private:
    sf::Font m_font;
    std::optional<sf::Text> m_hudText;
};

} // namespace UI

#endif // HUD_HPP
