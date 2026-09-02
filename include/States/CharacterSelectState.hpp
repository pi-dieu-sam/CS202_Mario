#pragma once
#include "GameState.hpp"
#include <SFML/Graphics.hpp>

/// CharacterSelectState — lets the player choose between Mario and Luigi,
/// and provides an interactive Audio Control Panel (Mute, Volume, Track Selector).
class CharacterSelectState : public GameState {
public:
    CharacterSelectState();

    void onEnter() override;
    void onExit() override;
    void handleEvent(const sf::Event& event) override;
    void update(float dt) override;
    void render(sf::RenderWindow& window) override;

private:
    void updateAudioUI();

    sf::Text m_title;
    sf::Text m_charNames[2]; // "Mario", "Luigi"
    sf::Text m_charStats[2]; // Stat descriptions
    sf::RectangleShape m_charBoxes[2];
    sf::Sprite m_charSprites[2];
    sf::RectangleShape m_background;
    int  m_selected = 0;

    // ── Audio Control Panel UI ──────────────────────────────────────────────
    sf::RectangleShape m_audioPanel;
    sf::Text           m_panelTitle;

    sf::RectangleShape m_muteBtn;
    sf::Text           m_muteText;

    sf::RectangleShape m_volMinusBtn;
    sf::Text           m_volMinusText;
    sf::RectangleShape m_volPlusBtn;
    sf::Text           m_volPlusText;
    sf::Text           m_volText;

    sf::RectangleShape m_trackBtn;
    sf::Text           m_trackText;
    sf::Text           m_helpText;
};
