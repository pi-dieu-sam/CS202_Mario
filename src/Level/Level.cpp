#include "Level/Level.hpp"
#include "AI/ChaseStrategy.hpp"
#include "Factory/EntityFactory.hpp"
#include "Level/LevelLoader.hpp"
#include "Observers/EventManager.hpp"
#include "Physics/CollisionDetector.hpp"
#include "Physics/PhysicsConstants.hpp"
#include <algorithm>

Level::Level() {}
Level::~Level() {}

bool Level::loadFromFile(const std::string &filename,
                         const std::string &characterName, LevelTheme theme) {
  auto data = LevelLoader::loadLevel(filename, theme);

  m_tiles = std::move(data.tiles);
  m_blocks = std::move(data.blocks);
  m_enemies = std::move(data.enemies);
  m_items = std::move(data.items);
  m_flagpole = std::move(data.flagpole);
  m_width = data.width;
  m_height = data.height;

  // Create player at spawn point
  m_player = EntityFactory::createPlayer(characterName, data.playerSpawn);

  if (!m_player)
    return false;

  m_background.load(theme);

  return true;
}

void Level::update(float dt) {
  if (!m_player)
    return;

  // Update player
  m_player->update(dt);

  // Update enemies
  for (auto &enemy : m_enemies) {
    if (!enemy->isActive())
      continue;

    // Feed chase strategies the player position (ignored by strategies
    // that don't use it, e.g. PatrolStrategy).
    enemy->updatePlayerPosition(m_player->getPosition());
    enemy->update(dt);
  }

  // Update items
  for (auto &item : m_items) {
    if (item->isActive())
      item->update(dt);
  }

  // Update blocks
  for (auto &block : m_blocks) {
    if (block->isActive())
      block->update(dt);
  }

  // Update fireballs
  for (auto &fb : m_fireballs) {
    if (fb->isActive())
      fb->update(dt);
  }

  // Update flagpole
  if (m_flagpole)
    m_flagpole->update(dt);

  // Handle collisions
  handleCollisions();

  // Remove inactive entities
  removeInactiveEntities();
}

void Level::render(sf::RenderWindow &window, float cameraCenterX) {
  m_background.render(window, cameraCenterX);

  // Draw tiles
  for (auto &tile : m_tiles) {
    tile->draw(window);
  }

  // Draw blocks
  for (auto &block : m_blocks) {
    if (block->isActive())
      block->draw(window);
  }

  // Draw items
  for (auto &item : m_items) {
    if (item->isActive())
      item->draw(window);
  }

  // Draw enemies
  for (auto &enemy : m_enemies) {
    if (enemy->isActive())
      enemy->draw(window);
  }

  // Draw fireballs
  for (auto &fb : m_fireballs) {
    if (fb->isActive())
      fb->draw(window);
  }

  // Draw flagpole
  if (m_flagpole)
    m_flagpole->draw(window);

  // Draw player last (on top)
  if (m_player)
    m_player->draw(window);
}

Player *Level::getPlayer() const { return m_player.get(); }
float Level::getWidth() const { return m_width; }
float Level::getHeight() const { return m_height; }

void Level::addFireball(std::unique_ptr<Fireball> fireball) {
  m_fireballs.push_back(std::move(fireball));
}

void Level::addItem(std::unique_ptr<Item> item) {
  m_items.push_back(std::move(item));
}

bool Level::isComplete() const { return m_flagpole && m_flagpole->isReached(); }

void Level::handleCollisions() {
  if (!m_player || m_player->isDead())
    return;

  // Reset grounded state each frame
  m_player->setGrounded(false);

  // Player vs Tiles
  for (auto &tile : m_tiles) {
    auto result = CollisionDetector::checkCollision(*m_player, *tile);
    if (result.collided) {
      CollisionDetector::resolveCollision(*m_player, *tile, result);
      if (result.side == CollisionDetector::Side::Bottom) {
        m_player->setGrounded(true);
      }
    }
  }

  // Player vs Blocks
  for (auto &block : m_blocks) {
    if (!block->isActive())
      continue;
    auto result = CollisionDetector::checkCollision(*m_player, *block);
    if (result.collided) {
      CollisionDetector::resolveCollision(*m_player, *block, result);
      if (result.side == CollisionDetector::Side::Bottom) {
        m_player->setGrounded(true);
      }
      if (result.side == CollisionDetector::Side::Top) {
        // Hit block from below
        auto spawnedItem =
            block->hit(m_player->getPowerUpState() != PowerUpState::Small);
        if (spawnedItem) {
          m_items.push_back(std::move(spawnedItem));
        }
      }
    }
  }

  // Player vs Enemies
  for (auto &enemy : m_enemies) {
    if (!enemy->isActive() || enemy->isDead())
      continue;
    auto result = CollisionDetector::checkCollision(*m_player, *enemy);
    if (result.collided) {
      if (m_player->hasStarPower()) {
        // Star power kills enemies on contact
        enemy->onStomped();
        EventManager::getInstance().publish(
            {EventType::EnemyDefeated, enemy->getScoreValue()});
      } else if (result.side == CollisionDetector::Side::Bottom &&
                 m_player->getVelocity().y > 0) {
        // Stomp from above
        enemy->onStomped();
        m_player->setVelocity(m_player->getVelocity().x, -250.0f); // Bounce
        EventManager::getInstance().publish(
            {EventType::EnemyDefeated, enemy->getScoreValue()});
      } else {
        // Side collision — player takes damage
        m_player->takeDamage();
      }
    }
  }

  // Player vs Items
  for (auto &item : m_items) {
    if (!item->isActive())
      continue;
    auto result = CollisionDetector::checkCollision(*m_player, *item);
    if (result.collided) {
      item->collect(*m_player);
    }
  }

  // Player vs Flagpole
  if (m_flagpole && !m_flagpole->isReached()) {
    auto result = CollisionDetector::checkCollision(*m_player, *m_flagpole);
    if (result.collided) {
      m_flagpole->setReached(true);
      int bonus = m_flagpole->calculateScore(m_player->getPosition().y);
      m_player->addScore(bonus);
      EventManager::getInstance().publish({EventType::LevelCompleted, bonus});
    }
  }

  // Enemy vs Tiles (for patrol direction reversal)
  for (auto &enemy : m_enemies) {
    if (!enemy->isActive() || enemy->isDead())
      continue;
    enemy->setGrounded(false);
    for (auto &tile : m_tiles) {
      auto result = CollisionDetector::checkCollision(*enemy, *tile);
      if (result.collided) {
        CollisionDetector::resolveCollision(*enemy, *tile, result);
        if (result.side == CollisionDetector::Side::Bottom) {
          enemy->setGrounded(true);
        }
        if (result.side == CollisionDetector::Side::Left ||
            result.side == CollisionDetector::Side::Right) {
          // Reverse direction
          sf::Vector2f vel = enemy->getVelocity();
          enemy->setVelocity(-vel.x, vel.y);
        }
      }
    }
    // Enemy vs Blocks (collision)
    for (auto &block : m_blocks) {
      if (!block->isActive())
        continue;
      auto result = CollisionDetector::checkCollision(*enemy, *block);
      if (result.collided) {
        CollisionDetector::resolveCollision(*enemy, *block, result);
        if (result.side == CollisionDetector::Side::Bottom) {
          enemy->setGrounded(true);
        }
        if (result.side == CollisionDetector::Side::Left ||
            result.side == CollisionDetector::Side::Right) {
          sf::Vector2f vel = enemy->getVelocity();
          enemy->setVelocity(-vel.x, vel.y);
        }
      }
    }
  }

  // Fireball vs Tiles & Enemies
  for (auto &fb : m_fireballs) {
    if (!fb->isActive())
      continue;

    // Fireball vs Tiles
    for (auto &tile : m_tiles) {
      auto result = CollisionDetector::checkCollision(*fb, *tile);
      if (result.collided) {
        if (result.side == CollisionDetector::Side::Bottom) {
          // Bounce
          sf::Vector2f vel = fb->getVelocity();
          fb->setVelocity(vel.x, -200.0f);
          sf::Vector2f pos = fb->getPosition();
          pos.y -= result.overlap;
          fb->setPosition(pos);
        } else if (result.side == CollisionDetector::Side::Left ||
                   result.side == CollisionDetector::Side::Right) {
          fb->setActive(false); // Destroy on wall
        }
      }
    }

    // Fireball vs Enemies
    for (auto &enemy : m_enemies) {
      if (!enemy->isActive() || enemy->isDead())
        continue;
      auto result = CollisionDetector::checkCollision(*fb, *enemy);
      if (result.collided) {
        enemy->onStomped(); // Fireball kills enemy
        fb->setActive(false);
        EventManager::getInstance().publish(
            {EventType::EnemyDefeated, enemy->getScoreValue()});
      }
    }
  }

  // Items vs Tiles (for moving items like mushroom)
  for (auto &item : m_items) {
    if (!item->isActive() || !item->isMoving())
      continue;
    for (auto &tile : m_tiles) {
      auto result = CollisionDetector::checkCollision(*item, *tile);
      if (result.collided) {
        CollisionDetector::resolveCollision(*item, *tile, result);
        if (result.side == CollisionDetector::Side::Left ||
            result.side == CollisionDetector::Side::Right) {
          sf::Vector2f vel = item->getVelocity();
          item->setVelocity(-vel.x, vel.y);
        }
      }
    }
  }
}

void Level::removeInactiveEntities() {
  m_enemies.erase(std::remove_if(m_enemies.begin(), m_enemies.end(),
                                 [](const auto &e) { return !e->isActive(); }),
                  m_enemies.end());

  m_items.erase(std::remove_if(m_items.begin(), m_items.end(),
                               [](const auto &i) { return !i->isActive(); }),
                m_items.end());

  m_fireballs.erase(
      std::remove_if(m_fireballs.begin(), m_fireballs.end(),
                     [](const auto &f) { return !f->isActive(); }),
      m_fireballs.end());

  m_blocks.erase(std::remove_if(m_blocks.begin(), m_blocks.end(),
                                [](const auto &b) { return !b->isActive(); }),
                 m_blocks.end());
}