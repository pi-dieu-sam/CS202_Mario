#include "Factory/EntityFactory.hpp"
#include "Entities/Goomba.hpp"
#include "Entities/Koopa.hpp"
#include "Entities/PiranhaPlant.hpp"
#include "Entities/Coin.hpp"
#include "Entities/Mushroom.hpp"
#include "Entities/FireFlower.hpp"
#include "Entities/Star.hpp"
#include "Entities/Tile.hpp"
#include "Entities/Block.hpp"
#include "Entities/Mario.hpp"
#include "Entities/Luigi.hpp"
#include "AI/PatrolStrategy.hpp"
#include "AI/ChaseStrategy.hpp"

std::unique_ptr<Enemy> EntityFactory::createEnemy(EnemyType type, sf::Vector2f pos, LevelTheme theme) {
    std::unique_ptr<Enemy> enemy;

    switch (type) {
        case EnemyType::Goomba:
            enemy = std::make_unique<Goomba>();
            break;
        case EnemyType::Koopa:
            enemy = std::make_unique<Koopa>();
            break;
        case EnemyType::PiranhaPlant:
            enemy = std::make_unique<PiranhaPlant>();
            break;
    }

    if (enemy) {
        enemy->setPosition(pos);
        enemy->setTheme(theme);
    }
    return enemy;
}

std::unique_ptr<Item> EntityFactory::createItem(ItemType type, sf::Vector2f pos, LevelTheme theme) {
    std::unique_ptr<Item> item;

    switch (type) {
        case ItemType::Coin:
            item = std::make_unique<Coin>();
            break;
        case ItemType::Mushroom:
            item = std::make_unique<Mushroom>();
            break;
        case ItemType::FireFlower:
            item = std::make_unique<FireFlower>();
            break;
        case ItemType::Star:
            item = std::make_unique<Star>();
            break;
    }

    if (item) {
        item->setPosition(pos);
        item->setTheme(theme);
    }
    return item;
}

std::unique_ptr<Tile> EntityFactory::createTile(char tileChar, float x, float y, LevelTheme theme) {
    TileType type;
    switch (tileChar) {
        case 'X': type = TileType::Ground;        break;
        case '<': type = TileType::PipeTopLeft;    break;
        case '>': type = TileType::PipeTopRight;   break;
        case '[': type = TileType::PipeBodyLeft;   break;
        case ']': type = TileType::PipeBodyRight;  break;
        // Bullet Bill cannons ('B' top / 'b' bottom) have no projectile
        // mechanic in this engine -- render as an inert solid block instead.
        case 'B':
        case 'b':
            type = TileType::Ground;
            break;
        default:  return nullptr;
    }
    return std::make_unique<Tile>(type, x, y, theme);
}

std::unique_ptr<Block> EntityFactory::createBlock(char blockChar, float x, float y, LevelTheme theme) {
    BlockType type;
    ObjectType contained = ObjectType::Coin; // Default
    bool startUsed = false;

    switch (blockChar) {
        case 'S': type = BlockType::Brick;    break;
        case '?': type = BlockType::Question; break;
        // VGLC's "already-used" question block -- solid, but spent.
        case 'Q': type = BlockType::Question; startUsed = true; break;
        // Extensions: not part of VGLC's own alphabet, safe to hand-author.
        case 'M': type = BlockType::Question; contained = ObjectType::Mushroom;    break;
        case 'F': type = BlockType::Question; contained = ObjectType::FireFlower;  break;
        case 's': type = BlockType::Question; contained = ObjectType::Star;        break;
        default:  return nullptr;
    }

    auto block = std::make_unique<Block>(type, x, y, theme, startUsed);
    block->setContainedItem(contained);
    return block;
}

std::unique_ptr<Player> EntityFactory::createPlayer(const std::string& characterName, sf::Vector2f pos) {
    std::unique_ptr<Player> player;

    if (characterName == "Luigi") {
        player = std::make_unique<Luigi>();
    } else {
        player = std::make_unique<Mario>();
    }

    player->setPosition(pos);
    return player;
}