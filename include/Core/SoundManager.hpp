#pragma once
#include <SFML/Audio.hpp>
#include <unordered_map>
#include <string>
#include <vector>

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
/// and background music via sf::Music.
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

    /// Volume control (0–100).
    void setSoundVolume(float volume);
    void setMusicVolume(float volume);

private:
    SoundManager();

    static constexpr int SOUND_POOL_SIZE = 16;

    std::unordered_map<SoundID, std::string> m_soundFiles;
    std::vector<sf::Sound>                   m_soundPool;
    int                                      m_nextSound = 0;

    sf::Music m_music;
    float     m_soundVolume = 70.0f;
    float     m_musicVolume = 50.0f;
};
