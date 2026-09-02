#include "UI/HUD.hpp"

#include "Core/AssetManager.hpp"
#include "Physics/PhysicsConstants.hpp"

#include <cctype>
#include <iomanip>
#include <sstream>

namespace {
void setupText(sf::Text& text, const sf::Font& font, unsigned int size = 14) {
    text.setFont(font);
    text.setCharacterSize(size);
    text.setFillColor(sf::Color::White);
    text.setOutlineColor(sf::Color::Black);
    text.setOutlineThickness(1.0f);
}
} // namespace

void HUD::init(GameMode mode) {
    sf::Font& font = AssetManager::getInstance().getFont("assets/fonts/mario_font.ttf");
    m_mode = mode;

    m_background.setSize({static_cast<float>(WINDOW_WIDTH), 64.0f});
    m_background.setFillColor(sf::Color(9, 17, 47, 186));

    setupText(m_characterText, font, 15);
    setupText(m_player2Text, font, 15);
    setupText(m_scoreLabel, font, 12);
    setupText(m_scoreText, font, 16);
    setupText(m_coinLabel, font, 12);
    setupText(m_coinText, font, 16);
    setupText(m_levelLabel, font, 12);
    setupText(m_levelText, font, 16);
    setupText(m_livesLabel, font, 12);
    setupText(m_livesText, font, 16);
    setupText(m_timeLabel, font, 12);
    setupText(m_timeText, font, 16);

    m_scoreLabel.setString("SCORE");
    m_coinLabel.setString("COINS");
    m_levelLabel.setString("WORLD");
    m_livesLabel.setString("LIVES");
    m_timeLabel.setString("TIME");
    setScore(0);
    setCoins(0);
    setLevel(1);
    setLives(3);
    setTime(300.0f);
    refreshLayout();
}

void HUD::update(float) {}

void HUD::render(sf::RenderWindow& window) {
    window.draw(m_background);
    window.draw(m_characterText);
    if (m_mode != GameMode::SinglePlayer) window.draw(m_player2Text);
    window.draw(m_scoreLabel);
    window.draw(m_scoreText);
    window.draw(m_coinLabel);
    window.draw(m_coinText);
    window.draw(m_levelLabel);
    window.draw(m_levelText);
    window.draw(m_livesLabel);
    window.draw(m_livesText);
    window.draw(m_timeLabel);
    window.draw(m_timeText);
}

void HUD::setScore(int score) {
    std::ostringstream value;
    value << std::setfill('0') << std::setw(6) << score;
    m_scoreText.setString(value.str());
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
    if (time < 0.0f) time = 0.0f;
    m_timeText.setString(std::to_string(static_cast<int>(time)));
}

void HUD::setCharacterName(const std::string& name) {
    m_characterName = uppercase(name);
    refreshLayout();
}

void HUD::setPlayer2Name(const std::string& name) {
    m_player2Name = uppercase(name);
    refreshLayout();
}

void HUD::setGameMode(GameMode mode) {
    m_mode = mode;
    refreshLayout();
}

void HUD::refreshLayout() {
    if (m_mode == GameMode::SinglePlayer) {
        m_characterText.setString(m_characterName);
        m_characterText.setPosition(20.0f, 8.0f);
        m_scoreLabel.setPosition(20.0f, 29.0f);
        m_scoreText.setPosition(20.0f, 44.0f);
        m_coinLabel.setPosition(190.0f, 8.0f);
        m_coinText.setPosition(190.0f, 28.0f);
        m_levelLabel.setPosition(350.0f, 8.0f);
        m_levelText.setPosition(350.0f, 28.0f);
        m_livesLabel.setPosition(500.0f, 8.0f);
        m_livesText.setPosition(500.0f, 28.0f);
        m_timeLabel.setPosition(650.0f, 8.0f);
        m_timeText.setPosition(650.0f, 28.0f);
    } else {
        m_characterText.setString("P1 " + m_characterName);
        m_player2Text.setString("P2 " + m_player2Name);
        m_characterText.setPosition(16.0f, 8.0f);
        m_player2Text.setPosition(16.0f, 35.0f);
        m_scoreLabel.setPosition(176.0f, 8.0f);
        m_scoreText.setPosition(176.0f, 29.0f);
        m_coinLabel.setPosition(314.0f, 8.0f);
        m_coinText.setPosition(314.0f, 29.0f);
        m_levelLabel.setPosition(437.0f, 8.0f);
        m_levelText.setPosition(437.0f, 29.0f);
        m_livesLabel.setPosition(548.0f, 8.0f);
        m_livesText.setPosition(548.0f, 29.0f);
        m_timeLabel.setPosition(667.0f, 8.0f);
        m_timeText.setPosition(667.0f, 29.0f);
    }
}

std::string HUD::uppercase(std::string value) {
    for (char& character : value) {
        character = static_cast<char>(std::toupper(static_cast<unsigned char>(character)));
    }
    return value;
}
