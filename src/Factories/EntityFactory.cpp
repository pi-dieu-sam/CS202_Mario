#include "Factory/EntityFactory.hpp"
#include "Entities/Goomba.hpp"
#include "Entities/Koopa.hpp"
#include "Entities/Troopa.hpp"
#include "Entities/Bowser.hpp"
#include "Entities/PiranhaPlant.hpp"
#include "Entities/Coin.hpp"
#include "Entities/Mushroom.hpp"
#include "Entities/FireFlower.hpp"
#include "Entities/Star.hpp"
#include "Entities/FlowersBuff.hpp"
#include "Entities/Tile.hpp"
#include "Entities/Block.hpp"
#include "Entities/Escalater.hpp"
#include "Entities/Mario.hpp"
#include "Entities/Luigi.hpp"
#include "AI/PatrolStrategy.hpp"

std::unique_ptr<Enemy> EntityFactory::createEnemy(EnemyType type, sf::Vector2f pos, LevelTheme theme) {
    std::unique_ptr<Enemy> enemy;

    switch (type) {
        case EnemyType::Goomba:
            enemy = std::make_unique<Goomba>();
            enemy->setStrategy(std::make_unique<PatrolStrategy>());
            break;
        case EnemyType::Koopa:
            enemy = std::make_unique<Koopa>();
            enemy->setStrategy(std::make_unique<PatrolStrategy>());
            break;
        case EnemyType::Troopa:
            enemy = std::make_unique<Troopa>();
            break;
        case EnemyType::Bowser:
            enemy = std::make_unique<Bowser>();
            break;
        case EnemyType::PiranhaPlant:
            enemy = std::make_unique<PiranhaPlant>();
            // No AI strategy: emerge/retract motion is driven directly by
            // PiranhaPlant::update(), not the Strategy pattern.
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
        case ItemType::FlowersBuff:
            item = std::make_unique<FlowersBuff>();
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
        // The legacy Bullet Bill cannon bottom ('b') has no projectile
        // mechanic in this engine, so it renders as an inert solid block.
        case 'b':
            type = TileType::Ground;
            break;
        case 'L': type = TileType::Lava;   break;
        case 'l': type = TileType::Flame;  break;
        case 'V': type = TileType::VineTop; break;
        default:  return nullptr;
    }
    return std::make_unique<Tile>(type, x, y, theme);
}

std::unique_ptr<Tile>
EntityFactory::createCastlePiece(char pieceChar, float x, float y, LevelTheme theme) {
    // Character -> 0-based sheet-cell index (row-major on the 4x2 sheet):
    //   Q 2 3 4   (top row = cells 0-3)
    //   6 S 7 5   (bottom row = cells 4-7)
    int subIndex;
    switch (pieceChar) {
        case 'Q': subIndex = 0; break;
        case '2': subIndex = 1; break;
        case '3': subIndex = 2; break;
        case '4': subIndex = 3; break;
        case '6': subIndex = 4; break;
        case 'S': subIndex = 5; break;
        case '7': subIndex = 6; break;
        case '5': subIndex = 7; break;
        default:  return nullptr;
    }
    return std::make_unique<Tile>(TileType::CastlePiece, x, y, theme, subIndex);
}

std::unique_ptr<Tile>
EntityFactory::createWardPipePiece(char pieceChar, float x, float y, LevelTheme theme) {
    // Character -> 0-based sheet-cell index (row-major on the 3x2 sheet):
    //   ( { \   (top row = cells 0-2)
    //   ) } /   (bottom row = cells 3-5)
    int subIndex;
    switch (pieceChar) {
        case '(': subIndex = 0; break;
        case '{': subIndex = 1; break;
        case '\\': subIndex = 2; break;
        case ')': subIndex = 3; break;
        case '}': subIndex = 4; break;
        case '/': subIndex = 5; break;
        default:  return nullptr;
    }
    return std::make_unique<Tile>(TileType::WardPipePiece, x, y, theme, subIndex);
}

std::unique_ptr<Block> EntityFactory::createBlock(char blockChar, float x, float y, LevelTheme theme) {
    BlockType type;
    ObjectType contained = ObjectType::Coin; // Default
    bool startUsed = false;

    switch (blockChar) {
        case '?': type = BlockType::Question; break;
        // Extensions: not part of VGLC's own alphabet, safe to hand-author.
        case 'S': type = BlockType::Brick;                                break;
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

std::unique_ptr<Escalater> EntityFactory::createEscalater(float x, float y,
                                                          bool horizontal,
                                                          LevelTheme /*theme*/) {
    return std::make_unique<Escalater>(
        x, y, horizontal ? Escalater::MovementAxis::Horizontal
                         : Escalater::MovementAxis::Vertical);
}
