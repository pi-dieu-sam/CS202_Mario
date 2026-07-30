#include "UI/HUD.hpp"
#include "Core/AssetManager.hpp"
#include "Physics/PhysicsConstants.hpp"
#include <sstream>
#include <iomanip>

HUD::HUD() {}

void HUD::init() {
    sf::Font& font = AssetManager::getInstance().getFont("assets/fonts/mario_font.ttf");

    auto setupText = [&](sf::Text& text, float x, float y, unsigned int size = 18) {
        text.setFont(font);
        text.setCharacterSize(size);
        text.setFillColor(sf::Color::White);
        text.setOutlineColor(sf::Color::Black);
        text.setOutlineThickness(1.0f);
        text.setPosition(x, y);
    };

    setupText(m_characterText, 20.0f,  8.0f);
    setupText(m_scoreText,     20.0f,  30.0f);
    setupText(m_coinText,      250.0f, 8.0f);
    setupText(m_levelText,     400.0f, 8.0f);
    setupText(m_livesText,     550.0f, 8.0f);
    setupText(m_timeText,      680.0f, 8.0f);

    m_characterText.setString("MARIO");
    m_scoreText.setString("000000");
    m_coinText.setString("x00");
    m_levelText.setString("1-1");
    m_livesText.setString("x3");
    m_timeText.setString("300");
}

void HUD::update(float dt) {
    // Nothing animated for now
}

void HUD::render(sf::RenderWindow& window) {
    // Draw a semi-transparent bar behind HUD
    sf::RectangleShape bar;
    bar.setSize(sf::Vector2f(WINDOW_WIDTH, 55.0f));
    bar.setFillColor(sf::Color(0, 0, 0, 120));
    window.draw(bar);

    window.draw(m_characterText);
    window.draw(m_scoreText);
    window.draw(m_coinText);
    window.draw(m_levelText);
    window.draw(m_livesText);
    window.draw(m_timeText);
}

void HUD::setScore(int score) {
    std::ostringstream ss;
    ss << std::setfill('0') << std::setw(6) << score;
    m_scoreText.setString(ss.str());
}

void HUD::setCoins(int coins) {
    m_coinText.setString("x" + std::to_string(coins));
}

void HUD::setLives(int lives) {
    m_livesText.setString("x" + std::to_string(lives));
}

void HUD::setLevel(int level) {
    m_levelText.setString(std::to_string(level) + "-1");
}

void HUD::setTime(float time) {
    if (time < 0) time = 0;
    m_timeText.setString(std::to_string(static_cast<int>(time)));
}

void HUD::setCharacterName(const std::string& name) {
    std::string upper = name;
    for (auto& c : upper) c = static_cast<char>(std::toupper(c));
    m_characterText.setString(upper);
}