#pragma once

#include "GameState.hpp"

#include <SFML/Graphics.hpp>

/// MenuState — the animated, enhanced-NES title screen. The background is a
/// presentation-only attract loop; it never changes gameplay state until a
/// menu option is explicitly confirmed.
class MenuState : public GameState {
public:
    MenuState() = default;

    void onEnter() override;
    void onExit() override;
    void handleEvent(const sf::Event& event) override;
    void update(float dt) override;
    void render(sf::RenderWindow& window) override;

private:
    static constexpr int OPTION_COUNT = 6;

    void activateSelectedOption();
    void updateOptionVisuals();
    void updateAttractScene(float dt);

    // Classic title-scene scenery.
    sf::RectangleShape m_background;
    sf::Sprite m_titleCardSprite;
    sf::Sprite m_hillSprite;
    sf::Sprite m_bushSprite;
    sf::Sprite m_groundSprite;
    sf::Sprite m_pipePieces[4];

    // Animated attract-mode actors.
    sf::Sprite m_marioSprite;
    sf::Sprite m_luigiSprite;
    sf::Sprite m_goombaSprite;
    sf::Sprite m_fireballSprite;

    // Decorative HUD row.
    sf::Sprite m_coinSprite;
    sf::Text m_hudCharacterText;
    sf::Text m_hudScoreText;
    sf::Text m_hudCoinCountText;
    sf::Text m_hudWorldLabelText;
    sf::Text m_hudWorldText;
    sf::Text m_hudTimeLabelText;
    sf::Text m_hudTimeText;
    sf::Text m_copyrightText;

    // Foreground menu surface.
    sf::RectangleShape m_menuPanel;
    sf::RectangleShape m_optionPanels[OPTION_COUNT];
    sf::Text m_menuHeader;
    sf::Text m_options[OPTION_COUNT];
    sf::Text m_selectionHint;
    sf::Text m_controlHint;
    sf::CircleShape m_cursor;
    int m_selectedOption = 0;

    float m_titleBounce = 0.0f;
    float m_titleBaseY = 0.0f;
    float m_attractTime = 0.0f;
};
