#include "States/LevelEditorState.hpp"
#include "States/GameStateManager.hpp"
#include "States/PlayingState.hpp"
#include "Core/SoundManager.hpp"
#include <fstream>
#include <iostream>

LevelEditorState::LevelEditorState(GameStateManager& stateManager)
    : GameState(stateManager),
      m_selectedTool(EditorTileType::Ground),
      m_grid(MAP_HEIGHT, std::vector<int>(MAP_WIDTH, 0)) {}

void LevelEditorState::init() {
    if (!m_font.openFromFile("assets/fonts/arial.ttf")) {
        std::cerr << "[LevelEditorState] Warning: font file missing." << std::endl;
    }

    m_editorView.setSize({800.0f, 600.0f});
    m_editorView.setCenter({400.0f, 300.0f});

    m_infoText.emplace(m_font, "", 16);
    m_infoText->setFillColor(sf::Color::Yellow);
    m_infoText->setPosition({10.0f, 10.0f});

    for (int x = 0; x < MAP_WIDTH; ++x) {
        m_grid[MAP_HEIGHT - 1][x] = static_cast<int>(EditorTileType::Ground);
        m_grid[MAP_HEIGHT - 2][x] = static_cast<int>(EditorTileType::Ground);
    }
}

void LevelEditorState::handleInput(const sf::Event& event) {
    if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
        if (keyPressed->code == sf::Keyboard::Key::Num1) m_selectedTool = EditorTileType::Ground;
        else if (keyPressed->code == sf::Keyboard::Key::Num2) m_selectedTool = EditorTileType::Brick;
        else if (keyPressed->code == sf::Keyboard::Key::Num3) m_selectedTool = EditorTileType::QuestionBlock;
        else if (keyPressed->code == sf::Keyboard::Key::Num4) m_selectedTool = EditorTileType::Coin;
        else if (keyPressed->code == sf::Keyboard::Key::Num5) m_selectedTool = EditorTileType::Goomba;
        else if (keyPressed->code == sf::Keyboard::Key::Num6) m_selectedTool = EditorTileType::Koopa;
        else if (keyPressed->code == sf::Keyboard::Key::Num7) m_selectedTool = EditorTileType::Pipe;
        else if (keyPressed->code == sf::Keyboard::Key::Num8) m_selectedTool = EditorTileType::Flagpole;
        else if (keyPressed->code == sf::Keyboard::Key::Num0) m_selectedTool = EditorTileType::Empty;

        else if (keyPressed->code == sf::Keyboard::Key::Right || keyPressed->code == sf::Keyboard::Key::D) {
            m_editorView.move({100.0f, 0.0f});
        }
        else if (keyPressed->code == sf::Keyboard::Key::Left || keyPressed->code == sf::Keyboard::Key::A) {
            if (m_editorView.getCenter().x > 400.0f) {
                m_editorView.move({-100.0f, 0.0f});
            }
        }
        else if (keyPressed->code == sf::Keyboard::Key::S && sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl)) {
            saveCustomLevel();
            SoundManager::getInstance().playSound("confirm");
        }
        else if (keyPressed->code == sf::Keyboard::Key::P) {
            saveCustomLevel();
            m_stateManager.pushState(std::make_unique<PlayingState>(m_stateManager, 99, "Mario"));
        }
        else if (keyPressed->code == sf::Keyboard::Key::Escape) {
            m_stateManager.popState();
        }
    }
}

void LevelEditorState::update(float dt) {
    (void)dt;

    if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
        sf::Vector2i mousePos = sf::Mouse::getPosition();
        float worldX = mousePos.x + (m_editorView.getCenter().x - 400.0f);
        float worldY = mousePos.y;

        int gridX = static_cast<int>(worldX / TILE_SIZE);
        int gridY = static_cast<int>(worldY / TILE_SIZE);

        if (gridX >= 0 && gridX < MAP_WIDTH && gridY >= 0 && gridY < MAP_HEIGHT) {
            m_grid[gridY][gridX] = static_cast<int>(m_selectedTool);
        }
    }
}

void LevelEditorState::saveCustomLevel(const std::string& filename) {
    std::ofstream outFile(filename);
    if (!outFile.is_open()) {
        std::cerr << "[LevelEditorState] Error: could not save to " << filename << std::endl;
        return;
    }

    for (int y = 0; y < MAP_HEIGHT; ++y) {
        for (int x = 0; x < MAP_WIDTH; ++x) {
            outFile << m_grid[y][x] << (x == MAP_WIDTH - 1 ? "" : " ");
        }
        outFile << "\n";
    }

    outFile.close();
    std::cout << "[LevelEditorState] Custom level saved to " << filename << std::endl;
}

void LevelEditorState::loadCustomLevel(const std::string& filename) {
    std::ifstream inFile(filename);
    if (!inFile.is_open()) return;

    for (int y = 0; y < MAP_HEIGHT; ++y) {
        for (int x = 0; x < MAP_WIDTH; ++x) {
            inFile >> m_grid[y][x];
        }
    }
    inFile.close();
}

void LevelEditorState::drawGrid(sf::RenderWindow& window) {
    sf::RectangleShape tileShape({TILE_SIZE - 1.0f, TILE_SIZE - 1.0f});

    for (int y = 0; y < MAP_HEIGHT; ++y) {
        for (int x = 0; x < MAP_WIDTH; ++x) {
            int tileType = m_grid[y][x];
            if (tileType == 0) continue;

            tileShape.setPosition({x * TILE_SIZE, y * TILE_SIZE});

            switch (static_cast<EditorTileType>(tileType)) {
                case EditorTileType::Ground: tileShape.setFillColor(sf::Color(139, 69, 19)); break;
                case EditorTileType::Brick: tileShape.setFillColor(sf::Color(178, 34, 34)); break;
                case EditorTileType::QuestionBlock: tileShape.setFillColor(sf::Color(255, 215, 0)); break;
                case EditorTileType::Coin: tileShape.setFillColor(sf::Color(255, 255, 0)); break;
                case EditorTileType::Goomba: tileShape.setFillColor(sf::Color(128, 0, 0)); break;
                case EditorTileType::Koopa: tileShape.setFillColor(sf::Color(0, 128, 0)); break;
                case EditorTileType::Pipe: tileShape.setFillColor(sf::Color(34, 139, 34)); break;
                case EditorTileType::Flagpole: tileShape.setFillColor(sf::Color::White); break;
                default: break;
            }

            window.draw(tileShape);
        }
    }
}

void LevelEditorState::render(sf::RenderWindow& window) {
    sf::View defaultView = window.getView();
    window.setView(m_editorView);

    drawGrid(window);

    window.setView(defaultView);

    std::string toolName = "Ground";
    switch (m_selectedTool) {
        case EditorTileType::Ground: toolName = "1: Ground"; break;
        case EditorTileType::Brick: toolName = "2: Brick"; break;
        case EditorTileType::QuestionBlock: toolName = "3: ? Block"; break;
        case EditorTileType::Coin: toolName = "4: Coin"; break;
        case EditorTileType::Goomba: toolName = "5: Goomba"; break;
        case EditorTileType::Koopa: toolName = "6: Koopa"; break;
        case EditorTileType::Pipe: toolName = "7: Pipe"; break;
        case EditorTileType::Flagpole: toolName = "8: Flagpole"; break;
        case EditorTileType::Empty: toolName = "0: Erase"; break;
    }

    if (m_infoText) {
        m_infoText->setString("LEVEL EDITOR | Active Tool: " + toolName + 
                             "\n[1-8] Select Tool | Left Click Place | Arrow Left/Right Pan | [Ctrl+S] Save | [P] Play Level | [ESC] Exit");
        window.draw(*m_infoText);
    }
}
