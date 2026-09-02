#include "UI/Button.hpp"

Button::Button() {}

Button::Button(const std::string& text, const sf::Font& font, unsigned int charSize) {
    m_text.setFont(font);
    m_text.setString(text);
    m_text.setCharacterSize(charSize);
    m_text.setFillColor(sf::Color::White);

    auto bounds = m_text.getLocalBounds();
    m_background.setSize(sf::Vector2f(bounds.width + 30.0f, bounds.height + 20.0f));
    m_background.setFillColor(m_normalColor);
}

void Button::setPosition(float x, float y) {
    m_background.setPosition(x, y);
    auto bgBounds = m_background.getLocalBounds();
    auto txtBounds = m_text.getLocalBounds();
    m_text.setPosition(
        x + (bgBounds.width - txtBounds.width) / 2.0f - txtBounds.left,
        y + (bgBounds.height - txtBounds.height) / 2.0f - txtBounds.top
    );
}

void Button::setCallback(std::function<void()> callback) {
    m_callback = std::move(callback);
}

bool Button::handleEvent(const sf::Event& event, const sf::RenderWindow& window) {
    if (event.type == sf::Event::MouseButtonPressed) {
        sf::Vector2f mousePos = window.mapPixelToCoords(
            sf::Mouse::getPosition(window));
        if (m_background.getGlobalBounds().contains(mousePos)) {
            if (m_callback) m_callback();
            return true;
        }
    }
    return false;
}

void Button::update(const sf::RenderWindow& window) {
    sf::Vector2f mousePos = window.mapPixelToCoords(
        sf::Mouse::getPosition(window));
    m_hovered = m_background.getGlobalBounds().contains(mousePos);

    if (m_selected) {
        m_background.setFillColor(m_selectedColor);
    } else if (m_hovered) {
        m_background.setFillColor(m_hoverColor);
    } else {
        m_background.setFillColor(m_normalColor);
    }
}

void Button::draw(sf::RenderWindow& window) {
    window.draw(m_background);
    window.draw(m_text);
}

void Button::setSelected(bool selected) { m_selected = selected; }
bool Button::isSelected() const { return m_selected; }