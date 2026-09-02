#include "States/SettingsState.hpp"

#include "Core/AssetManager.hpp"
#include "Core/Game.hpp"
#include "Core/SoundManager.hpp"
#include "Physics/PhysicsConstants.hpp"
#include "States/StateManager.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <sstream>
#include <vector>

namespace {
constexpr float PANEL_X = 70.0f;
constexpr float PANEL_Y = 142.0f;
constexpr float PANEL_WIDTH = 660.0f;
constexpr float ROW_X = 90.0f;
constexpr float ROW_Y = 170.0f;
constexpr float ROW_WIDTH = 620.0f;
constexpr float ROW_HEIGHT = 31.0f;
constexpr float ROW_GAP = 35.0f;

const std::array<sf::Vector2u, 4>& windowedResolutions() {
    static const std::array<sf::Vector2u, 4> values = {
        sf::Vector2u(800, 608), sf::Vector2u(1280, 720),
        sf::Vector2u(1600, 900), sf::Vector2u(1920, 1080)};
    return values;
}

std::string resolutionLabel(unsigned int width, unsigned int height) {
    return std::to_string(width) + " x " + std::to_string(height);
}

std::string fpsLabel(unsigned int fps) {
    return fps == 0 ? "UNLIMITED" : std::to_string(fps) + " FPS";
}

std::string volumeLabel(float volume) {
    return std::to_string(static_cast<int>(std::lround(volume))) + "%";
}

void centerText(sf::Text& text, float x, float y) {
    const sf::FloatRect bounds = text.getLocalBounds();
    text.setOrigin(bounds.left + bounds.width * 0.5f, bounds.top + bounds.height * 0.5f);
    text.setPosition(x, y);
}
} // namespace

void SettingsState::onEnter() {
    sf::Font& font = AssetManager::getInstance().getFont("assets/fonts/mario_font.ttf");
    m_openingSettings = Game::getInstance().getSettings();
    m_pendingSettings = m_openingSettings;
    m_tab = Tab::Video;
    m_selectedRow = 0;

    m_background.setSize({static_cast<float>(WINDOW_WIDTH), static_cast<float>(WINDOW_HEIGHT)});
    m_background.setFillColor(sf::Color(17, 22, 50));

    m_panel.setSize({PANEL_WIDTH, 358.0f});
    m_panel.setPosition(PANEL_X, PANEL_Y);
    m_panel.setFillColor(sf::Color(25, 34, 74, 242));
    m_panel.setOutlineThickness(3.0f);
    m_panel.setOutlineColor(sf::Color(103, 145, 220));

    m_title.setFont(font);
    m_title.setCharacterSize(34);
    m_title.setString("SETTINGS");
    m_title.setFillColor(sf::Color(255, 221, 102));
    centerText(m_title, WINDOW_WIDTH * 0.5f, 48.0f);

    const std::array<std::string, 2> tabNames = {"VIDEO", "AUDIO"};
    for (int i = 0; i < 2; ++i) {
        m_tabBoxes[i].setSize({184.0f, 34.0f});
        m_tabBoxes[i].setPosition(205.0f + i * 206.0f, 94.0f);
        m_tabBoxes[i].setOutlineThickness(2.0f);

        m_tabTexts[i].setFont(font);
        m_tabTexts[i].setCharacterSize(18);
        m_tabTexts[i].setString(tabNames[i]);
        centerText(m_tabTexts[i], m_tabBoxes[i].getPosition().x + 92.0f,
                   m_tabBoxes[i].getPosition().y + 17.0f);
    }

    for (int i = 0; i < MAX_ROWS; ++i) {
        const float y = ROW_Y + i * ROW_GAP;
        m_rows[i].setSize({ROW_WIDTH, ROW_HEIGHT});
        m_rows[i].setPosition(ROW_X, y);
        m_rows[i].setOutlineThickness(1.0f);

        m_rowLabels[i].setFont(font);
        m_rowLabels[i].setCharacterSize(16);
        m_rowLabels[i].setPosition(ROW_X + 17.0f, y + 7.0f);

        m_rowValues[i].setFont(font);
        m_rowValues[i].setCharacterSize(16);
    }

    m_status.setFont(font);
    m_status.setCharacterSize(14);
    centerText(m_status, WINDOW_WIDTH * 0.5f, 524.0f);

    m_help.setFont(font);
    m_help.setCharacterSize(12);
    m_help.setString("TAB: SWITCH PANEL   LEFT/RIGHT: ADJUST   ENTER: SELECT   ESC: BACK");
    m_help.setFillColor(sf::Color(184, 202, 255));
    centerText(m_help, WINDOW_WIDTH * 0.5f, 572.0f);

    setStatus("CHANGES APPLY ONLY WHEN YOU CHOOSE APPLY", sf::Color(184, 202, 255));
    updateVisuals();
}

void SettingsState::onExit() {
    // Audio values preview while the player adjusts sliders. Leaving without
    // Apply must restore the last committed preferences.
    SoundManager& sound = SoundManager::getInstance();
    sound.applySettings(Game::getInstance().getSettings().audio());
    if (!sound.getMusicTracks().empty()) {
        sound.selectTrack(sound.getCurrentTrackIndex());
    }
}

int SettingsState::rowCount() const {
    return m_tab == Tab::Video ? 6 : 9;
}

bool SettingsState::rowIsAdjustable(int row) const {
    return m_tab == Tab::Video ? row >= 0 && row <= 2 : row >= 0 && row <= 4;
}

void SettingsState::setTab(Tab tab) {
    m_tab = tab;
    m_selectedRow = std::min(m_selectedRow, rowCount() - 1);
    updateVisuals();
}

void SettingsState::cycleCurrentValue(int direction) {
    if (!rowIsAdjustable(m_selectedRow)) return;

    if (m_tab == Tab::Video) {
        GraphicsSettings graphics = m_pendingSettings.graphics();
        if (m_selectedRow == 0) {
            graphics.mode = graphics.mode == DisplayMode::Windowed
                ? DisplayMode::Fullscreen : DisplayMode::Windowed;
            if (graphics.mode == DisplayMode::Fullscreen) {
                const auto modes = Game::getInstance().getFullscreenModes();
                const bool currentModeIsValid = std::any_of(
                    modes.begin(), modes.end(), [&graphics](const sf::VideoMode& mode) {
                        return mode.width == graphics.width && mode.height == graphics.height;
                    });
                if (!currentModeIsValid && !modes.empty()) {
                    graphics.width = modes.front().width;
                    graphics.height = modes.front().height;
                }
            } else {
                const auto& choices = windowedResolutions();
                const bool currentPreset = std::any_of(
                    choices.begin(), choices.end(), [&graphics](const sf::Vector2u& size) {
                        return size.x == graphics.width && size.y == graphics.height;
                    });
                if (!currentPreset) {
                    graphics.width = choices.front().x;
                    graphics.height = choices.front().y;
                }
            }
        } else if (m_selectedRow == 1) {
            std::vector<sf::Vector2u> choices;
            if (graphics.mode == DisplayMode::Fullscreen) {
                for (const sf::VideoMode& mode : Game::getInstance().getFullscreenModes()) {
                    const bool duplicate = std::any_of(
                        choices.begin(), choices.end(), [&mode](const sf::Vector2u& size) {
                            return size.x == mode.width && size.y == mode.height;
                        });
                    if (!duplicate && mode.width >= 640 && mode.height >= 480) {
                        choices.emplace_back(mode.width, mode.height);
                    }
                }
            } else {
                choices.assign(windowedResolutions().begin(), windowedResolutions().end());
            }
            if (choices.empty()) {
                choices.assign(windowedResolutions().begin(), windowedResolutions().end());
            }

            auto current = std::find_if(choices.begin(), choices.end(), [&graphics](const sf::Vector2u& size) {
                return size.x == graphics.width && size.y == graphics.height;
            });
            int index = current == choices.end() ? 0 : static_cast<int>(current - choices.begin());
            index = (index + direction + static_cast<int>(choices.size())) % static_cast<int>(choices.size());
            graphics.width = choices[static_cast<std::size_t>(index)].x;
            graphics.height = choices[static_cast<std::size_t>(index)].y;
        } else {
            constexpr std::array<unsigned int, 5> fpsChoices = {30, 60, 120, 144, 0};
            auto current = std::find(fpsChoices.begin(), fpsChoices.end(), graphics.maxFps);
            int index = current == fpsChoices.end() ? 1 : static_cast<int>(current - fpsChoices.begin());
            index = (index + direction + static_cast<int>(fpsChoices.size())) % static_cast<int>(fpsChoices.size());
            graphics.maxFps = fpsChoices[static_cast<std::size_t>(index)];
        }
        m_pendingSettings.setGraphics(graphics);
    } else {
        AudioSettings audio = m_pendingSettings.audio();
        if (m_selectedRow == 0) {
            audio.muted = !audio.muted;
        } else if (m_selectedRow == 1) {
            audio.masterVolume = std::clamp(audio.masterVolume + 10.0f * direction, 0.0f, 100.0f);
        } else if (m_selectedRow == 2) {
            audio.musicVolume = std::clamp(audio.musicVolume + 10.0f * direction, 0.0f, 100.0f);
        } else if (m_selectedRow == 3) {
            audio.sfxVolume = std::clamp(audio.sfxVolume + 10.0f * direction, 0.0f, 100.0f);
        } else if (m_selectedRow == 4) {
            const auto& tracks = SoundManager::getInstance().getMusicTracks();
            if (!tracks.empty()) {
                const int trackCount = static_cast<int>(tracks.size());
                const int index = (static_cast<int>(audio.musicTrack) + direction + trackCount) % trackCount;
                audio.musicTrack = static_cast<std::size_t>(index);
            }
        }
        m_pendingSettings.setAudio(audio);
        syncAudioPreview();
    }
    updateVisuals();
}

void SettingsState::activateCurrentRow() {
    if (rowIsAdjustable(m_selectedRow)) {
        cycleCurrentValue(1);
        return;
    }

    const int applyRow = m_tab == Tab::Video ? 3 : 6;
    const int defaultsRow = applyRow + 1;
    if (m_selectedRow == applyRow) {
        applyChanges();
    } else if (m_selectedRow == defaultsRow) {
        m_pendingSettings.resetDefaults();
        syncAudioPreview();
        setStatus("DEFAULTS READY — CHOOSE APPLY TO KEEP THEM", sf::Color(255, 221, 102));
        updateVisuals();
    } else {
        cancelAndBack();
    }
}

void SettingsState::applyChanges() {
    Game& game = Game::getInstance();
    game.getSettings().setGraphics(m_pendingSettings.graphics());
    game.getSettings().setAudio(m_pendingSettings.audio());
    const bool videoApplied = game.applyGraphicsSettings(game.getSettings().graphics());
    game.applyAudioSettings();

    SoundManager& sound = SoundManager::getInstance();
    if (!sound.getMusicTracks().empty()) {
        sound.selectTrack(sound.getCurrentTrackIndex());
    }

    m_pendingSettings = game.getSettings();
    m_openingSettings = m_pendingSettings;
    const bool saved = game.getSettings().saveToFile();
    if (!saved) {
        setStatus("SETTINGS APPLIED, BUT COULD NOT SAVE settings.cfg", sf::Color(255, 150, 150));
    } else if (!videoApplied) {
        setStatus("UNSUPPORTED FULLSCREEN MODE — RESTORED SAFE WINDOW", sf::Color(255, 221, 102));
    } else {
        setStatus("SETTINGS SAVED", sf::Color(135, 240, 164));
    }
    updateVisuals();
}

void SettingsState::cancelAndBack() {
    SoundManager& sound = SoundManager::getInstance();
    sound.applySettings(Game::getInstance().getSettings().audio());
    if (!sound.getMusicTracks().empty()) {
        sound.selectTrack(sound.getCurrentTrackIndex());
    }
    Game::getInstance().getStateManager().popState();
}

void SettingsState::syncAudioPreview() {
    SoundManager& sound = SoundManager::getInstance();
    sound.applySettings(m_pendingSettings.audio());
    if (!sound.getMusicTracks().empty()) {
        sound.selectTrack(sound.getCurrentTrackIndex());
    }
}

void SettingsState::setStatus(const std::string& status, sf::Color color) {
    m_status.setString(status);
    m_status.setFillColor(color);
    centerText(m_status, WINDOW_WIDTH * 0.5f, 524.0f);
}

void SettingsState::updateVisuals() {
    const sf::Font* font = m_title.getFont();
    if (!font) return;

    const bool video = m_tab == Tab::Video;
    for (int i = 0; i < 2; ++i) {
        const bool active = (i == 0) == video;
        m_tabBoxes[i].setFillColor(active ? sf::Color(62, 104, 185) : sf::Color(35, 47, 90));
        m_tabBoxes[i].setOutlineColor(active ? sf::Color(255, 221, 102) : sf::Color(108, 132, 202));
        m_tabTexts[i].setFillColor(active ? sf::Color::White : sf::Color(180, 194, 238));
    }

    std::array<std::string, MAX_ROWS> labels{};
    std::array<std::string, MAX_ROWS> values{};
    if (video) {
        const GraphicsSettings& graphics = m_pendingSettings.graphics();
        labels = {"DISPLAY MODE", "RESOLUTION", "MAX FPS", "APPLY CHANGES",
                  "RESTORE DEFAULTS", "BACK", "", "", ""};
        values[0] = graphics.mode == DisplayMode::Fullscreen ? "FULLSCREEN" : "WINDOWED";
        values[1] = resolutionLabel(graphics.width, graphics.height);
        values[2] = fpsLabel(graphics.maxFps);
    } else {
        const AudioSettings& audio = m_pendingSettings.audio();
        labels = {"MUTE", "MASTER VOLUME", "MUSIC VOLUME", "SFX VOLUME",
                  "MUSIC TRACK", "TEST SFX", "APPLY CHANGES", "RESTORE DEFAULTS", "BACK"};
        values[0] = audio.muted ? "ON" : "OFF";
        values[1] = volumeLabel(audio.masterVolume);
        values[2] = volumeLabel(audio.musicVolume);
        values[3] = volumeLabel(audio.sfxVolume);
        const auto& tracks = SoundManager::getInstance().getMusicTracks();
        values[4] = tracks.empty() ? "NO TRACKS" : tracks[std::min(audio.musicTrack, tracks.size() - 1)].first;
        values[5] = "PLAY COIN SOUND";
    }

    const int visibleRows = rowCount();
    for (int i = 0; i < MAX_ROWS; ++i) {
        const bool visible = i < visibleRows;
        m_rows[i].setFillColor(i == m_selectedRow
            ? sf::Color(57, 90, 157, 240) : sf::Color(29, 43, 87, 215));
        m_rows[i].setOutlineColor(i == m_selectedRow
            ? sf::Color(255, 221, 102) : sf::Color(85, 113, 183));
        m_rows[i].setOutlineThickness(i == m_selectedRow ? 2.0f : 1.0f);

        m_rowLabels[i].setString(visible ? labels[i] : "");
        m_rowLabels[i].setFillColor(i == m_selectedRow ? sf::Color::White : sf::Color(209, 222, 255));

        m_rowValues[i].setString(visible ? values[i] : "");
        m_rowValues[i].setFillColor(rowIsAdjustable(i) ? sf::Color(255, 221, 102) : sf::Color(177, 232, 195));
        const sf::FloatRect bounds = m_rowValues[i].getLocalBounds();
        m_rowValues[i].setOrigin(bounds.left + bounds.width, bounds.top);
        m_rowValues[i].setPosition(ROW_X + ROW_WIDTH - 17.0f, ROW_Y + i * ROW_GAP + 7.0f);
    }
}

void SettingsState::handleEvent(const sf::Event& event) {
    Game& game = Game::getInstance();
    if (event.type == sf::Event::MouseMoved ||
        (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left)) {
        const sf::Vector2i pixel = event.type == sf::Event::MouseMoved
            ? sf::Vector2i(event.mouseMove.x, event.mouseMove.y)
            : sf::Vector2i(event.mouseButton.x, event.mouseButton.y);
        const sf::Vector2f mouse = game.mapPixelToUiCoords(pixel);

        for (int i = 0; i < 2; ++i) {
            if (m_tabBoxes[i].getGlobalBounds().contains(mouse)) {
                setTab(i == 0 ? Tab::Video : Tab::Audio);
                return;
            }
        }
        for (int i = 0; i < rowCount(); ++i) {
            if (m_rows[i].getGlobalBounds().contains(mouse)) {
                m_selectedRow = i;
                updateVisuals();
                if (event.type == sf::Event::MouseButtonPressed) {
                    activateCurrentRow();
                }
                return;
            }
        }
        return;
    }

    if (event.type != sf::Event::KeyPressed) return;
    switch (event.key.code) {
    case sf::Keyboard::Escape:
        cancelAndBack();
        break;
    case sf::Keyboard::Tab:
        setTab(m_tab == Tab::Video ? Tab::Audio : Tab::Video);
        break;
    case sf::Keyboard::Up:
    case sf::Keyboard::W:
        m_selectedRow = (m_selectedRow - 1 + rowCount()) % rowCount();
        updateVisuals();
        break;
    case sf::Keyboard::Down:
    case sf::Keyboard::S:
        m_selectedRow = (m_selectedRow + 1) % rowCount();
        updateVisuals();
        break;
    case sf::Keyboard::Left:
    case sf::Keyboard::A:
        cycleCurrentValue(-1);
        break;
    case sf::Keyboard::Right:
    case sf::Keyboard::D:
        cycleCurrentValue(1);
        break;
    case sf::Keyboard::Return:
    case sf::Keyboard::Space:
        activateCurrentRow();
        break;
    default:
        break;
    }
}

void SettingsState::update(float) {}

void SettingsState::render(sf::RenderWindow& window) {
    window.setView(Game::getInstance().getUiView());
    window.draw(m_background);
    window.draw(m_title);
    for (int i = 0; i < 2; ++i) {
        window.draw(m_tabBoxes[i]);
        window.draw(m_tabTexts[i]);
    }
    window.draw(m_panel);
    for (int i = 0; i < rowCount(); ++i) {
        window.draw(m_rows[i]);
        window.draw(m_rowLabels[i]);
        window.draw(m_rowValues[i]);
    }
    window.draw(m_status);
    window.draw(m_help);
}
