#include "States/ModeBriefingState.hpp"

#include "Core/AssetManager.hpp"
#include "Core/Game.hpp"
#include "Entities/Player.hpp"
#include "Graphics/SpriteRegistry.hpp"
#include "Physics/PhysicsConstants.hpp"
#include "States/Navigator.hpp"
#include "States/ScreenFlow.hpp"

#include <cmath>

namespace {
void centerText(sf::Text& text, float x, float y) {
    const sf::FloatRect bounds = text.getLocalBounds();
    text.setOrigin(bounds.left + bounds.width * 0.5f, bounds.top + bounds.height * 0.5f);
    text.setPosition(x, y);
}

CharacterId heroId(const std::string& name) {
    return name == "Luigi" ? CharacterId::Luigi : CharacterId::Mario;
}
} // namespace

ModeBriefingState::ModeBriefingState(GameMode mode) : m_mode(mode) {}

void ModeBriefingState::onEnter() {
    sf::Font& font = AssetManager::getInstance().getFont("assets/fonts/mario_font.ttf");
    Game::getInstance().getProgress().setGameMode(m_mode);
    m_animationTime = 0.0f;
    m_selectedAction = 1;

    const bool pvp = m_mode == GameMode::PvP;
    m_background.setSize({static_cast<float>(WINDOW_WIDTH), static_cast<float>(WINDOW_HEIGHT)});
    m_background.setFillColor(pvp ? sf::Color(45, 22, 55) : sf::Color(22, 52, 76));

    m_title.setFont(font);
    m_title.setCharacterSize(34);
    m_title.setString(pvp ? "ARENA READY" : "CO-OP READY");
    m_title.setFillColor(pvp ? sf::Color(255, 205, 90) : sf::Color(255, 236, 130));
    centerText(m_title, WINDOW_WIDTH * 0.5f, 45.0f);

    m_subtitle.setFont(font);
    m_subtitle.setCharacterSize(15);
    m_subtitle.setString(pvp ? "STOMP OR FIREBALL TO WIN" : "TWO HEROES. ONE ADVENTURE.");
    m_subtitle.setFillColor(sf::Color::White);
    centerText(m_subtitle, WINDOW_WIDTH * 0.5f, 83.0f);

    const sf::Color colors[2] = {sf::Color(168, 52, 52, 230), sf::Color(44, 145, 68, 230)};
    for (int i = 0; i < 2; ++i) {
        const float x = 95.0f + i * 365.0f;
        m_characterCards[i].setSize({245.0f, 218.0f});
        m_characterCards[i].setPosition(x, 118.0f);
        m_characterCards[i].setFillColor(colors[i]);
        m_characterCards[i].setOutlineThickness(3.0f);

        m_roleTexts[i].setFont(font);
        m_roleTexts[i].setCharacterSize(16);
        m_roleTexts[i].setString(i == 0 ? "PLAYER 1" : "PLAYER 2");
        m_roleTexts[i].setFillColor(sf::Color(255, 240, 170));
        centerText(m_roleTexts[i], x + 122.5f, 140.0f);

        m_nameTexts[i].setFont(font);
        m_nameTexts[i].setCharacterSize(26);
        m_nameTexts[i].setFillColor(sf::Color::White);
        // The names are set in updateVisuals; it will center them after that.
        centerText(m_nameTexts[i], x + 122.5f, 175.0f);
    }

    m_controlPanel.setSize({650.0f, 112.0f});
    m_controlPanel.setPosition(75.0f, 362.0f);
    m_controlPanel.setFillColor(sf::Color(18, 25, 52, 226));
    m_controlPanel.setOutlineThickness(2.0f);
    m_controlPanel.setOutlineColor(sf::Color(118, 150, 218));

    const std::array<std::string, 2> controls = {
        "P1: A/D MOVE  W/SPACE JUMP  LSHIFT/F FIRE",
        "P2: ARROWS MOVE  UP/NUM0 JUMP  RSHIFT FIRE"};
    for (int i = 0; i < 2; ++i) {
        m_controlTexts[i].setFont(font);
        m_controlTexts[i].setCharacterSize(11);
        m_controlTexts[i].setString(controls[i]);
        m_controlTexts[i].setFillColor(i == 0 ? sf::Color(255, 220, 180) : sf::Color(185, 255, 198));
        centerText(m_controlTexts[i], WINDOW_WIDTH * 0.5f, 392.0f + i * 38.0f);
    }

    const std::array<std::string, 3> actions = {"SWAP HEROES", pvp ? "START ARENA" : "SELECT LEVEL", "BACK"};
    const std::array<float, 3> widths = {200.0f, 230.0f, 130.0f};
    float x = 115.0f;
    for (int i = 0; i < 3; ++i) {
        m_actionBoxes[i].setSize({widths[i], 37.0f});
        m_actionBoxes[i].setPosition(x, 500.0f);
        m_actionBoxes[i].setOutlineThickness(2.0f);

        m_actionTexts[i].setFont(font);
        m_actionTexts[i].setCharacterSize(15);
        m_actionTexts[i].setString(actions[i]);
        centerText(m_actionTexts[i], x + widths[i] * 0.5f, 518.5f);
        x += widths[i] + 14.0f;
    }

    m_help.setFont(font);
    m_help.setCharacterSize(11);
    m_help.setString("ARROWS: SELECT   ENTER: CONFIRM   ESC: BACK");
    m_help.setFillColor(sf::Color(200, 214, 255));
    centerText(m_help, WINDOW_WIDTH * 0.5f, 568.0f);

    updateVisuals();
    updateHeroSprites(0.0f);
}

void ModeBriefingState::onExit() {}

void ModeBriefingState::toggleHeroes() {
    PlayerProgress& progress = Game::getInstance().getProgress();
    progress.setSelectedCharacter(progress.getSelectedCharacter() == "Mario" ? "Luigi" : "Mario");
    updateVisuals();
}

void ModeBriefingState::activateSelectedAction() {
    if (m_selectedAction == 0) {
        toggleHeroes();
        return;
    }
    if (m_selectedAction == 1) {
        Game& game = Game::getInstance();
        Navigator::apply(ScreenFlow::onConfirm(ScreenFlow::Screen::ModeBriefing,
                                                game.getProgress().getGameMode()),
                         game.getStateManager(), game.getProgress().getGameMode());
        return;
    }
    Game& game = Game::getInstance();
    Navigator::apply(ScreenFlow::onBack(ScreenFlow::Screen::ModeBriefing),
                     game.getStateManager(), game.getProgress().getGameMode());
}

void ModeBriefingState::updateVisuals() {
    const std::string p1 = Game::getInstance().getProgress().getSelectedCharacter();
    const std::string p2 = p1 == "Mario" ? "Luigi" : "Mario";
    m_nameTexts[0].setString(p1);
    m_nameTexts[1].setString(p2);
    for (int i = 0; i < 2; ++i) {
        const sf::FloatRect card = m_characterCards[i].getGlobalBounds();
        centerText(m_nameTexts[i], card.left + card.width * 0.5f, 175.0f);
        m_characterCards[i].setOutlineColor(sf::Color(255, 222, 100));
    }
    for (int i = 0; i < 3; ++i) {
        const bool selected = i == m_selectedAction;
        m_actionBoxes[i].setFillColor(selected ? sf::Color(74, 113, 196, 240) : sf::Color(33, 51, 102, 230));
        m_actionBoxes[i].setOutlineColor(selected ? sf::Color(255, 222, 100) : sf::Color(129, 158, 226));
        m_actionTexts[i].setFillColor(selected ? sf::Color::White : sf::Color(212, 224, 255));
    }
}

void ModeBriefingState::updateHeroSprites(float dt) {
    m_animationTime += dt;
    const int frame = static_cast<int>(m_animationTime * 9.0f);

    const std::string p1 = Game::getInstance().getProgress().getSelectedCharacter();
    const std::string p2 = p1 == "Mario" ? "Luigi" : "Mario";
    const std::array<std::string, 2> heroes = {p1, p2};
    for (int i = 0; i < 2; ++i) {
        // Keep the walking preview completely inside its card.  applyPlayerFrame
        // treats the box as a bottom-aligned rectangle, so its top must account
        // for the preview height.
        const float centerX = 217.5f + i * 365.0f +
            std::sin(m_animationTime * 2.4f + static_cast<float>(i)) * 10.0f;
        SpriteRegistry::applyPlayerFrame(m_heroSprites[i], heroId(heroes[i]), PowerUpState::Big,
                                         SpriteRegistry::PlayerAnim::Walk, frame,
                                         sf::FloatRect(centerX, 212.0f, 0.0f, 92.0f),
                                         i == 1);
    }
}

void ModeBriefingState::handleEvent(const sf::Event& event) {
    Game& game = Game::getInstance();
    if (event.type == sf::Event::MouseMoved ||
        (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left)) {
        const sf::Vector2i pixel = event.type == sf::Event::MouseMoved
            ? sf::Vector2i(event.mouseMove.x, event.mouseMove.y)
            : sf::Vector2i(event.mouseButton.x, event.mouseButton.y);
        const sf::Vector2f mouse = game.mapPixelToUiCoords(pixel);
        for (int i = 0; i < 2; ++i) {
            if (m_characterCards[i].getGlobalBounds().contains(mouse)) {
                if (event.type == sf::Event::MouseButtonPressed) toggleHeroes();
                return;
            }
        }
        for (int i = 0; i < 3; ++i) {
            if (m_actionBoxes[i].getGlobalBounds().contains(mouse)) {
                m_selectedAction = i;
                updateVisuals();
                if (event.type == sf::Event::MouseButtonPressed) activateSelectedAction();
                return;
            }
        }
        return;
    }

    if (event.type != sf::Event::KeyPressed) return;
    switch (event.key.code) {
    case sf::Keyboard::Escape:
        m_selectedAction = 2;
        activateSelectedAction();
        break;
    case sf::Keyboard::Left:
    case sf::Keyboard::A:
    case sf::Keyboard::Up:
    case sf::Keyboard::W:
        m_selectedAction = (m_selectedAction - 1 + 3) % 3;
        updateVisuals();
        break;
    case sf::Keyboard::Right:
    case sf::Keyboard::D:
    case sf::Keyboard::Down:
    case sf::Keyboard::S:
        m_selectedAction = (m_selectedAction + 1) % 3;
        updateVisuals();
        break;
    case sf::Keyboard::Return:
    case sf::Keyboard::Space:
        activateSelectedAction();
        break;
    default:
        break;
    }
}

void ModeBriefingState::update(float dt) {
    updateHeroSprites(dt);
}

void ModeBriefingState::render(sf::RenderWindow& window) {
    window.setView(Game::getInstance().getUiView());
    window.draw(m_background);
    window.draw(m_title);
    window.draw(m_subtitle);
    for (int i = 0; i < 2; ++i) {
        window.draw(m_characterCards[i]);
        window.draw(m_roleTexts[i]);
        window.draw(m_heroSprites[i]);
        window.draw(m_nameTexts[i]);
    }
    window.draw(m_controlPanel);
    for (int i = 0; i < 2; ++i) window.draw(m_controlTexts[i]);
    for (int i = 0; i < 3; ++i) {
        window.draw(m_actionBoxes[i]);
        window.draw(m_actionTexts[i]);
    }
    window.draw(m_help);
}
