#pragma once
#include "GameState.hpp"
#include <SFML/Graphics.hpp>
#include <vector>

/// LevelSelectState — lets the player select which level to play (1, 2, or 3).
class LevelSelectState : public GameState {
public:
    LevelSelectState();

    void onEnter() override;
    void onExit() override;
    void handleEvent(const sf::Event& event) override;
    void update(float dt) override;
    void render(sf::RenderWindow& window) override;

private:
    void updateSelectionVisuals();

    sf::Text           m_title;
    sf::Text           m_levelTitles[3]; // "LEVEL 1", "LEVEL 2", "LEVEL 3"
    sf::Text           m_levelThemes[3]; // "GRASSLAND", "UNDERGROUND", "CASTLE"
    sf::Text           m_levelDescs[3];  // Description text
    sf::RectangleShape m_levelBoxes[3];
    std::vector<sf::Sprite> m_previewTiles[3]; // small themed tile-strip preview
    sf::RectangleShape m_background;
    int                m_selected = 0;
};
