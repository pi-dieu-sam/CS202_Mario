#ifndef TEXTURE_MANAGER_HPP
#define TEXTURE_MANAGER_HPP

#include <SFML/Graphics.hpp>
#include <unordered_map>
#include <string>
#include <memory>
#include <iostream>

/**
 * @brief Singleton class to manage, load, and cache game textures.
 * Includes procedural fallback texture generation if image files are missing.
 */
class TextureManager {
public:
    static TextureManager& getInstance() {
        static TextureManager instance;
        return instance;
    }

    // Delete copy constructor and assignment operator for Singleton
    TextureManager(const TextureManager&) = delete;
    TextureManager& operator=(const TextureManager&) = delete;

    /**
     * @brief Load a texture from file with a specific key identifier.
     */
    bool loadTexture(const std::string& key, const std::string& filename) {
        auto texture = std::make_unique<sf::Texture>();
        if (!texture->loadFromFile(filename)) {
            std::cerr << "[TextureManager] Warning: Failed to load texture from " << filename 
                      << ". Creating fallback texture for key: " << key << std::endl;
            createFallbackTexture(key);
            return false;
        }
        m_textures[key] = std::move(texture);
        return true;
    }

    /**
     * @brief Get reference to cached texture. Returns fallback texture if key not found.
     */
    const sf::Texture& getTexture(const std::string& key) {
        auto it = m_textures.find(key);
        if (it == m_textures.end()) {
            createFallbackTexture(key);
            return *m_textures[key];
        }
        return *it->second;
    }

    /**
     * @brief Check if a texture key is registered.
     */
    bool hasTexture(const std::string& key) const {
        return m_textures.find(key) != m_textures.end();
    }

private:
    TextureManager() {
        createDefaultFallbackTextures();
    }

    ~TextureManager() = default;

    std::unordered_map<std::string, std::unique_ptr<sf::Texture>> m_textures;

    void createFallbackTexture(const std::string& key) {
        sf::Image img({32, 32}, sf::Color::Magenta);
        // Draw a checkerboard pattern for placeholder visual
        for (unsigned int x = 0; x < 32; ++x) {
            for (unsigned int y = 0; y < 32; ++y) {
                if ((x / 8 + y / 8) % 2 == 0) {
                    img.setPixel({x, y}, sf::Color(200, 0, 200));
                }
            }
        }
        auto texture = std::make_unique<sf::Texture>();
        if (texture->loadFromImage(img)) {
            m_textures[key] = std::move(texture);
        }
    }

    void createDefaultFallbackTextures() {
        // Pre-create basic color placeholder textures for common tiles/entities
        createFallbackTexture("mario_small");
        createFallbackTexture("luigi_small");
        createFallbackTexture("goomba");
        createFallbackTexture("koopa");
        createFallbackTexture("coin");
        createFallbackTexture("mushroom");
        createFallbackTexture("fireflower");
        createFallbackTexture("tile_ground");
        createFallbackTexture("tile_brick");
        createFallbackTexture("tile_question");
    }
};

#endif // TEXTURE_MANAGER_HPP
