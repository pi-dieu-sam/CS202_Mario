#ifndef GAME_OVER_STATE_HPP
#define GAME_OVER_STATE_HPP

#include "GameState.hpp"
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <optional>

class GameOverState : public GameState {
public:
    GameOverState(GameStateManager& stateManager, bool isVictory, int finalScore);
    ~GameOverState() override = default;

    void init() override;
    void handleInput(const sf::Event& event) override;
    void update(float dt) override;
    void render(sf::RenderWindow& window) override;

private:
    bool m_isVictory;
    int m_finalScore;

    sf::Font m_font;
    std::optional<sf::Text> m_titleText;
    std::optional<sf::Text> m_scoreText;
    std::vector<sf::Text> m_optionsText;
    std::vector<std::string> m_options;
    int m_selectedIndex;

    void updateSelection();
    void processSelection();
};

#endif // GAME_OVER_STATE_HPP
