#pragma once
#include "../Level/LevelTheme.hpp"
#include <memory>
#include <vector>
#include <SFML/Graphics.hpp>

// Forward declarations
class Enemy;
class Item;
class Tile;
class Block;
class Player;

/// Enemy types for factory creation.
enum class EnemyType {
    Goomba,
    Koopa,
    PiranhaPlant
};

/// Item types for factory creation.
enum class ItemType {
    Coin,
    Mushroom,
    FireFlower,
    Star
};

/// EntityFactory — Factory pattern.
/// Creates game entities dynamically based on type enums,
/// used by LevelLoader to populate levels from data files.
class EntityFactory {
public:
    static std::unique_ptr<Enemy> createEnemy(EnemyType type, sf::Vector2f pos, LevelTheme theme);
    static std::unique_ptr<Item>  createItem(ItemType type, sf::Vector2f pos, LevelTheme theme);
    static std::unique_ptr<Tile>  createTile(char tileChar, float x, float y, LevelTheme theme);
    static std::unique_ptr<Block> createBlock(char blockChar, float x, float y, LevelTheme theme);
    static std::unique_ptr<Player> createPlayer(const std::string& characterName, sf::Vector2f pos);

    /// Create the two pieces of a fork/warp pipe ('P'): the head (mouth up,
    /// right column) and the 2-tile-wide horizontal base strip below it.
    /// `x, y` is the top-left corner of the assembled 2x2 tile area.
    static std::vector<std::unique_ptr<Tile>> createForkedPipe(float x, float y, LevelTheme theme);

    /// Create a castle ('c' small / 'C' large) as one-tile-tall solid strips
    /// stacked upward. `x, y` is the bottom-left corner of the castle box —
    /// place the character on the ground row and the castle grows upward
    /// (so a level can drop a castle straight onto the floor without counting
    /// rows). Returns H strips at y, y-32, y-64, ...
    static std::vector<std::unique_ptr<Tile>> createCastle(char castleChar, float x, float y, LevelTheme theme);
};