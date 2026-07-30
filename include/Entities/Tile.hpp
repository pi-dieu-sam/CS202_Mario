#pragma once
#include "GameObject.hpp"
#include "../Level/LevelTheme.hpp"
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
  Underground
};

/// Tile — static, collidable terrain piece (ground, pipes, etc.).
class Tile : public GameObject {
public:
  Tile();
  Tile(TileType tileType, float x, float y, LevelTheme theme);

  void update(float dt) override;
  void draw(sf::RenderWindow &window) override;
  sf::FloatRect getBounds() const override;

  TileType getTileType() const;

private:
  TileType m_tileType = TileType::Empty;
  sf::Sprite m_sprite;
};
