#pragma once
#include "../Level/LevelTheme.hpp"
#include <memory>
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
};