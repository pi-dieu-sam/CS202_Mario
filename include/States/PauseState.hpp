#pragma once

#include "Core/GameSnapshot.hpp"
#include "GameState.hpp"

#include <SFML/Graphics.hpp>
#include <array>
#include <optional>

/// PauseState — transparent gameplay overlay with Resume, single-player Save,
/// shared Settings, and Quit to Menu actions.
class PauseState : public GameState {
public:
    explicit PauseState(std::optional<SaveData::GameSnapshot> snapshot = std::nullopt);

    void onEnter() override;
    void onExit() override;
    void handleEvent(const sf::Event& event) override;
    void update(float dt) override;
    void render(sf::RenderWindow& window) override;

    bool isTransparent() const override { return true; }

private:
    enum class Action { Resume, Save, Settings, QuitToMenu };

    void activateSelectedAction();
    void updateVisuals();

    sf::RectangleShape m_overlay;
    sf::RectangleShape m_panel;
    std::array<sf::RectangleShape, 4> m_optionBoxes;
    std::array<sf::Text, 4> m_options;
    sf::Text m_title;
    sf::Text m_subtitle;
    sf::Text m_help;
    std::array<Action, 4> m_actions{};
    int m_optionCount = 0;
    int m_selected = 0;
    std::optional<SaveData::GameSnapshot> m_snapshot;
    float m_pulseTime = 0.0f;
};
