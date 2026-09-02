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
class Escalater;
class FireBar;
class LavaFireball;

/// LevelLoader — parses VGLC-format ("Video Game Level Corpus") text level
/// files (see https://github.com/TheVGLC/TheVGLC) and creates entities via
/// EntityFactory. Files can be dropped in from that dataset unmodified.
///
/// Core VGLC tile legend:
/// - '-' = empty                   'X' = ground
/// - '?' = question block (contains a coin)
/// - 'E' = enemy, type unspecified by the format -- defaults to Goomba
/// - '<' '>' = pipe top-left/right   '[' ']' = pipe body-left/right
/// - 'o' = coin
/// - 'b' = legacy cannon-bottom tile, rendered as a solid block stand-in
/// - Ward pipe pieces: '(' '{' '\' (top row) and ')' '}' '/' (bottom row) —
///   each char is one 16x16 cell of WardPipe_piece.png scaled to one 32x32
///   tile. Assemble a pipe by stacking top-row pieces above bottom-row ones.
/// - Castle pieces: 'Q','2','3','4' (top row) and '6','S','7','5' (bottom
///   row) — each char is one 16x16 cell of Castle_piece.png scaled to one
///   32x32 tile. Assemble a castle with "Q234" above "6S75".
///
/// Extensions (not part of VGLC's own alphabet, safe to hand-add to a file):
/// - '@' = explicit player spawn      '9' = player 2 spawn (PvP/Co-op)
/// - 'f' = explicit flagpole
/// - 'K' = Koopa   'T' = flying Troopa   'B' = Bowser   'P' = Piranha Plant
/// - 'M' = question block w/ Mushroom   'F' = w/ Fire Flower   's' = w/ Star
/// - 'E' = vertical escalater; 'e' = horizontal escalater (five tiles each way)
/// - 'O' = fire-bar anchor with eight rotating, lethal fireballs
/// - 'i' = top lava tile with a repeating fireball launcher (waits 3 sec/arc)
///   (in main level2.txt only, each pipe top <> is also a launcher)
/// - 'V' = vine top; a non-solid 32x32 tile rendered from VineTop.png
///
/// VGLC has no notion of a spawn point or a flagpole. If a file has no '@'
/// or 'f', LevelLoader places them automatically: spawn near the left edge,
/// flagpole 5 tiles before the right edge of the map (scanning left if that
/// spot is blocked). Both sit one row above the ground row.
class LevelLoader {
public:
    struct LevelData {
        bool                                  loaded = false;
        std::vector<std::unique_ptr<Tile>>   tiles;
        std::vector<std::unique_ptr<Block>>  blocks;
        std::vector<std::unique_ptr<Enemy>>  enemies;
        std::vector<std::unique_ptr<Item>>   items;
        std::vector<std::unique_ptr<Escalater>> escalaters;
        std::vector<std::unique_ptr<FireBar>>   fireBars;
        std::vector<std::unique_ptr<LavaFireball>> lavaFireballs;
        std::unique_ptr<Flagpole>            flagpole;
        sf::Vector2f                         playerSpawn  = {100.0f, 100.0f};
        sf::Vector2f                         player2Spawn = {200.0f, 100.0f}; ///< P2 spawn ('9' in map)
        bool                                 hasPlayer2Spawn = false;
        float                                width  = 0.0f;
        float                                height = 0.0f;
    };

    /// Parse a level file and return all entities.
    static LevelData loadLevel(const std::string& filename, LevelTheme theme,
                               bool autoPlaceFlagpole = true);
};
