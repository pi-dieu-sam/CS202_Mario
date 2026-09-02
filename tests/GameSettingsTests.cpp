#include "Core/GameSettings.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>

static int g_failures = 0;

#define CHECK(cond, msg)                                                     \
  do {                                                                       \
    if (!(cond)) {                                                           \
      std::cerr << "FAIL: " << msg << " (" << __FILE__ << ":" << __LINE__    \
                << ")\n";                                                   \
      ++g_failures;                                                         \
    }                                                                        \
  } while (0)

namespace {
std::filesystem::path testPath(const char* name) {
    return std::filesystem::temp_directory_path() / name;
}
} // namespace

static void testDefaultsAndNormalization() {
    GameSettings settings;
    GraphicsSettings graphics;
    graphics.width = 0;
    graphics.height = 99999;
    graphics.maxFps = 17;
    settings.setGraphics(graphics);

    CHECK(settings.graphics().width == 800 && settings.graphics().height == 608,
          "invalid dimensions fall back to the logical canvas window size");
    CHECK(settings.graphics().maxFps == 60,
          "unsupported FPS caps fall back to 60 FPS");

    graphics = settings.graphics();
    graphics.maxFps = 0;
    graphics.mode = DisplayMode::Fullscreen;
    settings.setGraphics(graphics);
    CHECK(settings.graphics().maxFps == 0,
          "zero remains the explicit uncapped FPS setting");
    CHECK(settings.graphics().mode == DisplayMode::Fullscreen,
          "display mode persists through normalization");

    AudioSettings audio;
    audio.masterVolume = 180.0f;
    audio.musicVolume = -5.0f;
    audio.sfxVolume = 47.0f;
    audio.musicTrack = 1000;
    settings.setAudio(audio);
    CHECK(settings.audio().masterVolume == 100.0f && settings.audio().musicVolume == 0.0f,
          "audio values clamp to valid percentages");
    CHECK(settings.audio().sfxVolume == 47.0f && settings.audio().musicTrack == 99,
          "valid SFX and bounded track values survive normalization");
}

static void testRoundTrip() {
    const std::filesystem::path path = testPath("super_mario_game_settings_roundtrip.cfg");
    std::filesystem::remove(path);

    GameSettings written;
    GraphicsSettings graphics;
    graphics.mode = DisplayMode::Fullscreen;
    graphics.width = 1920;
    graphics.height = 1080;
    graphics.maxFps = 144;
    written.setGraphics(graphics);
    AudioSettings audio;
    audio.muted = true;
    audio.masterVolume = 61.0f;
    audio.musicVolume = 42.0f;
    audio.sfxVolume = 88.0f;
    audio.musicTrack = 2;
    written.setAudio(audio);

    CHECK(written.saveToFile(path.string()), "settings save writes a config file");
    GameSettings loaded;
    CHECK(loaded.loadFromFile(path.string()), "saved settings load successfully");
    CHECK(loaded.graphics().mode == DisplayMode::Fullscreen &&
              loaded.graphics().width == 1920 && loaded.graphics().height == 1080 &&
              loaded.graphics().maxFps == 144,
          "graphics settings round-trip exactly");
    CHECK(loaded.audio().muted && loaded.audio().masterVolume == 61.0f &&
              loaded.audio().musicVolume == 42.0f && loaded.audio().sfxVolume == 88.0f &&
              loaded.audio().musicTrack == 2,
          "audio settings round-trip exactly");
    std::filesystem::remove(path);
}

static void testMissingAndMalformedFilesStaySafe() {
    const std::filesystem::path missing = testPath("super_mario_game_settings_missing.cfg");
    std::filesystem::remove(missing);
    GameSettings settings;
    CHECK(!settings.loadFromFile(missing.string()), "a missing config does not count as a successful load");
    CHECK(settings.graphics().width == 800 && settings.audio().masterVolume == 70.0f,
          "a missing config leaves safe defaults active");

    const std::filesystem::path malformed = testPath("super_mario_game_settings_malformed.cfg");
    {
        std::ofstream output(malformed);
        output << "display_mode=fullscreen\n";
        output << "width=not-a-number\n";
        output << "height=-1\n";
        output << "max_fps=123\n";
        output << "master_volume=overdrive\n";
        output << "music_volume=-90\n";
        output << "sfx_volume=500\n";
        output << "muted=not-a-bool\n";
    }
    CHECK(settings.loadFromFile(malformed.string()), "a readable malformed config is still processed");
    CHECK(settings.graphics().width == 800 && settings.graphics().height == 608 &&
              settings.graphics().maxFps == 60,
          "malformed graphics keys fall back safely");
    CHECK(settings.audio().masterVolume == 70.0f && settings.audio().musicVolume == 0.0f &&
              settings.audio().sfxVolume == 100.0f && !settings.audio().muted,
          "each malformed or out-of-range audio key resolves independently");
    std::filesystem::remove(malformed);
}

int main() {
    testDefaultsAndNormalization();
    testRoundTrip();
    testMissingAndMalformedFilesStaySafe();

    if (g_failures == 0) {
        std::cout << "All game settings tests passed.\n";
        return 0;
    }
    std::cerr << g_failures << " test(s) failed.\n";
    return 1;
}
