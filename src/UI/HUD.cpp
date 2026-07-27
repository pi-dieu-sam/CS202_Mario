#include "UI/HUD.hpp"
#include <iostream>

namespace UI {

HUD::HUD() = default;

bool HUD::init(const std::string& fontPath) {
    if (!m_font.openFromFile(fontPath)) {
        std::cerr << "[HUD] Warning: Failed to load font from " << fontPath << std::endl;
    }
    m_hudText.emplace(m_font, "", 20);
    m_hudText->setFillColor(sf::Color::White);
    m_hudText->setPosition({20.0f, 15.0f});
    return true;
}

void HUD::update(const std::string& characterName, int levelIndex, int score, int coins, int lives, float levelTime) {
    if (!m_hudText) return;

    std::string hudString = "MARIO  " + characterName + 
                            "   WORLD 1-" + std::to_string(levelIndex) + 
                            "   SCORE " + std::to_string(score) + 
                            "   COINS x" + std::to_string(coins) + 
                            "   LIVES x" + std::to_string(lives) + 
                            "   TIME " + std::to_string(static_cast<int>(levelTime));
    m_hudText->setString(hudString);
}

void HUD::render(sf::RenderWindow& window) {
    sf::View defaultView = window.getDefaultView();
    window.setView(defaultView);
    if (m_hudText) {
        window.draw(*m_hudText);
    }
}

} // namespace UI
