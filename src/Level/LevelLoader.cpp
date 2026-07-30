#include "Level/LevelLoader.hpp"
#include "Factory/EntityFactory.hpp"
#include "Entities/Tile.hpp"
#include "Entities/Block.hpp"
#include "Entities/Enemy.hpp"
#include "Entities/Item.hpp"
#include "Entities/Flagpole.hpp"
#include "Physics/PhysicsConstants.hpp"
#include <fstream>
#include <iostream>
#include <sstream>

namespace {

// True for any character that results in a solid Tile or Block -- used by
// the spawn/flagpole auto-placement fallback below, not by the main parse.
bool isSolidChar(char c) {
    switch (c) {
        case 'X': case 'Q': case 'S': case '?':
        case '<': case '>': case '[': case ']':
        case 'B': case 'b':
        case 'M': case 'F': case 's':
            return true;
        default:
            return false;
    }
}

} // namespace

LevelLoader::LevelData LevelLoader::loadLevel(const std::string& filename, LevelTheme theme) {
    LevelData data;

    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "[LevelLoader] Failed to open: " << filename << std::endl;
        return data;
    }

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back(); // tolerate CRLF
        lines.push_back(line);
    }
    file.close();

    if (lines.empty()) return data;

    int rows = static_cast<int>(lines.size());
    int cols = 0;
    for (auto& l : lines) {
        if (static_cast<int>(l.size()) > cols) cols = static_cast<int>(l.size());
    }

    data.width  = cols * TILE_SIZE;
    data.height = rows * TILE_SIZE;

    bool foundSpawn = false;

    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < static_cast<int>(lines[row].size()); col++) {
            char c = lines[row][col];
            float x = col * TILE_SIZE;
            float y = row * TILE_SIZE;

            switch (c) {
                case '-': // Empty
                    break;

                // ── Tiles ──
                case 'X': case '<': case '>': case '[': case ']':
                case 'B': case 'b':
                {
                    auto tile = EntityFactory::createTile(c, x, y, theme);
                    if (tile) data.tiles.push_back(std::move(tile));
                    break;
                }

                // ── Blocks ──
                case 'S': case '?': case 'Q': case 'M': case 'F': case 's':
                {
                    auto block = EntityFactory::createBlock(c, x, y, theme);
                    if (block) data.blocks.push_back(std::move(block));
                    break;
                }

                // ── Enemies ── ('E' is VGLC's generic, type-unspecified enemy)
                case 'E':
                {
                    auto enemy = EntityFactory::createEnemy(EnemyType::Goomba, {x, y}, theme);
                    if (enemy) data.enemies.push_back(std::move(enemy));
                    break;
                }
                case 'k':
                {
                    auto enemy = EntityFactory::createEnemy(EnemyType::Koopa, {x, y}, theme);
                    if (enemy) data.enemies.push_back(std::move(enemy));
                    break;
                }
                case 'r':
                {
                    auto enemy = EntityFactory::createEnemy(EnemyType::PiranhaPlant, {x, y}, theme);
                    if (enemy) data.enemies.push_back(std::move(enemy));
                    break;
                }

                // ── Items ──
                case 'o': // Coin
                {
                    auto item = EntityFactory::createItem(ItemType::Coin, {x, y}, theme);
                    if (item) data.items.push_back(std::move(item));
                    break;
                }

                // ── Special (extensions -- not part of VGLC's own alphabet) ──
                case '@': // Player spawn
                    data.playerSpawn = {x, y};
                    foundSpawn = true;
                    break;

                case 'f': // Flagpole (end of level)
                    data.flagpole = std::make_unique<Flagpole>(x, y);
                    break;

                default:
                    break;
            }
        }
    }

    // VGLC has no concept of a spawn point or a flagpole. If the file didn't
    // include the '@'/'f' extension chars, place them programmatically: a
    // couple tiles in from the left edge / a few tiles in from the right
    // edge, one row above the first solid ground found there.
    int groundRow = rows - 1;
    auto rowAt = [&](int r, int c) -> char {
        if (r < 0 || r >= rows || c < 0 || c >= static_cast<int>(lines[r].size()))
            return '-';
        return lines[r][c];
    };

    if (!foundSpawn) {
        for (int col = 2; col < cols; col++) {
            // Player::getBounds() keeps a 2-tile-tall box with feet fixed at
            // m_position.y + TILE_SIZE*2 (so growing Small->Big extends
            // upward, not downward) -- spawn needs 2 clear rows above ground,
            // and the box's top-left goes 2 rows above the surface, not 1,
            // or the player's feet land a tile below ground level.
            if (isSolidChar(rowAt(groundRow, col)) &&
                !isSolidChar(rowAt(groundRow - 1, col)) &&
                !isSolidChar(rowAt(groundRow - 2, col))) {
                data.playerSpawn = {static_cast<float>(col) * TILE_SIZE,
                                     static_cast<float>(groundRow - 2) * TILE_SIZE};
                break;
            }
        }
    }

    if (!data.flagpole) {
        const int margin = 3;
        for (int col = cols - 1 - margin; col > 0; col--) {
            if (isSolidChar(rowAt(groundRow, col)) && !isSolidChar(rowAt(groundRow - 1, col))) {
                data.flagpole = std::make_unique<Flagpole>(
                    static_cast<float>(col) * TILE_SIZE,
                    static_cast<float>(groundRow - 1) * TILE_SIZE);
                break;
            }
        }
    }

    return data;
}