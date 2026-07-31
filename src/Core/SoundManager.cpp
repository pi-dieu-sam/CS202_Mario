#include "Core/SoundManager.hpp"
#include "Core/AssetManager.hpp"
#include <iostream>

SoundManager& SoundManager::getInstance() {
    static SoundManager instance;
    return instance;
}

SoundManager::SoundManager() {
    m_soundPool.resize(SOUND_POOL_SIZE);
}

void SoundManager::registerSound(SoundID id, const std::string& filename) {
    m_soundFiles[id] = filename;
    // Pre-load the sound buffer
    AssetManager::getInstance().getSoundBuffer(filename);
}

void SoundManager::playSound(SoundID id) {
    auto it = m_soundFiles.find(id);
    if (it == m_soundFiles.end()) return;

    sf::SoundBuffer& buffer = AssetManager::getInstance().getSoundBuffer(it->second);

    // Prefer a slot that isn't currently playing, so a fast burst of sounds
    // (coins, stomps) doesn't cut off something still audible. Only fall
    // back to stealing the round-robin slot if the whole pool is busy.
    int slot = -1;
    for (int i = 0; i < SOUND_POOL_SIZE; i++) {
        if (m_soundPool[i].getStatus() != sf::Sound::Status::Playing) {
            slot = i;
            break;
        }
    }
    if (slot == -1) {
        slot = m_nextSound;
        m_nextSound = (m_nextSound + 1) % SOUND_POOL_SIZE;
    }

    m_soundPool[slot].setBuffer(buffer);
    m_soundPool[slot].setVolume(m_soundVolume);
    m_soundPool[slot].play();
}

void SoundManager::playMusic(const std::string& filename, bool loop) {
    if (!m_music.openFromFile(filename)) {
        std::cerr << "[SoundManager] Failed to load music: " << filename << std::endl;
        return;
    }
    m_music.setLoop(loop);
    m_music.setVolume(m_musicVolume);
    m_music.play();
}

void SoundManager::stopMusic() {
    m_music.stop();
}

void SoundManager::pauseMusic() {
    m_music.pause();
}

void SoundManager::resumeMusic() {
    m_music.play();
}

void SoundManager::setSoundVolume(float volume) {
    m_soundVolume = volume;
}

void SoundManager::setMusicVolume(float volume) {
    m_musicVolume = volume;
    m_music.setVolume(volume);
}
