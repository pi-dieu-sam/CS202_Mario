#pragma once
#include "GameState.hpp"
#include "Core/GameSnapshot.hpp"
#include <SFML/Graphics.hpp>
#include <optional>

/// PauseState — overlay drawn on top of PlayingState.
/// Shows Resume, a single-player Save Game entry, Quit to Menu, and an
/// interactive Audio Control Panel (Mute, Vol +/-, Track Selector).
class PauseState : public GameState {
public:
    explicit PauseState(std::optional<SaveData::GameSnapshot> snapshot = std::nullopt);

    void onEnter() override;
    void onExit() override;
    void handleEvent(const sf::Event& event) override;
    void update(float dt) override;
    void render(sf::RenderWindow& window) override;

    /// Drawn as an overlay over the frozen PlayingState beneath it.
    bool isTransparent() const override { return true; }

private:
    void updateAudioUI();

    sf::RectangleShape m_overlay;  // semi-transparent dark overlay
    sf::Text           m_title;
    sf::Text           m_options[3]; // Resume, Save Game (single-player), Quit
    int                m_selected = 0;
    int                m_optionCount = 2;
    std::optional<SaveData::GameSnapshot> m_snapshot;

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
