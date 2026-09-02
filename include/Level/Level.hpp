#pragma once
#include "LevelTheme.hpp"
#include "Core/GameSnapshot.hpp"
#include "Background.hpp"
#include "TileGrid.hpp"
#include "../Entities/Block.hpp"
#include "../Entities/Enemy.hpp"
#include "../Entities/Fireball.hpp"
#include "../Entities/BowserFireball.hpp"
#include "../Entities/Flagpole.hpp"
#include "../Entities/Item.hpp"
#include "../Entities/Player.hpp"
#include "../Entities/Tile.hpp"
#include "../Entities/Escalater.hpp"
#include "../Entities/FireBar.hpp"
#include "../Entities/LavaFireball.hpp"
#include <optional>
#include <memory>
#include <string>
#include <vector>

/// Level — holds all entities for a single game level.
/// Manages updates, rendering, and collision checking.
class Level {
  friend class SnapshotAccess;
public:
  Level();
  ~Level();

  /// Load level from a data file.
  bool loadFromFile(const std::string &filename,
                    const std::string &characterName, LevelTheme theme,
                    bool autoPlaceFlagpole = true);

  /// Update all entities.
  void update(float dt);

  /// Update only the player completion animation and flag. This keeps normal
  /// physics, enemies, item collection, and collisions frozen during the
  /// end-of-level cutscene.
  void updateCompletion(float dt);

  /// Render all entities. cameraCenterX drives the parallax background.
  /// Pipe transitions draw players below terrain so the pipe can hide them.
  void render(sf::RenderWindow &window, float cameraCenterX,
              bool playersBehindTerrain = false);

  /// Get the player pointers (owned by this level).
  Player *getPlayer() const;
  Player *getPlayer2() const;
  Flagpole *getFlagpole() const;

  /// Get level dimensions in pixels.
  float getWidth() const;
  float getHeight() const;

  /// Add a fireball to the level.
  void addFireball(std::unique_ptr<Fireball> fireball);

  /// Add a spawned item (from blocks).
  void addItem(std::unique_ptr<Item> item);

  /// Return the bounds of an enterable pipe under the player, if any.
  std::optional<sf::FloatRect> getEnterablePipeBounds(const Player &player) const;

  /// Return the bounds of a pipe currently touched by the player, if any.
  std::optional<sf::FloatRect> getTouchedPipeBounds(const Player &player) const;

  /// Return the horizontal Ward-pipe entrance currently touched by the
  /// player. These pipes are entered from the side rather than from above.
  std::optional<sf::FloatRect> getHorizontalPipeEntranceBounds(const Player &player) const;

  /// Return a complete vertical pipe whose left edge is in the given map
  /// column. Used by scripted pipe routes to choose an exact destination.
  std::optional<sf::FloatRect> getPipeBoundsAtColumn(int column) const;

  /// Return the player anchor position centred on the castle door (the `4` /
  /// `5` tile pair), if this map contains a castle.
  std::optional<sf::Vector2f> getCastleDoorEntryPosition() const;

  /// Check if the level is complete.
  bool isComplete() const;

  /// Capture or restore the mutable state of a level. Static terrain is always
  /// rebuilt from the trusted map file; these methods cover every object whose
  /// gameplay state can change while the player is running the level.
  SaveData::LevelState captureSnapshot() const;
  bool restoreSnapshot(const SaveData::LevelState &snapshot);

private:
  /// Main collision loop.
  void handleCollisions(float dt);
  void handlePlayerCollisions(Player* player, float dt);
  void removeInactiveEntities();

  std::unique_ptr<Player> m_player;
  std::unique_ptr<Player> m_player2;
  std::vector<std::unique_ptr<Tile>> m_tiles;
  TileGrid m_tileGrid;
  std::vector<std::unique_ptr<Block>> m_blocks;
  std::vector<std::unique_ptr<Enemy>> m_enemies;
  std::vector<std::unique_ptr<Item>> m_items;
  std::vector<std::unique_ptr<Fireball>> m_fireballs;
  std::vector<std::unique_ptr<BowserFireball>> m_bowserFireballs;
  std::vector<std::unique_ptr<Escalater>>  m_escalaters;
  std::vector<std::unique_ptr<FireBar>>    m_fireBars;
  std::vector<std::unique_ptr<LavaFireball>> m_lavaFireballs;
  std::unique_ptr<Flagpole> m_flagpole;

  float m_width = 0.0f;
  float m_height = 0.0f;

  Background m_background;
};
