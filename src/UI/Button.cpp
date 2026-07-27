#include "UI/Button.hpp"

namespace UI {

Button::Button(const std::string& label, const sf::Vector2f& position, unsigned int characterSize)
    : m_label(label), m_position(position), m_characterSize(characterSize), m_isSelected(false) {}

void Button::init(const sf::Font& font) {
    m_text.emplace(font, m_label, m_characterSize);
    m_text->setPosition(m_position);
    setSelected(m_isSelected);
}

void Button::setSelected(bool selected) {
    m_isSelected = selected;
    if (m_text) {
        if (m_isSelected) {
            m_text->setFillColor(sf::Color::Yellow);
            m_text->setStyle(sf::Text::Bold);
        } else {
            m_text->setFillColor(sf::Color::White);
            m_text->setStyle(sf::Text::Regular);
        }
    }
}

void Button::setLabel(const std::string& label) {
    m_label = label;
    if (m_text) {
        m_text->setString(m_label);
    }
}

void Button::render(sf::RenderWindow& window) {
    if (m_text) {
        window.draw(*m_text);
    }
}

} // namespace UI
