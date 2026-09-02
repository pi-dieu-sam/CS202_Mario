#include "States/CharacterSelectState.hpp"

#include "Core/AssetManager.hpp"
#include "Core/Game.hpp"
#include "Entities/Player.hpp"
#include "Graphics/SpriteRegistry.hpp"
#include "Physics/PhysicsConstants.hpp"
#include "States/Navigator.hpp"

#include <array>
#include <cmath>

namespace {
void centerText(sf::Text& text, float x, float y) {
    const sf::FloatRect bounds = text.getLocalBounds();
    text.setOrigin(bounds.left + bounds.width * 0.5f, bounds.top + bounds.height * 0.5f);
    text.setPosition(x, y);
}
} // namespace

void CharacterSelectState::onEnter() {
    sf::Font& font = AssetManager::getInstance().getFont("assets/fonts/mario_font.ttf");
    m_showcaseTime = 0.0f;
    m_selected = Game::getInstance().getProgress().getSelectedCharacter() == "Luigi" ? 1 : 0;

    m_background.setSize({static_cast<float>(WINDOW_WIDTH), static_cast<float>(WINDOW_HEIGHT)});
    m_background.setFillColor(sf::Color(28, 39, 83));

    m_title.setFont(font);
    m_title.setCharacterSize(32);
    m_title.setString("SELECT YOUR HERO");
    m_title.setFillColor(sf::Color(255, 224, 108));
    centerText(m_title, WINDOW_WIDTH * 0.5f, 42.0f);

    m_subtitle.setFont(font);
    m_subtitle.setCharacterSize(13);
    m_subtitle.setString("WATCH THEIR MOVES, THEN START YOUR ADVENTURE");
    m_subtitle.setFillColor(sf::Color(218, 229, 255));
    centerText(m_subtitle, WINDOW_WIDTH * 0.5f, 76.0f);

    const std::array<std::string, 2> names = {"MARIO", "LUIGI"};
    const std::array<std::string, 2> stats = {
        "SPEED  ***\nJUMP   ***\nGRIP   ****",
        "SPEED  **\nJUMP   ****\nGRIP   **"};
    const std::array<sf::Color, 2> colors = {
        sf::Color(173, 52, 53, 235), sf::Color(40, 145, 70, 235)};
    for (int i = 0; i < 2; ++i) {
        const float x = 78.0f + i * 372.0f;
        m_characterCards[i].setSize({272.0f, 350.0f});
        m_characterCards[i].setPosition(x, 102.0f);
        m_characterCards[i].setFillColor(colors[i]);
        m_characterCards[i].setOutlineThickness(3.0f);

        m_stageFloors[i].setSize({226.0f, 7.0f});
        m_stageFloors[i].setPosition(x + 23.0f, 290.0f);
        m_stageFloors[i].setFillColor(sf::Color(255, 232, 154, 210));

        m_charNames[i].setFont(font);
        m_charNames[i].setCharacterSize(25);
        m_charNames[i].setString(names[static_cast<std::size_t>(i)]);
        m_charNames[i].setFillColor(sf::Color::White);
        centerText(m_charNames[i], x + 136.0f, 128.0f);

        m_charStats[i].setFont(font);
        m_charStats[i].setCharacterSize(15);
        m_charStats[i].setString(stats[static_cast<std::size_t>(i)]);
        m_charStats[i].setLineSpacing(1.55f);
        m_charStats[i].setFillColor(sf::Color(244, 246, 255));
        m_charStats[i].setPosition(x + 33.0f, 325.0f);
    }

    m_confirmButton.setSize({350.0f, 42.0f});
    m_confirmButton.setPosition(225.0f, 478.0f);
    m_confirmButton.setOutlineThickness(3.0f);

    m_actionText.setFont(font);
    m_actionText.setCharacterSize(18);
    centerText(m_actionText, WINDOW_WIDTH * 0.5f, 499.0f);

    m_helpText.setFont(font);
    m_helpText.setCharacterSize(12);
    m_helpText.setString("LEFT/RIGHT: CHOOSE HERO   ENTER: START   ESC: BACK");
    m_helpText.setFillColor(sf::Color(199, 214, 255));
    centerText(m_helpText, WINDOW_WIDTH * 0.5f, 560.0f);

    updateVisuals();
    updateShowcase(0.0f);
}

void CharacterSelectState::onExit() {}

void CharacterSelectState::confirmSelection() {
    Game& game = Game::getInstance();
    game.getProgress().setSelectedCharacter(m_selected == 0 ? "Mario" : "Luigi");
    Navigator::apply(ScreenFlow::onConfirm(ScreenFlow::Screen::CharacterSelect,
                                            game.getProgress().getGameMode()),
                     game.getStateManager(), game.getProgress().getGameMode());
}

void CharacterSelectState::updateVisuals() {
    for (int i = 0; i < 2; ++i) {
        const bool selected = i == m_selected;
        m_characterCards[i].setOutlineColor(selected ? sf::Color(255, 231, 105)
                                                     : sf::Color(42, 52, 101));
        m_characterCards[i].setOutlineThickness(selected ? 5.0f : 2.0f);
        m_charNames[i].setFillColor(selected ? sf::Color(255, 244, 183) : sf::Color::White);
    }
    m_confirmButton.setFillColor(sf::Color(73, 111, 198, 244));
    m_confirmButton.setOutlineColor(sf::Color(255, 224, 108));
    m_actionText.setString(m_selected == 0 ? "START WITH MARIO" : "START WITH LUIGI");
    m_actionText.setFillColor(sf::Color::White);
    centerText(m_actionText, WINDOW_WIDTH * 0.5f, 499.0f);
}

void CharacterSelectState::updateShowcase(float dt) {
    m_showcaseTime = std::fmod(m_showcaseTime + dt, 5.2f);
    for (int i = 0; i < 2; ++i) {
        const float phase = std::fmod(m_showcaseTime + i * 0.55f, 5.2f);
        SpriteRegistry::PlayerAnim anim = SpriteRegistry::PlayerAnim::Idle;
        float horizontal = 0.0f;
        float lift = 0.0f;
        bool fire = false;

        if (phase >= 1.0f && phase < 2.8f) {
            anim = SpriteRegistry::PlayerAnim::Walk;
            horizontal = std::sin((phase - 1.0f) * 2.1f) * 52.0f;
        } else if (phase >= 2.8f && phase < 3.75f) {
            anim = SpriteRegistry::PlayerAnim::Jump;
            const float jump = (phase - 2.8f) / 0.95f;
            lift = std::sin(jump * 3.14159265f) * 88.0f;
        } else if (phase >= 3.75f && phase < 4.45f) {
            anim = SpriteRegistry::PlayerAnim::Fire;
            fire = true;
        }

        const CharacterId hero = i == 0 ? CharacterId::Mario : CharacterId::Luigi;
        const float centerX = 214.0f + i * 372.0f + horizontal;
        SpriteRegistry::applyPlayerFrame(m_charSprites[i], hero,
                                         fire ? PowerUpState::Fire : PowerUpState::Big,
                                         anim, static_cast<int>(phase * 10.0f),
                                         sf::FloatRect(centerX, 290.0f - lift - 112.0f, 0.0f, 112.0f));

        if (fire) {
            const float progress = (phase - 3.75f) / 0.7f;
            const float direction = i == 0 ? 1.0f : -1.0f;
            const float fireX = centerX + direction * (34.0f + progress * 115.0f);
            const float fireY = 252.0f - std::sin(progress * 3.14159265f) * 16.0f;
            SpriteRegistry::applySheetFrame(m_fireballSprites[i], SpriteRegistry::fireballPath(),
                                            static_cast<int>(phase * 13.0f), 16, 0,
                                            sf::FloatRect(fireX, fireY, 22.0f, 22.0f), i == 1);
            m_fireballSprites[i].setColor(sf::Color::White);
        } else {
            m_fireballSprites[i].setColor(sf::Color(255, 255, 255, 0));
        }
    }
}

void CharacterSelectState::handleEvent(const sf::Event& event) {
    Game& game = Game::getInstance();
    if (event.type == sf::Event::MouseMoved ||
        (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left)) {
        const sf::Vector2i pixel = event.type == sf::Event::MouseMoved
            ? sf::Vector2i(event.mouseMove.x, event.mouseMove.y)
            : sf::Vector2i(event.mouseButton.x, event.mouseButton.y);
        const sf::Vector2f mouse = game.mapPixelToUiCoords(pixel);
        for (int i = 0; i < 2; ++i) {
            if (m_characterCards[i].getGlobalBounds().contains(mouse)) {
                m_selected = i;
                updateVisuals();
                return;
            }
        }
        if (event.type == sf::Event::MouseButtonPressed && m_confirmButton.getGlobalBounds().contains(mouse)) {
            confirmSelection();
        }
        return;
    }

    if (event.type != sf::Event::KeyPressed) return;
    switch (event.key.code) {
    case sf::Keyboard::Left:
    case sf::Keyboard::A:
        m_selected = 0;
        updateVisuals();
        break;
    case sf::Keyboard::Right:
    case sf::Keyboard::D:
        m_selected = 1;
        updateVisuals();
        break;
    case sf::Keyboard::Return:
    case sf::Keyboard::Space:
        confirmSelection();
        break;
    case sf::Keyboard::Escape:
        Navigator::apply(ScreenFlow::onBack(ScreenFlow::Screen::CharacterSelect),
                         game.getStateManager(), game.getProgress().getGameMode());
        break;
    default:
        break;
    }
}

void CharacterSelectState::update(float dt) {
    updateShowcase(dt);
}

void CharacterSelectState::render(sf::RenderWindow& window) {
    window.setView(Game::getInstance().getUiView());
    window.draw(m_background);
    window.draw(m_title);
    window.draw(m_subtitle);
    for (int i = 0; i < 2; ++i) {
        window.draw(m_characterCards[i]);
        window.draw(m_stageFloors[i]);
        window.draw(m_charNames[i]);
        window.draw(m_charSprites[i]);
        window.draw(m_fireballSprites[i]);
        window.draw(m_charStats[i]);
    }
    window.draw(m_confirmButton);
    window.draw(m_actionText);
    window.draw(m_helpText);
}
