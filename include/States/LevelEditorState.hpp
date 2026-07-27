#ifndef LEVEL_EDITOR_STATE_HPP
#define LEVEL_EDITOR_STATE_HPP

#include "GameState.hpp"
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <optional>

enum class EditorTileType {
    Empty = 0,
    Ground = 1,
    Brick = 2,
    QuestionBlock = 3,
    Coin = 4,
    Goomba = 5,
    Koopa = 6,
    Pipe = 7,
    Flagpole = 8
};

class LevelEditorState : public GameState {
public:
    explicit LevelEditorState(GameStateManager& stateManager);
    ~LevelEditorState() override = default;

    void init() override;
    void handleInput(const sf::Event& event) override;
    void update(float dt) override;
    void render(sf::RenderWindow& window) override;

    void saveCustomLevel(const std::string& filename = "assets/levels/custom_level.txt");
    void loadCustomLevel(const std::string& filename = "assets/levels/custom_level.txt");

private:
    static constexpr int MAP_WIDTH = 100;
    static constexpr int MAP_HEIGHT = 15;
    static constexpr float TILE_SIZE = 32.0f;

    std::vector<std::vector<int>> m_grid;
    EditorTileType m_selectedTool;
    sf::View m_editorView;
    sf::Font m_font;
    std::optional<sf::Text> m_infoText;

    void drawGrid(sf::RenderWindow& window);
};

#endif // LEVEL_EDITOR_STATE_HPP
