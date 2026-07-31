#include "Core/AssetManager.hpp"
#include <iostream>

AssetManager &AssetManager::getInstance() {
  static AssetManager instance;
  return instance;
}

sf::Texture &AssetManager::getTexture(const std::string &filename) {
  auto it = m_textures.find(filename);
  if (it != m_textures.end()) {
    return it->second;
  }

  sf::Texture &texture = m_textures[filename];
  if (!texture.loadFromFile(filename)) {
    std::cerr << "[AssetManager] Failed to load texture: " << filename
              << std::endl;
    // Return the (empty) texture anyway — avoids crash
  }
  return texture;
}

sf::Font &AssetManager::getFont(const std::string &filename) {
  auto it = m_fonts.find(filename);
  if (it != m_fonts.end()) {
    return it->second;
  }

  sf::Font &font = m_fonts[filename];
  if (!font.loadFromFile(filename)) {
    std::cerr << "[AssetManager] Failed to load font: " << filename
              << std::endl;
  }
  return font;
}

sf::SoundBuffer &AssetManager::getSoundBuffer(const std::string &filename) {
  auto it = m_soundBuffers.find(filename);
  if (it != m_soundBuffers.end()) {
    return it->second;
  }

  sf::SoundBuffer &buffer = m_soundBuffers[filename];
  if (!buffer.loadFromFile(filename)) {
    std::cerr << "[AssetManager] Failed to load sound: " << filename
              << std::endl;
  }
  return buffer;
}
