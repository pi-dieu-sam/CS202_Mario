#pragma once
#include "Core/GameSettings.hpp"
#include <SFML/Audio.hpp>
#include <unordered_map>
#include <string>
#include <vector>
#include <utility>

/// Sound IDs for quick access to pre-registered sound effects.
enum class SoundID {
    Jump,
    Coin,
    PowerUp,
    Stomp,
    Fireball,
    PlayerDeath,
    LevelComplete,
    GameOver,
    BlockBreak,
    BlockBump,
    OneUp,
    Pause
};

/// SoundManager — Singleton pattern.
/// Manages sound effect playback with a pool of sf::Sound objects,
/// background music streaming, volume controls, mute toggling, and track selection.
class SoundManager {
public:
    static SoundManager& getInstance();

    SoundManager(const SoundManager&)            = delete;
    SoundManager& operator=(const SoundManager&) = delete;

    /// Register a sound effect with an ID.
    void registerSound(SoundID id, const std::string& filename);

    /// Play a registered sound effect.
    void playSound(SoundID id);

    /// Play background music (streams from file).
    void playMusic(const std::string& filename, bool loop = true);

    /// Stop background music.
    void stopMusic();

    /// Pause / resume music.
    void pauseMusic();
    void resumeMusic();

    /// Mute controls
    void toggleMute();
    void setMuted(bool muted);
    bool isMuted() const;

    /// Volume control (0–100).
    void setMasterVolume(float percent);
    float getMasterVolume() const;

    void setSoundVolume(float volume);
    void setMusicVolume(float volume);
    float getSoundVolume() const;
    float getMusicVolume() const;

    /// Apply persisted audio preferences in one operation. Playback itself is
    /// left to the active screen, so loading preferences never forces music
    /// to start before the title screen is ready.
    void applySettings(const AudioSettings& settings);

    /// Track selection
    const std::vector<std::pair<std::string, std::string>>& getMusicTracks() const;
    size_t getCurrentTrackIndex() const;
    std::string getCurrentTrackName() const;
    void nextTrack();
    void prevTrack();
    void selectTrack(size_t index);

private:
    SoundManager();
    void applyVolumes();

    static constexpr int SOUND_POOL_SIZE = 16;

    std::unordered_map<SoundID, std::string> m_soundFiles;
    std::vector<sf::Sound>                   m_soundPool;
    int                                      m_nextSound = 0;

    sf::Music m_music;
    std::string m_currentMusicFile;
    bool      m_muted = false;
    float     m_masterVolume = 70.0f; // 0..100
    float     m_soundVolume  = 70.0f;
    float     m_musicVolume  = 50.0f;

    std::vector<std::pair<std::string, std::string>> m_musicTracks; // {Display Name, File Path}
    size_t m_currentTrackIdx = 0;
};
