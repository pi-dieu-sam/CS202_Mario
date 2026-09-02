#include "Core/SoundManager.hpp"
#include "Core/AssetManager.hpp"
#include <iostream>
#include <algorithm>

SoundManager& SoundManager::getInstance() {
    static SoundManager instance;
    return instance;
}

SoundManager::SoundManager() {
    m_soundPool.resize(SOUND_POOL_SIZE);

    // Pre-register sound effects
    registerSound(SoundID::Jump,          "assets/audio/jump.wav");
    registerSound(SoundID::Coin,          "assets/audio/coin.wav");
    registerSound(SoundID::PowerUp,       "assets/audio/powerup.wav");
    registerSound(SoundID::Stomp,         "assets/audio/stomp.wav");
    registerSound(SoundID::Fireball,      "assets/audio/fireball.wav");
    registerSound(SoundID::PlayerDeath,   "assets/audio/player_death.wav");
    registerSound(SoundID::LevelComplete, "assets/audio/level_complete.wav");
    registerSound(SoundID::GameOver,      "assets/audio/game_over.wav");
    registerSound(SoundID::BlockBreak,    "assets/audio/block_break.wav");
    registerSound(SoundID::BlockBump,     "assets/audio/block_bump.wav");
    registerSound(SoundID::OneUp,         "assets/audio/one_up.wav");
    registerSound(SoundID::Pause,         "assets/audio/pause.wav");

    // Track list — "Hide - Dorian Concept" set as DEFAULT INDEX 0
    m_musicTracks = {
        {"Hide - Dorian Concept", "assets/audio/hide_dorian_concept.wav"},
        {"Menu Theme",            "assets/audio/menu_theme.wav"},
        {"Overworld Theme",       "assets/audio/theme.wav"},
        {"Underground Theme",     "assets/audio/underground_theme.wav"}
    };
    m_currentTrackIdx = 0;
}

void SoundManager::registerSound(SoundID id, const std::string& filename) {
    m_soundFiles[id] = filename;
    AssetManager::getInstance().getSoundBuffer(filename);
}

void SoundManager::playSound(SoundID id) {
    if (m_muted || m_masterVolume <= 0.0f) return;

    auto it = m_soundFiles.find(id);
    if (it == m_soundFiles.end()) return;

    sf::SoundBuffer& buffer = AssetManager::getInstance().getSoundBuffer(it->second);

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

    float effectiveVolume = (m_soundVolume * m_masterVolume) / 100.0f;
    m_soundPool[slot].setBuffer(buffer);
    m_soundPool[slot].setVolume(effectiveVolume);
    m_soundPool[slot].play();
}

void SoundManager::playMusic(const std::string& filename, bool loop) {
    m_currentMusicFile = filename;
    if (!m_music.openFromFile(filename)) {
        std::cerr << "[SoundManager] Failed to load music: " << filename << std::endl;
        return;
    }
    m_music.setLoop(loop);
    applyVolumes();
    m_music.play();
}

void SoundManager::stopMusic() {
    m_music.stop();
}

void SoundManager::pauseMusic() {
    m_music.pause();
}

void SoundManager::resumeMusic() {
    if (m_muted) return;
    applyVolumes();
    m_music.play();
}

void SoundManager::toggleMute() {
    m_muted = !m_muted;
    applyVolumes();
}

void SoundManager::setMuted(bool muted) {
    m_muted = muted;
    applyVolumes();
}

bool SoundManager::isMuted() const {
    return m_muted;
}

void SoundManager::setMasterVolume(float percent) {
    m_masterVolume = std::clamp(percent, 0.0f, 100.0f);
    if (m_masterVolume > 0.0f && m_muted) {
        m_muted = false;
    }
    applyVolumes();
}

float SoundManager::getMasterVolume() const {
    return m_masterVolume;
}

void SoundManager::setSoundVolume(float volume) {
    m_soundVolume = std::clamp(volume, 0.0f, 100.0f);
    applyVolumes();
}

void SoundManager::setMusicVolume(float volume) {
    m_musicVolume = std::clamp(volume, 0.0f, 100.0f);
    applyVolumes();
}

float SoundManager::getSoundVolume() const {
    return m_soundVolume;
}

float SoundManager::getMusicVolume() const {
    return m_musicVolume;
}

void SoundManager::applySettings(const AudioSettings& settings) {
    m_muted = settings.muted;
    m_masterVolume = std::clamp(settings.masterVolume, 0.0f, 100.0f);
    m_musicVolume = std::clamp(settings.musicVolume, 0.0f, 100.0f);
    m_soundVolume = std::clamp(settings.sfxVolume, 0.0f, 100.0f);
    if (!m_musicTracks.empty()) {
        m_currentTrackIdx = std::min(settings.musicTrack, m_musicTracks.size() - 1);
    }
    applyVolumes();
}

void SoundManager::applyVolumes() {
    float effectiveMusicVol = m_muted ? 0.0f : (m_musicVolume * m_masterVolume) / 100.0f;
    float effectiveSoundVol = m_muted ? 0.0f : (m_soundVolume * m_masterVolume) / 100.0f;
    m_music.setVolume(effectiveMusicVol);
    for (sf::Sound& sound : m_soundPool) {
        sound.setVolume(effectiveSoundVol);
    }
}

const std::vector<std::pair<std::string, std::string>>& SoundManager::getMusicTracks() const {
    return m_musicTracks;
}

size_t SoundManager::getCurrentTrackIndex() const {
    return m_currentTrackIdx;
}

std::string SoundManager::getCurrentTrackName() const {
    if (m_currentTrackIdx < m_musicTracks.size()) {
        return m_musicTracks[m_currentTrackIdx].first;
    }
    return "Unknown";
}

void SoundManager::nextTrack() {
    if (m_musicTracks.empty()) return;
    m_currentTrackIdx = (m_currentTrackIdx + 1) % m_musicTracks.size();
    selectTrack(m_currentTrackIdx);
}

void SoundManager::prevTrack() {
    if (m_musicTracks.empty()) return;
    m_currentTrackIdx = (m_currentTrackIdx + m_musicTracks.size() - 1) % m_musicTracks.size();
    selectTrack(m_currentTrackIdx);
}

void SoundManager::selectTrack(size_t index) {
    if (index >= m_musicTracks.size()) return;
    m_currentTrackIdx = index;
    const std::string& targetFile = m_musicTracks[m_currentTrackIdx].second;
    if (m_currentMusicFile != targetFile || m_music.getStatus() != sf::Music::Status::Playing) {
        playMusic(targetFile, true);
    }
}
