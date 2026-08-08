#include "Level/TileGrid.hpp"
#include "Entities/Tile.hpp"
#include "Physics/PhysicsConstants.hpp"
#include <cmath>

int64_t TileGrid::cellKey(int col, int row) {
    return (static_cast<int64_t>(row) << 32) ^ static_cast<int64_t>(static_cast<uint32_t>(col));
}

void TileGrid::build(const std::vector<std::unique_ptr<Tile>>& tiles) {
    m_cells.clear();
    for (const auto& tile : tiles) {
        sf::FloatRect bounds = tile->getBounds();
        int col = static_cast<int>(std::floor(bounds.left / TILE_SIZE));
        int row = static_cast<int>(std::floor(bounds.top / TILE_SIZE));
        m_cells[cellKey(col, row)].push_back(tile.get());
    }
}

std::vector<Tile*> TileGrid::query(const sf::FloatRect& bounds) const {
    std::vector<Tile*> result;

    int minCol = static_cast<int>(std::floor(bounds.left / TILE_SIZE));
    int maxCol = static_cast<int>(std::floor((bounds.left + bounds.width) / TILE_SIZE));
    int minRow = static_cast<int>(std::floor(bounds.top / TILE_SIZE));
    int maxRow = static_cast<int>(std::floor((bounds.top + bounds.height) / TILE_SIZE));

    for (int row = minRow; row <= maxRow; ++row) {
        for (int col = minCol; col <= maxCol; ++col) {
            auto it = m_cells.find(cellKey(col, row));
            if (it != m_cells.end()) {
                result.insert(result.end(), it->second.begin(), it->second.end());
            }
        }
    }

    return result;
}
