#include "Core/AssetManager.hpp"
#include <iostream>
#include <fstream>
#include <vector>

// Forward declaration of STBGif wrapper functions defined in stb_image_impl.cpp
namespace STBGif {
unsigned char* loadGifFromMemory(const unsigned char* buffer, int len,
                                 int** delays, int* x, int* y, int* z,
                                 int* comp, int req_comp);
void freeGifData(void* ptr);
}

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

const std::vector<sf::Texture>& AssetManager::getGifFrames(const std::string& filename) {
  // Return cached frames if already loaded.
  auto it = m_gifFrames.find(filename);
  if (it != m_gifFrames.end()) {
    return it->second;
  }

  std::vector<sf::Texture>& frames = m_gifFrames[filename];

  // ── Try STBGif loader ──────────────────────────────────────────────────
  // Read the raw file bytes first.
  std::ifstream file(filename, std::ios::binary | std::ios::ate);
  bool gifDecoded = false;

  if (file.is_open()) {
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<unsigned char> fileData(static_cast<size_t>(size));
    if (file.read(reinterpret_cast<char*>(fileData.data()), size)) {
      int* delays = nullptr;
      int frameCount = 0, w = 0, h = 0, comp = 0;

      // STBGif::loadGifFromMemory returns all frames tightly packed:
      // frameCount × w × h × 4 bytes (RGBA).
      unsigned char* gifData = STBGif::loadGifFromMemory(
          fileData.data(), static_cast<int>(fileData.size()),
          &delays, &w, &h, &frameCount, &comp, 4);

      if (gifData && frameCount > 0 && w > 0 && h > 0) {
        size_t frameBytes = static_cast<size_t>(w) * static_cast<size_t>(h) * 4;
        frames.resize(static_cast<size_t>(frameCount));

        for (int i = 0; i < frameCount; ++i) {
          sf::Image img;
          img.create(static_cast<unsigned>(w), static_cast<unsigned>(h),
                     gifData + static_cast<size_t>(i) * frameBytes);
          if (!frames[static_cast<size_t>(i)].loadFromImage(img)) {
            std::cerr << "[AssetManager] GIF frame " << i
                      << " texture load failed: " << filename << "\n";
          }
        }
        gifDecoded = true;
      }
      // Free unconditionally: a structurally-parseable but degenerate GIF
      // (gifData non-null yet frameCount/w/h <= 0) used to skip this and
      // leak the decode buffer, since the free lived inside the branch above.
      if (gifData) STBGif::freeGifData(gifData);
      if (delays) STBGif::freeGifData(delays);
    }
  }

  // ── Fallback: use SFML's single-frame load ───────────────────────────────
  if (!gifDecoded || frames.empty()) {
    std::cerr << "[AssetManager] GIF multi-frame decode failed, using SFML fallback: "
              << filename << "\n";
    frames.resize(1);
    if (!frames[0].loadFromFile(filename)) {
      std::cerr << "[AssetManager] SFML fallback also failed: " << filename << "\n";
    }
  }

  return frames;
}

int AssetManager::getGifFrameCount(const std::string& filename) {
  return static_cast<int>(getGifFrames(filename).size());
}
