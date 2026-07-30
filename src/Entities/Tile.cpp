#include "Entities/Tile.hpp"
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

  SpriteRegistry::applyFrame(m_sprite, SpriteRegistry::tilePath(tileType, theme),
                              sf::FloatRect(x, y, TILE_SIZE, TILE_SIZE));
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
