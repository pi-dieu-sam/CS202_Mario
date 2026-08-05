#include "States/PauseState.hpp"
#include "States/MenuState.hpp"
#include "Core/Game.hpp"
#include "Core/AssetManager.hpp"
#include "Core/SaveManager.hpp"
#include "Core/SoundManager.hpp"
#include "States/StateManager.hpp"
#include "Physics/PhysicsConstants.hpp"

PauseState::PauseState() {}

void PauseState::onEnter() {
    sf::Font& font = AssetManager::getInstance().getFont("assets/fonts/mario_font.ttf");

    // Semi-transparent overlay
    m_overlay.setSize(sf::Vector2f(WINDOW_WIDTH, WINDOW_HEIGHT));
    m_overlay.setFillColor(sf::Color(0, 0, 0, 180));

    m_title.setFont(font);
    m_title.setString("PAUSED");
    m_title.setCharacterSize(40);
    m_title.setFillColor(sf::Color::White);
    auto tb = m_title.getLocalBounds();
    m_title.setOrigin(tb.left + tb.width / 2.0f, tb.top + tb.height / 2.0f);
    m_title.setPosition(WINDOW_WIDTH / 2.0f, 75.0f);

    std::string labels[] = {"RESUME", "QUIT TO MENU"};
    for (int i = 0; i < 2; i++) {
        m_options[i].setFont(font);
        m_options[i].setString(labels[i]);
        m_options[i].setCharacterSize(22);
        auto ob = m_options[i].getLocalBounds();
        m_options[i].setOrigin(ob.left + ob.width / 2.0f, ob.top + ob.height / 2.0f);
        m_options[i].setPosition(WINDOW_WIDTH / 2.0f, 150.0f + i * 55.0f);
        m_options[i].setFillColor(i == 0 ? sf::Color::Yellow : sf::Color::White);
    }

    m_selected = 0;

    // ── Audio Control Panel Layout ──────────────────────────────────────────
    float panelY = 320.0f;
    float panelW = 730.0f;
    float panelH = 195.0f;
    float panelX = (WINDOW_WIDTH - panelW) / 2.0f;

    m_audioPanel.setSize(sf::Vector2f(panelW, panelH));
    m_audioPanel.setPosition(panelX, panelY);
    m_audioPanel.setFillColor(sf::Color(20, 20, 40, 235));
    m_audioPanel.setOutlineThickness(2.0f);
    m_audioPanel.setOutlineColor(sf::Color(100, 100, 180));

    m_panelTitle.setFont(font);
    m_panelTitle.setString("AUDIO SETTINGS & MUSIC SELECTOR");
    m_panelTitle.setCharacterSize(16);
    m_panelTitle.setFillColor(sf::Color(255, 220, 100));
    auto ptb = m_panelTitle.getLocalBounds();
    m_panelTitle.setOrigin(ptb.left + ptb.width / 2.0f, ptb.top);
    m_panelTitle.setPosition(WINDOW_WIDTH / 2.0f, panelY + 15.0f);

    float btnY = panelY + 55.0f;
    float btnH = 42.0f;

    // 1. Mute Button Box
    m_muteBtn.setSize(sf::Vector2f(165.0f, btnH));
    m_muteBtn.setPosition(panelX + 25.0f, btnY);
    m_muteBtn.setOutlineThickness(2.0f);

    m_muteText.setFont(font);
    m_muteText.setCharacterSize(12);

    // 2. Volume Control Buttons & Text
    m_volMinusBtn.setSize(sf::Vector2f(38.0f, btnH));
    m_volMinusBtn.setPosition(panelX + 205.0f, btnY);
    m_volMinusBtn.setFillColor(sf::Color(60, 60, 95));
    m_volMinusBtn.setOutlineThickness(2.0f);
    m_volMinusBtn.setOutlineColor(sf::Color::White);

    m_volMinusText.setFont(font);
    m_volMinusText.setString("-");
    m_volMinusText.setCharacterSize(18);
    m_volMinusText.setFillColor(sf::Color::White);
    auto b1 = m_volMinusText.getLocalBounds();
    m_volMinusText.setOrigin(b1.left + b1.width / 2.0f, b1.top + b1.height / 2.0f);
    m_volMinusText.setPosition(panelX + 205.0f + 19.0f, btnY + btnH / 2.0f);

    m_volText.setFont(font);
    m_volText.setCharacterSize(12);
    m_volText.setFillColor(sf::Color::White);

    m_volPlusBtn.setSize(sf::Vector2f(38.0f, btnH));
    m_volPlusBtn.setPosition(panelX + 360.0f, btnY);
    m_volPlusBtn.setFillColor(sf::Color(60, 60, 95));
    m_volPlusBtn.setOutlineThickness(2.0f);
    m_volPlusBtn.setOutlineColor(sf::Color::White);

    m_volPlusText.setFont(font);
    m_volPlusText.setString("+");
    m_volPlusText.setCharacterSize(18);
    m_volPlusText.setFillColor(sf::Color::White);
    auto b2 = m_volPlusText.getLocalBounds();
    m_volPlusText.setOrigin(b2.left + b2.width / 2.0f, b2.top + b2.height / 2.0f);
    m_volPlusText.setPosition(panelX + 360.0f + 19.0f, btnY + btnH / 2.0f);

    // 3. Music Selector Button Box
    m_trackBtn.setSize(sf::Vector2f(285.0f, btnH));
    m_trackBtn.setPosition(panelX + 415.0f, btnY);
    m_trackBtn.setFillColor(sf::Color(25, 75, 135));
    m_trackBtn.setOutlineThickness(2.0f);
    m_trackBtn.setOutlineColor(sf::Color::Cyan);

    m_trackText.setFont(font);
    m_trackText.setCharacterSize(11);
    m_trackText.setFillColor(sf::Color::Cyan);

    // 4. Help Text
    m_helpText.setFont(font);
    m_helpText.setString("[M] Mute  |  [-/+] Vol  |  [N] Music  |  [ESC] Resume");
    m_helpText.setCharacterSize(10);
    m_helpText.setFillColor(sf::Color(180, 180, 210));
    auto hb = m_helpText.getLocalBounds();
    m_helpText.setOrigin(hb.left + hb.width / 2.0f, hb.top + hb.height / 2.0f);
    m_helpText.setPosition(WINDOW_WIDTH / 2.0f, panelY + 162.0f);

    updateAudioUI();
}

void PauseState::updateAudioUI() {
    SoundManager& snd = SoundManager::getInstance();

    if (snd.isMuted()) {
        m_muteBtn.setFillColor(sf::Color(160, 40, 40));
        m_muteBtn.setOutlineColor(sf::Color::Red);
        m_muteText.setString("[ MUTE: ON ]");
        m_muteText.setFillColor(sf::Color::White);
    } else {
        m_muteBtn.setFillColor(sf::Color(40, 140, 60));
        m_muteBtn.setOutlineColor(sf::Color::Green);
        m_muteText.setString("[ MUTE: OFF ]");
        m_muteText.setFillColor(sf::Color::White);
    }

    auto mb = m_muteText.getLocalBounds();
    m_muteText.setOrigin(mb.left + mb.width / 2.0f, mb.top + mb.height / 2.0f);
    m_muteText.setPosition(m_muteBtn.getPosition().x + m_muteBtn.getSize().x / 2.0f,
                           m_muteBtn.getPosition().y + m_muteBtn.getSize().y / 2.0f);

    int volPct = static_cast<int>(snd.getMasterVolume());
    m_volText.setString("VOL: " + std::to_string(volPct) + "%");
    auto vb = m_volText.getLocalBounds();
    m_volText.setOrigin(vb.left + vb.width / 2.0f, vb.top + vb.height / 2.0f);
    float volMidX = (m_volMinusBtn.getPosition().x + m_volMinusBtn.getSize().x + m_volPlusBtn.getPosition().x) / 2.0f;
    m_volText.setPosition(volMidX, m_volMinusBtn.getPosition().y + m_volMinusBtn.getSize().y / 2.0f);

    m_trackText.setString("< " + snd.getCurrentTrackName() + " >");
    auto tb = m_trackText.getLocalBounds();
    m_trackText.setOrigin(tb.left + tb.width / 2.0f, tb.top + tb.height / 2.0f);
    m_trackText.setPosition(m_trackBtn.getPosition().x + m_trackBtn.getSize().x / 2.0f,
                           m_trackBtn.getPosition().y + m_trackBtn.getSize().y / 2.0f);
}

void PauseState::onExit() {}

void PauseState::handleEvent(const sf::Event& event) {
    sf::RenderWindow& window = Game::getInstance().getWindow();
    SoundManager& snd = SoundManager::getInstance();

    // Mouse movement
    if (event.type == sf::Event::MouseMoved) {
        sf::Vector2f mousePos = window.mapPixelToCoords({event.mouseMove.x, event.mouseMove.y});
        for (int i = 0; i < 2; i++) {
            if (m_options[i].getGlobalBounds().contains(mousePos)) {
                if (m_selected != i) {
                    m_selected = i;
                    for (int j = 0; j < 2; j++)
                        m_options[j].setFillColor(j == m_selected ? sf::Color::Yellow : sf::Color::White);
                }
                break;
            }
        }
        return;
    }

    // Mouse clicks
    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2f mousePos = window.mapPixelToCoords({event.mouseButton.x, event.mouseButton.y});

        // Click Resume or Quit
        for (int i = 0; i < 2; i++) {
            if (m_options[i].getGlobalBounds().contains(mousePos)) {
                m_selected = i;
                if (m_selected == 0) {
                    Game::getInstance().getStateManager().popState();
                } else {
                    SaveManager::saveGame();
                    Game::getInstance().getStateManager().clearStates();
                    Game::getInstance().getStateManager().pushState(
                        std::make_unique<MenuState>());
                }
                return;
            }
        }

        // Click Mute button
        if (m_muteBtn.getGlobalBounds().contains(mousePos)) {
            snd.toggleMute();
            snd.playSound(SoundID::Coin);
            updateAudioUI();
            return;
        }

        // Click Vol -
        if (m_volMinusBtn.getGlobalBounds().contains(mousePos)) {
            snd.setMasterVolume(snd.getMasterVolume() - 10.0f);
            snd.playSound(SoundID::BlockBump);
            updateAudioUI();
            return;
        }

        // Click Vol +
        if (m_volPlusBtn.getGlobalBounds().contains(mousePos)) {
            snd.setMasterVolume(snd.getMasterVolume() + 10.0f);
            snd.playSound(SoundID::Coin);
            updateAudioUI();
            return;
        }

        // Click Music Selector button
        if (m_trackBtn.getGlobalBounds().contains(mousePos)) {
            snd.nextTrack();
            updateAudioUI();
            return;
        }
    }

    // Keyboard shortcuts
    if (event.type == sf::Event::KeyPressed) {
        switch (event.key.code) {
            case sf::Keyboard::Escape:
                Game::getInstance().getStateManager().popState();
                break;

            case sf::Keyboard::Up:
            case sf::Keyboard::W:
                m_selected = (m_selected - 1 + 2) % 2;
                for (int i = 0; i < 2; i++)
                    m_options[i].setFillColor(i == m_selected ? sf::Color::Yellow : sf::Color::White);
                break;

            case sf::Keyboard::Down:
            case sf::Keyboard::S:
                m_selected = (m_selected + 1) % 2;
                for (int i = 0; i < 2; i++)
                    m_options[i].setFillColor(i == m_selected ? sf::Color::Yellow : sf::Color::White);
                break;

            case sf::Keyboard::M: // Mute shortcut
                snd.toggleMute();
                snd.playSound(SoundID::Coin);
                updateAudioUI();
                break;

            case sf::Keyboard::Dash: // Vol - shortcut
            case sf::Keyboard::LBracket:
                snd.setMasterVolume(snd.getMasterVolume() - 10.0f);
                snd.playSound(SoundID::BlockBump);
                updateAudioUI();
                break;

            case sf::Keyboard::Equal: // Vol + shortcut
            case sf::Keyboard::RBracket:
                snd.setMasterVolume(snd.getMasterVolume() + 10.0f);
                snd.playSound(SoundID::Coin);
                updateAudioUI();
                break;

            case sf::Keyboard::N: // Next track shortcut
                snd.nextTrack();
                updateAudioUI();
                break;

            case sf::Keyboard::Return:
            case sf::Keyboard::Space:
                if (m_selected == 0) {
                    Game::getInstance().getStateManager().popState();
                } else {
                    SaveManager::saveGame();
                    Game::getInstance().getStateManager().clearStates();
                    Game::getInstance().getStateManager().pushState(
                        std::make_unique<MenuState>());
                }
                break;

            default:
                break;
        }
    }
}

void PauseState::update(float dt) {}

void PauseState::render(sf::RenderWindow& window) {
    window.setView(window.getDefaultView());
    window.draw(m_overlay);
    window.draw(m_title);
    for (int i = 0; i < 2; i++) {
        window.draw(m_options[i]);
    }

    // Audio Control Panel
    window.draw(m_audioPanel);
    window.draw(m_panelTitle);
    window.draw(m_muteBtn);
    window.draw(m_muteText);
    window.draw(m_volMinusBtn);
    window.draw(m_volMinusText);
    window.draw(m_volText);
    window.draw(m_volPlusBtn);
    window.draw(m_volPlusText);
    window.draw(m_trackBtn);
    window.draw(m_trackText);
    window.draw(m_helpText);
}
