#include "Core/GameSettings.hpp"
#include "Core/SoundManager.hpp"

#include <cmath>
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

static void testGraphicsSettingsBoundaryValuesAreExact() {
    GameSettings settings;

    GraphicsSettings atMin;
    atMin.width = 640;
    atMin.height = 480;
    settings.setGraphics(atMin);
    CHECK(settings.graphics().width == 640 && settings.graphics().height == 480,
          "the exact minimum resolution is accepted unchanged");

    GraphicsSettings atMax;
    atMax.width = 7680;
    atMax.height = 4320;
    settings.setGraphics(atMax);
    CHECK(settings.graphics().width == 7680 && settings.graphics().height == 4320,
          "the exact maximum resolution is accepted unchanged");

    GraphicsSettings belowMin;
    belowMin.width = 639;
    belowMin.height = 480;
    settings.setGraphics(belowMin);
    CHECK(settings.graphics().width == 800 && settings.graphics().height == 608,
          "one pixel below the minimum width falls back to the default resolution");

    GraphicsSettings aboveMax;
    aboveMax.width = 7680;
    aboveMax.height = 4321;
    settings.setGraphics(aboveMax);
    CHECK(settings.graphics().width == 800 && settings.graphics().height == 608,
          "one pixel above the maximum height falls back to the default resolution");

    for (unsigned fps : {0u, 30u, 60u, 120u, 144u}) {
        GraphicsSettings graphics;
        graphics.maxFps = fps;
        settings.setGraphics(graphics);
        CHECK(settings.graphics().maxFps == fps,
              "each officially supported FPS cap is accepted unchanged");
    }

    GraphicsSettings unsupportedFps;
    unsupportedFps.maxFps = 90;
    settings.setGraphics(unsupportedFps);
    CHECK(settings.graphics().maxFps == 60,
          "an unsupported FPS value falls back to 60 FPS");
}

static void testSoundManagerVolumeClampingAndAutoUnmute() {
    SoundManager &sound = SoundManager::getInstance();

    sound.setMasterVolume(150.0f);
    CHECK(sound.getMasterVolume() == 100.0f,
          "master volume clamps to 100 at the high end");
    sound.setMasterVolume(-10.0f);
    CHECK(sound.getMasterVolume() == 0.0f,
          "master volume clamps to 0 at the low end");

    sound.setMuted(true);
    CHECK(sound.isMuted(), "setMuted(true) mutes the sound manager");
    sound.setMasterVolume(50.0f);
    CHECK(!sound.isMuted(),
          "raising the master volume above zero automatically unmutes");

    sound.setSoundVolume(200.0f);
    CHECK(sound.getSoundVolume() == 100.0f, "SFX volume clamps to 100");
    sound.setMusicVolume(-5.0f);
    CHECK(sound.getMusicVolume() == 0.0f, "music volume clamps to 0");

    sound.toggleMute();
    const bool afterFirstToggle = sound.isMuted();
    sound.toggleMute();
    CHECK(sound.isMuted() != afterFirstToggle,
          "toggleMute() flips the mute state each call");

    // Leave the manager in a known-safe state for anything that runs after
    // this test in the same process.
    sound.setMuted(false);
    sound.setMasterVolume(100.0f);
}

static void testSoundManagerTrackNavigationWrapsAround() {
    SoundManager &sound = SoundManager::getInstance();
    const auto &tracks = sound.getMusicTracks();
    CHECK(!tracks.empty(),
          "setup: the sound manager has at least one music track");
    if (tracks.empty()) return;

    sound.selectTrack(0);
    CHECK(sound.getCurrentTrackIndex() == 0,
          "setup: track selection lands exactly on index 0");
    CHECK(sound.getCurrentTrackName() == tracks[0].first,
          "getCurrentTrackName() matches the selected track's stored name");

    sound.prevTrack();
    CHECK(sound.getCurrentTrackIndex() == tracks.size() - 1,
          "stepping back from the first track wraps around to the last track");

    sound.nextTrack();
    CHECK(sound.getCurrentTrackIndex() == 0,
          "stepping forward from the last track wraps back to the first track");
}

static void testNonFiniteAudioValuesAreRejected() {
    // Unlike "overdrive" (rejected by std::stof, caught before normalize()
    // ever sees it), the literal string "nan" parses successfully into a
    // real NaN float -- std::clamp leaves NaN unchanged since every
    // comparison against it is false, so normalize() must reject it itself.
    AudioSettings audio;
    audio.masterVolume = std::nanf("");
    audio.musicVolume = std::nanf("");
    audio.sfxVolume = std::nanf("");
    GameSettings settings;
    settings.setAudio(audio);

    CHECK(std::isfinite(settings.audio().masterVolume) &&
              settings.audio().masterVolume >= 0.0f && settings.audio().masterVolume <= 100.0f,
          "a NaN master volume normalizes to a finite value in range");
    CHECK(std::isfinite(settings.audio().musicVolume) &&
              settings.audio().musicVolume >= 0.0f && settings.audio().musicVolume <= 100.0f,
          "a NaN music volume normalizes to a finite value in range");
    CHECK(std::isfinite(settings.audio().sfxVolume) &&
              settings.audio().sfxVolume >= 0.0f && settings.audio().sfxVolume <= 100.0f,
          "a NaN SFX volume normalizes to a finite value in range");

    const std::filesystem::path path = testPath("super_mario_game_settings_nan.cfg");
    {
        std::ofstream output(path);
        output << "master_volume=nan\n";
    }
    GameSettings loaded;
    CHECK(loaded.loadFromFile(path.string()),
          "a config with a literal 'nan' value is still processed");
    CHECK(std::isfinite(loaded.audio().masterVolume),
          "loading a file with master_volume=nan does not leave a NaN active setting");
    std::filesystem::remove(path);
}

int main() {
    testDefaultsAndNormalization();
    testRoundTrip();
    testMissingAndMalformedFilesStaySafe();
    testGraphicsSettingsBoundaryValuesAreExact();
    testSoundManagerVolumeClampingAndAutoUnmute();
    testSoundManagerTrackNavigationWrapsAround();
    testNonFiniteAudioValuesAreRejected();

    if (g_failures == 0) {
        std::cout << "All game settings tests passed.\n";
        return 0;
    }
    std::cerr << g_failures << " test(s) failed.\n";
    return 1;
}
