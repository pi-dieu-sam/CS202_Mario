#pragma once

#include "Core/PlayerProgress.hpp"
#include "GameState.hpp"

#include <SFML/Graphics.hpp>
#include <array>

/// ModeBriefingState gives multiplayer players a short, animated ready screen
/// before the level picker (co-op) or arena (PvP). P1 always owns the chosen
/// hero; P2 automatically uses the other hero, matching Level::loadFromFile.
class ModeBriefingState : public GameState {
public:
    explicit ModeBriefingState(GameMode mode);

    void onEnter() override;
    void onExit() override;
    void handleEvent(const sf::Event& event) override;
    void update(float dt) override;
    void render(sf::RenderWindow& window) override;

private:
    void toggleHeroes();
    void activateSelectedAction();
    void updateVisuals();
    void updateHeroSprites(float dt);

    GameMode m_mode;
    int m_selectedAction = 1; // Swap, Continue, Back
    float m_animationTime = 0.0f;

    sf::RectangleShape m_background;
    sf::RectangleShape m_characterCards[2];
    sf::RectangleShape m_controlPanel;
    std::array<sf::RectangleShape, 3> m_actionBoxes;
    sf::Sprite m_heroSprites[2];
    sf::Text m_title;
    sf::Text m_subtitle;
    sf::Text m_roleTexts[2];
    sf::Text m_nameTexts[2];
    sf::Text m_controlTexts[2];
    std::array<sf::Text, 3> m_actionTexts;
    sf::Text m_help;
};
