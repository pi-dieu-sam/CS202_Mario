#include "Entities/Tile.hpp"
#include "Core/AssetManager.hpp"
#include "Graphics/SpriteRegistry.hpp"
#include "Physics/PhysicsConstants.hpp"
#include <algorithm>
#include <cmath>

Tile::Tile() { m_type = ObjectType::Tile; }

Tile::Tile(TileType tileType, float x, float y, LevelTheme theme,
           int subIndex)
    : m_tileType(tileType) {
  m_type = ObjectType::Tile;
  m_position = {x, y};

  if (tileType == TileType::Empty) {
    return;
  }

  // Forked/warp pipe ('P'): the 64x64 sprite is split into two solid pieces —
  // a 1-tile head (mouth up, right column) and a 2-tile-wide horizontal base
  // strip. EntityFactory positions them so they assemble into the 2x2 pipe.
  if (tileType == TileType::ForkedPipeHead ||
      tileType == TileType::ForkedPipeBase) {
    const std::string& path = SpriteRegistry::tilePath(tileType, theme);
    sf::Texture& tex = AssetManager::getInstance().getTexture(path);

    // Each source cell of the 64x64 sprite is FORKED_PIPE_SCALE tiles big.
    const float cell = TILE_SIZE * FORKED_PIPE_SCALE;

    sf::IntRect crop;
    if (tileType == TileType::ForkedPipeHead) {
      crop = {32, 0, 32, 32};
      m_size = {cell, cell};
    } else {
      crop = {0, 32, 64, 32};
      m_size = {cell * 2.0f, cell};
    }

    SpriteRegistry::applyFrame(m_sprite, tex, crop,
                               sf::FloatRect(x, y, m_size.x, m_size.y));
    return;
  }

  // Castle ('c' small / 'C' large): the sprite fills a (W x H) tile box but
  // is split into H one-tile-tall solid strips. subIndex counts up from the
  // bottom strip; each strip crops the matching horizontal slice of the
  // source image and is scaled to one tile height.
  if (tileType == TileType::CastleSmall || tileType == TileType::CastleLarge) {
    const int boxWidth = (tileType == TileType::CastleSmall)
                             ? CASTLE_SMALL_W_TILES
                             : CASTLE_LARGE_W_TILES;
    const int stripCount = (tileType == TileType::CastleSmall)
                               ? CASTLE_SMALL_H_TILES
                               : CASTLE_LARGE_H_TILES;

    const std::string& path = SpriteRegistry::tilePath(tileType, theme);
    sf::Texture& tex = AssetManager::getInstance().getTexture(path);
    sf::Vector2u size = tex.getSize();

    // Source slice for this strip: split the source height into stripCount
    // equal slices using round-to-nearest boundaries (so the slices tile the
    // whole image without gaps or overlap).
    int sliceTop = static_cast<int>(
        std::lround((stripCount - 1 - subIndex) * size.y / stripCount));
    int sliceBottom = static_cast<int>(
        std::lround((stripCount - subIndex) * size.y / stripCount));
    sf::IntRect crop(0, sliceTop, static_cast<int>(size.x),
                     std::max(1, sliceBottom - sliceTop));

    m_size = {boxWidth * TILE_SIZE, TILE_SIZE};
    SpriteRegistry::applyFrame(m_sprite, tex, crop,
                               sf::FloatRect(x, y, m_size.x, m_size.y));
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
  return sf::FloatRect(m_position.x, m_position.y, m_size.x, m_size.y);
}

TileType Tile::getTileType() const { return m_tileType; }
