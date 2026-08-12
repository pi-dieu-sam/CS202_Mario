#include "UI/ScorePopup.hpp"
#include "Core/AssetManager.hpp"
#include <algorithm>
#include <string>
#include <utility>

namespace {
constexpr float POPUP_LIFETIME = 0.6f;
constexpr float POPUP_RISE_DISTANCE = 32.0f;
constexpr unsigned int POPUP_CHARACTER_SIZE = 16;

sf::Uint8 popupAlpha(float progress) {
    const float clampedProgress = std::clamp(progress, 0.0f, 1.0f);
    return static_cast<sf::Uint8>(255.0f * (1.0f - clampedProgress));
}
}

void ScorePopupManager::init() {
    m_font = &AssetManager::getInstance().getFont("assets/fonts/mario_font.ttf");
}

void ScorePopupManager::add(int score, const sf::Vector2f& worldPosition) {
    if (!m_font) {
        init();
    }

    Popup popup;
    popup.text.setFont(*m_font);
    popup.text.setCharacterSize(POPUP_CHARACTER_SIZE);
    popup.text.setString("+" + std::to_string(score));
    popup.text.setFillColor(sf::Color::White);
    popup.text.setOutlineColor(sf::Color::Black);
    popup.text.setOutlineThickness(1.0f);

    const sf::FloatRect textBounds = popup.text.getLocalBounds();
    popup.text.setOrigin(textBounds.left + textBounds.width * 0.5f,
                         textBounds.top + textBounds.height * 0.5f);
    popup.startPosition = worldPosition;
    popup.position = popup.startPosition;
    popup.text.setPosition(popup.position);

    m_popups.push_back(std::move(popup));
}

void ScorePopupManager::update(float dt) {
    for (auto& popup : m_popups) {
        popup.age += dt;
        const float progress = popup.age / POPUP_LIFETIME;
        const float riseProgress = std::clamp(progress, 0.0f, 1.0f);
        popup.position = popup.startPosition;
        popup.position.y -= POPUP_RISE_DISTANCE * riseProgress;
        popup.text.setPosition(popup.position);

        const sf::Uint8 alpha = popupAlpha(progress);
        popup.text.setFillColor(sf::Color(255, 255, 255, alpha));
        popup.text.setOutlineColor(sf::Color(0, 0, 0, alpha));
    }

    m_popups.erase(
        std::remove_if(m_popups.begin(), m_popups.end(),
                       [](const Popup& popup) {
                           return popup.age >= POPUP_LIFETIME;
                       }),
        m_popups.end());
}

void ScorePopupManager::render(sf::RenderWindow& window) const {
    for (const auto& popup : m_popups) {
        window.draw(popup.text);
    }
}

void ScorePopupManager::clear() {
    m_popups.clear();
}
