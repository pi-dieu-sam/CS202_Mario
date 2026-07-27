#include "UI/CardWidget.hpp"

namespace UI {

CardWidget::CardWidget(const std::string& heroName, const std::string& statsText, 
                       const sf::Vector2f& position, const sf::Color& cardColor)
    : m_heroName(heroName), m_statsText(statsText), m_position(position), 
      m_cardColor(cardColor), m_isSelected(false) {}

void CardWidget::init(const sf::Font& font) {
    m_cardBackground.setSize({260.0f, 320.0f});
    m_cardBackground.setPosition(m_position);
    m_cardBackground.setFillColor(m_cardColor);
    m_cardBackground.setOutlineThickness(4.0f);

    m_titleText.emplace(font, m_heroName, 30);
    m_titleText->setFillColor(sf::Color::White);
    m_titleText->setPosition({m_position.x + 70.0f, m_position.y + 20.0f});

    m_bodyText.emplace(font, m_statsText, 18);
    m_bodyText->setFillColor(sf::Color::White);
    m_bodyText->setPosition({m_position.x + 20.0f, m_position.y + 100.0f});

    setSelected(m_isSelected);
}

void CardWidget::setSelected(bool selected) {
    m_isSelected = selected;
    if (m_isSelected) {
        m_cardBackground.setOutlineColor(sf::Color::Yellow);
    } else {
        m_cardBackground.setOutlineColor(sf::Color::Transparent);
    }
}

void CardWidget::render(sf::RenderWindow& window) {
    window.draw(m_cardBackground);
    if (m_titleText) window.draw(*m_titleText);
    if (m_bodyText) window.draw(*m_bodyText);
}

} // namespace UI
