#pragma once

#include "GameState.hpp"
#include "Core/SaveManager.hpp"
#include <SFML/Graphics.hpp>
#include <array>
#include <optional>
#include <string>

/// One reusable five-slot screen. Load mode is reached from the main menu;
/// Save mode is pushed over a paused, single-player game.
enum class SaveSlotMode {
    Load,
    Save,
};

class SaveSlotState : public GameState {
public:
    explicit SaveSlotState(
        SaveSlotMode mode,
        std::optional<SaveData::GameSnapshot> snapshot = std::nullopt);

    void onEnter() override;
    void onExit() override;
    void handleEvent(const sf::Event& event) override;
    void update(float dt) override;
    void render(sf::RenderWindow& window) override;

private:
    void refreshSlots();
    void updateVisuals();
    void activateSelectedSlot();
    void saveToSlot(int slot);
    void loadFromSlot(int slot);
    void cancelOrBack();
    void setStatus(const std::string& message);
    std::string slotLabel(const SaveSlotInfo& info) const;

    SaveSlotMode m_mode;
    std::optional<SaveData::GameSnapshot> m_snapshot;
    std::array<SaveSlotInfo, SaveManager::SLOT_COUNT> m_slots{};
    std::array<sf::RectangleShape, SaveManager::SLOT_COUNT> m_slotPanels;
    std::array<sf::Text, SaveManager::SLOT_COUNT> m_slotTexts;
    sf::RectangleShape m_background;
    sf::RectangleShape m_headerBar;
    sf::Text m_title;
    sf::Text m_subtitle;
    sf::Text m_help;
    sf::Text m_status;
    int m_selected = 0;
    bool m_confirmOverwrite = false;
    int m_pendingSlot = 0;
    bool m_saveCompleted = false;
};
