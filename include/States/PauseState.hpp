#pragma once
#include "GameState.hpp"
#include <SFML/Graphics.hpp>

/// PauseState — overlay drawn on top of PlayingState.
/// Shows "Resume", "Quit to Menu", and an interactive Audio Control Panel (Mute, Vol +/-, Track Selector).
class PauseState : public GameState {
public:
    PauseState();

    void onEnter() override;
    void onExit() override;
    void handleEvent(const sf::Event& event) override;
    void update(float dt) override;
    void render(sf::RenderWindow& window) override;

private:
    void updateAudioUI();

    sf::RectangleShape m_overlay;  // semi-transparent dark overlay
    sf::Text           m_title;
    sf::Text           m_options[2]; // Resume, Quit
    int                m_selected = 0;

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
