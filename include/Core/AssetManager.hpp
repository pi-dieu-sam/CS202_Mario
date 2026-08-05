#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <unordered_map>
#include <string>
#include <vector>
#include <stdexcept>

/// AssetManager — Singleton pattern.
/// Caches textures, fonts, sound buffers, and GIF animation frame sets so each
/// file is loaded only once.
class AssetManager {
public:
    static AssetManager& getInstance();

    AssetManager(const AssetManager&)            = delete;
    AssetManager& operator=(const AssetManager&) = delete;

    /// Load (if needed) and return a texture by filename.
    sf::Texture& getTexture(const std::string& filename);

    /// Load (if needed) and return a font by filename.
    sf::Font& getFont(const std::string& filename);

    /// Load (if needed) and return a sound buffer by filename.
    sf::SoundBuffer& getSoundBuffer(const std::string& filename);

    /// Load (if needed) all animation frames of a GIF file.
    /// Returns a reference to a cached vector of sf::Texture, one per frame.
    /// Falls back gracefully to a single frame via SFML if stb_image fails.
    const std::vector<sf::Texture>& getGifFrames(const std::string& filename);

    /// Returns the number of frames for a GIF (1 for non-GIFs / fallback).
    int getGifFrameCount(const std::string& filename);

private:
    AssetManager() = default;

    std::unordered_map<std::string, sf::Texture>                m_textures;
    std::unordered_map<std::string, sf::Font>                   m_fonts;
    std::unordered_map<std::string, sf::SoundBuffer>            m_soundBuffers;
    std::unordered_map<std::string, std::vector<sf::Texture>>   m_gifFrames;
};

