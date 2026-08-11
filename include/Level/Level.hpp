#pragma once
#include "LevelTheme.hpp"
#include "Background.hpp"
#include "TileGrid.hpp"
#include "../Entities/Block.hpp"
#include "../Entities/Enemy.hpp"
#include "../Entities/Fireball.hpp"
#include "../Entities/Flagpole.hpp"
#include "../Entities/Item.hpp"
#include "../Entities/Player.hpp"
#include "../Entities/Tile.hpp"
#include <memory>
#include <string>
#include <vector>

/// Level — holds all entities for a single game level.
/// Manages updates, rendering, and collision checking.
class Level {
public:
  Level();
  ~Level();

  /// Load level from a data file.
  bool loadFromFile(const std::string &filename,
                    const std::string &characterName, LevelTheme theme);

  /// Update all entities.
  void update(float dt);

  /// Render all entities. cameraCenterX drives the parallax background.
  void render(sf::RenderWindow &window, float cameraCenterX);

  /// Get the player pointer (owned by this level).
  Player *getPlayer() const;

  /// Get level dimensions in pixels.
  float getWidth() const;
  float getHeight() const;

  /// Add a fireball to the level.
  void addFireball(std::unique_ptr<Fireball> fireball);

  /// Add a spawned item (from blocks).
  void addItem(std::unique_ptr<Item> item);

  /// Check if the level is complete.
  bool isComplete() const;

private:
  void handleCollisions(float dt);
  void removeInactiveEntities();

  std::unique_ptr<Player> m_player;
  std::vector<std::unique_ptr<Tile>> m_tiles;
  TileGrid m_tileGrid;
  std::vector<std::unique_ptr<Block>> m_blocks;
  std::vector<std::unique_ptr<Enemy>> m_enemies;
  std::vector<std::unique_ptr<Item>> m_items;
  std::vector<std::unique_ptr<Fireball>> m_fireballs;
  std::unique_ptr<Flagpole> m_flagpole;

  float m_width = 0.0f;
  float m_height = 0.0f;

  Background m_background;
};
