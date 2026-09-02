#include "States/GameOverState.hpp"
#include "States/Navigator.hpp"
#include "Core/Game.hpp"
#include "Core/AssetManager.hpp"
#include "Entities/Player.hpp"
#include "Graphics/SpriteRegistry.hpp"
#include "Physics/PhysicsConstants.hpp"
#include <cmath>
#include <string>

GameOverState::GameOverState(GameResult result, const std::string& winnerName)
    : m_result(result), m_winnerName(winnerName) {}

void GameOverState::onEnter() {
    sf::Font& font = AssetManager::getInstance().getFont("assets/fonts/mario_font.ttf");

    bool pvpResult = (m_result == GameResult::P1Won || m_result == GameResult::P2Won);
    bool won = (m_result == GameResult::Won || pvpResult);
    m_animTime = 0.0f;
    m_title.setScale(1.0f, 1.0f);

    // Background: deep gold for PvP win, blue for normal win, dark for loss
    if (pvpResult) {
        m_background.setSize(sf::Vector2f(WINDOW_WIDTH, WINDOW_HEIGHT));
        m_background.setFillColor(sf::Color(15, 10, 35)); // deep dark purple
    } else {
        m_background.setSize(sf::Vector2f(WINDOW_WIDTH, WINDOW_HEIGHT));
        m_background.setFillColor(won ? sf::Color(20, 45, 75) : sf::Color(20, 20, 20));
    }

    m_resultPanel.setSize({500.0f, 410.0f});
    m_resultPanel.setPosition(150.0f, 96.0f);
    m_resultPanel.setFillColor(sf::Color(13, 20, 55, 230));
    m_resultPanel.setOutlineThickness(4.0f);
    m_resultPanel.setOutlineColor(pvpResult ? sf::Color(255, 190, 77)
                                            : (won ? sf::Color(116, 201, 255) : sf::Color(216, 75, 75)));

    // Main title
    m_title.setFont(font);
    if (pvpResult) {
        m_title.setString(m_winnerName + " WINS!");
        m_title.setCharacterSize(64);
        m_title.setFillColor(sf::Color(255, 215, 0)); // Gold
        m_title.setOutlineColor(sf::Color(180, 100, 0));
        m_title.setOutlineThickness(4.0f);
    } else {
        m_title.setString(won ? "YOU WIN!" : "GAME OVER");
        m_title.setCharacterSize(52);
        m_title.setFillColor(won ? sf::Color::Yellow : sf::Color::Red);
        m_title.setOutlineThickness(0.0f);
    }
    auto tb = m_title.getLocalBounds();
    m_title.setOrigin(tb.width / 2.0f, tb.height / 2.0f);
    m_title.setPosition(WINDOW_WIDTH / 2.0f, 145.0f);

    // Subtitle (PvP only)
    m_subtitleText.setFont(font);
    if (pvpResult) {
        std::string whoWon = (m_result == GameResult::P1Won) ? "PLAYER 1" : "PLAYER 2";
        m_subtitleText.setString(whoWon + " IS THE WINNER!");
        m_subtitleText.setCharacterSize(26);
        m_subtitleText.setFillColor(sf::Color(255, 180, 80));
        m_subtitleText.setOutlineColor(sf::Color(100, 60, 0));
        m_subtitleText.setOutlineThickness(2.0f);
        auto sb2 = m_subtitleText.getLocalBounds();
        m_subtitleText.setOrigin(sb2.width / 2.0f, sb2.height / 2.0f);
        m_subtitleText.setPosition(WINDOW_WIDTH / 2.0f, 205.0f);
    }

    m_detailText.setFont(font);
    m_detailText.setCharacterSize(15);
    m_detailText.setFillColor(sf::Color(210, 224, 255));
    if (pvpResult) {
        m_detailText.setString("THE ARENA HAS A NEW CHAMPION");
    } else if (won) {
        m_detailText.setString("THE KINGDOM IS SAFE — FOR NOW");
    } else {
        m_detailText.setString("TAKE A BREATH, THEN TRY AGAIN");
    }
    auto detailBounds = m_detailText.getLocalBounds();
    m_detailText.setOrigin(detailBounds.left + detailBounds.width / 2.0f,
                           detailBounds.top + detailBounds.height / 2.0f);
    m_detailText.setPosition(WINDOW_WIDTH / 2.0f, pvpResult ? 238.0f : 208.0f);

    // Score text
    m_scoreText.setFont(font);
    m_scoreText.setString(pvpResult ? "" : ("SCORE: " + std::to_string(Game::getInstance().getProgress().getScore())));
    m_scoreText.setCharacterSize(28);
    m_scoreText.setFillColor(sf::Color::White);
    auto sb = m_scoreText.getLocalBounds();
    m_scoreText.setOrigin(sb.width / 2.0f, sb.height / 2.0f);
    m_scoreText.setPosition(WINDOW_WIDTH / 2.0f, 368.0f);

    std::string labels[] = {
        (m_result == GameResult::Won) ? "NEW GAME" : (pvpResult ? "PLAY AGAIN" : "RETRY"),
        "MAIN MENU"};
    for (int i = 0; i < 2; i++) {
        m_options[i].setFont(font);
        m_options[i].setString(labels[i]);
        m_options[i].setCharacterSize(pvpResult ? 28 : 24);
        m_options[i].setOutlineColor(sf::Color::Black);
        m_options[i].setOutlineThickness(pvpResult ? 2.0f : 0.0f);
        auto ob = m_options[i].getLocalBounds();
        m_options[i].setOrigin(ob.width / 2.0f, ob.height / 2.0f);
        m_options[i].setPosition(WINDOW_WIDTH / 2.0f, 423.0f + i * 55.0f);
        m_options[i].setFillColor(i == 0 ? sf::Color::Yellow : sf::Color::White);
    }

    m_selected = 0;
}

void GameOverState::onExit() {}

void GameOverState::activateSelectedOption() {
    Game& game = Game::getInstance();

    if (m_selected == 0) {
        bool pvpResult = (m_result == GameResult::P1Won || m_result == GameResult::P2Won);
        if (m_result == GameResult::Won) {
            // SinglePlayer victory: back to a fresh character/level pick.
            // ResetTo(CharacterSelect) rebuilds the stack as
            // [MainMenu, CharacterSelect], so Escape from there still works.
            game.getProgress().resetGameData();
        } else if (!pvpResult) {
            // Lost: retry the level that was failed, keeping score/coins.
            game.getProgress().retryCurrentLevel();
        }
        // PvP win falls through with no PlayerProgress changes — just
        // replay the arena.
        Navigator::apply(ScreenFlow::onGameOverPrimary(m_result),
                          game.getStateManager(), game.getProgress().getGameMode());
    } else {
        Navigator::apply({ScreenFlow::Op::ResetTo, ScreenFlow::Screen::MainMenu},
                          game.getStateManager(), game.getProgress().getGameMode());
    }
}

void GameOverState::handleEvent(const sf::Event& event) {
    sf::RenderWindow& window = Game::getInstance().getWindow();

    if (event.type == sf::Event::MouseMoved) {
        sf::Vector2f mousePos = Game::getInstance().mapPixelToUiCoords({event.mouseMove.x, event.mouseMove.y});
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
        sf::Vector2f mousePos = Game::getInstance().mapPixelToUiCoords({event.mouseButton.x, event.mouseButton.y});
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

void GameOverState::update(float dt) {
    bool pvpResult = (m_result == GameResult::P1Won || m_result == GameResult::P2Won);
    bool won = (m_result == GameResult::Won || pvpResult);
    m_animTime += dt;
    std::string heroName = pvpResult ? m_winnerName : Game::getInstance().getProgress().getSelectedCharacter();
    const CharacterId hero = heroName == "LUIGI" || heroName == "Luigi"
        ? CharacterId::Luigi : CharacterId::Mario;
    SpriteRegistry::applyPlayerFrame(
        m_heroSprite, hero, won ? PowerUpState::Big : PowerUpState::Small,
        won ? SpriteRegistry::PlayerAnim::Jump : SpriteRegistry::PlayerAnim::Idle,
        static_cast<int>(m_animTime * 9.0f),
        sf::FloatRect(WINDOW_WIDTH * 0.5f, 344.0f, 0.0f, 96.0f));
    if (pvpResult) {
        // Pulse the title scale for celebration effect
        float pulse = 1.0f + 0.06f * std::sin(m_animTime * 4.0f);
        m_title.setScale(pulse, pulse);
        // Oscillate subtitle color
        float c = (std::sin(m_animTime * 3.0f) + 1.0f) * 0.5f;
        m_subtitleText.setFillColor(sf::Color(
            static_cast<uint8_t>(200 + 55 * c),
            static_cast<uint8_t>(140 + 40 * (1.0f - c)),
            50));
    }
}

void GameOverState::render(sf::RenderWindow& window) {
    window.setView(Game::getInstance().getUiView());
    window.draw(m_background);
    window.draw(m_resultPanel);
    window.draw(m_heroSprite);
    window.draw(m_title);
    bool pvpResult = (m_result == GameResult::P1Won || m_result == GameResult::P2Won);
    if (pvpResult) {
        window.draw(m_subtitleText);
    } else {
        window.draw(m_scoreText);
    }
    window.draw(m_detailText);
    for (int i = 0; i < 2; i++) {
        window.draw(m_options[i]);
    }
}
