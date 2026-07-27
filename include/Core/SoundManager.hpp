#ifndef SOUND_MANAGER_HPP
#define SOUND_MANAGER_HPP

#include <SFML/Audio.hpp>
#include <unordered_map>
#include <string>
#include <memory>
#include <vector>
#include <algorithm>
#include <iostream>

/**
 * @brief Singleton class to manage sound effects and background music playback.
 */
class SoundManager {
public:
    static SoundManager& getInstance() {
        static SoundManager instance;
        return instance;
    }

    SoundManager(const SoundManager&) = delete;
    SoundManager& operator=(const SoundManager&) = delete;

    /**
     * @brief Load a sound buffer with key identifier.
     */
    bool loadSound(const std::string& key, const std::string& filename) {
        sf::SoundBuffer buffer;
        if (!buffer.loadFromFile(filename)) {
            std::cerr << "[SoundManager] Warning: Failed to load sound from " << filename << std::endl;
            return false;
        }
        m_soundBuffers[key] = buffer;
        return true;
    }

    /**
     * @brief Play a registered sound effect.
     */
    void playSound(const std::string& key) {
        if (!m_soundMuted) {
            auto it = m_soundBuffers.find(key);
            if (it != m_soundBuffers.end()) {
                // Clean up finished sounds
                m_activeSounds.erase(
                    std::remove_if(m_activeSounds.begin(), m_activeSounds.end(),
                        [](const sf::Sound& s) { return s.getStatus() == sf::Sound::Status::Stopped; }),
                    m_activeSounds.end()
                );
                
                sf::Sound sound(it->second);
                sound.setVolume(m_sfxVolume);
                sound.play();
                m_activeSounds.push_back(sound);
            }
        }
    }

    /**
     * @brief Play music from file.
     */
    bool playMusic(const std::string& filename, bool loop = true) {
        if (m_music.openFromFile(filename)) {
            m_music.setLooping(loop);
            m_music.setVolume(m_musicVolume);
            if (!m_musicMuted) {
                m_music.play();
            }
            return true;
        }
        std::cerr << "[SoundManager] Warning: Failed to load music from " << filename << std::endl;
        return false;
    }

    void stopMusic() {
        m_music.stop();
    }

    void pauseMusic() {
        m_music.pause();
    }

    void resumeMusic() {
        if (!m_musicMuted && m_music.getStatus() == sf::Music::Status::Paused) {
            m_music.play();
        }
    }

    void setSFXVolume(float volume) {
        m_sfxVolume = volume;
        for (auto& sound : m_activeSounds) {
            sound.setVolume(volume);
        }
    }

    void setMusicVolume(float volume) {
        m_musicVolume = volume;
        m_music.setVolume(volume);
    }

    void toggleSoundMute() {
        m_soundMuted = !m_soundMuted;
    }

    void toggleMusicMute() {
        m_musicMuted = !m_musicMuted;
        if (m_musicMuted) {
            m_music.pause();
        } else {
            m_music.play();
        }
    }

    bool isSoundMuted() const { return m_soundMuted; }
    bool isMusicMuted() const { return m_musicMuted; }

private:
    SoundManager() : m_sfxVolume(100.0f), m_musicVolume(70.0f), m_soundMuted(false), m_musicMuted(false) {}
    ~SoundManager() = default;

    std::unordered_map<std::string, sf::SoundBuffer> m_soundBuffers;
    std::vector<sf::Sound> m_activeSounds;
    sf::Music m_music;

    float m_sfxVolume;
    float m_musicVolume;
    bool m_soundMuted;
    bool m_musicMuted;
};

#endif // SOUND_MANAGER_HPP
