#pragma once

#include <cstddef>
#include <string>

/// DisplayMode intentionally stays independent of SFML so settings parsing can
/// be tested without constructing a window.
enum class DisplayMode {
    Windowed,
    Fullscreen,
};

struct GraphicsSettings {
    DisplayMode mode = DisplayMode::Windowed;
    unsigned int width = 800;
    unsigned int height = 608;
    /// Zero means uncapped rendering. Simulation remains fixed at 60 Hz.
    unsigned int maxFps = 60;
};

struct AudioSettings {
    bool muted = false;
    float masterVolume = 70.0f;
    float musicVolume = 50.0f;
    float sfxVolume = 70.0f;
    std::size_t musicTrack = 0;
};

/// GameSettings owns user preferences that should survive restart.  It is a
/// deliberately small, dependency-free model: the Game class validates video
/// modes against the active display before recreating an SFML window.
class GameSettings {
public:
    static constexpr const char* DEFAULT_PATH = "settings.cfg";

    GameSettings() = default;

    const GraphicsSettings& graphics() const noexcept;
    const AudioSettings& audio() const noexcept;

    void setGraphics(GraphicsSettings settings);
    void setAudio(AudioSettings settings);
    void resetDefaults();

    /// Loads key/value preferences. Missing or malformed values fall back to
    /// safe defaults; unknown keys are intentionally ignored for forwards
    /// compatibility.
    bool loadFromFile(const std::string& path = DEFAULT_PATH);

    /// Writes the current normalized preferences. Returns false if the file
    /// cannot be opened; callers can keep running with in-memory values.
    bool saveToFile(const std::string& path = DEFAULT_PATH) const;

    /// Normalization is public so UI and tests can share the same validation
    /// rules before values reach the window/audio layers.
    static GraphicsSettings normalize(GraphicsSettings settings);
    static AudioSettings normalize(AudioSettings settings);

private:
    GraphicsSettings m_graphics;
    AudioSettings m_audio;
};
