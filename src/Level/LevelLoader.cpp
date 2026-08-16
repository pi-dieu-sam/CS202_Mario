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
#include <algorithm>

namespace {

// True for any character that results in a solid Tile or Block -- used by
// the spawn/flagpole auto-placement fallback below, not by the main parse.
bool isSolidChar(char c) {
    switch (c) {
        case 'X':
        case '<': case '>': case '[': case ']':
        case 'B': case 'b':
        case 'M': case 'F': case 's':
        case '(': case '{': case '\\': case ')': case '}': case '/':
        case 'Q': case '2': case '3': case '4':
        case '6': case 'S': case '7': case '5':
            return true;
        default:
            return false;
    }
}

} // namespace

LevelLoader::LevelData LevelLoader::loadLevel(const std::string& filename,
                                              LevelTheme theme,
                                              bool autoPlaceFlagpole) {
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

    // Align level rows to the bottom of the window (19 tile rows tall).
    // VGLC maps are 14 rows tall; adding a rowOffset of (19 - 14 = 5) rows
    // grounds the level flush to the bottom of the screen (Y = 576), leaving
    // top rows 0..4 as open sky for jumping, camera, and cloud scenery.
    const int targetRows = static_cast<int>(WINDOW_HEIGHT / TILE_SIZE); // 19 rows
    const int rowOffset = (targetRows > rows) ? (targetRows - rows) : 0;

    data.width  = cols * TILE_SIZE;
    data.height = std::max(rows + rowOffset, targetRows) * TILE_SIZE;

    bool foundSpawn = false;

    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < static_cast<int>(lines[row].size()); col++) {
            char c = lines[row][col];
            float x = col * TILE_SIZE;
            float y = (row + rowOffset) * TILE_SIZE;

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

                // Castle pieces ('Q','2','3','4','6','S','7','5') — each char
                // is one 32x32 tile cut from Castle_piece.png. Assemble a
                // castle with "Q234" above "6S75".
                case 'Q': case '2': case '3': case '4':
                case '6': case 'S': case '7': case '5':
                {
                    auto piece = EntityFactory::createCastlePiece(c, x, y, theme);
                    if (piece) data.tiles.push_back(std::move(piece));
                    break;
                }

                // Ward pipe pieces ('(','{','\\',')','}','/') — each char is
                // one 32x32 tile cut from WardPipe_piece.png. Stack the top
                // row ('(','{','\\') above the bottom row (')','}','/').
                case '(': case '{': case '\\':
                case ')': case '}': case '/':
                {
                    auto piece = EntityFactory::createWardPipePiece(c, x, y, theme);
                    if (piece) data.tiles.push_back(std::move(piece));
                    break;
                }

                // ── Blocks ──
                case '?': case 'M': case 'F': case 's':
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
                case '@': // Player 1 spawn
                    data.playerSpawn = {x, y};
                    foundSpawn = true;
                    break;

                case '9': // Player 2 spawn (PvP/Co-op)
                    data.player2Spawn = {x, y};
                    data.hasPlayer2Spawn = true;
                    break;

                case 'f': // Flagpole (end of level)
                    data.flagpole = std::make_unique<Flagpole>(x, y);
                    break;

                default:
                    break;
            }
        }
    }

    // Auto-place spawn point and flagpole on ground level if not explicitly provided
    int groundRow = rows - 1;
    auto rowAt = [&](int r, int c) -> char {
        if (r < 0 || r >= rows || c < 0 || c >= static_cast<int>(lines[r].size()))
            return '-';
        return lines[r][c];
    };

    if (!foundSpawn) {
        for (int col = 2; col < cols; col++) {
            if (isSolidChar(rowAt(groundRow, col)) &&
                !isSolidChar(rowAt(groundRow - 1, col)) &&
                !isSolidChar(rowAt(groundRow - 2, col))) {
                data.playerSpawn = {static_cast<float>(col) * TILE_SIZE,
                                     static_cast<float>(groundRow - 2 + rowOffset) * TILE_SIZE};
                break;
            }
        }
    }

    if (autoPlaceFlagpole && !data.flagpole) {
        const float flagY =
            static_cast<float>(groundRow - 1 + rowOffset) * TILE_SIZE;
        // Stand the flagpole 5 tiles before the right edge of the map; if that
        // spot is blocked by a solid object (no open sky above), scan left for
        // a clear spot with solid ground below and sky above.
        const int margin = 5;
        for (int col = cols - 1 - margin; col > 0; col--) {
            if (isSolidChar(rowAt(groundRow, col)) && !isSolidChar(rowAt(groundRow - 1, col))) {
                data.flagpole = std::make_unique<Flagpole>(
                    static_cast<float>(col) * TILE_SIZE, flagY);
                break;
            }
        }
    }

    return data;
}