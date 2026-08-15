#pragma once
#include "LevelTheme.hpp"
#include <string>
#include <vector>
#include <memory>
#include <SFML/Graphics.hpp>

class Tile;
class Block;
class Enemy;
class Item;
class Flagpole;
class Player;

/// LevelLoader — parses VGLC-format ("Video Game Level Corpus") text level
/// files (see https://github.com/TheVGLC/TheVGLC) and creates entities via
/// EntityFactory. Files can be dropped in from that dataset unmodified.
///
/// Core VGLC tile legend:
/// - '-' = empty                   'X' = ground
/// - 'S' = breakable brick         '?' = question block (contains a coin)
/// - 'Q' = already-used question block (solid, spent -- no item, no bump)
/// - 'E' = enemy, type unspecified by the format -- defaults to Goomba
/// - '<' '>' = pipe top-left/right   '[' ']' = pipe body-left/right
/// - 'o' = coin
/// - 'B' 'b' = cannon top/bottom -- no projectile mechanic exists here,
///   rendered as a solid block stand-in
/// - 'P' = forked/warp pipe (two mouths: up + left). One char, occupies a
///   (2*FORKED_PIPE_SCALE) x (2*FORKED_PIPE_SCALE) tile area (4x4 at the
///   default 2x scale) -- assembled from a 1-tile head and a 2-tile-wide base.
/// - 'c' = small castle, 'C' = large castle. One char, the bottom-left corner
///   of the castle box (place it on the ground row and the castle rises up):
///   'c' is 3x3 tiles, 'C' is 5x6 tiles. Rendered as solid 1-tile strips so
///   collision holds at every height.
///
/// Extensions (not part of VGLC's own alphabet, safe to hand-add to a file):
/// - '@' = explicit player spawn      'f' = explicit flagpole
/// - 'k' = Koopa (override for 'E')   'r' = Piranha Plant (override for 'E')
/// - 'M' = question block w/ Mushroom   'F' = w/ Fire Flower   's' = w/ Star
///
/// VGLC has no notion of a spawn point or a flagpole. If a file has no '@'
/// or 'f', LevelLoader places them automatically: spawn near the left edge,
/// flagpole 3 tiles before the castle ('c'/'C') when one exists, otherwise
/// near the right edge. Both sit one row above the ground row.
class LevelLoader {
public:
    struct LevelData {
        std::vector<std::unique_ptr<Tile>>   tiles;
        std::vector<std::unique_ptr<Block>>  blocks;
        std::vector<std::unique_ptr<Enemy>>  enemies;
        std::vector<std::unique_ptr<Item>>   items;
        std::unique_ptr<Flagpole>            flagpole;
        sf::Vector2f                         playerSpawn  = {100.0f, 100.0f};
        sf::Vector2f                         player2Spawn = {200.0f, 100.0f}; ///< P2 spawn (@2 in map)
        bool                                 hasPlayer2Spawn = false;
        int                                  castleCol = -1;  ///< column of the castle char ('c'/'C'), if any
        float                                width  = 0.0f;
        float                                height = 0.0f;
    };

    /// Parse a level file and return all entities.
    static LevelData loadLevel(const std::string& filename, LevelTheme theme,
                               bool autoPlaceFlagpole = true);
};