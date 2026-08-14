#include "States/MenuState.hpp"
#include "States/CharacterSelectState.hpp"
#include "States/PlayingState.hpp"
#include "Core/Game.hpp"
#include "Core/AssetManager.hpp"
#include "Core/PlayerProgress.hpp"
#include "Core/SaveManager.hpp"
#include "Core/SoundManager.hpp"
#include "States/StateManager.hpp"
#include "Physics/PhysicsConstants.hpp"
#include "Graphics/SpriteRegistry.hpp"
#include <cmath>

namespace {
// Source sheet all the scenery crops below come from.
const std::string TITLE_SHEET_PATH =
    "assets/textures/NES - Super Mario Bros. - Miscellaneous - Title Screen, "
    "HUD and Miscellaneous.png";

// Pixel-sampled crop rects within TITLE_SHEET_PATH. Each is inset 1-2px
// from its raw color-detected bounding box to cut off the anti-aliased
// fringe pixels the source sheet has at every sprite boundary — left in,
// those blend to a visible cyan/green sliver once scaled up 2-3x.
const sf::IntRect TITLE_CARD_RECT(40, 224, 176, 88);
const sf::IntRect HILL_RECT(0, 364, 80, 37);
const sf::IntRect BUSH_RECT(183, 383, 65, 18);
const sf::IntRect GROUND_RECT(0, 401, 256, 30);

// Sky color baked into the crops above — the menu background matches it
// exactly so the sprites blend in with no visible seam.
const sf::Color SKY_COLOR(148, 148, 255);

// Layout constants (window is WINDOW_WIDTH x WINDOW_HEIGHT).
constexpr float TITLE_CARD_TOP    = 60.0f;
constexpr float TITLE_CARD_HEIGHT = 200.0f;
constexpr float TITLE_CARD_BOTTOM = TITLE_CARD_TOP + TITLE_CARD_HEIGHT;

constexpr float GROUND_HEIGHT = 80.0f;

constexpr float HILL_CENTER_X = 140.0f;
constexpr float HILL_HEIGHT   = 100.0f;

constexpr float BUSH_CENTER_X = 660.0f;
constexpr float BUSH_HEIGHT   = 50.0f;

constexpr float MARIO_CENTER_X = HILL_CENTER_X + 150.0f;
constexpr float MARIO_HEIGHT   = 50.0f;
} // namespace

MenuState::MenuState() {}

void MenuState::onEnter() {
    SoundManager::getInstance().playMusic("assets/audio/hide_dorian_concept.wav", true);
    sf::Font& font = AssetManager::getInstance().getFont("assets/fonts/mario_font.ttf");
    sf::Texture& titleSheet = AssetManager::getInstance().getTexture(TITLE_SHEET_PATH);

    // Background — matches the sky color baked into the cropped sprites.
    m_background.setSize(sf::Vector2f(WINDOW_WIDTH, WINDOW_HEIGHT));
    m_background.setFillColor(SKY_COLOR);

    // Ground strip: single sprite stretched to exactly span the window width.
    {
        float groundTop = WINDOW_HEIGHT - GROUND_HEIGHT;
        m_groundSprite.setTexture(titleSheet);
        m_groundSprite.setTextureRect(GROUND_RECT);
        m_groundSprite.setOrigin(0.0f, 0.0f);
        m_groundSprite.setScale(WINDOW_WIDTH / static_cast<float>(GROUND_RECT.width),
                                 GROUND_HEIGHT / static_cast<float>(GROUND_RECT.height));
        m_groundSprite.setPosition(0.0f, groundTop);
    }

    // Hill, bottom-anchored on top of the ground strip.
    {
        float groundTop = WINDOW_HEIGHT - GROUND_HEIGHT;
        sf::FloatRect box(HILL_CENTER_X, groundTop - HILL_HEIGHT, 0.0f, HILL_HEIGHT);
        SpriteRegistry::applyFrame(m_hillSprite, titleSheet, HILL_RECT, box);
    }

    // Bushes, bottom-anchored on the same ground line, off to the right.
    {
        float groundTop = WINDOW_HEIGHT - GROUND_HEIGHT;
        sf::FloatRect box(BUSH_CENTER_X, groundTop - BUSH_HEIGHT, 0.0f, BUSH_HEIGHT);
        SpriteRegistry::applyFrame(m_bushSprite, titleSheet, BUSH_RECT, box);
    }

    // Small Mario, standing partway up the hill's slope (not balanced right
    // on the peak tip — matches how he's placed on the reference screen).
    {
        float groundTop = WINDOW_HEIGHT - GROUND_HEIGHT;
        float hillTop = groundTop - HILL_HEIGHT;
        float marioFeetY = hillTop + HILL_HEIGHT * 0.35f + 65.0f;
        sf::FloatRect box(MARIO_CENTER_X, marioFeetY - MARIO_HEIGHT, 0.0f, MARIO_HEIGHT);
        SpriteRegistry::applyFrame(m_marioSprite, "assets/textures/SMB_Smallmario.png", box);
    }

    // Title card — centered horizontally; bounce animation moves it in update().
    {
        sf::FloatRect box(WINDOW_WIDTH / 2.0f, TITLE_CARD_TOP, 0.0f, TITLE_CARD_HEIGHT);
        SpriteRegistry::applyFrame(m_titleCardSprite, titleSheet, TITLE_CARD_RECT, box);
    }
    m_titleBaseY = TITLE_CARD_BOTTOM;

    // Decorative HUD row.
    auto setupHudText = [&](sf::Text& text, const std::string& str, float x, float y,
                             unsigned int size = 16) {
        text.setFont(font);
        text.setString(str);
        text.setCharacterSize(size);
        text.setFillColor(sf::Color::White);
        text.setOutlineColor(sf::Color::Black);
        text.setOutlineThickness(1.0f);
        text.setPosition(x, y);
    };
    setupHudText(m_hudCharacterText, "MARIO", 90.0f, 10.0f);
    setupHudText(m_hudScoreText, "000000", 90.0f, 32.0f);
    setupHudText(m_hudCoinCountText, "x00", 250.0f, 20.0f);
    setupHudText(m_hudWorldLabelText, "WORLD", 430.0f, 10.0f);
    setupHudText(m_hudWorldText, "1-1", 445.0f, 32.0f);
    setupHudText(m_hudTimeLabelText, "TIME", 610.0f, 10.0f);
    setupHudText(m_hudTimeText, "300", 615.0f, 32.0f);

    // Coin icon next to the coin count.
    {
        sf::FloatRect box(230.0f, 26.0f, 0.0f, 22.0f);
        SpriteRegistry::applyFrame(m_coinSprite, "assets/textures/SMBCoin.gif", box);
    }

    // Copyright line under the title card.
    m_copyrightText.setFont(font);
    // mario_font.ttf has no "©" glyph (renders as a tofu box) — use "(C)" instead.
    m_copyrightText.setString("(C)1985 NINTENDO");
    m_copyrightText.setCharacterSize(14);
    m_copyrightText.setFillColor(sf::Color(255, 206, 197));
    {
        auto bounds = m_copyrightText.getLocalBounds();
        m_copyrightText.setOrigin(bounds.width / 2.0f, bounds.height / 2.0f);
        m_copyrightText.setPosition(WINDOW_WIDTH / 2.0f, 285.0f);
    }

    // "TOP- 000000" flavor text removed per request

    // Menu options.
    std::string labels[] = {"1 PLAYER GAME", "2 PLAYER CO-OP", "2 PLAYER PvP", "LOAD GAME", "EXIT"};
    for (int i = 0; i < 5; i++) {
        m_options[i].setFont(font);
        m_options[i].setString(labels[i]);
        m_options[i].setCharacterSize(24);
        m_options[i].setOutlineColor(sf::Color::Black);
        m_options[i].setOutlineThickness(2.0f);
        auto bounds = m_options[i].getLocalBounds();
        m_options[i].setOrigin(bounds.width / 2.0f, bounds.height / 2.0f);
        m_options[i].setPosition(WINDOW_WIDTH / 2.0f, 300.0f + i * 38.0f);
    }

    m_cursor.setRadius(6.0f);
    m_cursor.setOrigin(6.0f, 6.0f);
    m_cursor.setFillColor(sf::Color(216, 40, 0));

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
        for (int i = 0; i < 5; i++) {
            if (m_options[i].getGlobalBounds().contains(mousePos)) {
                m_selectedOption = i;
                updateOptionVisuals();

                switch (m_selectedOption) {
                    case 0: // 1 Player Game
                        Game::getInstance().getProgress().resetGameData();
                        Game::getInstance().getProgress().setGameMode(GameMode::SinglePlayer);
                        Game::getInstance().getStateManager().changeState(
                            std::make_unique<CharacterSelectState>());
                        break;
                    case 1: // 2 Player Co-op
                        Game::getInstance().getProgress().resetGameData();
                        Game::getInstance().getProgress().setGameMode(GameMode::Coop);
                        Game::getInstance().getStateManager().changeState(
                            std::make_unique<CharacterSelectState>());
                        break;
                    case 2: // 2 Player PvP
                        Game::getInstance().getProgress().resetGameData();
                        Game::getInstance().getProgress().setGameMode(GameMode::PvP);
                        Game::getInstance().getStateManager().changeState(
                            std::make_unique<CharacterSelectState>());
                        break;
                    case 3: // Load Game
                        if (SaveManager::loadGame()) {
                            Game::getInstance().getStateManager().changeState(
                                std::make_unique<PlayingState>());
                        }
                        break;
                    case 4: // Exit
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
                m_selectedOption = (m_selectedOption - 1 + 5) % 5;
                updateOptionVisuals();
                break;

            case sf::Keyboard::Down:
            case sf::Keyboard::S:
                m_selectedOption = (m_selectedOption + 1) % 5;
                updateOptionVisuals();
                break;

            case sf::Keyboard::Return:
            case sf::Keyboard::Space:
                switch (m_selectedOption) {
                    case 0: // 1 Player Game
                        Game::getInstance().getProgress().resetGameData();
                        Game::getInstance().getProgress().setGameMode(GameMode::SinglePlayer);
                        Game::getInstance().getStateManager().changeState(
                            std::make_unique<CharacterSelectState>());
                        break;
                    case 1: // 2 Player Co-op
                        Game::getInstance().getProgress().resetGameData();
                        Game::getInstance().getProgress().setGameMode(GameMode::Coop);
                        Game::getInstance().getStateManager().changeState(
                            std::make_unique<PlayingState>());
                        break;
                    case 2: // 2 Player PvP
                        Game::getInstance().getProgress().resetGameData();
                        Game::getInstance().getProgress().setGameMode(GameMode::PvP);
                        Game::getInstance().getStateManager().changeState(
                            std::make_unique<PlayingState>());
                        break;
                    case 3: // Load Game
                        if (SaveManager::loadGame()) {
                            Game::getInstance().getStateManager().changeState(
                                std::make_unique<PlayingState>());
                        }
                        break;
                    case 4: // Exit
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
    float bounceY = m_titleBaseY + std::sin(m_titleBounce) * 8.0f;
    m_titleCardSprite.setPosition(WINDOW_WIDTH / 2.0f, bounceY);
}

void MenuState::render(sf::RenderWindow& window) {
    // Reset view to default for menu rendering
    window.setView(window.getDefaultView());

    window.draw(m_background);
    window.draw(m_groundSprite);
    window.draw(m_hillSprite);
    window.draw(m_bushSprite);
    window.draw(m_marioSprite);
    window.draw(m_titleCardSprite);

    window.draw(m_hudCharacterText);
    window.draw(m_hudScoreText);
    window.draw(m_coinSprite);
    window.draw(m_hudCoinCountText);
    window.draw(m_hudWorldLabelText);
    window.draw(m_hudWorldText);
    window.draw(m_hudTimeLabelText);
    window.draw(m_hudTimeText);

    window.draw(m_copyrightText);

    for (int i = 0; i < 5; i++) {
        window.draw(m_options[i]);
    }
    window.draw(m_cursor);
}

void MenuState::updateOptionVisuals() {
    for (int i = 0; i < 5; i++) {
        if (i == m_selectedOption) {
            m_options[i].setFillColor(sf::Color(255, 220, 100));
        } else {
            m_options[i].setFillColor(sf::Color::White);
        }
    }

    auto bounds = m_options[m_selectedOption].getGlobalBounds();
    m_cursor.setPosition(bounds.left - 18.0f, bounds.top + bounds.height / 2.0f);
}
