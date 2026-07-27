#ifndef PAUSE_STATE_HPP
#define PAUSE_STATE_HPP

#include "GameState.hpp"
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <optional>

class PauseState : public GameState {
public:
    explicit PauseState(GameStateManager& stateManager);
    ~PauseState() override = default;

    void init() override;
    void handleInput(const sf::Event& event) override;
    void update(float dt) override;
    void render(sf::RenderWindow& window) override;

private:
    sf::Font m_font;
    std::optional<sf::Text> m_titleText;
    std::vector<sf::Text> m_optionsText;
    std::vector<std::string> m_options;
    sf::RectangleShape m_overlay;
    int m_selectedIndex;

    void updateSelection();
    void processSelection();
};

#endif // PAUSE_STATE_HPP
