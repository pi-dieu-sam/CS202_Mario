#include "Entities/Tile.hpp"
#include "Core/AssetManager.hpp"
#include "Graphics/SpriteRegistry.hpp"
#include "Physics/PhysicsConstants.hpp"

namespace {
/// Crop one 16x16 cell out of a sprite sheet with `cols` columns and 1px gaps
/// between cells, then scale it to fill `box` (one 32x32 tile). Used by
/// CastlePiece (4 cols) and WardPipePiece (3 cols).
void applySheetCell(sf::Sprite &sprite, sf::Texture &tex, int cell, int cols,
                    const sf::FloatRect &box) {
  constexpr int CELL = 16;
  constexpr int STEP = CELL + 1; // 16px cell + 1px gap

  const int cellCol = cell % cols;
  const int cellRow = cell / cols;
  sf::IntRect crop(cellCol * STEP, cellRow * STEP, CELL, CELL);
  SpriteRegistry::applyFrame(sprite, tex, crop, box);
}
} // namespace

/// Default-construct an empty tile at the origin.
Tile::Tile() { m_type = ObjectType::Tile; }

Tile::Tile(TileType tileType, float x, float y, LevelTheme theme,
           int subIndex)
    : m_tileType(tileType), m_subIndex(subIndex) {
  m_type = ObjectType::Tile;
  m_position = {x, y};

  if (tileType == TileType::Empty) {
    return;
  }

  // Castle piece: one 16x16 cell cut from the 4x2 Castle_piece.png sheet
  // (1px gaps between cells), scaled up to a single 32x32 tile. subIndex is
  // the 0-based sheet-cell index (row-major: 0-3 top row, 4-7 bottom row).
  if (tileType == TileType::CastlePiece) {
    const std::string& path = SpriteRegistry::tilePath(tileType, theme);
    sf::Texture& tex = AssetManager::getInstance().getTexture(path);

    m_size = {TILE_SIZE, TILE_SIZE};
    applySheetCell(m_sprite, tex, subIndex, 4,
                   sf::FloatRect(x, y, m_size.x, m_size.y));
    return;
  }

  // Ward pipe piece: one 16x16 cell cut from the 3x2 WardPipe_piece.png sheet
  // (1px gaps between cells), scaled up to a single 32x32 tile. subIndex is
  // the 0-based sheet-cell index (row-major: 0-2 top row, 3-5 bottom row).
  if (tileType == TileType::WardPipePiece) {
    const std::string& path = SpriteRegistry::tilePath(tileType, theme);
    sf::Texture& tex = AssetManager::getInstance().getTexture(path);

    m_size = {TILE_SIZE, TILE_SIZE};
    applySheetCell(m_sprite, tex, subIndex, 3,
                   sf::FloatRect(x, y, m_size.x, m_size.y));
    return;
  }

  // Lava: a 16x16 texture scaled up to fill the 32x32 tile.
  if (tileType == TileType::Lava) {
    m_size = {TILE_SIZE, TILE_SIZE};
    SpriteRegistry::applyFrame(m_sprite, SpriteRegistry::lavaPath(),
                               sf::FloatRect(x, y, TILE_SIZE, TILE_SIZE));
    return;
  }

  // Flame: 4 frames of 16x16 from flame.png (64x16 sheet), scaled to 32x32.
  if (tileType == TileType::Flame) {
    m_size = {TILE_SIZE, TILE_SIZE};
    SpriteRegistry::applySheetFrame(m_sprite, SpriteRegistry::flamePath(),
                                    0, 16, 0,
                                    sf::FloatRect(x, y, TILE_SIZE, TILE_SIZE));
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
  if (m_tileType == TileType::Flame) {
    constexpr float FLAME_FRAME_TIME = 0.12f;
    m_flameAnimTimer += dt;
    if (m_flameAnimTimer >= FLAME_FRAME_TIME) {
      m_flameAnimTimer = 0.0f;
      m_flameAnimFrame = (m_flameAnimFrame + 1) % 4;
    }
  }
}

void Tile::draw(sf::RenderWindow &window) {
  if (!m_active || m_tileType == TileType::Empty)
    return;
  if (m_tileType == TileType::Flame) {
    SpriteRegistry::applySheetFrame(m_sprite, SpriteRegistry::flamePath(),
                                    m_flameAnimFrame, 16, 0,
                                    sf::FloatRect(m_position.x, m_position.y,
                                                  TILE_SIZE, TILE_SIZE));
  }
  window.draw(m_sprite);
}

sf::FloatRect Tile::getBounds() const {
  return sf::FloatRect(m_position.x, m_position.y, m_size.x, m_size.y);
}

TileType Tile::getTileType() const { return m_tileType; }
int Tile::getSubIndex() const { return m_subIndex; }
