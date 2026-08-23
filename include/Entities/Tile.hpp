#pragma once
#include "GameObject.hpp"
#include "../Level/LevelTheme.hpp"
#include "../Physics/PhysicsConstants.hpp"
#include <SFML/Graphics.hpp>

/// TileType — identifies the type of tile for rendering and collision.
enum class TileType {
  Empty,
  Ground,
  Brick,
  PipeTopLeft,
  PipeTopRight,
  PipeBodyLeft,
  PipeBodyRight,
  CastleBlock,
  Underground,
  /// Two-piece fork/warp pipe ('P'): the head (mouth up, 1 tile) and the
  /// horizontal base strip (2 tiles wide). Both crop the same 64x64
  /// SMBWarpPipeForked.png sprite.
  ForkedPipeHead,
  ForkedPipeBase,
  /// Castle ('c' small / 'C' large): rendered as several one-tile-tall solid
  /// strips stacked vertically (subIndex counts up from the bottom strip) so
  /// TileGrid collision works at every height.
  CastleSmall,
  CastleLarge
};

/// Tile — static, collidable terrain piece (ground, pipes, etc.).
class Tile : public GameObject {
public:
  Tile();
  /// subIndex selects a vertical slice for multi-piece tiles (castles):
  /// 0 = bottom strip, rising upward. Ignored by ordinary tiles.
  Tile(TileType tileType, float x, float y, LevelTheme theme, int subIndex = 0);

  void update(float dt) override;
  void draw(sf::RenderWindow &window) override;
  sf::FloatRect getBounds() const override;

  TileType getTileType() const;

private:
  TileType m_tileType = TileType::Empty;
  sf::Vector2f m_size = {TILE_SIZE, TILE_SIZE};
  sf::Sprite m_sprite;
};
