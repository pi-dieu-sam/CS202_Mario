#include "States/MenuState.hpp"

#include "Core/AssetManager.hpp"
#include "Core/Game.hpp"
#include "Core/SoundManager.hpp"
#include "Entities/Player.hpp"
#include "Graphics/SpriteRegistry.hpp"
#include "Physics/PhysicsConstants.hpp"
#include "States/Navigator.hpp"
#include "UI/MenuAttractTimeline.hpp"

#include <array>
#include <cmath>

namespace {
const std::string TITLE_SHEET_PATH =
    "assets/textures/NES - Super Mario Bros. - Miscellaneous - Title Screen, HUD and Miscellaneous.png";

const sf::IntRect TITLE_CARD_RECT(40, 224, 176, 88);
const sf::IntRect HILL_RECT(0, 364, 80, 37);
const sf::IntRect BUSH_RECT(183, 383, 65, 18);
const sf::IntRect GROUND_RECT(0, 401, 256, 30);
const sf::Color SKY_COLOR(148, 148, 255);

constexpr float GROUND_HEIGHT = 80.0f;
constexpr float PIPE_X = 280.0f;
constexpr float PIPE_TOP_Y = 464.0f;

void centerText(sf::Text& text, float x, float y) {
    const sf::FloatRect bounds = text.getLocalBounds();
    text.setOrigin(bounds.left + bounds.width * 0.5f, bounds.top + bounds.height * 0.5f);
    text.setPosition(x, y);
}

const std::array<std::string, 6> MENU_LABELS = {
    "1 PLAYER GAME", "2 PLAYER CO-OP", "2 PLAYER PvP",
    "LOAD GAME", "SETTINGS", "EXIT"};

const std::array<std::string, 6> MENU_HINTS = {
    "CHOOSE MARIO OR LUIGI, THEN PICK A WORLD",
    "TEAM UP — REVIEW BOTH PLAYERS' CONTROLS BEFORE SELECTING A WORLD",
    "ENTER THE ARENA — STOMP OR FIREBALL YOUR RIVAL",
    "CONTINUE A SAVED 1 PLAYER ADVENTURE",
    "VIDEO, FRAME RATE, MUSIC, AND SOUND EFFECTS",
    "CLOSE THE GAME"};
} // namespace

void MenuState::onEnter() {
    SoundManager& sound = SoundManager::getInstance();
    sound.selectTrack(sound.getCurrentTrackIndex());

    sf::Font& font = AssetManager::getInstance().getFont("assets/fonts/mario_font.ttf");
    sf::Texture& titleSheet = AssetManager::getInstance().getTexture(TITLE_SHEET_PATH);

    m_background.setSize({static_cast<float>(WINDOW_WIDTH), static_cast<float>(WINDOW_HEIGHT)});
    m_background.setFillColor(SKY_COLOR);

    const float groundTop = WINDOW_HEIGHT - GROUND_HEIGHT;
    m_groundSprite.setTexture(titleSheet);
    m_groundSprite.setTextureRect(GROUND_RECT);
    m_groundSprite.setOrigin(0.0f, 0.0f);
    m_groundSprite.setScale(WINDOW_WIDTH / static_cast<float>(GROUND_RECT.width),
                            GROUND_HEIGHT / static_cast<float>(GROUND_RECT.height));
    m_groundSprite.setPosition(0.0f, groundTop);

    SpriteRegistry::applyFrame(m_hillSprite, titleSheet, HILL_RECT,
                               sf::FloatRect(130.0f, groundTop - 102.0f, 0.0f, 102.0f));
    SpriteRegistry::applyFrame(m_bushSprite, titleSheet, BUSH_RECT,
                               sf::FloatRect(690.0f, groundTop - 54.0f, 0.0f, 54.0f));

    // Assemble a two-tile-tall warp pipe from the four quadrants used by
    // gameplay tiles, giving the attract loop a real obstacle to clear.
    sf::Texture& pipeTexture = AssetManager::getInstance().getTexture("assets/textures/Warp_Pipe_SMB.png");
    const sf::Vector2u pipeSize = pipeTexture.getSize();
    const int halfW = static_cast<int>(pipeSize.x) / 2;
    const int halfH = static_cast<int>(pipeSize.y) / 2;
    const std::array<sf::IntRect, 4> pipeRects = {
        sf::IntRect(0, 0, halfW, halfH), sf::IntRect(halfW, 0, halfW, halfH),
        sf::IntRect(0, halfH, halfW, halfH), sf::IntRect(halfW, halfH, halfW, halfH)};
    const std::array<sf::Vector2f, 4> pipePositions = {
        sf::Vector2f(PIPE_X, PIPE_TOP_Y), sf::Vector2f(PIPE_X + 32.0f, PIPE_TOP_Y),
        sf::Vector2f(PIPE_X, PIPE_TOP_Y + 32.0f), sf::Vector2f(PIPE_X + 32.0f, PIPE_TOP_Y + 32.0f)};
    for (int i = 0; i < 4; ++i) {
        SpriteRegistry::applyFrame(m_pipePieces[i], pipeTexture, pipeRects[i],
                                   sf::FloatRect(pipePositions[i].x, pipePositions[i].y, 32.0f, 32.0f));
    }

    SpriteRegistry::applyFrame(m_titleCardSprite, titleSheet, TITLE_CARD_RECT,
                               sf::FloatRect(WINDOW_WIDTH * 0.5f, 56.0f, 0.0f, 174.0f));
    m_titleBaseY = 230.0f;

    auto setupHudText = [&](sf::Text& text, const std::string& value, float x, float y,
                            unsigned int size = 16) {
        text.setFont(font);
        text.setString(value);
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
    SpriteRegistry::applyFrame(m_coinSprite, "assets/textures/SMBCoin.gif",
                               sf::FloatRect(230.0f, 26.0f, 0.0f, 22.0f));

    m_copyrightText.setFont(font);
    m_copyrightText.setString("(C)1985 NINTENDO");
    m_copyrightText.setCharacterSize(13);
    m_copyrightText.setFillColor(sf::Color(255, 206, 197));
    centerText(m_copyrightText, WINDOW_WIDTH * 0.5f, 252.0f);

    m_menuPanel.setSize({320.0f, 239.0f});
    m_menuPanel.setPosition(430.0f, 270.0f);
    m_menuPanel.setFillColor(sf::Color(17, 29, 69, 226));
    m_menuPanel.setOutlineThickness(3.0f);
    m_menuPanel.setOutlineColor(sf::Color(96, 133, 212));

    m_menuHeader.setFont(font);
    m_menuHeader.setString("CHOOSE A MODE");
    m_menuHeader.setCharacterSize(16);
    m_menuHeader.setFillColor(sf::Color(255, 221, 102));
    centerText(m_menuHeader, 590.0f, 284.0f);

    for (int i = 0; i < OPTION_COUNT; ++i) {
        const float y = 298.0f + i * 33.0f;
        m_optionPanels[i].setSize({282.0f, 28.0f});
        m_optionPanels[i].setPosition(449.0f, y);
        m_optionPanels[i].setOutlineThickness(1.0f);

        m_options[i].setFont(font);
        m_options[i].setString(MENU_LABELS[static_cast<std::size_t>(i)]);
        m_options[i].setCharacterSize(17);
        centerText(m_options[i], 590.0f, y + 14.0f);
    }

    m_selectionHint.setFont(font);
    m_selectionHint.setCharacterSize(11);
    m_selectionHint.setFillColor(sf::Color(207, 221, 255));
    centerText(m_selectionHint, 590.0f, 521.0f);

    m_controlHint.setFont(font);
    m_controlHint.setString("UP/DOWN: SELECT    ENTER: CONFIRM");
    m_controlHint.setCharacterSize(11);
    m_controlHint.setFillColor(sf::Color(232, 236, 255));
    centerText(m_controlHint, 590.0f, 548.0f);

    m_cursor.setRadius(5.0f);
    m_cursor.setOrigin(5.0f, 5.0f);
    m_cursor.setFillColor(sf::Color(255, 221, 102));

    m_selectedOption = 0;
    m_titleBounce = 0.0f;
    m_attractTime = 0.0f;
    updateAttractScene(0.0f);
    updateOptionVisuals();
}

void MenuState::onExit() {}

void MenuState::activateSelectedOption() {
    Game& game = Game::getInstance();
    PlayerProgress& progress = game.getProgress();
    switch (m_selectedOption) {
    case 0:
        progress.resetGameData();
        progress.setGameMode(GameMode::SinglePlayer);
        break;
    case 1:
        progress.resetGameData();
        progress.setGameMode(GameMode::Coop);
        break;
    case 2:
        progress.resetGameData();
        progress.setGameMode(GameMode::PvP);
        break;
    default:
        break;
    }
    Navigator::apply(ScreenFlow::onMenuOption(m_selectedOption, false),
                     game.getStateManager(), progress.getGameMode());
}

void MenuState::handleEvent(const sf::Event& event) {
    Game& game = Game::getInstance();
    if (event.type == sf::Event::MouseMoved ||
        (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left)) {
        const sf::Vector2i pixel = event.type == sf::Event::MouseMoved
            ? sf::Vector2i(event.mouseMove.x, event.mouseMove.y)
            : sf::Vector2i(event.mouseButton.x, event.mouseButton.y);
        const sf::Vector2f mouse = game.mapPixelToUiCoords(pixel);
        for (int i = 0; i < OPTION_COUNT; ++i) {
            if (m_optionPanels[i].getGlobalBounds().contains(mouse)) {
                m_selectedOption = i;
                updateOptionVisuals();
                if (event.type == sf::Event::MouseButtonPressed) activateSelectedOption();
                return;
            }
        }
        return;
    }

    if (event.type != sf::Event::KeyPressed) return;
    switch (event.key.code) {
    case sf::Keyboard::Up:
    case sf::Keyboard::W:
        m_selectedOption = (m_selectedOption - 1 + OPTION_COUNT) % OPTION_COUNT;
        updateOptionVisuals();
        break;
    case sf::Keyboard::Down:
    case sf::Keyboard::S:
        m_selectedOption = (m_selectedOption + 1) % OPTION_COUNT;
        updateOptionVisuals();
        break;
    case sf::Keyboard::Return:
    case sf::Keyboard::Space:
        activateSelectedOption();
        break;
    default:
        break;
    }
}

void MenuState::update(float dt) {
    m_titleBounce += dt * 2.0f;
    m_titleCardSprite.setPosition(WINDOW_WIDTH * 0.5f,
                                  m_titleBaseY + std::sin(m_titleBounce) * 5.0f);
    updateAttractScene(dt);
}

void MenuState::updateAttractScene(float dt) {
    m_attractTime = std::fmod(m_attractTime + dt, 10.0f);
    const float groundTop = WINDOW_HEIGHT - GROUND_HEIGHT;
    const sf::Color hidden(255, 255, 255, 0);
    m_marioSprite.setColor(hidden);
    m_luigiSprite.setColor(hidden);
    m_goombaSprite.setColor(hidden);
    m_fireballSprite.setColor(hidden);

    const MenuAttractTimeline::Frame timeline = MenuAttractTimeline::evaluate(m_attractTime);
    if (timeline.scene == MenuAttractTimeline::Scene::MarioChase) {
        const float sceneTime = timeline.sceneTime;
        const float marioX = -44.0f + sceneTime * 100.0f;
        const bool jumping = timeline.marioJumping;
        const float jumpPhase = jumping ? (sceneTime - 2.55f) / 0.9f : 0.0f;
        const float jumpLift = jumping ? std::sin(jumpPhase * 3.14159265f) * 84.0f : 0.0f;
        const SpriteRegistry::PlayerAnim anim = jumping
            ? SpriteRegistry::PlayerAnim::Jump : SpriteRegistry::PlayerAnim::Walk;
        SpriteRegistry::applyPlayerFrame(m_marioSprite, CharacterId::Mario, PowerUpState::Big,
                                         anim, static_cast<int>(sceneTime * 10.0f),
                                         sf::FloatRect(marioX, groundTop - jumpLift - 64.0f, 0.0f, 64.0f));
        m_marioSprite.setColor(sf::Color::White);
        SpriteRegistry::applyGoombaFrame(m_goombaSprite, static_cast<int>(sceneTime * 11.0f),
                                         sf::FloatRect(marioX - 92.0f, groundTop - 38.0f, 0.0f, 38.0f));
        m_goombaSprite.setColor(sf::Color::White);
    } else {
        const float sceneTime = timeline.sceneTime;
        const float luigiX = -44.0f + sceneTime * 98.0f;
        const bool firing = timeline.luigiFiring;
        const SpriteRegistry::PlayerAnim anim = firing
            ? SpriteRegistry::PlayerAnim::Fire : SpriteRegistry::PlayerAnim::Walk;
        SpriteRegistry::applyPlayerFrame(m_luigiSprite, CharacterId::Luigi, PowerUpState::Fire,
                                         anim, static_cast<int>(sceneTime * 10.0f),
                                         sf::FloatRect(luigiX, groundTop - 64.0f, 0.0f, 64.0f));
        m_luigiSprite.setColor(sf::Color::White);
        SpriteRegistry::applyGoombaFrame(m_goombaSprite, static_cast<int>(sceneTime * 11.0f),
                                         sf::FloatRect(350.0f, groundTop - 38.0f, 0.0f, 38.0f), true);
        m_goombaSprite.setColor(sf::Color::White);
        if (firing) {
            const float fireX = luigiX + 28.0f + (sceneTime - 2.15f) * 250.0f;
            const float fireY = groundTop - 44.0f - std::sin((sceneTime - 2.15f) * 12.0f) * 12.0f;
            SpriteRegistry::applySheetFrame(m_fireballSprite, SpriteRegistry::fireballPath(),
                                            static_cast<int>(sceneTime * 12.0f), 16, 0,
                                            sf::FloatRect(fireX, fireY, 20.0f, 20.0f));
            m_fireballSprite.setColor(sf::Color::White);
        }
    }
}

void MenuState::render(sf::RenderWindow& window) {
    window.setView(Game::getInstance().getUiView());
    window.draw(m_background);
    window.draw(m_groundSprite);
    window.draw(m_hillSprite);
    window.draw(m_bushSprite);
    for (const sf::Sprite& pipe : m_pipePieces) window.draw(pipe);
    window.draw(m_goombaSprite);
    window.draw(m_marioSprite);
    window.draw(m_luigiSprite);
    window.draw(m_fireballSprite);
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

    window.draw(m_menuPanel);
    window.draw(m_menuHeader);
    for (int i = 0; i < OPTION_COUNT; ++i) {
        window.draw(m_optionPanels[i]);
        window.draw(m_options[i]);
    }
    window.draw(m_selectionHint);
    window.draw(m_controlHint);
    window.draw(m_cursor);
}

void MenuState::updateOptionVisuals() {
    for (int i = 0; i < OPTION_COUNT; ++i) {
        const bool selected = i == m_selectedOption;
        m_optionPanels[i].setFillColor(selected ? sf::Color(73, 112, 194, 245)
                                                : sf::Color(32, 49, 99, 220));
        m_optionPanels[i].setOutlineColor(selected ? sf::Color(255, 221, 102)
                                                    : sf::Color(103, 137, 210));
        m_options[i].setFillColor(selected ? sf::Color::White : sf::Color(214, 225, 255));
    }
    m_selectionHint.setString(MENU_HINTS[static_cast<std::size_t>(m_selectedOption)]);
    centerText(m_selectionHint, 590.0f, 521.0f);
    const sf::FloatRect selected = m_optionPanels[m_selectedOption].getGlobalBounds();
    m_cursor.setPosition(selected.left - 10.0f, selected.top + selected.height * 0.5f);
}
