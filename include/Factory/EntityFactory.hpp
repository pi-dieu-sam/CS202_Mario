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

    /// Create a single castle piece ('Q','2','3','4','6','S','7','5'). Each
    /// char maps to one 16x16 cell of Castle_piece.png (Q=top-left, 2/3/4
    /// across the top row, 6/S/7/5 across the bottom row), scaled to one
    /// 32x32 tile at `x, y`. Assemble a castle in the level file as two rows:
    /// "Q234" above "6S75".
    static std::unique_ptr<Tile> createCastlePiece(char pieceChar, float x, float y, LevelTheme theme);

    /// Create a single ward pipe piece ('(','{','\\',')','}','/'). Each char
    /// maps to one 16x16 cell of WardPipe_piece.png ('(' '{' '\' across the
    /// top row, ')' '}' '/' across the bottom row), scaled to one 32x32 tile
    /// at `x, y`. Assemble a pipe in the level file by stacking top-row pieces
    /// above bottom-row pieces.
    static std::unique_ptr<Tile> createWardPipePiece(char pieceChar, float x, float y, LevelTheme theme);
};