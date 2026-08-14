#pragma once
#include "GameState.hpp"
#include <SFML/Graphics.hpp>

/// MenuState — the main title screen.
/// Recreates the classic NES Super Mario Bros. title screen look (title
/// card, hill, bushes, brick ground, standing Mario, decorative HUD row)
/// while keeping the existing New Game / Load Game / Exit options.
class MenuState : public GameState {
public:
    MenuState();

    void onEnter() override;
    void onExit() override;
    void handleEvent(const sf::Event& event) override;
    void update(float dt) override;
    void render(sf::RenderWindow& window) override;

private:
    void updateOptionVisuals();

    // Background
    sf::RectangleShape m_background;

    // Scenery sprites, all cropped from the same NES title-screen sheet
    // (except m_marioSprite and m_coinSprite, which are their own files).
    sf::Sprite m_titleCardSprite;
    sf::Sprite m_hillSprite;
    sf::Sprite m_bushSprite;
    sf::Sprite m_groundSprite;
    sf::Sprite m_marioSprite;
    sf::Sprite m_coinSprite;

    // Decorative HUD row ("MARIO"/"000000", "WORLD"/"1-1", "TIME"/"300")
    sf::Text m_hudCharacterText;
    sf::Text m_hudScoreText;
    sf::Text m_hudCoinCountText;
    sf::Text m_hudWorldLabelText;
    sf::Text m_hudWorldText;
    sf::Text m_hudTimeLabelText;
    sf::Text m_hudTimeText;

    // Copyright + top score flavor text
    sf::Text m_copyrightText;
    sf::Text m_topScoreText;

    // Menu options
    sf::Text  m_options[5]; // 1 Player, Co-op, PvP, Load Game, Exit
    int       m_selectedOption = 0;
    sf::CircleShape m_cursor;

    // Title card bounce animation
    float m_titleBounce = 0.0f;
    float m_titleBaseY  = 0.0f;
};
