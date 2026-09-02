#include "States/PauseState.hpp"

#include "Core/AssetManager.hpp"
#include "Core/Game.hpp"
#include "Physics/PhysicsConstants.hpp"
#include "States/Navigator.hpp"
#include "States/SaveSlotState.hpp"
#include "States/SettingsState.hpp"
#include "States/StateManager.hpp"

#include <cmath>
#include <memory>

namespace {
void centerText(sf::Text& text, float x, float y) {
    const sf::FloatRect bounds = text.getLocalBounds();
    text.setOrigin(bounds.left + bounds.width * 0.5f, bounds.top + bounds.height * 0.5f);
    text.setPosition(x, y);
}
} // namespace

PauseState::PauseState(std::optional<SaveData::GameSnapshot> snapshot)
    : m_snapshot(std::move(snapshot)) {}

void PauseState::onEnter() {
    sf::Font& font = AssetManager::getInstance().getFont("assets/fonts/mario_font.ttf");
    m_pulseTime = 0.0f;

    m_overlay.setSize({static_cast<float>(WINDOW_WIDTH), static_cast<float>(WINDOW_HEIGHT)});
    m_overlay.setFillColor(sf::Color(7, 10, 31, 178));

    m_panel.setSize({420.0f, 366.0f});
    m_panel.setPosition(190.0f, 112.0f);
    m_panel.setFillColor(sf::Color(20, 31, 76, 246));
    m_panel.setOutlineThickness(4.0f);
    m_panel.setOutlineColor(sf::Color(255, 220, 101));

    m_title.setFont(font);
    m_title.setString("PAUSED");
    m_title.setCharacterSize(34);
    m_title.setFillColor(sf::Color(255, 224, 109));
    centerText(m_title, WINDOW_WIDTH * 0.5f, 155.0f);

    m_subtitle.setFont(font);
    // The pixel font does not include an em dash, and this line needs to fit
    // cleanly inside the pause panel at every supported resolution.
    m_subtitle.setCharacterSize(11);
    m_subtitle.setString("TAKE A BREATH. YOUR WORLD AWAITS.");
    m_subtitle.setFillColor(sf::Color(205, 220, 255));
    centerText(m_subtitle, WINDOW_WIDTH * 0.5f, 194.0f);

    m_optionCount = 0;
    m_actions[m_optionCount++] = Action::Resume;
    if (m_snapshot) m_actions[m_optionCount++] = Action::Save;
    m_actions[m_optionCount++] = Action::Settings;
    m_actions[m_optionCount++] = Action::QuitToMenu;

    for (int i = 0; i < m_optionCount; ++i) {
        const float y = 226.0f + i * 52.0f;
        m_optionBoxes[i].setSize({326.0f, 40.0f});
        m_optionBoxes[i].setPosition(237.0f, y);
        m_optionBoxes[i].setOutlineThickness(2.0f);
        m_options[i].setFont(font);
        m_options[i].setCharacterSize(19);
        centerText(m_options[i], WINDOW_WIDTH * 0.5f, y + 20.0f);
    }

    m_help.setFont(font);
    m_help.setCharacterSize(12);
    m_help.setString("UP/DOWN: SELECT   ENTER: CONFIRM   ESC: RESUME");
    m_help.setFillColor(sf::Color(194, 211, 255));
    centerText(m_help, WINDOW_WIDTH * 0.5f, 516.0f);

    m_selected = 0;
    updateVisuals();
}

void PauseState::onExit() {}

void PauseState::activateSelectedAction() {
    Game& game = Game::getInstance();
    switch (m_actions[m_selected]) {
    case Action::Resume:
        Navigator::apply(ScreenFlow::onPauseOption(0), game.getStateManager(), game.getProgress().getGameMode());
        break;
    case Action::Save:
        if (m_snapshot) {
            game.getStateManager().pushState(std::make_unique<SaveSlotState>(SaveSlotMode::Save, *m_snapshot));
        }
        break;
    case Action::Settings:
        game.getStateManager().pushState(std::make_unique<SettingsState>());
        break;
    case Action::QuitToMenu:
        Navigator::apply(ScreenFlow::onPauseOption(2), game.getStateManager(), game.getProgress().getGameMode());
        break;
    }
}

void PauseState::updateVisuals() {
    for (int i = 0; i < m_optionCount; ++i) {
        std::string label;
        switch (m_actions[i]) {
        case Action::Resume: label = "RESUME"; break;
        case Action::Save: label = "SAVE GAME"; break;
        case Action::Settings: label = "SETTINGS"; break;
        case Action::QuitToMenu: label = "QUIT TO MENU"; break;
        }
        m_options[i].setString(label);
        // Labels are assigned here, after onEnter has placed the boxes. Recompute
        // their origins now that their actual bounds are known.
        const sf::FloatRect box = m_optionBoxes[i].getGlobalBounds();
        centerText(m_options[i], box.left + box.width * 0.5f,
                   box.top + box.height * 0.5f);
        const bool selected = i == m_selected;
        m_optionBoxes[i].setFillColor(selected ? sf::Color(71, 112, 198, 246)
                                               : sf::Color(34, 50, 103, 230));
        m_optionBoxes[i].setOutlineColor(selected ? sf::Color(255, 224, 109)
                                                   : sf::Color(115, 149, 222));
        m_options[i].setFillColor(selected ? sf::Color::White : sf::Color(212, 225, 255));
    }
}

void PauseState::handleEvent(const sf::Event& event) {
    Game& game = Game::getInstance();
    if (event.type == sf::Event::MouseMoved ||
        (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left)) {
        const sf::Vector2i pixel = event.type == sf::Event::MouseMoved
            ? sf::Vector2i(event.mouseMove.x, event.mouseMove.y)
            : sf::Vector2i(event.mouseButton.x, event.mouseButton.y);
        const sf::Vector2f mouse = game.mapPixelToUiCoords(pixel);
        for (int i = 0; i < m_optionCount; ++i) {
            if (m_optionBoxes[i].getGlobalBounds().contains(mouse)) {
                m_selected = i;
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
        m_selected = 0;
        activateSelectedAction();
        break;
    case sf::Keyboard::Up:
    case sf::Keyboard::W:
        m_selected = (m_selected - 1 + m_optionCount) % m_optionCount;
        updateVisuals();
        break;
    case sf::Keyboard::Down:
    case sf::Keyboard::S:
        m_selected = (m_selected + 1) % m_optionCount;
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

void PauseState::update(float dt) {
    m_pulseTime += dt;
    const float pulse = 1.0f + std::sin(m_pulseTime * 2.0f) * 0.015f;
    m_title.setScale(pulse, pulse);
}

void PauseState::render(sf::RenderWindow& window) {
    window.setView(Game::getInstance().getUiView());
    window.draw(m_overlay);
    window.draw(m_panel);
    window.draw(m_title);
    window.draw(m_subtitle);
    for (int i = 0; i < m_optionCount; ++i) {
        window.draw(m_optionBoxes[i]);
        window.draw(m_options[i]);
    }
    window.draw(m_help);
}
