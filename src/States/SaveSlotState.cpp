#include "States/SaveSlotState.hpp"

#include "Core/AssetManager.hpp"
#include "Core/Game.hpp"
#include "Core/SoundManager.hpp"
#include "Physics/PhysicsConstants.hpp"
#include "States/PlayingState.hpp"
#include "States/StateManager.hpp"

#include <algorithm>
#include <memory>

namespace {
constexpr float SLOT_X = 110.0f;
constexpr float SLOT_WIDTH = 580.0f;
constexpr float SLOT_HEIGHT = 72.0f;
constexpr float SLOT_TOP = 92.0f;
constexpr float SLOT_GAP = 82.0f;

std::string limitText(const std::string& value, std::size_t maxCharacters) {
    if (value.size() <= maxCharacters) return value;
    if (maxCharacters <= 3) return value.substr(0, maxCharacters);
    return value.substr(0, maxCharacters - 3) + "...";
}
}

SaveSlotState::SaveSlotState(
    SaveSlotMode mode, std::optional<SaveData::GameSnapshot> snapshot)
    : m_mode(mode), m_snapshot(std::move(snapshot)) {}

void SaveSlotState::onEnter() {
    sf::Font& font = AssetManager::getInstance().getFont("assets/fonts/mario_font.ttf");

    m_background.setSize(
        sf::Vector2f(static_cast<float>(WINDOW_WIDTH), static_cast<float>(WINDOW_HEIGHT)));
    m_background.setFillColor(sf::Color(18, 22, 52));

    m_title.setFont(font);
    m_title.setCharacterSize(30);
    m_title.setFillColor(sf::Color::Yellow);
    m_title.setString(m_mode == SaveSlotMode::Save ? "SAVE GAME" : "LOAD GAME");
    auto titleBounds = m_title.getLocalBounds();
    m_title.setOrigin(titleBounds.left + titleBounds.width * 0.5f,
                      titleBounds.top + titleBounds.height * 0.5f);
    m_title.setPosition(WINDOW_WIDTH * 0.5f, 44.0f);

    for (int i = 0; i < SaveManager::SLOT_COUNT; ++i) {
        const float y = SLOT_TOP + SLOT_GAP * static_cast<float>(i);
        m_slotPanels[static_cast<std::size_t>(i)].setSize({SLOT_WIDTH, SLOT_HEIGHT});
        m_slotPanels[static_cast<std::size_t>(i)].setPosition(SLOT_X, y);
        m_slotPanels[static_cast<std::size_t>(i)].setOutlineThickness(2.0f);

        sf::Text& text = m_slotTexts[static_cast<std::size_t>(i)];
        text.setFont(font);
        text.setCharacterSize(14);
        text.setLineSpacing(0.9f);
        text.setPosition(SLOT_X + 18.0f, y + 7.0f);
    }

    m_status.setFont(font);
    m_status.setCharacterSize(14);
    m_status.setFillColor(sf::Color::White);
    setStatus("");

    m_help.setFont(font);
    m_help.setCharacterSize(12);
    m_help.setFillColor(sf::Color(190, 200, 255));
    m_help.setString("UP/DOWN: SELECT   ENTER: CONFIRM   ESC: BACK");
    auto helpBounds = m_help.getLocalBounds();
    m_help.setOrigin(helpBounds.left + helpBounds.width * 0.5f, 0.0f);
    m_help.setPosition(WINDOW_WIDTH * 0.5f, 570.0f);

    refreshSlots();
}

void SaveSlotState::onExit() {}

void SaveSlotState::refreshSlots() {
    m_slots = SaveManager::listSlots();
    m_selected = std::clamp(m_selected, 0, SaveManager::SLOT_COUNT - 1);
    updateVisuals();
}

std::string SaveSlotState::slotLabel(const SaveSlotInfo& info) const {
    const std::string prefix = "SLOT " + std::to_string(info.slot);
    switch (info.status) {
    case SaveSlotStatus::Empty:
        return prefix + "\nEMPTY SLOT";
    case SaveSlotStatus::Corrupt:
        return prefix + "\nCORRUPT SAVE: " + limitText(info.error, 47);
    case SaveSlotStatus::Occupied:
        return prefix + "    " + limitText(info.character, 16) +
               "\nLEVEL " + std::to_string(info.level) +
               "   SCORE " + std::to_string(info.score) +
               "   LIVES " + std::to_string(info.lives) +
               "   COINS " + std::to_string(info.coins) +
               "   TIME " + std::to_string(info.remainingSeconds);
    }
    return prefix;
}

void SaveSlotState::setStatus(const std::string& message) {
    m_status.setString(message);
    const sf::FloatRect bounds = m_status.getLocalBounds();
    m_status.setOrigin(bounds.left + bounds.width * 0.5f,
                       bounds.top + bounds.height * 0.5f);
    m_status.setPosition(WINDOW_WIDTH * 0.5f, 532.0f);
}

void SaveSlotState::updateVisuals() {
    for (int i = 0; i < SaveManager::SLOT_COUNT; ++i) {
        const SaveSlotInfo& info = m_slots[static_cast<std::size_t>(i)];
        sf::RectangleShape& panel = m_slotPanels[static_cast<std::size_t>(i)];
        sf::Text& text = m_slotTexts[static_cast<std::size_t>(i)];
        const bool selected = i == m_selected;

        if (info.status == SaveSlotStatus::Corrupt) {
            panel.setFillColor(sf::Color(90, 32, 42));
            text.setFillColor(sf::Color(255, 190, 190));
        } else if (info.status == SaveSlotStatus::Empty) {
            panel.setFillColor(sf::Color(46, 52, 82));
            text.setFillColor(sf::Color(175, 185, 220));
        } else {
            panel.setFillColor(sf::Color(32, 76, 70));
            text.setFillColor(sf::Color::White);
        }
        panel.setOutlineColor(selected ? sf::Color::Yellow : sf::Color(130, 145, 215));
        text.setString(slotLabel(info));
    }
}

void SaveSlotState::saveToSlot(int slot) {
    if (!m_snapshot) {
        setStatus("NO SINGLE-PLAYER SNAPSHOT IS AVAILABLE");
        return;
    }
    std::string error;
    if (!SaveManager::saveSlot(slot, *m_snapshot, &error)) {
        setStatus("SAVE FAILED: " + error);
        return;
    }
    m_confirmOverwrite = false;
    m_saveCompleted = true;
    setStatus("GAME SAVED TO SLOT " + std::to_string(slot) +
              "\nPRESS ENTER OR ESC");
    SoundManager::getInstance().playSound(SoundID::Coin);
    refreshSlots();
}

void SaveSlotState::loadFromSlot(int slot) {
    std::string error;
    const auto snapshot = SaveManager::loadSlot(slot, &error);
    if (!snapshot) {
        setStatus("LOAD FAILED: " + error);
        refreshSlots();
        return;
    }

    // No global PlayerProgress fields are modified until PlayingState restores
    // this already validated snapshot.
    StateManager& states = Game::getInstance().getStateManager();
    states.clearStates();
    states.pushState(std::make_unique<PlayingState>(*snapshot));
}

void SaveSlotState::activateSelectedSlot() {
    if (m_saveCompleted) {
        Game::getInstance().getStateManager().popState();
        return;
    }

    const SaveSlotInfo& slot = m_slots[static_cast<std::size_t>(m_selected)];
    if (m_mode == SaveSlotMode::Load) {
        if (slot.status != SaveSlotStatus::Occupied) {
            setStatus(slot.status == SaveSlotStatus::Empty
                ? "THIS SLOT IS EMPTY"
                : "THIS SLOT IS CORRUPT\nAND CANNOT BE LOADED");
            return;
        }
        loadFromSlot(slot.slot);
        return;
    }

    if (!m_snapshot) {
        setStatus("SAVING IS AVAILABLE ONLY IN 1 PLAYER MODE");
        return;
    }
    if (slot.status == SaveSlotStatus::Empty) {
        saveToSlot(slot.slot);
        return;
    }

    m_confirmOverwrite = true;
    m_pendingSlot = slot.slot;
    setStatus("OVERWRITE SLOT " + std::to_string(slot.slot) +
              "?\nY: YES    N / ESC: NO");
}

void SaveSlotState::cancelOrBack() {
    if (m_confirmOverwrite) {
        m_confirmOverwrite = false;
        m_pendingSlot = 0;
        setStatus("OVERWRITE CANCELLED");
        return;
    }
    Game::getInstance().getStateManager().popState();
}

void SaveSlotState::handleEvent(const sf::Event& event) {
    sf::RenderWindow& window = Game::getInstance().getWindow();

    if (event.type == sf::Event::MouseMoved && !m_confirmOverwrite && !m_saveCompleted) {
        const sf::Vector2f mouse = window.mapPixelToCoords({event.mouseMove.x, event.mouseMove.y});
        for (int i = 0; i < SaveManager::SLOT_COUNT; ++i) {
            if (m_slotPanels[static_cast<std::size_t>(i)].getGlobalBounds().contains(mouse)) {
                m_selected = i;
                updateVisuals();
                return;
            }
        }
    }

    if (event.type == sf::Event::MouseButtonPressed &&
        event.mouseButton.button == sf::Mouse::Left && !m_confirmOverwrite && !m_saveCompleted) {
        const sf::Vector2f mouse = window.mapPixelToCoords({event.mouseButton.x, event.mouseButton.y});
        for (int i = 0; i < SaveManager::SLOT_COUNT; ++i) {
            if (m_slotPanels[static_cast<std::size_t>(i)].getGlobalBounds().contains(mouse)) {
                m_selected = i;
                updateVisuals();
                activateSelectedSlot();
                return;
            }
        }
    }

    if (event.type != sf::Event::KeyPressed) return;

    if (m_confirmOverwrite) {
        if (event.key.code == sf::Keyboard::Y) {
            saveToSlot(m_pendingSlot);
        } else if (event.key.code == sf::Keyboard::N || event.key.code == sf::Keyboard::Escape) {
            cancelOrBack();
        }
        return;
    }

    switch (event.key.code) {
    case sf::Keyboard::Escape:
        cancelOrBack();
        break;
    case sf::Keyboard::Up:
    case sf::Keyboard::W:
        if (!m_saveCompleted) {
            m_selected = (m_selected - 1 + SaveManager::SLOT_COUNT) % SaveManager::SLOT_COUNT;
            updateVisuals();
        }
        break;
    case sf::Keyboard::Down:
    case sf::Keyboard::S:
        if (!m_saveCompleted) {
            m_selected = (m_selected + 1) % SaveManager::SLOT_COUNT;
            updateVisuals();
        }
        break;
    case sf::Keyboard::Return:
    case sf::Keyboard::Space:
        activateSelectedSlot();
        break;
    default:
        break;
    }
}

void SaveSlotState::update(float) {}

void SaveSlotState::render(sf::RenderWindow& window) {
    window.setView(window.getDefaultView());
    window.draw(m_background);
    window.draw(m_title);
    for (int i = 0; i < SaveManager::SLOT_COUNT; ++i) {
        window.draw(m_slotPanels[static_cast<std::size_t>(i)]);
        window.draw(m_slotTexts[static_cast<std::size_t>(i)]);
    }
    window.draw(m_status);
    window.draw(m_help);
}
