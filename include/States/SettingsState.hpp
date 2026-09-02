#pragma once

#include "Core/GameSettings.hpp"
#include "GameState.hpp"

#include <SFML/Graphics.hpp>
#include <array>

/// SettingsState is a shared, opaque settings surface. It can be pushed from
/// the title screen or the transparent pause overlay; Back simply resumes the
/// screen that opened it.
class SettingsState : public GameState {
public:
    SettingsState() = default;

    void onEnter() override;
    void onExit() override;
    void handleEvent(const sf::Event& event) override;
    void update(float dt) override;
    void render(sf::RenderWindow& window) override;

private:
    enum class Tab { Video, Audio };

    static constexpr int MAX_ROWS = 9;

    int rowCount() const;
    bool rowIsAdjustable(int row) const;
    void setTab(Tab tab);
    void cycleCurrentValue(int direction);
    void activateCurrentRow();
    void applyChanges();
    void cancelAndBack();
    void syncAudioPreview();
    void updateVisuals();
    void setStatus(const std::string& status, sf::Color color = sf::Color::White);

    Tab m_tab = Tab::Video;
    int m_selectedRow = 0;
    GameSettings m_openingSettings;
    GameSettings m_pendingSettings;

    sf::RectangleShape m_background;
    sf::RectangleShape m_panel;
    std::array<sf::RectangleShape, 2> m_tabBoxes;
    std::array<sf::Text, 2> m_tabTexts;
    std::array<sf::RectangleShape, MAX_ROWS> m_rows;
    std::array<sf::Text, MAX_ROWS> m_rowLabels;
    std::array<sf::Text, MAX_ROWS> m_rowValues;
    sf::Text m_title;
    sf::Text m_status;
    sf::Text m_help;
};
