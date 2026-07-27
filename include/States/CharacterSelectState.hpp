#ifndef CHARACTER_SELECT_STATE_HPP
#define CHARACTER_SELECT_STATE_HPP

#include "GameState.hpp"
#include "UI/CardWidget.hpp"
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <optional>

class CharacterSelectState : public GameState {
public:
    explicit CharacterSelectState(GameStateManager& stateManager);
    ~CharacterSelectState() override = default;

    void init() override;
    void handleInput(const sf::Event& event) override;
    void update(float dt) override;
    void render(sf::RenderWindow& window) override;

    static std::string getSelectedCharacter() { return s_selectedCharacter; }
    static void setSelectedCharacter(const std::string& name) { s_selectedCharacter = name; }

private:
    sf::Font m_font;
    std::optional<sf::Text> m_titleText;
    std::optional<sf::Text> m_confirmText;

    UI::CardWidget m_marioCard;
    UI::CardWidget m_luigiCard;

    int m_selectedIndex; // 0: Mario, 1: Luigi
    static std::string s_selectedCharacter;

    void updateCardHighlight();
};

#endif // CHARACTER_SELECT_STATE_HPP
