#include "States/MenuState.hpp"
#include "States/CharacterSelectState.hpp"
#include "States/PlayingState.hpp"
#include "Core/Game.hpp"
#include "Core/AssetManager.hpp"
#include "Core/SaveManager.hpp"
#include "States/StateManager.hpp"
#include "Physics/PhysicsConstants.hpp"
#include <cmath>

MenuState::MenuState() {}

void MenuState::onEnter() {
    sf::Font& font = AssetManager::getInstance().getFont("assets/fonts/mario_font.ttf");

    // Title
    m_title.setFont(font);
    m_title.setString("SUPER MARIO");
    m_title.setCharacterSize(56);
    m_title.setFillColor(sf::Color(228, 0, 0));
    m_title.setOutlineColor(sf::Color::Black);
    m_title.setOutlineThickness(3.0f);
    auto titleBounds = m_title.getLocalBounds();
    m_title.setOrigin(titleBounds.width / 2.0f, titleBounds.height / 2.0f);
    m_title.setPosition(WINDOW_WIDTH / 2.0f, 120.0f);

    // Menu options
    std::string labels[] = {"NEW GAME", "LOAD GAME", "EXIT"};
    for (int i = 0; i < 3; i++) {
        m_options[i].setFont(font);
        m_options[i].setString(labels[i]);
        m_options[i].setCharacterSize(28);
        auto bounds = m_options[i].getLocalBounds();
        m_options[i].setOrigin(bounds.width / 2.0f, bounds.height / 2.0f);
        m_options[i].setPosition(WINDOW_WIDTH / 2.0f, 280.0f + i * 70.0f);
    }

    // Background
    m_background.setSize(sf::Vector2f(WINDOW_WIDTH, WINDOW_HEIGHT));
    m_background.setFillColor(sf::Color(92, 148, 252));

    m_selectedOption = 0;
    updateOptionVisuals();
}

void MenuState::onExit() {}

void MenuState::handleEvent(const sf::Event& event) {
    sf::RenderWindow& window = Game::getInstance().getWindow();

    if (event.type == sf::Event::MouseMoved) {
        sf::Vector2f mousePos = window.mapPixelToCoords({event.mouseMove.x, event.mouseMove.y});
        for (int i = 0; i < 3; i++) {
            if (m_options[i].getGlobalBounds().contains(mousePos)) {
                if (m_selectedOption != i) {
                    m_selectedOption = i;
                    updateOptionVisuals();
                }
                break;
            }
        }
        return;
    }

    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2f mousePos = window.mapPixelToCoords({event.mouseButton.x, event.mouseButton.y});
        for (int i = 0; i < 3; i++) {
            if (m_options[i].getGlobalBounds().contains(mousePos)) {
                m_selectedOption = i;
                updateOptionVisuals();

                switch (m_selectedOption) {
                    case 0: // New Game
                        Game::getInstance().resetGameData();
                        Game::getInstance().getStateManager().changeState(
                            std::make_unique<CharacterSelectState>());
                        break;
                    case 1: // Load Game
                        if (SaveManager::loadGame()) {
                            Game::getInstance().getStateManager().changeState(
                                std::make_unique<PlayingState>());
                        }
                        break;
                    case 2: // Exit
                        Game::getInstance().getStateManager().clearStates();
                        break;
                }
                return;
            }
        }
    }

    if (event.type == sf::Event::KeyPressed) {
        switch (event.key.code) {
            case sf::Keyboard::Up:
            case sf::Keyboard::W:
                m_selectedOption = (m_selectedOption - 1 + 3) % 3;
                updateOptionVisuals();
                break;

            case sf::Keyboard::Down:
            case sf::Keyboard::S:
                m_selectedOption = (m_selectedOption + 1) % 3;
                updateOptionVisuals();
                break;

            case sf::Keyboard::Return:
            case sf::Keyboard::Space:
                switch (m_selectedOption) {
                    case 0: // New Game
                        Game::getInstance().resetGameData();
                        Game::getInstance().getStateManager().changeState(
                            std::make_unique<CharacterSelectState>());
                        break;
                    case 1: // Load Game
                        if (SaveManager::loadGame()) {
                            Game::getInstance().getStateManager().changeState(
                                std::make_unique<PlayingState>());
                        }
                        break;
                    case 2: // Exit
                        Game::getInstance().getStateManager().clearStates();
                        break;
                }
                break;

            default:
                break;
        }
    }
}

void MenuState::update(float dt) {
    m_titleBounce += dt * 2.0f;
    m_title.setPosition(WINDOW_WIDTH / 2.0f, 120.0f + std::sin(m_titleBounce) * 8.0f);
}

void MenuState::render(sf::RenderWindow& window) {
    // Reset view to default for menu rendering
    window.setView(window.getDefaultView());

    window.draw(m_background);
    window.draw(m_title);
    for (int i = 0; i < 3; i++) {
        window.draw(m_options[i]);
    }
}

void MenuState::updateOptionVisuals() {
    for (int i = 0; i < 3; i++) {
        if (i == m_selectedOption) {
            m_options[i].setFillColor(sf::Color::Yellow);
            m_options[i].setCharacterSize(32);
        } else {
            m_options[i].setFillColor(sf::Color::White);
            m_options[i].setCharacterSize(28);
        }
        // Re-center after size change
        auto bounds = m_options[i].getLocalBounds();
        m_options[i].setOrigin(bounds.width / 2.0f, bounds.height / 2.0f);
    }
}
