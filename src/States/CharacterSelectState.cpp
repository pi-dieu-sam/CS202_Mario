#include "States/CharacterSelectState.hpp"
#include "States/LevelSelectState.hpp"
#include "States/PlayingState.hpp"
#include "Core/Game.hpp"
#include "Core/AssetManager.hpp"
#include "Entities/Player.hpp"
#include "Graphics/SpriteRegistry.hpp"
#include "States/StateManager.hpp"
#include "Physics/PhysicsConstants.hpp"

CharacterSelectState::CharacterSelectState() {}

void CharacterSelectState::onEnter() {
    sf::Font& font = AssetManager::getInstance().getFont("assets/fonts/mario_font.ttf");

    m_background.setSize(sf::Vector2f(WINDOW_WIDTH, WINDOW_HEIGHT));
    m_background.setFillColor(sf::Color(40, 40, 80));

    m_title.setFont(font);
    m_title.setString("SELECT CHARACTER");
    m_title.setCharacterSize(36);
    m_title.setFillColor(sf::Color::White);
    auto tb = m_title.getLocalBounds();
    m_title.setOrigin(tb.width / 2.0f, tb.height / 2.0f);
    m_title.setPosition(WINDOW_WIDTH / 2.0f, 60.0f);

    // Character boxes
    float boxW = 250.0f, boxH = 300.0f;
    float gap = 60.0f;
    float startX = (WINDOW_WIDTH - (2 * boxW + gap)) / 2.0f;

    std::string names[] = {"MARIO", "LUIGI"};
    std::string stats[] = {
        "Speed: ***\nJump:  ***\nGrip:  ****",
        "Speed: **\nJump:  ****\nGrip:  **"
    };
    sf::Color boxColors[] = {
        sf::Color(200, 50, 50, 180),
        sf::Color(50, 180, 50, 180)
    };

    for (int i = 0; i < 2; i++) {
        m_charBoxes[i].setSize(sf::Vector2f(boxW, boxH));
        m_charBoxes[i].setPosition(startX + i * (boxW + gap), 130.0f);
        m_charBoxes[i].setFillColor(boxColors[i]);
        m_charBoxes[i].setOutlineThickness(3.0f);
        m_charBoxes[i].setOutlineColor(sf::Color::Transparent);

        m_charNames[i].setFont(font);
        m_charNames[i].setString(names[i]);
        m_charNames[i].setCharacterSize(28);
        m_charNames[i].setFillColor(sf::Color::White);
        auto nb = m_charNames[i].getLocalBounds();
        m_charNames[i].setOrigin(nb.width / 2.0f, 0.0f);
        m_charNames[i].setPosition(startX + i * (boxW + gap) + boxW / 2.0f, 150.0f);

        m_charStats[i].setFont(font);
        m_charStats[i].setString(stats[i]);
        m_charStats[i].setCharacterSize(16);
        m_charStats[i].setFillColor(sf::Color(220, 220, 220));
        m_charStats[i].setPosition(startX + i * (boxW + gap) + 20.0f, 300.0f);

        // Character preview sprite
        float centerX = startX + i * (boxW + gap) + boxW / 2.0f;
        CharacterId charId = (i == 0) ? CharacterId::Mario : CharacterId::Luigi;
        std::string path = SpriteRegistry::playerPath(
            charId, PowerUpState::Big, SpriteRegistry::PlayerAnim::Idle, 0);
        SpriteRegistry::applyFrame(m_charSprites[i], path,
                                    sf::FloatRect(centerX, 170.0f, 0.0f, 120.0f));
    }

    m_selected = 0;
    m_charBoxes[0].setOutlineColor(sf::Color::Yellow);
}

void CharacterSelectState::onExit() {}

void CharacterSelectState::handleEvent(const sf::Event& event) {
    sf::RenderWindow& window = Game::getInstance().getWindow();

    if (event.type == sf::Event::MouseMoved) {
        sf::Vector2f mousePos = window.mapPixelToCoords({event.mouseMove.x, event.mouseMove.y});
        for (int i = 0; i < 2; i++) {
            if (m_charBoxes[i].getGlobalBounds().contains(mousePos)) {
                if (m_selected != i) {
                    m_selected = i;
                    m_charBoxes[0].setOutlineColor(m_selected == 0 ? sf::Color::Yellow : sf::Color::Transparent);
                    m_charBoxes[1].setOutlineColor(m_selected == 1 ? sf::Color::Yellow : sf::Color::Transparent);
                }
                break;
            }
        }
        return;
    }

    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2f mousePos = window.mapPixelToCoords({event.mouseButton.x, event.mouseButton.y});
        for (int i = 0; i < 2; i++) {
            if (m_charBoxes[i].getGlobalBounds().contains(mousePos)) {
                m_selected = i;
                std::string charName = (m_selected == 0) ? "Mario" : "Luigi";
                Game::getInstance().getProgress().setSelectedCharacter(charName);
                Game::getInstance().getStateManager().changeState(
                    std::make_unique<LevelSelectState>());
                return;
            }
        }
    }

    if (event.type == sf::Event::KeyPressed) {
        switch (event.key.code) {
            case sf::Keyboard::Left:
            case sf::Keyboard::A:
                m_selected = 0;
                m_charBoxes[0].setOutlineColor(sf::Color::Yellow);
                m_charBoxes[1].setOutlineColor(sf::Color::Transparent);
                break;

            case sf::Keyboard::Right:
            case sf::Keyboard::D:
                m_selected = 1;
                m_charBoxes[0].setOutlineColor(sf::Color::Transparent);
                m_charBoxes[1].setOutlineColor(sf::Color::Yellow);
                break;

            case sf::Keyboard::Return:
            case sf::Keyboard::Space: {
                std::string charName = (m_selected == 0) ? "Mario" : "Luigi";
                Game::getInstance().getProgress().setSelectedCharacter(charName);
                Game::getInstance().getStateManager().changeState(
                    std::make_unique<LevelSelectState>());
                break;
            }

            case sf::Keyboard::Escape:
                Game::getInstance().getStateManager().popState();
                break;

            default:
                break;
        }
    }
}

void CharacterSelectState::update(float dt) {
    // Could animate character previews here
}

void CharacterSelectState::render(sf::RenderWindow& window) {
    window.setView(window.getDefaultView());
    window.draw(m_background);
    window.draw(m_title);
    for (int i = 0; i < 2; i++) {
        window.draw(m_charBoxes[i]);
        window.draw(m_charSprites[i]);
        window.draw(m_charNames[i]);
        window.draw(m_charStats[i]);
    }
}
