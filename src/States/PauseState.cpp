#include "States/PauseState.hpp"
#include "States/MenuState.hpp"
#include "Core/Game.hpp"
#include "Core/AssetManager.hpp"
#include "Core/SaveManager.hpp"
#include "States/StateManager.hpp"
#include "Physics/PhysicsConstants.hpp"

PauseState::PauseState() {}

void PauseState::onEnter() {
    sf::Font& font = AssetManager::getInstance().getFont("assets/fonts/mario_font.ttf");

    // Semi-transparent overlay
    m_overlay.setSize(sf::Vector2f(WINDOW_WIDTH, WINDOW_HEIGHT));
    m_overlay.setFillColor(sf::Color(0, 0, 0, 150));

    m_title.setFont(font);
    m_title.setString("PAUSED");
    m_title.setCharacterSize(48);
    m_title.setFillColor(sf::Color::White);
    auto tb = m_title.getLocalBounds();
    m_title.setOrigin(tb.width / 2.0f, tb.height / 2.0f);
    m_title.setPosition(WINDOW_WIDTH / 2.0f, 180.0f);

    std::string labels[] = {"RESUME", "QUIT TO MENU"};
    for (int i = 0; i < 2; i++) {
        m_options[i].setFont(font);
        m_options[i].setString(labels[i]);
        m_options[i].setCharacterSize(24);
        auto ob = m_options[i].getLocalBounds();
        m_options[i].setOrigin(ob.width / 2.0f, ob.height / 2.0f);
        m_options[i].setPosition(WINDOW_WIDTH / 2.0f, 300.0f + i * 60.0f);
        m_options[i].setFillColor(i == 0 ? sf::Color::Yellow : sf::Color::White);
    }

    m_selected = 0;
}

void PauseState::onExit() {}

void PauseState::handleEvent(const sf::Event& event) {
    sf::RenderWindow& window = Game::getInstance().getWindow();

    if (event.type == sf::Event::MouseMoved) {
        sf::Vector2f mousePos = window.mapPixelToCoords({event.mouseMove.x, event.mouseMove.y});
        for (int i = 0; i < 2; i++) {
            if (m_options[i].getGlobalBounds().contains(mousePos)) {
                if (m_selected != i) {
                    m_selected = i;
                    for (int j = 0; j < 2; j++)
                        m_options[j].setFillColor(j == m_selected ? sf::Color::Yellow : sf::Color::White);
                }
                break;
            }
        }
        return;
    }

    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2f mousePos = window.mapPixelToCoords({event.mouseButton.x, event.mouseButton.y});
        for (int i = 0; i < 2; i++) {
            if (m_options[i].getGlobalBounds().contains(mousePos)) {
                m_selected = i;
                if (m_selected == 0) {
                    Game::getInstance().getStateManager().popState();
                } else {
                    SaveManager::saveGame();
                    Game::getInstance().getStateManager().clearStates();
                    Game::getInstance().getStateManager().pushState(
                        std::make_unique<MenuState>());
                }
                return;
            }
        }
    }

    if (event.type == sf::Event::KeyPressed) {
        switch (event.key.code) {
            case sf::Keyboard::Escape:
                Game::getInstance().getStateManager().popState();
                break;

            case sf::Keyboard::Up:
            case sf::Keyboard::W:
                m_selected = (m_selected - 1 + 2) % 2;
                for (int i = 0; i < 2; i++)
                    m_options[i].setFillColor(i == m_selected ? sf::Color::Yellow : sf::Color::White);
                break;

            case sf::Keyboard::Down:
            case sf::Keyboard::S:
                m_selected = (m_selected + 1) % 2;
                for (int i = 0; i < 2; i++)
                    m_options[i].setFillColor(i == m_selected ? sf::Color::Yellow : sf::Color::White);
                break;

            case sf::Keyboard::Return:
            case sf::Keyboard::Space:
                if (m_selected == 0) {
                    Game::getInstance().getStateManager().popState();
                } else {
                    SaveManager::saveGame();
                    Game::getInstance().getStateManager().clearStates();
                    Game::getInstance().getStateManager().pushState(
                        std::make_unique<MenuState>());
                }
                break;

            default:
                break;
        }
    }
}

void PauseState::update(float dt) {
    // Pause state doesn't update game logic
}

void PauseState::render(sf::RenderWindow& window) {
    // The playing state underneath still renders (from StateManager's bottom-to-top render)
    // We just draw our overlay on top
    window.setView(window.getDefaultView());
    window.draw(m_overlay);
    window.draw(m_title);
    for (int i = 0; i < 2; i++) {
        window.draw(m_options[i]);
    }
}
