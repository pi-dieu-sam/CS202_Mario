#pragma once
#include <SFML/Graphics.hpp>
#include <functional>
#include <string>

/// Button — clickable menu element with hover/press states.
class Button {
public:
    Button();
    Button(const std::string& text, const sf::Font& font, unsigned int charSize = 24);

    /// Set position.
    void setPosition(float x, float y);

    /// Set callback for click.
    void setCallback(std::function<void()> callback);

    /// Handle mouse events. Returns true if clicked.
    bool handleEvent(const sf::Event& event, const sf::RenderWindow& window);

    /// Update hover state.
    void update(const sf::RenderWindow& window);

    /// Draw the button.
    void draw(sf::RenderWindow& window);

    /// Selection state (for keyboard navigation).
    void setSelected(bool selected);
    bool isSelected() const;

private:
    sf::RectangleShape       m_background;
    sf::Text                 m_text;
    std::function<void()>    m_callback;
    bool                     m_hovered  = false;
    bool                     m_selected = false;

    sf::Color m_normalColor   = sf::Color(50, 50, 50, 200);
    sf::Color m_hoverColor    = sf::Color(80, 80, 80, 220);
    sf::Color m_selectedColor = sf::Color(200, 100, 0, 220);
};