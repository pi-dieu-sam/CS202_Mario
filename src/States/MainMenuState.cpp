#include "States/MainMenuState.hpp"
#include "States/GameStateManager.hpp"
#include "States/CharacterSelectState.hpp"
#include "States/PlayingState.hpp"
#include "States/LevelEditorState.hpp"
#include "Core/SoundManager.hpp"
#include "Core/SaveSystem.hpp"
#include "Core/Engine.hpp"
#include <iostream>

MainMenuState::MainMenuState(GameStateManager& stateManager)
    : GameState(stateManager), m_selectedIndex(0) {
    m_options = {
        "1. START GAME",
        "2. SELECT CHARACTER",
        "3. LEVEL EDITOR",
        "4. LOAD SAVED GAME",
        "5. TOGGLE AUDIO (ON)",
        "6. QUIT GAME"
    };
}

void MainMenuState::init() {
    std::cout << "[MainMenuState] Initialized." << std::endl;
    
    if (!m_font.openFromFile("assets/fonts/arial.ttf")) {
        std::cerr << "[MainMenuState] Warning: font missing." << std::endl;
    }

    m_titleText.emplace(m_font, "SUPER MARIO BROS C++", 38);
    m_titleText->setFillColor(sf::Color::Yellow);
    m_titleText->setPosition({180.0f, 60.0f});

    m_buttons.clear();
    for (size_t i = 0; i < m_options.size(); ++i) {
        m_buttons.emplace_back(m_options[i], sf::Vector2f{250.0f, 180.0f + static_cast<float>(i) * 50.0f}, 24);
        m_buttons.back().init(m_font);
        m_buttons.back().setSelected(i == 0);
    }
}

void MainMenuState::handleInput(const sf::Event& event) {
    if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
        if (keyPressed->code == sf::Keyboard::Key::Up || keyPressed->code == sf::Keyboard::Key::W) {
            m_selectedIndex = (m_selectedIndex - 1 + static_cast<int>(m_options.size())) % m_options.size();
            updateSelection();
            SoundManager::getInstance().playSound("select");
        }
        else if (keyPressed->code == sf::Keyboard::Key::Down || keyPressed->code == sf::Keyboard::Key::S) {
            m_selectedIndex = (m_selectedIndex + 1) % m_options.size();
            updateSelection();
            SoundManager::getInstance().playSound("select");
        }
        else if (keyPressed->code == sf::Keyboard::Key::Enter || keyPressed->code == sf::Keyboard::Key::Space) {
            processSelection();
            SoundManager::getInstance().playSound("confirm");
        }
    }
}

void MainMenuState::updateSelection() {
    for (size_t i = 0; i < m_buttons.size(); ++i) {
        m_buttons[i].setSelected(static_cast<int>(i) == m_selectedIndex);
    }
}

void MainMenuState::processSelection() {
    switch (m_selectedIndex) {
        case 0: // Start Game
            m_stateManager.pushState(std::make_unique<PlayingState>(
                m_stateManager, 1, CharacterSelectState::getSelectedCharacter()));
            break;
        case 1: // Select Character
            m_stateManager.pushState(std::make_unique<CharacterSelectState>(m_stateManager));
            break;
        case 2: // Level Editor
            m_stateManager.pushState(std::make_unique<LevelEditorState>(m_stateManager));
            break;
        case 3: // Load Saved Game
            {
                SaveData save;
                if (SaveSystem::getInstance().loadGame(save)) {
                    m_stateManager.pushState(std::make_unique<PlayingState>(
                        m_stateManager, save.currentLevel, save.characterName));
                }
            }
            break;
        case 4: // Toggle Audio
            SoundManager::getInstance().toggleSoundMute();
            SoundManager::getInstance().toggleMusicMute();
            m_options[4] = SoundManager::getInstance().isSoundMuted() ? "5. TOGGLE AUDIO (OFF)" : "5. TOGGLE AUDIO (ON)";
            m_buttons[4].setLabel(m_options[4]);
            break;
        case 5: // Quit
            m_stateManager.getEngine().getWindow().close();
            break;
    }
}

void MainMenuState::update(float dt) {
    (void)dt;
}

void MainMenuState::render(sf::RenderWindow& window) {
    if (m_titleText) window.draw(*m_titleText);
    for (auto& btn : m_buttons) {
        btn.render(window);
    }
}
