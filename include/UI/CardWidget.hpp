#ifndef CARD_WIDGET_HPP
#define CARD_WIDGET_HPP

#include <SFML/Graphics.hpp>
#include <string>
#include <optional>

namespace UI {

/**
 * @brief Card widget for displaying character stats and selection highlights.
 */
class CardWidget {
public:
    CardWidget(const std::string& heroName, const std::string& statsText, 
               const sf::Vector2f& position, const sf::Color& cardColor);
    ~CardWidget() = default;

    void init(const sf::Font& font);
    void setSelected(bool selected);
    bool isSelected() const { return m_isSelected; }

    void render(sf::RenderWindow& window);

private:
    std::string m_heroName;
    std::string m_statsText;
    sf::Vector2f m_position;
    sf::Color m_cardColor;
    bool m_isSelected;

    sf::RectangleShape m_cardBackground;
    std::optional<sf::Text> m_titleText;
    std::optional<sf::Text> m_bodyText;
};

} // namespace UI

#endif // CARD_WIDGET_HPP
