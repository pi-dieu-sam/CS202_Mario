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
#include "Core/Game.hpp"

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
  m_escalaters = std::move(data.escalaters);
  m_flagpole = std::move(data.flagpole);
  m_width = data.width;
  m_height = data.height;

  // Create player at spawn point
  m_player = EntityFactory::createPlayer(characterName, data.playerSpawn);
  if (m_player) m_player->setPlayerId(1);

  if (Game::getInstance().getProgress().isMultiplayer()) {
      std::string char2 = (characterName == "Mario") ? "Luigi" : "Mario";
      // Use explicit P2 spawn if the map defines one ('2'), otherwise offset from P1
      sf::Vector2f spawn2 = data.hasPlayer2Spawn
                              ? data.player2Spawn
                              : sf::Vector2f(data.playerSpawn.x - 24.0f, data.playerSpawn.y);
      m_player2 = EntityFactory::createPlayer(char2, spawn2);
      if (m_player2) m_player2->setPlayerId(2);
  }

  if (!m_player)
    return false;

  m_background.load(theme, m_width);

  return true;
}

void Level::update(float dt) {
  if (!m_player)
    return;

  // Update players
  bool anyDead = (m_player && m_player->isDead()) || (m_player2 && m_player2->isDead());
  
  if (m_player) {
      if (m_player->isDead() || !anyDead) m_player->update(dt);
  }
  if (m_player2) {
      if (m_player2->isDead() || !anyDead) m_player2->update(dt);
  }

  // Use level height instead of a hardcoded world-space Y to decide abyss death.
  if (!m_player->isDead() &&
      m_player->getBounds().top > m_height + PLAYER_FALL_DEATH_MARGIN) {
    m_player->die();
  }
  if (m_player2 && !m_player2->isDead() &&
      m_player2->getBounds().top > m_height + PLAYER_FALL_DEATH_MARGIN) {
    m_player2->die();
  }

  // During the visible death sequence, Mario/Luigi is the only entity that
  // advances. Freezing the rest of the level keeps enemies, items, fireballs,
  // and block animations at their exact death-frame positions until respawn.
  if (m_player->isDead() || (m_player2 && m_player2->isDead())) {
    return;
  }

  // Update enemies
  for (auto &enemy : m_enemies) {
    if (!enemy->isActive())
      continue;

    // Feed chase strategies the player position (ignored by strategies
    // that don't use it, e.g. PatrolStrategy).
    // Note: for simplicity in co-op, enemies will just track m_player.
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

  // Update escalaters (moving platforms)
  for (auto &esc : m_escalaters) {
    if (esc->isActive())
      esc->update(dt);
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

void Level::updateCompletion(float dt) {
  if (!m_player) {
    return;
  }

  // Only the player cutscene and falling flag progress here. Letting the
  // normal update path run would re-enable gravity, enemy contacts, pickups,
  // and tile collision responses partway through the flag-pole sequence.
  m_player->update(dt);
  if (m_player2) m_player2->update(dt);
  if (m_flagpole) {
    m_flagpole->update(dt);
  }
}

void Level::render(sf::RenderWindow &window, float cameraCenterX) {
  m_background.render(window, cameraCenterX);

  // Draw tiles
  for (auto &tile : m_tiles) {
    tile->draw(window);
  }

  // Draw escalaters (moving platforms)
  for (auto &esc : m_escalaters) {
    if (esc->isActive())
      esc->draw(window);
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
  if (m_player2)
    m_player2->draw(window);
}

Player *Level::getPlayer() const { return m_player.get(); }
Player *Level::getPlayer2() const { return m_player2.get(); }
Flagpole *Level::getFlagpole() const { return m_flagpole.get(); }
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

void Level::handlePlayerCollisions(Player* player, float dt) {
  if (!player || player->isDead()) return;

  player->setGrounded(false);

  // Player vs Blocks
  // Resolve block contacts before terrain. An enlarged player can overlap a
  // low block with its upper body, and terrain resolution would otherwise
  // zero the jump or push the player sideways before the block sees the hit.
  const bool enlargedPlayer =
      player->getPowerUpState() != PowerUpState::Small ||
      player->hasSizeBuff();
  for (auto &block : m_blocks) {
    if (!block->isActive())
      continue;

    const sf::FloatRect interactionBounds =
        player->getBlockInteractionBounds();
    const sf::FloatRect blockBounds = block->getBounds();
    const auto result = CollisionDetector::checkCollision(
        interactionBounds, player->getVelocity(), blockBounds,
        block->getVelocity());

    // The generic AABB side choice intentionally treats a very shallow
    // upward corner overlap as a wall. For a player whose centre is inside
    // the block's width, however, an upward overlap is a head hit even when
    // horizontal movement makes that generic result report Left/Right.
    const float playerCenterX =
        interactionBounds.left + interactionBounds.width * 0.5f;
    const float blockBottom = blockBounds.top + blockBounds.height;
    const bool centeredUnderBlock =
        playerCenterX > blockBounds.left &&
        playerCenterX < blockBounds.left + blockBounds.width;
    const bool risingHeadHit =
        result.collided && player->getVelocity().y < 0.0f &&
        centeredUnderBlock && interactionBounds.top >= blockBounds.top &&
        interactionBounds.top < blockBottom;

    if (risingHeadHit) {
      sf::Vector2f position = player->getPosition();
      position.y += blockBottom - interactionBounds.top;
      player->setPosition(position);
      player->setVelocity(player->getVelocity().x, 0.0f);

      auto spawnedItem =
          block->hit(player->getPowerUpState() != PowerUpState::Small);
      if (spawnedItem) {
        m_items.push_back(std::move(spawnedItem));
      }
      continue;
    }

    if (result.collided) {
      CollisionDetector::resolveCollision(*player, *block, result);
      if (result.side == CollisionDetector::Side::Bottom) {
        player->setGrounded(true);
      }
      if (result.side == CollisionDetector::Side::Top) {
        auto spawnedItem =
            block->hit(player->getPowerUpState() != PowerUpState::Small);
        if (spawnedItem) {
          m_items.push_back(std::move(spawnedItem));
        }
      }
    }
  }

  // Player vs Tiles
  for (Tile *tile : m_tileGrid.query(player->getBounds())) {
    auto result = CollisionDetector::checkCollision(*player, *tile);
    if (!result.collided)
      continue;

    if (enlargedPlayer) {
      const sf::FloatRect interactionBounds =
          player->getBlockInteractionBounds();
      const sf::FloatRect tileBounds = tile->getBounds();
      const auto compactResult = CollisionDetector::checkCollision(
          interactionBounds, player->getVelocity(), tileBounds,
          tile->getVelocity());
      const bool enlargedOnlyOverlap =
          !compactResult.collided &&
          tileBounds.top + tileBounds.height <= interactionBounds.top + 0.01f;
      if (enlargedOnlyOverlap)
        continue;
    }

    CollisionDetector::resolveCollision(*player, *tile, result);
    if (result.side == CollisionDetector::Side::Bottom) {
      player->setGrounded(true);
    }
  }

  // Player vs Escalaters (moving platforms)
  for (auto &esc : m_escalaters) {
    if (!esc->isActive()) continue;
    auto result = CollisionDetector::checkCollision(*player, *esc);
    if (result.collided) {
      CollisionDetector::resolveCollision(*player, *esc, result);
      if (result.side == CollisionDetector::Side::Bottom) {
        player->setGrounded(true);
        // Transfer the escalater's vertical velocity to the player so they
        // ride the platform up and down.
        player->setVelocity(player->getVelocity().x, esc->getVelocity().y);
      }
    }
  }

  // Player vs Enemies
  Enemy *firstEnemyHit = nullptr;
  Enemy *stompedEnemy = nullptr;
  CollisionDetector::CollisionResult firstEnemyResult;
  float firstImpactTime = 2.0f;
  for (auto &enemy : m_enemies) {
    if (!enemy->isActive() || enemy->isDead() || !enemy->isVulnerable())
      continue;

    auto result = CollisionDetector::checkSweptCollision(*player, *enemy, dt);
    if (!result.collided) {
      result = CollisionDetector::checkCollision(*player, *enemy);
    }
    if (!result.collided)
      continue;

    if (player->hasStarPower()) {
      const sf::Vector2f scorePosition = boundsCenter(*enemy);
      enemy->onStomped();
      publishEnemyDefeated(*enemy, scorePosition);
      continue;
    }

    const float impactTime = result.swept ? result.timeOfImpact : 1.0f;
    if (impactTime < firstImpactTime) {
      firstEnemyHit = enemy.get();
      firstEnemyResult = result;
      firstImpactTime = impactTime;
    }
  }

  if (firstEnemyHit) {
    if (firstEnemyResult.side == CollisionDetector::Side::Bottom &&
        player->getVelocity().y > 0 && firstEnemyHit->canBeStomped()) {
      CollisionDetector::moveToImpact(*player, firstEnemyResult);
      const sf::Vector2f scorePosition = boundsCenter(*firstEnemyHit);
      firstEnemyHit->onStomped();
      // A walking Koopa becomes a shell here. Do not immediately process that
      // newly-created shell as a second collision in the same frame, or the
      // shell resolver would cancel the player's stomp bounce.
      stompedEnemy = firstEnemyHit;
      const float bounceVelocity =
          player->isJumpHeld() ? player->getJumpForce() : -250.0f;
      player->setVelocity(player->getVelocity().x, bounceVelocity);
      publishEnemyDefeated(*firstEnemyHit, scorePosition);
    } else {
      player->takeDamage();
    }
  }

  // Player vs Shell enemies (kickable / solid)
  for (auto &enemy : m_enemies) {
    if (!enemy->isActive() || enemy->isDead() || enemy->isVulnerable())
      continue;
    if (enemy.get() == stompedEnemy)
      continue;

    auto result = CollisionDetector::checkCollision(*player, *enemy);
    if (!result.collided)
      continue;

    auto *koopa = dynamic_cast<Koopa*>(enemy.get());
    if (koopa && koopa->getKoopaState() == KoopaState::Shell) {
      if (result.side == CollisionDetector::Side::Bottom) {
        const bool wasFalling = player->getVelocity().y > 0.0f;
        CollisionDetector::resolveCollision(*player, *enemy, result);
        if (wasFalling) {
          const bool wasSliding = koopa->isSliding();
          const float playerCenter =
              player->getBounds().left + player->getBounds().width * 0.5f;
          const float shellCenter =
              enemy->getBounds().left + enemy->getBounds().width * 0.5f;
          if (wasSliding) {
            koopa->stopSliding();
          } else {
            koopa->kick(playerCenter < shellCenter ? 1.0f : -1.0f);
          }
          const float bounceVelocity =
              player->isJumpHeld() ? player->getJumpForce() : -250.0f;
          player->setVelocity(player->getVelocity().x, bounceVelocity);
          player->setGrounded(false);
        } else {
          // A resting player must be grounded on a sitting shell so jump()
          // remains available on the next input frame.
          player->setGrounded(true);
        }
        continue;
      }

      if (!koopa->isSliding() &&
          (result.side == CollisionDetector::Side::Left ||
           result.side == CollisionDetector::Side::Right)) {
        CollisionDetector::resolveCollision(*player, *enemy, result);
        const float kickDir = result.side == CollisionDetector::Side::Left
                                  ? -1.0f
                                  : 1.0f;
        koopa->kick(kickDir);
        continue;
      }
    }

    CollisionDetector::resolveCollision(*player, *enemy, result);
  }

  if (player->isDead()) return;

  // Player vs Items
  for (auto &item : m_items) {
    if (!item->isActive()) continue;
    auto result = CollisionDetector::checkCollision(*player, *item);
    if (result.collided) {
      item->collect(*player);
    }
  }

  // Player vs Flagpole
  if (m_flagpole && !m_flagpole->isReached()) {
    auto result = CollisionDetector::checkCollision(*player, *m_flagpole);
    if (result.collided) {
      m_flagpole->setReached(true);
      int bonus = m_flagpole->calculateScore(player->getPosition().y);
      EventManager::getInstance().publish({EventType::LevelCompleted, bonus});
    }
  }
}

void Level::handleCollisions(float dt) {
  // Player 1 and Player 2 collisions with level
  if (m_player) handlePlayerCollisions(m_player.get(), dt);
  if (m_player2) handlePlayerCollisions(m_player2.get(), dt);

  // Player vs Player collisions (only when 2 players exist)
  if (m_player && !m_player->isDead() && m_player2 && !m_player2->isDead()) {
    bool pvp = Game::getInstance().getProgress().isPvP();
    auto result = CollisionDetector::checkSweptCollision(*m_player, *m_player2, dt);
    if (!result.collided) result = CollisionDetector::checkCollision(*m_player, *m_player2);
    if (result.collided) {
        if (pvp && result.side == CollisionDetector::Side::Bottom && m_player->getVelocity().y > 0) {
            // PvP: P1 stomped P2
            CollisionDetector::moveToImpact(*m_player, result);
            const float bounceVelocity = m_player->isJumpHeld() ? m_player->getJumpForce() : -250.0f;
            m_player->setVelocity(m_player->getVelocity().x, bounceVelocity);
            m_player2->takeDamage();
        } else if (pvp && result.side == CollisionDetector::Side::Top && m_player2->getVelocity().y > 0) {
            // PvP: P2 stomped P1
            CollisionDetector::CollisionResult res2 = result;
            res2.side = CollisionDetector::Side::Bottom;
            CollisionDetector::moveToImpact(*m_player2, res2);
            const float bounceVelocity = m_player2->isJumpHeld() ? m_player2->getJumpForce() : -250.0f;
            m_player2->setVelocity(m_player2->getVelocity().x, bounceVelocity);
            m_player->takeDamage();
        } else {
            // Co-op or side collision: push them apart equally
            float pushAmount = result.overlap / 2.0f;
            sf::Vector2f pos1 = m_player->getPosition();
            sf::Vector2f pos2 = m_player2->getPosition();
            if (result.side == CollisionDetector::Side::Left) {
                m_player->setPosition(pos1 + sf::Vector2f(pushAmount, 0));
                m_player2->setPosition(pos2 - sf::Vector2f(pushAmount, 0));
            } else if (result.side == CollisionDetector::Side::Right) {
                m_player->setPosition(pos1 - sf::Vector2f(pushAmount, 0));
                m_player2->setPosition(pos2 + sf::Vector2f(pushAmount, 0));
            } else if (result.side == CollisionDetector::Side::Top) {
                m_player->setPosition(pos1 + sf::Vector2f(0, pushAmount));
                m_player2->setPosition(pos2 - sf::Vector2f(0, pushAmount));
            } else if (result.side == CollisionDetector::Side::Bottom) {
                m_player->setPosition(pos1 - sf::Vector2f(0, pushAmount));
                m_player2->setPosition(pos2 + sf::Vector2f(0, pushAmount));
            }
            // Stop horizontal velocity if colliding horizontally
            if (result.side == CollisionDetector::Side::Left || result.side == CollisionDetector::Side::Right) {
                m_player->setVelocity(0.0f, m_player->getVelocity().y);
                m_player2->setVelocity(0.0f, m_player2->getVelocity().y);
            }
        }
    }
  }

  // Fireballs vs Players (only in PvP)
  if (Game::getInstance().getProgress().isPvP()) {
      for (auto& fb : m_fireballs) {
          if (!fb->isActive()) continue;
          if (m_player && !m_player->isDead()) {
              auto result = CollisionDetector::checkCollision(*fb, *m_player);
              if (result.collided) {
                  m_player->takeDamage();
                  fb->setActive(false);
              }
          }
          if (m_player2 && !m_player2->isDead()) {
              auto result = CollisionDetector::checkCollision(*fb, *m_player2);
              if (result.collided) {
                  m_player2->takeDamage();
                  fb->setActive(false);
              }
          }
      }
  }

  // Enemy vs Tiles (for patrol direction reversal)
  for (auto &enemy : m_enemies) {
    if (!enemy->isActive() || enemy->isDead())
      continue;
    // Pipe-anchored hazards intentionally overlap solid terrain. Resolving
    // that overlap would push their scripted position out of the pipe and
    // prevent their emergence/retraction cycle from reaching its endpoints.
    if (!enemy->usesTerrainCollisions())
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
          auto *koopa = dynamic_cast<Koopa*>(enemy.get());
          if (koopa && koopa->getKoopaState() == KoopaState::Shell && koopa->isSliding()) {
            koopa->bounce(preVel.x);
          } else {
            // Reverse direction using the pre-collision velocity — resolveCollision()
            // zeroes vel.x (needed for the player's wall-stop), so reading it after
            // the call would always negate zero.
            float newVx = CollisionDetector::reflectHorizontalVelocity(
                preVel.x, result.side, enemy->getSpeed());
            enemy->setVelocity(newVx, enemy->getVelocity().y);
          }
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
          auto *koopa = dynamic_cast<Koopa*>(enemy.get());
          if (koopa && koopa->getKoopaState() == KoopaState::Shell && koopa->isSliding()) {
            koopa->bounce(preVel.x);
          } else {
            float newVx = CollisionDetector::reflectHorizontalVelocity(
                preVel.x, result.side, enemy->getSpeed());
            enemy->setVelocity(newVx, enemy->getVelocity().y);
          }
        }
      }
    }

    // Edge guard: a grounded enemy walking toward a pit (or the end of the
    // map) turns around instead of stepping off and falling. Probes a small
    // column just past the leading edge, at foot level, for any solid tile
    // or active block. Sliding shells skip this so they can fall into pits.
    auto *koopaEdge = dynamic_cast<Koopa*>(enemy.get());
    bool isSlidingShell = koopaEdge && koopaEdge->getKoopaState() == KoopaState::Shell && koopaEdge->isSliding();
    sf::Vector2f enemyVel = enemy->getVelocity();
    if (!isSlidingShell && enemy->isGrounded() && enemyVel.x != 0.0f) {
      const sf::FloatRect bounds = enemy->getBounds();
      constexpr float PROBE_WIDTH = 8.0f;
      const sf::FloatRect probe(
          enemyVel.x > 0.0f ? bounds.left + bounds.width + 2.0f
                            : bounds.left - 2.0f - PROBE_WIDTH,
          bounds.top + bounds.height + 2.0f, PROBE_WIDTH, TILE_SIZE);

      bool groundAhead = false;
      for (Tile *tile : m_tileGrid.query(probe)) {
        if (tile->getBounds().intersects(probe)) {
          groundAhead = true;
          break;
        }
      }
      if (!groundAhead) {
        for (auto &block : m_blocks) {
          if (block->isActive() && block->getBounds().intersects(probe)) {
            groundAhead = true;
            break;
          }
        }
      }
      if (!groundAhead) {
        enemy->setVelocity(-enemyVel.x, enemyVel.y);
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

      auto *koopaA = dynamic_cast<Koopa*>(enemyA.get());
      auto *koopaB = dynamic_cast<Koopa*>(enemyB.get());
      bool shellA = koopaA && koopaA->getKoopaState() == KoopaState::Shell && koopaA->isSliding();
      bool shellB = koopaB && koopaB->getKoopaState() == KoopaState::Shell && koopaB->isSliding();

      if (shellA && !shellB) {
        // Sliding shell A kills/hits enemy B
        if (enemyB->isVulnerable()) {
          const sf::Vector2f pos = boundsCenter(*enemyB);
          enemyB->kill();
          publishEnemyDefeated(*enemyB, pos);
        } else if (koopaB && koopaB->getKoopaState() == KoopaState::Walking) {
          koopaB->onStomped();
        }
      } else if (shellB && !shellA) {
        // Sliding shell B kills/hits enemy A
        if (enemyA->isVulnerable()) {
          const sf::Vector2f pos = boundsCenter(*enemyA);
          enemyA->kill();
          publishEnemyDefeated(*enemyA, pos);
        } else if (koopaA && koopaA->getKoopaState() == KoopaState::Walking) {
          koopaA->onStomped();
        }
      } else {
        // Both not sliding shells: bounce off each other
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
          // Bounce off the ground (counts as one surface hit)
          fb->noteSurfaceHit();
          if (fb->isActive()) {
            sf::Vector2f vel = fb->getVelocity();
            fb->setVelocity(vel.x, -200.0f);
            sf::Vector2f pos = fb->getPosition();
            pos.y -= result.overlap;
            fb->setPosition(pos);
          }
        } else if (result.side == CollisionDetector::Side::Left ||
                   result.side == CollisionDetector::Side::Right) {
          // Reflect off a wall (counts as one surface hit)
          fb->noteSurfaceHit();
          if (fb->isActive()) {
            sf::Vector2f vel = fb->getVelocity();
            fb->setVelocity(-vel.x, vel.y);
          }
        }
      }
    }

    // Fireball vs Enemies
    for (auto &enemy : m_enemies) {
      if (!enemy->isActive() || enemy->isDead())
        continue;
      auto result = CollisionDetector::checkCollision(*fb, *enemy);
      if (result.collided) {
        if (enemy->isVulnerable()) {
          const sf::Vector2f scorePosition = boundsCenter(*enemy);
          enemy->kill();
          publishEnemyDefeated(*enemy, scorePosition);
        }
        fb->setActive(false);
        break;
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
