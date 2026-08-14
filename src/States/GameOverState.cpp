#include "States/GameOverState.hpp"
#include "States/CharacterSelectState.hpp"
#include "States/MenuState.hpp"
#include "States/PlayingState.hpp"
#include "Core/Game.hpp"
#include "Core/AssetManager.hpp"
#include "States/StateManager.hpp"
#include "Physics/PhysicsConstants.hpp"

GameOverState::GameOverState(GameResult result, const std::string& winnerName)
    : m_result(result), m_winnerName(winnerName) {}

void GameOverState::onEnter() {
    sf::Font& font = AssetManager::getInstance().getFont("assets/fonts/mario_font.ttf");

    bool pvpResult = (m_result == GameResult::P1Won || m_result == GameResult::P2Won);
    bool won = (m_result == GameResult::Won || pvpResult);

    m_background.setSize(sf::Vector2f(WINDOW_WIDTH, WINDOW_HEIGHT));
    m_background.setFillColor(won ? sf::Color(20, 45, 75) : sf::Color(20, 20, 20));

    m_title.setFont(font);
    if (pvpResult) {
        m_title.setString(m_winnerName + " WINS!");
    } else {
        m_title.setString(won ? "YOU WIN!" : "GAME OVER");
    }
    m_title.setCharacterSize(52);
    m_title.setFillColor(won ? sf::Color::Yellow : sf::Color::Red);
    auto tb = m_title.getLocalBounds();
    m_title.setOrigin(tb.width / 2.0f, tb.height / 2.0f);
    m_title.setPosition(WINDOW_WIDTH / 2.0f, 150.0f);

    m_scoreText.setFont(font);
    m_scoreText.setString("SCORE: " + std::to_string(Game::getInstance().getProgress().getScore()));
    m_scoreText.setCharacterSize(28);
    m_scoreText.setFillColor(sf::Color::White);
    auto sb = m_scoreText.getLocalBounds();
    m_scoreText.setOrigin(sb.width / 2.0f, sb.height / 2.0f);
    m_scoreText.setPosition(WINDOW_WIDTH / 2.0f, 250.0f);

    std::string labels[] = {
        (m_result == GameResult::Won) ? "NEW GAME" : (pvpResult ? "PLAY AGAIN" : "RETRY"),
        "MAIN MENU"};
    for (int i = 0; i < 2; i++) {
        m_options[i].setFont(font);
        m_options[i].setString(labels[i]);
        m_options[i].setCharacterSize(24);
        auto ob = m_options[i].getLocalBounds();
        m_options[i].setOrigin(ob.width / 2.0f, ob.height / 2.0f);
        m_options[i].setPosition(WINDOW_WIDTH / 2.0f, 350.0f + i * 60.0f);
        m_options[i].setFillColor(i == 0 ? sf::Color::Yellow : sf::Color::White);
    }

    m_selected = 0;
}

void GameOverState::onExit() {}

void GameOverState::activateSelectedOption() {
    Game& game = Game::getInstance();

    if (m_selected == 0) {
        bool pvpResult = (m_result == GameResult::P1Won || m_result == GameResult::P2Won);
        if (m_result == GameResult::Won || pvpResult) {
            if (pvpResult) {
                // PvP: replay the arena
                game.getStateManager().changeState(std::make_unique<PlayingState>());
            } else {
                game.getProgress().resetGameData();
                game.getStateManager().changeState(
                    std::make_unique<CharacterSelectState>());
            }
        } else {
            game.getProgress().retryCurrentLevel();
            game.getStateManager().changeState(
                std::make_unique<PlayingState>());
        }
    } else {
        game.getStateManager().changeState(std::make_unique<MenuState>());
    }
}

void GameOverState::handleEvent(const sf::Event& event) {
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
                activateSelectedOption();
                return;
            }
        }
    }

    if (event.type == sf::Event::KeyPressed) {
        switch (event.key.code) {
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
                activateSelectedOption();
                break;

            default:
                break;
        }
    }
}

void GameOverState::update(float dt) {}

void GameOverState::render(sf::RenderWindow& window) {
    window.setView(window.getDefaultView());
    window.draw(m_background);
    window.draw(m_title);
    window.draw(m_scoreText);
    for (int i = 0; i < 2; i++) {
        window.draw(m_options[i]);
    }
}
