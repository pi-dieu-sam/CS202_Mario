#include "Core/GameSettings.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

namespace {
constexpr unsigned int DEFAULT_WIDTH = 800;
constexpr unsigned int DEFAULT_HEIGHT = 608;
constexpr unsigned int DEFAULT_FPS = 60;
constexpr unsigned int MIN_WIDTH = 640;
constexpr unsigned int MIN_HEIGHT = 480;
constexpr unsigned int MAX_WIDTH = 7680;
constexpr unsigned int MAX_HEIGHT = 4320;

std::string trim(std::string value) {
    const auto isSpace = [](unsigned char c) { return std::isspace(c) != 0; };
    const auto first = std::find_if_not(value.begin(), value.end(), isSpace);
    const auto last = std::find_if_not(value.rbegin(), value.rend(), isSpace).base();
    if (first >= last) return {};
    return std::string(first, last);
}

bool parseBool(const std::string& value, bool fallback) {
    if (value == "1" || value == "true" || value == "TRUE") return true;
    if (value == "0" || value == "false" || value == "FALSE") return false;
    return fallback;
}

bool isSupportedFps(unsigned int value) {
    return value == 0 || value == 30 || value == 60 || value == 120 || value == 144;
}
} // namespace

const GraphicsSettings& GameSettings::graphics() const noexcept {
    return m_graphics;
}

const AudioSettings& GameSettings::audio() const noexcept {
    return m_audio;
}

void GameSettings::setGraphics(GraphicsSettings settings) {
    m_graphics = normalize(settings);
}

void GameSettings::setAudio(AudioSettings settings) {
    m_audio = normalize(settings);
}

void GameSettings::resetDefaults() {
    m_graphics = GraphicsSettings{};
    m_audio = AudioSettings{};
}

GraphicsSettings GameSettings::normalize(GraphicsSettings settings) {
    if (settings.width < MIN_WIDTH || settings.height < MIN_HEIGHT ||
        settings.width > MAX_WIDTH || settings.height > MAX_HEIGHT) {
        settings.width = DEFAULT_WIDTH;
        settings.height = DEFAULT_HEIGHT;
    }
    if (!isSupportedFps(settings.maxFps)) {
        settings.maxFps = DEFAULT_FPS;
    }
    return settings;
}

AudioSettings GameSettings::normalize(AudioSettings settings) {
    settings.masterVolume = std::clamp(settings.masterVolume, 0.0f, 100.0f);
    settings.musicVolume = std::clamp(settings.musicVolume, 0.0f, 100.0f);
    settings.sfxVolume = std::clamp(settings.sfxVolume, 0.0f, 100.0f);
    settings.musicTrack = std::min<std::size_t>(settings.musicTrack, 99);
    return settings;
}

bool GameSettings::loadFromFile(const std::string& path) {
    resetDefaults();

    std::ifstream input(path);
    if (!input.is_open()) {
        return false;
    }

    GraphicsSettings graphics = m_graphics;
    AudioSettings audio = m_audio;
    std::string line;
    while (std::getline(input, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;

        const std::size_t separator = line.find('=');
        if (separator == std::string::npos) continue;

        const std::string key = trim(line.substr(0, separator));
        const std::string value = trim(line.substr(separator + 1));
        try {
            if (key == "display_mode") {
                graphics.mode = value == "fullscreen" ? DisplayMode::Fullscreen
                                                       : DisplayMode::Windowed;
            } else if (key == "width") {
                graphics.width = static_cast<unsigned int>(std::stoul(value));
            } else if (key == "height") {
                graphics.height = static_cast<unsigned int>(std::stoul(value));
            } else if (key == "max_fps") {
                graphics.maxFps = static_cast<unsigned int>(std::stoul(value));
            } else if (key == "muted") {
                audio.muted = parseBool(value, audio.muted);
            } else if (key == "master_volume") {
                audio.masterVolume = std::stof(value);
            } else if (key == "music_volume") {
                audio.musicVolume = std::stof(value);
            } else if (key == "sfx_volume") {
                audio.sfxVolume = std::stof(value);
            } else if (key == "music_track") {
                audio.musicTrack = static_cast<std::size_t>(std::stoull(value));
            }
        } catch (const std::exception&) {
            // Keep the previous/default value for a malformed individual key.
        }
    }

    setGraphics(graphics);
    setAudio(audio);
    return true;
}

bool GameSettings::saveToFile(const std::string& path) const {
    std::ofstream output(path, std::ios::trunc);
    if (!output.is_open()) {
        return false;
    }

    output << "# Super Mario settings v1\n";
    output << "display_mode=" << (m_graphics.mode == DisplayMode::Fullscreen ? "fullscreen" : "windowed") << '\n';
    output << "width=" << m_graphics.width << '\n';
    output << "height=" << m_graphics.height << '\n';
    output << "max_fps=" << m_graphics.maxFps << '\n';
    output << "muted=" << (m_audio.muted ? 1 : 0) << '\n';
    output << "master_volume=" << m_audio.masterVolume << '\n';
    output << "music_volume=" << m_audio.musicVolume << '\n';
    output << "sfx_volume=" << m_audio.sfxVolume << '\n';
    output << "music_track=" << m_audio.musicTrack << '\n';
    return static_cast<bool>(output);
}
