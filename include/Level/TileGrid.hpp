#pragma once
#include <SFML/Graphics.hpp>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

class Tile;

/// TileGrid — spatial index over a level's tiles, bucketed into TILE_SIZE
/// cells by (col, row). Lets collision checks query only the handful of
/// tiles near an entity's bounds instead of scanning every tile in the level.
class TileGrid {
public:
    /// Bucket every tile by its grid cell. Call once after a level's tiles
    /// are loaded — tiles never move or get added/removed afterward.
    void build(const std::vector<std::unique_ptr<Tile>>& tiles);

    /// Return the tiles occupying every cell that `bounds` overlaps.
    std::vector<Tile*> query(const sf::FloatRect& bounds) const;

private:
    static int64_t cellKey(int col, int row);

    std::unordered_map<int64_t, std::vector<Tile*>> m_cells;
};
