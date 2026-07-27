#ifndef BUTTON_HPP
#define BUTTON_HPP

#include <SFML/Graphics.hpp>
#include <string>
#include <optional>

namespace UI {

/**
 * @brief Reusable UI Button widget for game menus and user selection screens.
 */
class Button {
public:
    Button(const std::string& label, const sf::Vector2f& position, unsigned int characterSize = 24);
    ~Button() = default;

    void init(const sf::Font& font);
    void setSelected(bool selected);
    bool isSelected() const { return m_isSelected; }

    void setLabel(const std::string& label);
    const std::string& getLabel() const { return m_label; }

    void render(sf::RenderWindow& window);

private:
    std::string m_label;
    sf::Vector2f m_position;
    unsigned int m_characterSize;
    bool m_isSelected;

    std::optional<sf::Text> m_text;
};

} // namespace UI

#endif // BUTTON_HPP
