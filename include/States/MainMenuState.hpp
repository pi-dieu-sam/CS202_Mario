#ifndef MAIN_MENU_STATE_HPP
#define MAIN_MENU_STATE_HPP

#include "GameState.hpp"
#include "UI/Button.hpp"
#include <vector>
#include <string>
#include <optional>
#include <SFML/Graphics.hpp>

class MainMenuState : public GameState {
public:
    explicit MainMenuState(GameStateManager& stateManager);
    ~MainMenuState() override = default;

    void init() override;
    void handleInput(const sf::Event& event) override;
    void update(float dt) override;
    void render(sf::RenderWindow& window) override;

private:
    sf::Font m_font;
    std::optional<sf::Text> m_titleText;
    std::vector<UI::Button> m_buttons;
    std::vector<std::string> m_options;
    int m_selectedIndex;

    void updateSelection();
    void processSelection();
};

#endif // MAIN_MENU_STATE_HPP
