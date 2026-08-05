#include "Entities/Tile.hpp"
#include "Core/AssetManager.hpp"
#include "Graphics/SpriteRegistry.hpp"
#include "Physics/PhysicsConstants.hpp"

Tile::Tile() { m_type = ObjectType::Tile; }

Tile::Tile(TileType tileType, float x, float y, LevelTheme theme)
    : m_tileType(tileType) {
  m_type = ObjectType::Tile;
  m_position = {x, y};

  if (tileType == TileType::Empty) {
    return;
  }

  // Pipe tiles each represent one quadrant of the assembled pipe texture.
  // Loading the whole texture and selecting only the correct sub-rect avoids
  // the "bundle of 4" bug where 4 full-sized pipes overlapped each other.
  bool isPipe = (tileType == TileType::PipeTopLeft  ||
                 tileType == TileType::PipeTopRight ||
                 tileType == TileType::PipeBodyLeft ||
                 tileType == TileType::PipeBodyRight);

  if (isPipe) {
    const std::string& path = SpriteRegistry::tilePath(tileType, theme);
    sf::Texture& tex = AssetManager::getInstance().getTexture(path);
    sf::Vector2u size = tex.getSize();

    // Half-dimensions of the texture — each quadrant is one tile-sized cell.
    int halfW = static_cast<int>(size.x) / 2;
    int halfH = static_cast<int>(size.y) / 2;

    sf::IntRect crop;
    switch (tileType) {
      case TileType::PipeTopLeft:   crop = {0,     0,     halfW, halfH}; break;
      case TileType::PipeTopRight:  crop = {halfW, 0,     halfW, halfH}; break;
      case TileType::PipeBodyLeft:  crop = {0,     halfH, halfW, halfH}; break;
      case TileType::PipeBodyRight: crop = {halfW, halfH, halfW, halfH}; break;
      default:                      crop = {0, 0, static_cast<int>(size.x), static_cast<int>(size.y)}; break;
    }

    SpriteRegistry::applyFrame(m_sprite, tex, crop,
                               sf::FloatRect(x, y, TILE_SIZE, TILE_SIZE));
  } else {
    SpriteRegistry::applyFrame(m_sprite, SpriteRegistry::tilePath(tileType, theme),
                               sf::FloatRect(x, y, TILE_SIZE, TILE_SIZE));
  }
}

void Tile::update(float dt) {
  // Static — no updates needed
}

void Tile::draw(sf::RenderWindow &window) {
  if (!m_active || m_tileType == TileType::Empty)
    return;
  window.draw(m_sprite);
}

sf::FloatRect Tile::getBounds() const {
  return sf::FloatRect(m_position.x, m_position.y, TILE_SIZE, TILE_SIZE);
}

TileType Tile::getTileType() const { return m_tileType; }
