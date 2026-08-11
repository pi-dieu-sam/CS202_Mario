#include "Core/PlayerProgress.hpp"
#include "Physics/PhysicsConstants.hpp"
#include "Core/SoundManager.hpp"

// ── Score ──
int PlayerProgress::getScore() const { return m_score; }
void PlayerProgress::addScore(int points) { m_score += points; }
void PlayerProgress::setScore(int score) { m_score = score; }

// ── Lives ──
int PlayerProgress::getLives() const { return m_lives; }
void PlayerProgress::setLives(int lives) { m_lives = lives; }
void PlayerProgress::loseLife() {
    m_lives--;
    if (m_lives < 0) m_lives = 0;
}

// ── Level ──
int PlayerProgress::getCurrentLevel() const { return m_currentLevel; }
void PlayerProgress::setCurrentLevel(int level) { m_currentLevel = level; }

// ── Coins ──
int PlayerProgress::getCoins() const { return m_coins; }
void PlayerProgress::addCoin() {
    m_coins++;
    addScore(COIN_SCORE);
    // Every 100 coins = extra life
    if (m_coins % 100 == 0) {
        m_lives++;
        SoundManager::getInstance().playSound(SoundID::OneUp);
    }
}
void PlayerProgress::setCoins(int coins) { m_coins = coins; }

// ── Character ──
const std::string& PlayerProgress::getSelectedCharacter() const { return m_selectedChar; }
void PlayerProgress::setSelectedCharacter(const std::string& name) { m_selectedChar = name; }

// ── Reset ──
void PlayerProgress::resetGameData() {
    m_score        = 0;
    m_lives        = STARTING_LIVES;
    m_coins        = 0;
    m_currentLevel = 1;
}
