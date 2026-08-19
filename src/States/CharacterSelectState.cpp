#include "States/CharacterSelectState.hpp"
#include "States/Navigator.hpp"
#include "Core/Game.hpp"
#include "Core/AssetManager.hpp"
#include "Core/SoundManager.hpp"
#include "Entities/Player.hpp"
#include "Graphics/SpriteRegistry.hpp"
#include "Physics/PhysicsConstants.hpp"
#include <iomanip>
#include <sstream>

CharacterSelectState::CharacterSelectState() {}

void CharacterSelectState::onEnter() {
    sf::Font& font = AssetManager::getInstance().getFont("assets/fonts/mario_font.ttf");

    m_background.setSize(sf::Vector2f(WINDOW_WIDTH, WINDOW_HEIGHT));
    m_background.setFillColor(sf::Color(35, 35, 65));

    m_title.setFont(font);
    m_title.setString("SELECT CHARACTER");
    m_title.setCharacterSize(30);
    m_title.setFillColor(sf::Color::White);
    auto tb = m_title.getLocalBounds();
    m_title.setOrigin(tb.left + tb.width / 2.0f, tb.top + tb.height / 2.0f);
    m_title.setPosition(WINDOW_WIDTH / 2.0f, 40.0f);

    // ── Character Cards (Mario vs Luigi) ──────────────────────────────────
    float boxW = 240.0f, boxH = 260.0f;
    float gap = 60.0f;
    float startX = (WINDOW_WIDTH - (2 * boxW + gap)) / 2.0f;

    std::string names[] = {"MARIO", "LUIGI"};
    std::string stats[] = {
        "Speed: ***\nJump:  ***\nGrip:  ****",
        "Speed: **\nJump:  ****\nGrip:  **"
    };
    sf::Color boxColors[] = {
        sf::Color(200, 50, 50, 180),
        sf::Color(50, 180, 50, 180)
    };

    for (int i = 0; i < 2; i++) {
        m_charBoxes[i].setSize(sf::Vector2f(boxW, boxH));
        m_charBoxes[i].setPosition(startX + i * (boxW + gap), 85.0f);
        m_charBoxes[i].setFillColor(boxColors[i]);
        m_charBoxes[i].setOutlineThickness(3.0f);
        m_charBoxes[i].setOutlineColor(sf::Color::Transparent);

        m_charNames[i].setFont(font);
        m_charNames[i].setString(names[i]);
        m_charNames[i].setCharacterSize(24);
        m_charNames[i].setFillColor(sf::Color::White);
        auto nb = m_charNames[i].getLocalBounds();
        m_charNames[i].setOrigin(nb.left + nb.width / 2.0f, nb.top);
        m_charNames[i].setPosition(startX + i * (boxW + gap) + boxW / 2.0f, 100.0f);

        m_charStats[i].setFont(font);
        m_charStats[i].setString(stats[i]);
        m_charStats[i].setCharacterSize(14);
        m_charStats[i].setFillColor(sf::Color(220, 220, 220));
        m_charStats[i].setPosition(startX + i * (boxW + gap) + 20.0f, 235.0f);

        float centerX = startX + i * (boxW + gap) + boxW / 2.0f;
        CharacterId charId = (i == 0) ? CharacterId::Mario : CharacterId::Luigi;
        SpriteRegistry::applyPlayerFrame(
            m_charSprites[i], charId, PowerUpState::Big,
            SpriteRegistry::PlayerAnim::Idle, 0,
            sf::FloatRect(centerX, 120.0f, 0.0f, 95.0f));
    }

    m_selected = 0;
    m_charBoxes[0].setOutlineColor(sf::Color::Yellow);

    // ── Audio Control Panel Layout ──────────────────────────────────────────
    float panelY = 370.0f;
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

    // 4. Help Text (Compact shortcuts display)
    m_helpText.setFont(font);
    m_helpText.setString("[M] Mute  |  [-/+] Vol  |  [N] Music  |  [A/D] Hero");
    m_helpText.setCharacterSize(10);
    m_helpText.setFillColor(sf::Color(180, 180, 210));
    auto hb = m_helpText.getLocalBounds();
    m_helpText.setOrigin(hb.left + hb.width / 2.0f, hb.top + hb.height / 2.0f);
    m_helpText.setPosition(WINDOW_WIDTH / 2.0f, panelY + 162.0f);

    updateAudioUI();
}

void CharacterSelectState::updateAudioUI() {
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

    // Center Mute text inside m_muteBtn box
    auto mb = m_muteText.getLocalBounds();
    m_muteText.setOrigin(mb.left + mb.width / 2.0f, mb.top + mb.height / 2.0f);
    m_muteText.setPosition(m_muteBtn.getPosition().x + m_muteBtn.getSize().x / 2.0f,
                           m_muteBtn.getPosition().y + m_muteBtn.getSize().y / 2.0f);

    // Center Volume text between minus and plus buttons
    int volPct = static_cast<int>(snd.getMasterVolume());
    m_volText.setString("VOL: " + std::to_string(volPct) + "%");
    auto vb = m_volText.getLocalBounds();
    m_volText.setOrigin(vb.left + vb.width / 2.0f, vb.top + vb.height / 2.0f);
    float volMidX = (m_volMinusBtn.getPosition().x + m_volMinusBtn.getSize().x + m_volPlusBtn.getPosition().x) / 2.0f;
    m_volText.setPosition(volMidX, m_volMinusBtn.getPosition().y + m_volMinusBtn.getSize().y / 2.0f);

    // Center Track text inside m_trackBtn box
    m_trackText.setString("< " + snd.getCurrentTrackName() + " >");
    auto tb = m_trackText.getLocalBounds();
    m_trackText.setOrigin(tb.left + tb.width / 2.0f, tb.top + tb.height / 2.0f);
    m_trackText.setPosition(m_trackBtn.getPosition().x + m_trackBtn.getSize().x / 2.0f,
                           m_trackBtn.getPosition().y + m_trackBtn.getSize().y / 2.0f);
}

void CharacterSelectState::onExit() {}

void CharacterSelectState::handleEvent(const sf::Event& event) {
    sf::RenderWindow& window = Game::getInstance().getWindow();
    SoundManager& snd = SoundManager::getInstance();

    // Mouse movement
    if (event.type == sf::Event::MouseMoved) {
        sf::Vector2f mousePos = window.mapPixelToCoords({event.mouseMove.x, event.mouseMove.y});
        for (int i = 0; i < 2; i++) {
            if (m_charBoxes[i].getGlobalBounds().contains(mousePos)) {
                if (m_selected != i) {
                    m_selected = i;
                    m_charBoxes[0].setOutlineColor(m_selected == 0 ? sf::Color::Yellow : sf::Color::Transparent);
                    m_charBoxes[1].setOutlineColor(m_selected == 1 ? sf::Color::Yellow : sf::Color::Transparent);
                }
                break;
            }
        }
        return;
    }

    // Mouse clicks
    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2f mousePos = window.mapPixelToCoords({event.mouseButton.x, event.mouseButton.y});

        // Click character boxes
        for (int i = 0; i < 2; i++) {
            if (m_charBoxes[i].getGlobalBounds().contains(mousePos)) {
                m_selected = i;
                std::string charName = (m_selected == 0) ? "Mario" : "Luigi";
                Game::getInstance().getProgress().setSelectedCharacter(charName);
                Navigator::apply(
                    ScreenFlow::onConfirm(ScreenFlow::Screen::CharacterSelect,
                                          Game::getInstance().getProgress().getGameMode()),
                    Game::getInstance().getStateManager(),
                    Game::getInstance().getProgress().getGameMode());
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
            case sf::Keyboard::Left:
            case sf::Keyboard::A:
                m_selected = 0;
                m_charBoxes[0].setOutlineColor(sf::Color::Yellow);
                m_charBoxes[1].setOutlineColor(sf::Color::Transparent);
                break;

            case sf::Keyboard::Right:
            case sf::Keyboard::D:
                m_selected = 1;
                m_charBoxes[0].setOutlineColor(sf::Color::Transparent);
                m_charBoxes[1].setOutlineColor(sf::Color::Yellow);
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
            case sf::Keyboard::Space: {
                std::string charName = (m_selected == 0) ? "Mario" : "Luigi";
                Game::getInstance().getProgress().setSelectedCharacter(charName);
                Navigator::apply(
                    ScreenFlow::onConfirm(ScreenFlow::Screen::CharacterSelect,
                                          Game::getInstance().getProgress().getGameMode()),
                    Game::getInstance().getStateManager(),
                    Game::getInstance().getProgress().getGameMode());
                break;
            }

            case sf::Keyboard::Escape:
                Navigator::apply(ScreenFlow::onBack(ScreenFlow::Screen::CharacterSelect),
                                  Game::getInstance().getStateManager(),
                                  Game::getInstance().getProgress().getGameMode());
                break;

            default:
                break;
        }
    }
}

void CharacterSelectState::update(float dt) {}

void CharacterSelectState::render(sf::RenderWindow& window) {
    window.setView(window.getDefaultView());
    window.draw(m_background);
    window.draw(m_title);

    for (int i = 0; i < 2; i++) {
        window.draw(m_charBoxes[i]);
        window.draw(m_charSprites[i]);
        window.draw(m_charNames[i]);
        window.draw(m_charStats[i]);
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
