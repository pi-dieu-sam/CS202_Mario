#include "Entities/Block.hpp"
#include "Core/SoundManager.hpp"
#include "Entities/Coin.hpp"
#include "Entities/FireFlower.hpp"
#include "Entities/Mushroom.hpp"
#include "Entities/Star.hpp"
#include "Graphics/SpriteRegistry.hpp"
#include "Observers/EventManager.hpp"
#include "Physics/PhysicsConstants.hpp"

Block::Block() { m_type = ObjectType::Block; }

Block::Block(BlockType blockType, float x, float y, LevelTheme theme,
             bool startUsed)
    : m_blockType(blockType), m_used(startUsed), m_theme(theme) {
  m_type = ObjectType::Block;
  m_position = {x, y};

  SpriteRegistry::applyFrame(
      m_sprite,
      SpriteRegistry::blockPath(blockType, theme,
                                 m_used ? SpriteRegistry::BlockVisualState::Used
                                        : SpriteRegistry::BlockVisualState::Idle),
      sf::FloatRect(x, y, TILE_SIZE, TILE_SIZE));
}

void Block::update(float dt) {
  // Bump animation
  if (m_bumping) {
    m_bumpTimer += dt;
    float bumpDuration = 0.15f;
    if (m_bumpTimer < bumpDuration) {
      m_bumpOffset = -8.0f * (1.0f - m_bumpTimer / bumpDuration);
    } else {
      m_bumping = false;
      m_bumpOffset = 0.0f;
    }
  }

  SpriteRegistry::BlockVisualState state =
      m_bumping ? SpriteRegistry::BlockVisualState::Hit
                : (m_used ? SpriteRegistry::BlockVisualState::Used
                          : SpriteRegistry::BlockVisualState::Idle);
  SpriteRegistry::applyFrame(
      m_sprite, SpriteRegistry::blockPath(m_blockType, m_theme, state),
      sf::FloatRect(m_position.x, m_position.y + m_bumpOffset, TILE_SIZE,
                    TILE_SIZE));
}

void Block::draw(sf::RenderWindow &window) {
  if (!m_active)
    return;
  window.draw(m_sprite);
}

sf::FloatRect Block::getBounds() const {
  return sf::FloatRect(m_position.x, m_position.y + m_bumpOffset, TILE_SIZE,
                       TILE_SIZE);
}

std::unique_ptr<Item> Block::hit(bool playerIsBig) {
  if (m_used)
    return nullptr;

  // Start bump animation.
  m_bumping = true;
  m_bumpTimer = 0.0f;

  if (m_blockType == BlockType::Question) {
    EventManager::getInstance().publish({EventType::BlockHit});
    m_used = true;

    // Create the contained item above the block
    sf::Vector2f spawnPos = {m_position.x, m_position.y - TILE_SIZE};

    switch (m_containedItem) {
    case ObjectType::Coin: {
      auto coin = std::make_unique<Coin>();
      coin->setPosition(spawnPos);
      coin->setTheme(m_theme);
      return coin;
    }
    case ObjectType::Mushroom: {
      auto mush = std::make_unique<Mushroom>();
      mush->setPosition(spawnPos);
      mush->setTheme(m_theme);
      return mush;
    }
    case ObjectType::FireFlower: {
      auto fire = std::make_unique<FireFlower>();
      fire->setPosition(spawnPos);
      fire->setTheme(m_theme);
      return fire;
    }
    case ObjectType::Star: {
      auto star = std::make_unique<Star>();
      star->setPosition(spawnPos);
      star->setTheme(m_theme);
      return star;
    }
    default:
      break;
    }
  } else if (m_blockType == BlockType::Brick) {
    if (playerIsBig) {
      // A brick takes three empowered head hits. The break sound gives
      // feedback for the first two hits as well as the final break.
      ++m_brickHits;
      SoundManager::getInstance().playSound(SoundID::BlockBreak);
      if (m_brickHits >= 3) {
        m_active = false;
      }
    } else {
      // A normal player can bump it but cannot damage it.
      SoundManager::getInstance().playSound(SoundID::BlockBump);
    }
  }

  return nullptr;
}

BlockType Block::getBlockType() const { return m_blockType; }
bool Block::isUsed() const { return m_used; }

void Block::setContainedItem(ObjectType itemType) {
  m_containedItem = itemType;
}
