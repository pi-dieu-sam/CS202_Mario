#include "Level/Level.hpp"
#include "AI/ChaseStrategy.hpp"
#include "Entities/Koopa.hpp"
#include "Factory/EntityFactory.hpp"
#include "Level/LevelLoader.hpp"
#include "Observers/EventManager.hpp"
#include "Physics/CollisionDetector.hpp"
#include "Physics/PhysicsConstants.hpp"
#include <algorithm>
#include <cmath>
#include <optional>

namespace {
sf::Vector2f boundsCenter(const GameObject& object) {
  const sf::FloatRect bounds = object.getBounds();
  return {bounds.left + bounds.width * 0.5f,
          bounds.top + bounds.height * 0.5f};
}

void publishEnemyDefeated(Enemy& enemy, const sf::Vector2f& worldPosition) {
  GameEvent event{EventType::EnemyDefeated, enemy.getScoreValue()};
  event.worldPosition = worldPosition;
  EventManager::getInstance().publish(event);
}
} // namespace

Level::Level() {}
Level::~Level() {}

bool Level::loadFromFile(const std::string &filename,
                         const std::string &characterName, LevelTheme theme,
                         bool autoPlaceFlagpole) {
  auto data = LevelLoader::loadLevel(filename, theme, autoPlaceFlagpole);

  m_tiles = std::move(data.tiles);
  m_tileGrid.build(m_tiles);
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

  m_background.load(theme, m_width);

  return true;
}

void Level::update(float dt) {
  if (!m_player)
    return;

  // Update player
  m_player->update(dt);

  // Use level height instead of a hardcoded world-space Y to decide abyss death.
  if (!m_player->isDead() &&
      m_player->getBounds().top > m_height + PLAYER_FALL_DEATH_MARGIN) {
    m_player->die();
  }

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
  handleCollisions(dt);

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

std::optional<sf::FloatRect>
Level::getEnterablePipeBounds(const Player &player) const {
  if (!player.isGrounded()) {
    return std::nullopt;
  }

  const sf::FloatRect playerBounds = player.getBounds();
  const sf::FloatRect feetProbe(playerBounds.left + 2.0f,
                                playerBounds.top + playerBounds.height - 6.0f,
                                playerBounds.width - 4.0f,
                                8.0f);

  auto hasPipePart = [&](float x, float y, TileType type) {
    for (const auto &tile : m_tiles) {
      if (tile->getTileType() != type)
        continue;
      const sf::FloatRect bounds = tile->getBounds();
      if (std::abs(bounds.left - x) < 0.5f && std::abs(bounds.top - y) < 0.5f) {
        return true;
      }
    }
    return false;
  };

  for (const auto &tile : m_tiles) {
    if (tile->getTileType() != TileType::PipeTopLeft)
      continue;

    const sf::FloatRect topLeft = tile->getBounds();
    const float x = topLeft.left;
    const float y = topLeft.top;

    const bool hasTopRight = hasPipePart(x + TILE_SIZE, y, TileType::PipeTopRight);
    const bool hasBodyLeft = hasPipePart(x, y + TILE_SIZE, TileType::PipeBodyLeft);
    const bool hasBodyRight = hasPipePart(x + TILE_SIZE, y + TILE_SIZE, TileType::PipeBodyRight);

    if (!hasTopRight || !hasBodyLeft || !hasBodyRight)
      continue;

    const sf::FloatRect pipeBounds(x, y, TILE_SIZE * 2.0f, TILE_SIZE * 2.0f);
    if (feetProbe.intersects(pipeBounds)) {
      return pipeBounds;
    }
  }

  return std::nullopt;
}

std::optional<sf::FloatRect>
Level::getTouchedPipeBounds(const Player &player) const {
  const sf::FloatRect playerBounds = player.getBounds();

  auto hasPipePart = [&](float x, float y, TileType type) {
    for (const auto &tile : m_tiles) {
      if (tile->getTileType() != type)
        continue;
      const sf::FloatRect bounds = tile->getBounds();
      if (std::abs(bounds.left - x) < 0.5f && std::abs(bounds.top - y) < 0.5f) {
        return true;
      }
    }
    return false;
  };

  for (const auto &tile : m_tiles) {
    if (tile->getTileType() != TileType::PipeTopLeft)
      continue;

    const sf::FloatRect topLeft = tile->getBounds();
    const float x = topLeft.left;
    const float y = topLeft.top;

    const bool hasTopRight = hasPipePart(x + TILE_SIZE, y, TileType::PipeTopRight);
    const bool hasBodyLeft = hasPipePart(x, y + TILE_SIZE, TileType::PipeBodyLeft);
    const bool hasBodyRight = hasPipePart(x + TILE_SIZE, y + TILE_SIZE, TileType::PipeBodyRight);
    if (!hasTopRight || !hasBodyLeft || !hasBodyRight)
      continue;

    const sf::FloatRect pipeBounds(x, y, TILE_SIZE * 2.0f, TILE_SIZE * 4.0f);
    if (playerBounds.intersects(pipeBounds)) {
      return sf::FloatRect(x, y, TILE_SIZE * 2.0f, TILE_SIZE * 2.0f);
    }
  }

  return std::nullopt;
}

bool Level::isComplete() const { return m_flagpole && m_flagpole->isReached(); }

void Level::handleCollisions(float dt) {
  if (!m_player || m_player->isDead())
    return;

  // Reset grounded state each frame
  m_player->setGrounded(false);

  // Player vs Tiles
  for (Tile *tile : m_tileGrid.query(m_player->getBounds())) {
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
  Enemy *firstEnemyHit = nullptr;
  CollisionDetector::CollisionResult firstEnemyResult;
  float firstImpactTime = 2.0f;
  for (auto &enemy : m_enemies) {
    if (!enemy->isActive() || enemy->isDead())
      continue;

    // Prefer the time-of-impact result for a new player/enemy contact. A
    // final-frame overlap can be deep enough to make a legitimate stomp look
    // like a side collision, and can miss an enemy entirely at higher speed.
    auto result = CollisionDetector::checkSweptCollision(*m_player, *enemy, dt);
    if (!result.collided) {
      result = CollisionDetector::checkCollision(*m_player, *enemy);
    }
    if (!result.collided)
      continue;

    if (m_player->hasStarPower()) {
      // Star power kills every enemy it touches this frame.
      const sf::Vector2f scorePosition = boundsCenter(*enemy);
      enemy->onStomped();
      publishEnemyDefeated(*enemy, scorePosition);
      continue;
    }

    // Resolve only the first contact in the step. This makes a cluster
    // deterministic and avoids evaluating later enemies after a stomp has
    // already moved the player back to its impact point and reversed vy.
    const float impactTime = result.swept ? result.timeOfImpact : 1.0f;
    if (impactTime < firstImpactTime) {
      firstEnemyHit = enemy.get();
      firstEnemyResult = result;
      firstImpactTime = impactTime;
    }
  }

  if (firstEnemyHit) {
    if (firstEnemyResult.side == CollisionDetector::Side::Bottom &&
        m_player->getVelocity().y > 0) {
      // Stomp from above
      CollisionDetector::moveToImpact(*m_player, firstEnemyResult);
      const sf::Vector2f scorePosition = boundsCenter(*firstEnemyHit);
      firstEnemyHit->onStomped();
      const float bounceVelocity =
          m_player->isJumpHeld() ? m_player->getJumpForce() : -250.0f;
      m_player->setVelocity(m_player->getVelocity().x, bounceVelocity);
      publishEnemyDefeated(*firstEnemyHit, scorePosition);
    } else {
      // Side collision — player takes damage
      m_player->takeDamage();
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
      EventManager::getInstance().publish({EventType::LevelCompleted, bonus});
    }
  }

  // Enemy vs Tiles (for patrol direction reversal)
  for (auto &enemy : m_enemies) {
    if (!enemy->isActive() || enemy->isDead())
      continue;
    enemy->setGrounded(false);
    for (Tile *tile : m_tileGrid.query(enemy->getBounds())) {
      auto result = CollisionDetector::checkCollision(*enemy, *tile);
      if (result.collided) {
        sf::Vector2f preVel = enemy->getVelocity(); // capture BEFORE resolve zeroes vel.x
        CollisionDetector::resolveCollision(*enemy, *tile, result);
        if (result.side == CollisionDetector::Side::Bottom) {
          enemy->setGrounded(true);
        }
        if (result.side == CollisionDetector::Side::Left ||
            result.side == CollisionDetector::Side::Right) {
          // Reverse direction using the pre-collision velocity — resolveCollision()
          // zeroes vel.x (needed for the player's wall-stop), so reading it after
          // the call would always negate zero.
          float newVx = CollisionDetector::reflectHorizontalVelocity(
              preVel.x, result.side, enemy->getSpeed());
          enemy->setVelocity(newVx, enemy->getVelocity().y);
        }
      }
    }
    // Enemy vs Blocks (collision)
    for (auto &block : m_blocks) {
      if (!block->isActive())
        continue;
      auto result = CollisionDetector::checkCollision(*enemy, *block);
      if (result.collided) {
        sf::Vector2f preVel = enemy->getVelocity();
        CollisionDetector::resolveCollision(*enemy, *block, result);
        if (result.side == CollisionDetector::Side::Bottom) {
          enemy->setGrounded(true);
        }
        if (result.side == CollisionDetector::Side::Left ||
            result.side == CollisionDetector::Side::Right) {
          float newVx = CollisionDetector::reflectHorizontalVelocity(
              preVel.x, result.side, enemy->getSpeed());
          enemy->setVelocity(newVx, enemy->getVelocity().y);
        }
      }
    }
  }

  // Enemy vs Enemy
  for (size_t i = 0; i < m_enemies.size(); ++i) {
    auto &enemyA = m_enemies[i];
    if (!enemyA || !enemyA->isActive() || enemyA->isDead())
      continue;

    for (size_t j = i + 1; j < m_enemies.size(); ++j) {
      auto &enemyB = m_enemies[j];
      if (!enemyB || !enemyB->isActive() || enemyB->isDead())
        continue;

      auto result = CollisionDetector::checkCollision(*enemyA, *enemyB);
      if (!result.collided)
        continue;

      auto *koopaA = dynamic_cast<Koopa *>(enemyA.get());
      auto *koopaB = dynamic_cast<Koopa *>(enemyB.get());
      const bool aIsSlidingShell = koopaA && koopaA->getKoopaState() == KoopaState::Sliding;
      const bool bIsSlidingShell = koopaB && koopaB->getKoopaState() == KoopaState::Sliding;

      if (aIsSlidingShell || bIsSlidingShell) {
        if (aIsSlidingShell) {
          const sf::Vector2f scorePosition = boundsCenter(*enemyB);
          enemyB->onStomped();
          publishEnemyDefeated(*enemyB, scorePosition);
        }
        if (bIsSlidingShell) {
          const sf::Vector2f scorePosition = boundsCenter(*enemyA);
          enemyA->onStomped();
          publishEnemyDefeated(*enemyA, scorePosition);
        }
      } else {
        const sf::Vector2f velA = enemyA->getVelocity();
        const sf::Vector2f velB = enemyB->getVelocity();
        enemyA->setVelocity(-velA.x, velA.y);
        enemyB->setVelocity(-velB.x, velB.y);
      }
    }
  }

  // Fireball vs Tiles & Enemies
  for (auto &fb : m_fireballs) {
    if (!fb->isActive())
      continue;

    // Fireball vs Tiles
    for (Tile *tile : m_tileGrid.query(fb->getBounds())) {
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
        const sf::Vector2f scorePosition = boundsCenter(*enemy);
        enemy->onStomped(); // Fireball kills enemy
        fb->setActive(false);
        publishEnemyDefeated(*enemy, scorePosition);
      }
    }
  }

  // Items vs Tiles (for moving items like mushroom)
  for (auto &item : m_items) {
    if (!item->isActive() || !item->isMoving())
      continue;
    for (Tile *tile : m_tileGrid.query(item->getBounds())) {
      auto result = CollisionDetector::checkCollision(*item, *tile);
      if (result.collided) {
        sf::Vector2f preVel = item->getVelocity();
        CollisionDetector::resolveCollision(*item, *tile, result);
        if (result.side == CollisionDetector::Side::Left ||
            result.side == CollisionDetector::Side::Right) {
          float newVx = CollisionDetector::reflectHorizontalVelocity(
              preVel.x, result.side, 0.0f);
          item->setVelocity(newVx, item->getVelocity().y);
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
