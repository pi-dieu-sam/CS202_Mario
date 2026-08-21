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
  /// Castle piece: one 16x16 cell cut from Castle_piece.png (4x2 sheet,
  /// 1px gaps), scaled up to a single 32x32 tile. subIndex selects the sheet
  /// cell (0..7, reading left-to-right top-to-bottom). A castle is assembled
  /// in the level file by placing the piece characters Q 2 3 4 / 6 S 7 5
  /// (top row above bottom row).
  CastlePiece,
  /// Ward pipe piece: one 16x16 cell cut from WardPipe_piece.png (3x2 sheet,
  /// 1px gaps), scaled up to a single 32x32 tile. subIndex selects the sheet
  /// cell (0..5, reading left-to-right top-to-bottom). A pipe is assembled in
  /// the level file by placing the piece characters ( { \ (top row) above
  /// ) } / (bottom row).
  WardPipePiece,
  /// Lava tile — a 16x16 hazard texture scaled to 32x32. Kills the player
  /// on contact (not solid — the player falls through and dies).
  Lava,
  /// Flame tile — 4 animated frames (16x16 each) from flame.png (64x16 sheet),
  /// scaled to 32x32. Kills the player on contact.
  Flame
};

/// Tile — static, collidable terrain piece (ground, pipes, etc.).
class Tile : public GameObject {
public:
  Tile();
  /// subIndex selects a sheet cell for CastlePiece (0..7). Ignored by
  /// ordinary tiles.
  Tile(TileType tileType, float x, float y, LevelTheme theme, int subIndex = 0);

  void update(float dt) override;
  void draw(sf::RenderWindow &window) override;
  sf::FloatRect getBounds() const override;

  TileType getTileType() const;

private:
  TileType m_tileType = TileType::Empty;
  sf::Vector2f m_size = {TILE_SIZE, TILE_SIZE};
  sf::Sprite m_sprite;
  float m_flameAnimTimer = 0.0f;
  int   m_flameAnimFrame = 0;
};
