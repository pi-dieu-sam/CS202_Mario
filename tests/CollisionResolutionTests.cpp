// Regression tests for issue #23: wall collisions must reverse horizontal
// velocity reliably instead of destroying it. See CollisionDetector.hpp's
// reflectHorizontalVelocity() and Level::handleCollisions()'s three
// wall-reversal call sites (Enemy vs Tiles, Enemy vs Blocks, Items vs Tiles).
//
// Uses a small always-on CHECK macro instead of assert(), since Release
// builds (as used in CI) define NDEBUG and would silently strip assert().

#include "Physics/CollisionDetector.hpp"
#include "Physics/PhysicsConstants.hpp"
#include "Core/AssetManager.hpp"
#include "Graphics/SpriteRegistry.hpp"
#include "Level/Level.hpp"
#include "Level/TileGrid.hpp"
#include "Observers/EventManager.hpp"
#include "Entities/Goomba.hpp"
#include "Entities/Koopa.hpp"
#include "Entities/Troopa.hpp"
#include "Entities/Bowser.hpp"
#include "Entities/BowserFireball.hpp"
#include "Entities/Block.hpp"
#include "Factory/EntityFactory.hpp"
#include "Entities/Escalater.hpp"
#include "Entities/PiranhaPlant.hpp"
#include "Entities/Luigi.hpp"
#include "Entities/Mario.hpp"
#include "Entities/Mushroom.hpp"
#include "Entities/FireFlower.hpp"
#include "Entities/FlowersBuff.hpp"
#include "Entities/Coin.hpp"
#include "Entities/Star.hpp"
#include "Entities/Flagpole.hpp"
#include "Entities/Tile.hpp"
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <vector>

static int g_failures = 0;

#define CHECK(cond, msg)                                                     \
  do {                                                                       \
    if (!(cond)) {                                                           \
      std::cerr << "FAIL: " << msg << " (" << __FILE__ << ":" << __LINE__    \
                << ")\n";                                                    \
      ++g_failures;                                                          \
    }                                                                        \
  } while (0)

// Simulates exactly what the fixed Level.cpp call sites do for one entity
// vs. one wall tile: capture velocity, resolve (which zeroes it on a
// horizontal hit), then reflect using the captured pre-collision velocity.
static CollisionDetector::CollisionResult
bounceOnce(GameObject &obj, GameObject &wall, float fallbackSpeed) {
  auto result = CollisionDetector::checkCollision(obj, wall);
  if (result.collided) {
    sf::Vector2f preVel = obj.getVelocity();
    CollisionDetector::resolveCollision(obj, wall, result);
    if (result.side == CollisionDetector::Side::Left ||
        result.side == CollisionDetector::Side::Right) {
      float newVx = CollisionDetector::reflectHorizontalVelocity(
          preVel.x, result.side, fallbackSpeed);
      obj.setVelocity(newVx, obj.getVelocity().y);
    }
  }
  return result;
}

// Positions `wall` so it overlaps `obj`'s right edge by 1px, with a large
// vertical overlap -> checkCollision() resolves this as Side::Right.
// Takes GameObject& (not Tile&) so callers can also place a Block wall.
static void placeWallRightOf(const GameObject &obj, GameObject &wall) {
  sf::FloatRect b = obj.getBounds();
  wall.setPosition(b.left + b.width - 1.0f, b.top);
}

// Mirror of placeWallRightOf: overlaps obj's left edge by 1px -> Side::Left.
static void placeWallLeftOf(const GameObject &obj, GameObject &wall) {
  sf::FloatRect b = obj.getBounds();
  wall.setPosition(b.left - (TILE_SIZE - 1.0f), b.top);
}

// Overlaps obj's bottom edge by 1px, full horizontal overlap -> Side::Bottom.
static void placeWallBelow(const GameObject &obj, Tile &wall) {
  sf::FloatRect b = obj.getBounds();
  wall.setPosition(b.left, b.top + b.height - 1.0f);
}

static void testReflectHelperPure() {
  using Side = CollisionDetector::Side;
  CHECK(CollisionDetector::reflectHorizontalVelocity(-60.0f, Side::Left, 60.0f) == 60.0f,
        "left-hit reversal: -60 -> +60");
  CHECK(CollisionDetector::reflectHorizontalVelocity(60.0f, Side::Right, 60.0f) == -60.0f,
        "right-hit reversal: +60 -> -60");
  CHECK(CollisionDetector::reflectHorizontalVelocity(0.0f, Side::Right, 60.0f) == -60.0f,
        "zero-velocity fallback derives direction from side (Right -> push left)");
  CHECK(CollisionDetector::reflectHorizontalVelocity(0.0f, Side::Left, 60.0f) == 60.0f,
        "zero-velocity fallback derives direction from side (Left -> push right)");
}

// EventManager::publish() used to iterate the live subscriber vector
// directly, so a handler that subscribed or unsubscribed (including its own
// ScopedEventSubscription being destroyed as a side effect of handling the
// very event it's reacting to) mid-dispatch could invalidate the iterator
// being walked. It now dispatches over a snapshot instead.
static void testPublishIsSafeWhenHandlersMutateSubscriptionsMidDispatch() {
  constexpr EventType type = EventType::BlockHit;

  int selfUnsubCount = 0;
  int otherCount = 0;
  ScopedEventSubscription selfUnsubSub;
  selfUnsubSub = ScopedEventSubscription(type, [&](const GameEvent &) {
    ++selfUnsubCount;
    selfUnsubSub.reset(); // unsubscribe self, mid-dispatch
  });
  auto otherSub =
      ScopedEventSubscription(type, [&](const GameEvent &) { ++otherCount; });

  EventManager::getInstance().publish(GameEvent{type});
  CHECK(selfUnsubCount == 1,
        "a handler that unsubscribes itself still runs exactly once");
  CHECK(otherCount == 1,
        "a sibling subscriber still runs even though another handler "
        "mutated the subscriber list mid-dispatch");

  EventManager::getInstance().publish(GameEvent{type});
  CHECK(selfUnsubCount == 1,
        "the self-unsubscribing handler does not run again on a later publish");
  CHECK(otherCount == 2, "the sibling subscriber keeps running normally");

  int lateSubscriberCount = 0;
  ScopedEventSubscription lateSub;
  auto spawnerSub = ScopedEventSubscription(type, [&](const GameEvent &) {
    lateSub = ScopedEventSubscription(
        type, [&](const GameEvent &) { ++lateSubscriberCount; });
  });

  EventManager::getInstance().publish(GameEvent{type});
  CHECK(lateSubscriberCount == 0,
        "a subscriber added mid-dispatch does not receive the publish that "
        "spawned it");

  EventManager::getInstance().publish(GameEvent{type});
  CHECK(lateSubscriberCount == 1,
        "the newly-added subscriber does receive the next publish");
}

static void testGoombaBouncesBothDirections() {
  using Side = CollisionDetector::Side;
  {
    Goomba enemy;
    enemy.setVelocity(enemy.getSpeed(), 0.0f); // moving right into a right-side wall
    Tile wall;
    placeWallRightOf(enemy, wall);
    auto r = bounceOnce(enemy, wall, enemy.getSpeed());
    CHECK(r.side == Side::Right, "Goomba: expected Right-side hit");
    CHECK(enemy.getVelocity().x == -enemy.getSpeed(),
          "Goomba reverses off right wall to -speed");
  }
  {
    Goomba enemy;
    enemy.setVelocity(-enemy.getSpeed(), 0.0f); // moving left into a left-side wall
    Tile wall;
    placeWallLeftOf(enemy, wall);
    auto r = bounceOnce(enemy, wall, enemy.getSpeed());
    CHECK(r.side == Side::Left, "Goomba: expected Left-side hit");
    CHECK(enemy.getVelocity().x == enemy.getSpeed(),
          "Goomba reverses off left wall to +speed");
  }
}

static void testMushroomReversesInsteadOfStopping() {
  {
    Mushroom item; // constructor sets velocity.x = +80
    Tile wall;
    placeWallRightOf(item, wall);
    bounceOnce(item, wall, 0.0f);
    CHECK(item.getVelocity().x == -80.0f,
          "Mushroom reverses (not stops) off right wall");
  }
  {
    Mushroom item;
    item.setVelocity(-80.0f, item.getVelocity().y);
    Tile wall;
    placeWallLeftOf(item, wall);
    bounceOnce(item, wall, 0.0f);
    CHECK(item.getVelocity().x == 80.0f,
          "Mushroom reverses (not stops) off left wall");
  }
}

static void testStarBouncesOffFloor() {
  Star star;
  star.setPosition(0.0f, 0.0f);
  star.setVelocity(100.0f, 300.0f); // falling toward the floor below it
  Tile floor;
  placeWallBelow(star, floor);

  auto result = CollisionDetector::checkCollision(star, floor);
  CHECK(result.collided && result.side == CollisionDetector::Side::Bottom,
        "setup lands the Star on a floor tile below it");

  CollisionDetector::resolveCollision(star, floor, result);
  CHECK(star.getVelocity().y == 0.0f,
        "resolveCollision zeroes vertical velocity like any other landing");

  star.onLanded();
  CHECK(star.getVelocity().y < 0.0f,
        "Star::onLanded() restores upward bounce velocity instead of resting at zero");
}

static void testMovingItemCollidesWithBlocks() {
  Mushroom item; // constructor sets velocity.x = +80
  Block wall(BlockType::Question, 0.0f, 0.0f, LevelTheme::Overworld);
  placeWallRightOf(item, wall);

  auto result = CollisionDetector::checkCollision(item, wall);
  CHECK(result.collided && result.side == CollisionDetector::Side::Right,
        "setup overlaps the Mushroom's right edge against a solid block");

  bounceOnce(item, wall, 0.0f);
  CHECK(item.getVelocity().x == -80.0f,
        "a moving item reverses off a block just like it does off a tile");
}

static void testFallingItemIsCulledBelowLevel() {
  Level level;
  CHECK(level.loadFromFile("assets/levels/level1.txt", "Mario",
                           LevelTheme::Overworld),
        "level 1 loads for the item-culling test");

  // level1.txt already places map items (coins) directly, so compare
  // against a baseline count instead of assuming the level starts empty.
  const std::size_t baseline = level.captureSnapshot().items.size();

  auto mushroom = std::make_unique<Mushroom>();
  mushroom->setPosition(100.0f, level.getHeight() + TILE_SIZE * 2.0f);
  mushroom->setVelocity(0.0f, 0.0f);
  level.addItem(std::move(mushroom));

  CHECK(level.captureSnapshot().items.size() == baseline + 1,
        "the item was actually added to the level before the culling check");

  level.update(FIXED_DT);

  CHECK(level.captureSnapshot().items.size() == baseline,
        "an item far below the level bottom is deactivated and swept up "
        "by removeInactiveEntities() in the same update()");
}

static void testShellIsCulledAfterFallingOffMap() {
  // Koopa::update()'s Shell branch applied gravity and moved the position
  // without ever checking the same fall-off-map bound Enemy::update()
  // enforces for the Walking state, so a shell kicked off a ledge fell
  // forever and stayed active indefinitely instead of being culled like
  // every other enemy.
  Koopa koopa;
  koopa.onStomped();
  koopa.setPosition(koopa.getPosition().x, 700.0f);
  CHECK(koopa.getKoopaState() == KoopaState::Shell,
        "setup: the shell is in Shell state before it falls");
  CHECK(koopa.isActive(), "setup: a freshly-stomped shell starts active");

  bool culled = false;
  for (int i = 0; i < 80; ++i) { // up to 4s of simulated fall, well inside
                                  // the 5s shell-respawn timer
    koopa.update(0.05f);
    if (!koopa.isActive()) {
      culled = true;
      break;
    }
  }
  CHECK(culled,
        "a shell that falls below the level is deactivated instead of "
        "falling forever off-screen");
}

static void testKoopaDyingAndRespawn() {
  {
    Koopa koopa;
    CHECK(koopa.getKoopaState() == KoopaState::Walking,
          "Koopa starts in Walking state");
    CHECK(koopa.isVulnerable() == true,
          "Walking Koopa is vulnerable");

    koopa.onStomped();
    CHECK(koopa.getKoopaState() == KoopaState::Shell,
          "Koopa enters Shell state after stomp");
    CHECK(koopa.isVulnerable() == false,
          "Shell Koopa is not vulnerable");

    // Simulate 5 seconds of update
    for (int i = 0; i < 500; i++) koopa.update(0.01f);
    CHECK(koopa.getKoopaState() == KoopaState::Walking,
          "Koopa respawns to Walking after 5 seconds");
    CHECK(koopa.isVulnerable() == true,
          "Respawned Koopa is vulnerable again");
  }
  {
    Koopa koopa;
    koopa.onStomped();
    koopa.kick(1.0f);
    const float incomingVx = koopa.getVelocity().x;
    koopa.bounce(incomingVx);
    CHECK(koopa.isSliding(),
          "a shell rebounds after it hits a horizontal obstacle");
    CHECK(koopa.getVelocity().x < 0.0f &&
              std::abs(koopa.getVelocity().x) < std::abs(incomingVx),
          "a wall rebound is gentler than the original kicked speed");

    const float reboundStart = koopa.getPosition().x;
    for (int i = 0; i < 500 && koopa.isSliding(); ++i) {
      koopa.update(0.005f);
    }
    const float reboundDistance = std::abs(koopa.getPosition().x - reboundStart);
    CHECK(!koopa.isSliding(),
          "a wall-rebounded shell slows down and stops");
    CHECK(reboundDistance > TILE_SIZE * 3.5f && reboundDistance < TILE_SIZE * 4.1f,
          "a wall-rebounded shell travels about four tiles before stopping");
  }
  {
    Koopa koopa;
    koopa.onStomped();
    for (int i = 0; i < 400; ++i) koopa.update(0.01f);
    koopa.kick(1.0f);
    for (int i = 0; i < 400; ++i) koopa.update(0.01f);
    CHECK(koopa.getKoopaState() == KoopaState::Shell,
          "kicking a shell refreshes its respawn timer");
    for (int i = 0; i < 110; ++i) koopa.update(0.01f);
    CHECK(koopa.getKoopaState() == KoopaState::Walking,
          "a kicked shell respawns after the refreshed timer expires");
  }
  {
    Koopa koopa;
    koopa.kill();
    CHECK(koopa.getKoopaState() == KoopaState::Shell,
          "Koopa enters Shell state after kill()");
    CHECK(koopa.isActive() == true,
          "Shell Koopa remains active");
    CHECK(koopa.isDead() == false,
          "Shell Koopa is not dead (will respawn)");
  }
}

// Regression test for issue #22: the Walking-state sprite stands 1.5
// tiles tall, drawn raised half a tile above position.y, but getBounds()
// used to inherit Enemy's plain 1-tile box -- the top half of the visible
// Koopa could not be hit by anything landing from above.
static void testWalkingKoopaHitboxMatchesSprite() {
  Koopa koopa;
  koopa.setPosition(100.0f, 200.0f);

  sf::FloatRect bounds = koopa.getBounds();
  CHECK(std::abs(bounds.height - (TILE_SIZE * 1.5f - 2.0f)) < 0.001f,
        "a Walking Koopa's hitbox is 1.5 tiles tall, matching its sprite");
  CHECK(std::abs(bounds.top - (200.0f - TILE_SIZE * 0.5f + 1.0f)) < 0.001f,
        "a Walking Koopa's hitbox extends upward to cover its raised head");
  CHECK(std::abs((bounds.top + bounds.height) - (200.0f + TILE_SIZE - 1.0f)) <
            0.001f,
        "a Walking Koopa's hitbox keeps the same ground-aligned bottom edge "
        "its old 1-tile box had");

  koopa.onStomped();
  sf::FloatRect shellBounds = koopa.getBounds();
  CHECK(std::abs(shellBounds.height - (TILE_SIZE - 2.0f)) < 0.001f &&
            std::abs(shellBounds.top - 201.0f) < 0.001f,
        "a Shell Koopa keeps the standard 1-tile hitbox");
}

static void testFlyingTroopa() {
  CHECK(std::filesystem::exists(SpriteRegistry::troopaPath()),
        "Troopa animation uses the checked-in asset");
  CHECK(SpriteRegistry::troopaFrameCount() == 4,
        "Troopa exposes all four animation frames");

  sf::Sprite sprite;
  SpriteRegistry::applyTroopaFrame(
      sprite, 3, sf::FloatRect(0.0f, 0.0f, TILE_SIZE, TILE_SIZE));
  CHECK(sprite.getTextureRect() == sf::IntRect(752, 0, 250, 253),
        "Troopa fourth frame selects its own source image region");

  Troopa troopa;
  troopa.setPosition(100.0f, 200.0f);
  troopa.setVelocity(troopa.getSpeed(), 300.0f);
  troopa.update(1.0f);
  CHECK(std::abs(troopa.getPosition().y - 200.0f) < 0.001f &&
            std::abs(troopa.getVelocity().y) < 0.001f,
        "Troopa ignores gravity and keeps its flying altitude");

  troopa.setVelocity(troopa.getSpeed(), 0.0f);
  Tile wall;
  placeWallRightOf(troopa, wall);
  bounceOnce(troopa, wall, troopa.getSpeed());
  CHECK(troopa.getVelocity().x == -troopa.getSpeed(),
        "Troopa reverses after it collides with a solid obstacle");

  troopa.onStomped();
  CHECK(troopa.isDead() && !troopa.isActive(),
        "stomping a Troopa defeats it immediately");
}

static void testBowserBreathingCycle() {
  CHECK(std::filesystem::exists(SpriteRegistry::bowserPath()) &&
            std::filesystem::exists(SpriteRegistry::bowserBreathPath()),
        "Bowser idle and breathing assets are checked in");
  CHECK(SpriteRegistry::bowserBreathFrameCount() == 6,
        "Bowser breathing animation has six frames");
  CHECK(SpriteRegistry::bowserBreathFrameRect(0) == sf::IntRect(0, 0, 78, 60) &&
            SpriteRegistry::bowserBreathFrameRect(3) == sf::IntRect(234, 0, 91, 60) &&
            SpriteRegistry::bowserBreathFrameRect(5) == sf::IntRect(430, 0, 103, 60),
        "Bowser breathing frame crops use the supplied variable widths");

  Bowser bowser;
  CHECK(bowser.getBounds().width == TILE_SIZE * 2.0f &&
            bowser.getBounds().height == TILE_SIZE * 2.0f,
        "Bowser occupies a two-by-two tile area");
  CHECK(!bowser.canBeStomped() && !bowser.usesTerrainCollisions(),
        "Bowser cannot be stomped and remains fixed at its map position");
  bowser.update(2.0f);
  CHECK(bowser.getState() == Bowser::State::Breathing &&
            bowser.getBreathFrame() == 0,
        "Bowser begins breathing after two seconds idle");
  bowser.update(0.5f);
  CHECK(bowser.getBreathFrame() == 1,
        "Bowser advances to the next breathing frame every half second");
  bowser.update(2.5f);
  CHECK(bowser.getState() == Bowser::State::Idle,
        "Bowser returns to idle after three seconds breathing");

  for (int hit = 0; hit < 4; ++hit) bowser.hitByFireball();
  CHECK(bowser.getFireballHits() == 4 && bowser.isActive() && !bowser.isDead(),
        "Bowser survives the first four fireballs");
  bowser.hitByFireball();
  CHECK(bowser.getFireballHits() == 5 && !bowser.isActive() && bowser.isDead(),
        "Bowser dies on the fifth fireball");
}

static void testBowserFireballs() {
  CHECK(std::filesystem::exists(SpriteRegistry::bowserFirePath()) &&
            SpriteRegistry::bowserFireFrameCount() == 3,
        "Bowser fire uses its three-frame checked-in sprite sheet");

  Bowser bowser;
  bowser.setPosition(1000.0f, 200.0f);
  bowser.updatePlayerPosition(
      sf::Vector2f(1000.0f - TILE_SIZE * 30.0f, 200.0f));
  bowser.update(0.01f);
  CHECK(bowser.takePendingFireballs() == 1,
        "Bowser fires the first idle-phase shot at a player within left range");
  bowser.update(0.70f);
  CHECK(bowser.takePendingFireballs() == 1,
        "Bowser fires the second idle-phase shot on schedule");
  bowser.update(0.70f);
  CHECK(bowser.takePendingFireballs() == 1,
        "Bowser fires the third idle-phase shot on schedule");

  Bowser outOfRangeBowser;
  outOfRangeBowser.setPosition(1000.0f, 200.0f);
  outOfRangeBowser.updatePlayerPosition(sf::Vector2f(1001.0f, 200.0f));
  outOfRangeBowser.update(1.5f);
  CHECK(outOfRangeBowser.takePendingFireballs() == 0,
        "Bowser does not fire at a player to its right or outside left range");

  BowserFireball fireball(200.0f, 100.0f);
  fireball.update(0.1f);
  CHECK(fireball.getPosition().x < 200.0f,
        "Bowser fireball travels left from Bowser");
  fireball.update(5.0f);
  CHECK(!fireball.isActive(),
        "Bowser fireball expires if it does not hit a solid object");
}

// Regression tests for issue #22: Level's star-power branch used to call
// onStomped() and publish EnemyDefeated unconditionally. onStomped() only
// changes state for some enemies -- a no-op for Bowser (he is immune to
// contact and dies only to five fireballs), Walking -> Shell for Koopa --
// so touching them with star power falsely reported a kill and paid out
// score while the enemy stayed fully alive on the field. The fix only
// publishes when onStomped() actually leaves the enemy dead.
static void testStarPowerOnlyDefeatsEnemiesItActuallyKills() {
  // level2.txt places a Bowser ('B') at map column 180, row 6 (world row
  // 10 once the four-row offset for its 15-row map is applied).
  {
    Level level;
    CHECK(level.loadFromFile("assets/levels/level2.txt", "Mario",
                             LevelTheme::Castle),
          "level 2 loads for the star-power-vs-Bowser test");
    Player *player = level.getPlayer();
    CHECK(player != nullptr, "level 2 creates a player");
    if (!player) return;

    int defeatedEvents = 0;
    auto sub = ScopedEventSubscription(
        EventType::EnemyDefeated,
        [&defeatedEvents](const GameEvent &) { ++defeatedEvents; });

    player->setStarPower(5.0f);
    player->setPosition(180.0f * TILE_SIZE, 10.0f * TILE_SIZE);
    level.update(0.0f);

    CHECK(defeatedEvents == 0,
          "star power touching Bowser does not publish a false EnemyDefeated");
  }

  // level1.txt places a Koopa ('K') and, on the same row, a Goomba ('G')
  // at world row 17 (map row 12 plus the five-row offset for its 14-row
  // map) -- map columns 16 and 24, eight tiles apart.
  {
    Level level;
    CHECK(level.loadFromFile("assets/levels/level1.txt", "Mario",
                             LevelTheme::Overworld),
          "level 1 loads for the star-power-vs-Koopa test");
    Player *player = level.getPlayer();
    CHECK(player != nullptr, "level 1 creates a player");
    if (!player) return;

    int defeatedEvents = 0;
    auto sub = ScopedEventSubscription(
        EventType::EnemyDefeated,
        [&defeatedEvents](const GameEvent &) { ++defeatedEvents; });

    player->setStarPower(5.0f);
    player->setPosition(16.0f * TILE_SIZE, 17.0f * TILE_SIZE);
    level.update(0.0f);

    CHECK(defeatedEvents == 0,
          "star power turning a Walking Koopa into a Shell is a state "
          "change, not a defeat, and must not publish EnemyDefeated");

    // Move to the Goomba eight tiles over -- far enough from the Koopa's
    // tile that only this second touch can collide.
    player->setPosition(24.0f * TILE_SIZE, 17.0f * TILE_SIZE);
    level.update(0.0f);

    CHECK(defeatedEvents == 1,
          "star power still defeats -- and reports -- an enemy that "
          "onStomped() actually kills");
  }
}

// Level publishes EnemyDefeated with enemy.getScoreValue() as the score
// payload (see publishEnemyDefeated() in Level.cpp). This locks in the
// per-enemy score tier so a future change to one enemy's difficulty
// weighting doesn't silently change its payout.
static void testEnemyScoreValuesMatchDifficultyTier() {
  Goomba goomba;
  CHECK(goomba.getScoreValue() == 100, "Goomba is worth 100 points");

  Koopa koopa;
  CHECK(koopa.getScoreValue() == 200, "Koopa is worth 200 points");

  Troopa troopa;
  CHECK(troopa.getScoreValue() == 400, "Flying Troopa is worth 400 points");

  PiranhaPlant piranha;
  CHECK(piranha.getScoreValue() == 100, "Piranha Plant is worth 100 points");

  Bowser bowser;
  CHECK(bowser.getScoreValue() == 1000, "Bowser is worth 1000 points");
}

static void testHorizontalEscalaterMovement() {
  Escalater platform(400.0f, 200.0f,
                     Escalater::MovementAxis::Horizontal);
  CHECK(platform.movesHorizontally(),
        "horizontal escalater reports its movement axis");

  float minX = platform.getPosition().x;
  float maxX = minX;
  for (int frame = 0; frame < 1000; ++frame) {
    platform.update(0.01f);
    minX = std::min(minX, platform.getPosition().x);
    maxX = std::max(maxX, platform.getPosition().x);
  }
  CHECK(minX >= 400.0f - TILE_SIZE * 5.0f - 0.01f &&
            maxX <= 400.0f + TILE_SIZE * 5.0f + 0.01f,
        "horizontal escalater remains within five tiles of its map position");
  CHECK(maxX > 400.0f + TILE_SIZE * 4.5f &&
            minX < 400.0f - TILE_SIZE * 4.5f,
        "horizontal escalater reaches both sides of its five-tile range");

  platform.reverseDirection();
  CHECK(platform.getVelocity().x < 0.0f && platform.getVelocity().y == 0.0f,
        "reversing a horizontal escalater changes only horizontal velocity");
}

static void testLevel2LavaTilesKillPlayer() {
  // level2 uses `l` for the animated flame surface (map row 13) and `L` for
  // the lava beneath it (map row 14). LevelLoader offsets this 15-row map by
  // four rows to align it to the 19-row window.
  const auto checkLavaRow = [](int worldRow, const char *name) {
    Level level;
    CHECK(level.loadFromFile("assets/levels/level2.txt", "Mario",
                             LevelTheme::Castle),
          "level 2 loads for lava collision test");
    Player *player = level.getPlayer();
    CHECK(player != nullptr, "level 2 creates a player");
    if (!player) return;

    player->setPosition(7.0f * TILE_SIZE,
                        worldRow * TILE_SIZE - TILE_SIZE);
    level.update(0.0f);
    CHECK(player->isDead(), name);
  };

  checkLavaRow(17, "`l` flame tile kills a player on contact");
  checkLavaRow(18, "`L` lava tile kills a player on contact");
}

static void testLevel2VineEntersClimbState() {
  // level2.txt places a vertical V vine at map column 85, rows 3-9. Its
  // first cell lands at world row 6 after the three-row display offset; the
  // X tile directly above it verifies that upward climbing stops cleanly.
  Level level;
  CHECK(level.loadFromFile("assets/levels/level2.txt", "Mario",
                           LevelTheme::Castle),
        "level 2 loads for vine climbing test");
  Player *player = level.getPlayer();
  CHECK(player != nullptr, "level 2 creates a player for vine climbing test");
  if (!player) return;

  // Player position is a two-tile anchor; its small-form body begins 32px
  // below it. Position that body inside the first V tile at (85, 6).
  player->setPosition(85.0f * TILE_SIZE - 26.0f,
                      6.0f * TILE_SIZE - TILE_SIZE);
  level.update(0.0f);
  CHECK(player->isClimbing(),
        "touching a V map tile enters the climb state without terrain resolve");
  CHECK(std::abs(player->getPosition().x - 85.0f * TILE_SIZE) < 0.001f,
        "entering a V tile centres the player on the vine");

  const float startY = player->getPosition().y;
  player->climbUp(FIXED_DT);
  level.update(FIXED_DT);
  CHECK(player->isClimbing() &&
            std::abs(player->getPosition().y -
                     startY) < 0.01f,
        "a solid tile above the vine stops upward climbing without ejecting the player");

  player->climbDown(FIXED_DT);
  level.update(FIXED_DT);
  CHECK(player->isClimbing() &&
            std::abs(player->getPosition().y -
                     (startY + 120.0f * FIXED_DT)) < 0.01f,
        "climbing down moves smoothly when the vine path is clear");
}

static void testGoombaStompDisablesCollisionImmediately() {
  Goomba goomba;
  goomba.onStomped();
  CHECK(goomba.isActive(),
        "a squished Goomba remains active only for its death sprite");
  CHECK(goomba.isDead(), "a stomped Goomba is marked dead immediately");
  CHECK(!goomba.isVulnerable(),
        "a squished Goomba is excluded from collision damage immediately");

  goomba.update(0.2f);
  CHECK(goomba.isActive(), "the squished sprite remains visible during its timer");
  goomba.update(0.4f);
  CHECK(!goomba.isActive(),
        "the Goomba is removed after its squished sprite timer expires");
}

static void testEnlargedPlayersCanHitBlocksWithCompactBody() {
  Block block(BlockType::Question, 100.0f, 128.0f,
              LevelTheme::Overworld);

  auto checkHeadHit = [&](Player& player, const char* form) {
    // The compact body is just one pixel into the underside of the block.
    // A Fire player's full body would already overlap most of the block here.
    player.setPosition(100.0f, 127.0f);
    player.setVelocity(0.0f, PLAYER_JUMP);
    const auto result = CollisionDetector::checkCollision(
        player.getBlockInteractionBounds(), player.getVelocity(),
        block.getBounds(), block.getVelocity());
    CHECK(result.collided && result.side == CollisionDetector::Side::Top,
          form);
    CHECK(player.getBlockInteractionBounds().height == TILE_SIZE - 2.0f,
          "enlarged players retain the compact question-block interaction body");
  };

  Mario fireMario;
  fireMario.enableFire();
  CHECK(fireMario.getBounds().height > fireMario.getBlockInteractionBounds().height,
        "Fire Mario has an enlarged terrain/render body");
  checkHeadHit(fireMario,
               "Fire Mario's compact body reports a hit from below");

  Luigi buffLuigi;
  buffLuigi.applySizeBuff();
  for (int frame = 0; frame < 5; ++frame) {
    buffLuigi.update(0.15f);
  }
  CHECK(buffLuigi.getBounds().height >
            buffLuigi.getBlockInteractionBounds().height,
        "FlowersBuff Luigi has an enlarged terrain/render body");
  checkHeadHit(buffLuigi,
               "FlowersBuff Luigi's compact body reports a hit from below");
}

static void testBrickBreaksOnThirdHit() {
  auto brick = EntityFactory::createBlock('S', 100.0f, 128.0f,
                                          LevelTheme::Overworld);
  CHECK(brick && brick->getBlockType() == BlockType::Brick,
        "S loads as a brick block");
  if (!brick) return;

  brick->hit(false);
  CHECK(brick->isActive(), "brick remains after its first hit");
  brick->hit(false);
  CHECK(brick->isActive(), "brick remains after its second hit");
  brick->hit(false);
  CHECK(!brick->isActive(), "brick breaks on its third hit");
}

static void testMushroomBlockSpawnsMushroomItem() {
  auto block = EntityFactory::createBlock('M', 100.0f, 128.0f,
                                          LevelTheme::Overworld);
  CHECK(block && block->getBlockType() == BlockType::Question,
        "M loads as a Question block");
  if (!block) return;

  auto spawned = block->hit(false);
  CHECK(spawned != nullptr, "hitting an M block spawns an item");
  if (!spawned) return;
  CHECK(spawned->getType() == ObjectType::Mushroom,
        "an M block spawns a Mushroom, not a Coin or anything else");
}

static void testFireFlowerBlockSpawnsFireFlowerItem() {
  auto block = EntityFactory::createBlock('F', 100.0f, 128.0f,
                                          LevelTheme::Overworld);
  CHECK(block && block->getBlockType() == BlockType::Question,
        "F loads as a Question block");
  if (!block) return;

  auto spawned = block->hit(false);
  CHECK(spawned != nullptr, "hitting an F block spawns an item");
  if (!spawned) return;
  CHECK(spawned->getType() == ObjectType::FireFlower,
        "an F block spawns a Fire Flower, not a Coin or anything else");
}

static void testCollectingMushroomGrowsPlayer() {
  auto block = EntityFactory::createBlock('M', 100.0f, 128.0f,
                                          LevelTheme::Overworld);
  if (!block) return;
  auto spawned = block->hit(false);
  if (!spawned) return;

  Mario player; // starts PowerUpState::Small
  CHECK(player.getPowerUpState() == PowerUpState::Small,
        "setup: player starts Small before collecting the Mushroom");

  spawned->activate(player);
  CHECK(player.getPowerUpState() == PowerUpState::Big,
        "collecting the spawned Mushroom actually grows the player to Big");
}

static void testCollectingFireFlowerGrantsFireState() {
  auto block = EntityFactory::createBlock('F', 100.0f, 128.0f,
                                          LevelTheme::Overworld);
  if (!block) return;
  auto spawned = block->hit(false);
  if (!spawned) return;

  Mario player; // starts PowerUpState::Small
  CHECK(player.getPowerUpState() == PowerUpState::Small,
        "setup: player starts Small before collecting the Fire Flower");

  spawned->activate(player);
  CHECK(player.getPowerUpState() == PowerUpState::Fire,
        "collecting the spawned Fire Flower actually grants the Fire state");
}

static void testCoinCollectionPublishesConfiguredScore() {
  Coin coin;
  Mario player;

  int lastScore = -1;
  int publishCount = 0;
  auto sub = ScopedEventSubscription(
      EventType::CoinCollected, [&](const GameEvent &e) {
        ++publishCount;
        lastScore = e.intData;
      });

  coin.activate(player);

  CHECK(publishCount == 1, "collecting a coin publishes exactly one CoinCollected event");
  CHECK(lastScore == COIN_SCORE,
        "the CoinCollected payload carries the configured coin score");
}

static void testPowerUpCollectionPublishesFlatBonusScore() {
  // Every power-up (Mushroom, Fire Flower, Flowers Buff, Star) publishes
  // PowerUpCollected with the same flat 1000-point bonus regardless of
  // which effect it grants (see each Item subclass's activate()).
  Mario player;
  int lastScore = -1;
  int publishCount = 0;
  auto sub = ScopedEventSubscription(
      EventType::PowerUpCollected, [&](const GameEvent &e) {
        ++publishCount;
        lastScore = e.intData;
      });

  Mushroom mushroom;
  mushroom.activate(player);
  CHECK(publishCount == 1 && lastScore == 1000,
        "collecting a Mushroom publishes a 1000-point PowerUpCollected event");

  FireFlower fireFlower;
  fireFlower.activate(player);
  CHECK(publishCount == 2 && lastScore == 1000,
        "collecting a Fire Flower publishes a 1000-point PowerUpCollected event");

  FlowersBuff flowersBuff;
  flowersBuff.activate(player);
  CHECK(publishCount == 3 && lastScore == 1000,
        "collecting a Flowers Buff publishes a 1000-point PowerUpCollected event");

  Star star;
  star.activate(player);
  CHECK(publishCount == 4 && lastScore == 1000,
        "collecting a Star publishes a 1000-point PowerUpCollected event");
}

static void testLevel1PlacesReachableCoinBlocks() {
  Level level;
  CHECK(level.loadFromFile("assets/levels/level1.txt", "Mario",
                           LevelTheme::Overworld),
        "level 1 loads for the coin-block placement guard test");

  const auto blocks = level.captureSnapshot().blocks;
  int coinQuestionBlockCount = 0;
  for (const auto &block : blocks) {
    if (block.containedItem == static_cast<int>(ObjectType::Coin)) {
      ++coinQuestionBlockCount;
    }
  }
  CHECK(coinQuestionBlockCount >= 2,
        "level 1's shipped map retains its two coin question blocks");
}

static void testLevel2PlacesAReachableCoinBlock() {
  Level level;
  CHECK(level.loadFromFile("assets/levels/level2.txt", "Mario",
                           LevelTheme::Castle),
        "level 2 loads for the coin-block placement guard test");

  const auto blocks = level.captureSnapshot().blocks;
  bool foundCoinQuestionBlock = false;
  for (const auto &block : blocks) {
    if (block.containedItem == static_cast<int>(ObjectType::Coin)) {
      foundCoinQuestionBlock = true;
      break;
    }
  }
  CHECK(foundCoinQuestionBlock,
        "level 2's shipped map contains at least one coin question block");
}

static void testShippedLevelsAdvertiseConfiguredEnemiesAndPowerUps() {
  const char *levelFiles[] = {"assets/levels/level1.txt",
                              "assets/levels/level2.txt",
                              "assets/levels/level3.txt"};

  std::string combined;
  for (const char *path : levelFiles) {
    std::ifstream file(path);
    CHECK(file.is_open(), std::string("shipped level file opens: ") + path);
    combined += std::string((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());
  }

  struct AdvertisedSymbol {
    char symbol;
    const char *label;
  };
  const AdvertisedSymbol advertised[] = {
      {'G', "Goomba"},      {'K', "Koopa"},      {'P', "Piranha Plant"},
      {'?', "coin Question Block"}, {'s', "Star"}, {'W', "Flowers Buff"},
  };
  for (const auto &entry : advertised) {
    CHECK(combined.find(entry.symbol) != std::string::npos,
          std::string("the shipped levels contain at least one ") +
              entry.label + " (regression guard for issue #26)");
  }
}

// Confirms resolveCollision() itself is unchanged: the player's wall-stop
// behavior (which never calls reflectHorizontalVelocity) must keep zeroing
// horizontal velocity on Left/Right and vertical velocity on Bottom/Top.
static void testResolveCollisionAloneStillZeroesVelocity() {
  using Side = CollisionDetector::Side;
  {
    Goomba enemy;
    enemy.setVelocity(50.0f, 0.0f);
    Tile wall;
    placeWallRightOf(enemy, wall);
    auto r = CollisionDetector::checkCollision(enemy, wall);
    CollisionDetector::resolveCollision(enemy, wall, r);
    CHECK(r.side == Side::Right, "expected Right-side hit for non-regression check");
    CHECK(enemy.getVelocity().x == 0.0f,
          "resolveCollision alone still zeroes vel.x (player wall-stop unchanged)");
  }
  {
    Goomba enemy;
    enemy.setVelocity(0.0f, 100.0f);
    Tile ground;
    placeWallBelow(enemy, ground);
    auto r = CollisionDetector::checkCollision(enemy, ground);
    CollisionDetector::resolveCollision(enemy, ground, r);
    CHECK(r.side == Side::Bottom, "expected Bottom-side hit for non-regression check");
    CHECK(enemy.getVelocity().y == 0.0f,
          "resolveCollision still zeroes vel.y on landing (unchanged)");
  }
}

static void testTileGridExcludesDistantTiles() {
  std::vector<std::unique_ptr<Tile>> tiles;
  auto nearby = std::make_unique<Tile>();
  nearby->setPosition(0.0f, 0.0f);
  Tile* nearbyPtr = nearby.get();
  tiles.push_back(std::move(nearby));

  auto distant = std::make_unique<Tile>();
  distant->setPosition(TILE_SIZE * 100.0f, TILE_SIZE * 100.0f);
  tiles.push_back(std::move(distant));

  TileGrid grid;
  grid.build(tiles);
  auto candidates = grid.query(sf::FloatRect(1.0f, 1.0f, 8.0f, 8.0f));
  CHECK(candidates.size() == 1 && candidates.front() == nearbyPtr,
        "tile grid returns only tiles adjacent to the queried bounds");
}

static void testUpwardEdgeHitResolvesAsWall() {
  Goomba jumper;
  // Jumper bounds: x=[1,31], y=[29,59]. The block has only 1px horizontal
  // overlap but 3px vertical overlap at its lower-left corner.
  jumper.setPosition(0.0f, 28.0f);
  jumper.setVelocity(0.0f, -100.0f);
  Tile block;
  block.setPosition(30.0f, 0.0f);

  auto result = CollisionDetector::checkCollision(jumper, block);
  CHECK(result.collided, "upward edge-hit setup overlaps the block");
  CHECK(result.side == CollisionDetector::Side::Right,
        "upward edge hit resolves as a wall, not a vertical collision");

  // This is the former snagging case: the player only catches 3px of the
  // block's edge, while the upward head penetration is only 1px. Comparing
  // overlap magnitudes alone selects Side::Top and immediately stops a jump.
  jumper.setPosition(0.0f, 30.0f);
  block.setPosition(28.0f, 0.0f);
  result = CollisionDetector::checkCollision(jumper, block);
  CHECK(result.collided, "upward corner-snag setup overlaps the block");
  CHECK(result.side == CollisionDetector::Side::Right,
        "narrow upward corner overlap resolves as a wall, not a head hit");
}

static void testSweptStompCatchesTunneling() {
  Mario player;
  Goomba enemy;
  enemy.setPosition(0.0f, 100.0f);
  enemy.setVelocity(0.0f, 0.0f);

  // End the step fully below the Goomba so a discrete overlap test misses it.
  // The player's prior bounds were above the Goomba and crossed its top.
  player.setPosition(0.0f, 128.0f);
  player.setVelocity(0.0f, 1400.0f);
  constexpr float dt = 0.1f;

  CHECK(!CollisionDetector::checkCollision(player, enemy).collided,
        "tunneling setup has no final-frame overlap");
  auto result = CollisionDetector::checkSweptCollision(player, enemy, dt);
  CHECK(result.collided && result.swept,
        "swept collision catches an enemy crossed during the frame");
  CHECK(result.side == CollisionDetector::Side::Bottom,
        "downward swept collision is classified as a stomp");

  CollisionDetector::moveToImpact(player, result);
  const float playerBottom = player.getBounds().top + player.getBounds().height;
  const float enemyTop = enemy.getBounds().top;
  CHECK(std::abs(playerBottom - enemyTop) < 0.001f,
        "swept stomp moves the player back to the impact point");
}

static void testSprintDoesNotCompoundVelocity() {
  Mario player;
  const float sprintSpeed = PLAYER_SPEED * PLAYER_SPRINT;
  player.setVelocity(sprintSpeed, 0.0f);
  player.setSprinting(true); // Sprint held, but no direction command this frame.
  player.update(FIXED_DT);

  CHECK(player.getVelocity().x > 0.0f && player.getVelocity().x < sprintSpeed,
        "releasing direction while sprinting decelerates instead of compounding speed");

  player.setSprinting(true);
  player.moveRight(FIXED_DT);
  CHECK(player.getVelocity().x == sprintSpeed,
        "sprint changes commanded movement speed to the configured maximum");
}

static void testAllLuigiSpriteStatesLoad() {
  const std::vector<PowerUpState> powers = {
      PowerUpState::Small, PowerUpState::Big, PowerUpState::Fire};
  const std::vector<SpriteRegistry::PlayerAnim> animations = {
      SpriteRegistry::PlayerAnim::Idle, SpriteRegistry::PlayerAnim::Walk,
      SpriteRegistry::PlayerAnim::Jump, SpriteRegistry::PlayerAnim::Fire,
      SpriteRegistry::PlayerAnim::Skid, SpriteRegistry::PlayerAnim::Climb};

  for (PowerUpState power : powers) {
    for (SpriteRegistry::PlayerAnim animation : animations) {
      const int frameCount = SpriteRegistry::playerFrameCount(
          CharacterId::Luigi, power, animation);
      CHECK(frameCount > 0,
            "Luigi animation exposes at least one frame");

      for (int frame = 0; frame < frameCount; ++frame) {
        const std::string& path = SpriteRegistry::playerPath(
            CharacterId::Luigi, power, animation, frame);
        CHECK(std::filesystem::exists(path),
              "every modeled Luigi state has a registered asset file");

        sf::Image image;
        CHECK(image.loadFromFile(path) && image.getSize().x > 0 &&
                  image.getSize().y > 0,
              "every registered Luigi asset decodes successfully");

        sf::Sprite sprite;
        SpriteRegistry::applyPlayerFrame(
            sprite, CharacterId::Luigi, power, animation, frame,
            sf::FloatRect(0.0f, 0.0f, 32.0f, 32.0f));
        const sf::IntRect rect = sprite.getTextureRect();
        CHECK(rect.left >= 0 && rect.top >= 0 &&
                  rect.left + rect.width <=
                      static_cast<int>(image.getSize().x) &&
                  rect.top + rect.height <=
                      static_cast<int>(image.getSize().y),
              "Luigi animation frame crop stays inside its sheet");
      }
    }
  }

  // Exact frame counts for the new Luigi Character/ sheets.
  for (PowerUpState power : powers) {
    CHECK(SpriteRegistry::playerFrameCount(
              CharacterId::Luigi, power, SpriteRegistry::PlayerAnim::Idle) == 3,
          "Luigi Stand sheet exposes its 3 idle frames");
    CHECK(SpriteRegistry::playerFrameCount(
              CharacterId::Luigi, power, SpriteRegistry::PlayerAnim::Walk) == 8,
          "Luigi Walk sheet exposes its 8 walk frames");
    CHECK(SpriteRegistry::playerFrameCount(
              CharacterId::Luigi, power, SpriteRegistry::PlayerAnim::Jump) == 5,
          "Luigi Jump sheet exposes its 5 jump frames");
    CHECK(SpriteRegistry::playerFrameCount(
              CharacterId::Luigi, power, SpriteRegistry::PlayerAnim::Fire) == 1,
          "Luigi Fire sheet exposes its 1 shoot frame");
    CHECK(SpriteRegistry::playerFrameCount(
              CharacterId::Luigi, power, SpriteRegistry::PlayerAnim::Climb) == 4,
          "Luigi Climb sheet exposes its 4 climbing frames");
  }
}

static void testAllMarioSpriteStatesLoad() {
  const std::vector<PowerUpState> powers = {
      PowerUpState::Small, PowerUpState::Big, PowerUpState::Fire};
  const std::vector<SpriteRegistry::PlayerAnim> animations = {
      SpriteRegistry::PlayerAnim::Idle, SpriteRegistry::PlayerAnim::Walk,
      SpriteRegistry::PlayerAnim::Jump, SpriteRegistry::PlayerAnim::Fire,
      SpriteRegistry::PlayerAnim::Climb};

  for (PowerUpState power : powers) {
    for (SpriteRegistry::PlayerAnim animation : animations) {
      const int frameCount = SpriteRegistry::playerFrameCount(
          CharacterId::Mario, power, animation);
      CHECK(frameCount > 1,
            "every new Mario animation exposes its multi-frame sheet");

      for (int frame = 0; frame < frameCount; ++frame) {
        const std::string &path = SpriteRegistry::playerPath(
            CharacterId::Mario, power, animation, frame);
        CHECK(std::filesystem::exists(path),
              "every modeled Mario state has a registered asset file");

        sf::Image image;
        CHECK(image.loadFromFile(path) && image.getSize().x > 0 &&
                  image.getSize().y > 0,
              "every registered Mario asset decodes successfully");

        sf::Sprite sprite;
        SpriteRegistry::applyPlayerFrame(
            sprite, CharacterId::Mario, power, animation, frame,
            sf::FloatRect(0.0f, 0.0f, 32.0f, 32.0f));
        const sf::IntRect rect = sprite.getTextureRect();
        CHECK(rect.left >= 0 && rect.top >= 0 &&
                  rect.left + rect.width <=
                      static_cast<int>(image.getSize().x) &&
                  rect.top + rect.height <=
                      static_cast<int>(image.getSize().y),
              "Mario animation frame crop stays inside its sheet");
      }
    }
  }

  // Exact frame counts for the new Mario Character/ sheets.
  for (PowerUpState power : powers) {
    CHECK(SpriteRegistry::playerFrameCount(
              CharacterId::Mario, power, SpriteRegistry::PlayerAnim::Idle) == 4,
          "Mario Stand sheet exposes its 4 idle frames");
    CHECK(SpriteRegistry::playerFrameCount(
              CharacterId::Mario, power, SpriteRegistry::PlayerAnim::Walk) == 6,
          "Mario Walk sheet exposes its 6 walk frames");
    CHECK(SpriteRegistry::playerFrameCount(
              CharacterId::Mario, power, SpriteRegistry::PlayerAnim::Jump) == 3,
          "Mario Jump sheet exposes its 3 jump frames");
    CHECK(SpriteRegistry::playerFrameCount(
              CharacterId::Mario, power, SpriteRegistry::PlayerAnim::Fire) == 2,
          "Mario Fire sheet exposes its 2 shoot frames");
    CHECK(SpriteRegistry::playerFrameCount(
              CharacterId::Mario, power, SpriteRegistry::PlayerAnim::Climb) == 2,
          "Mario Climb sheet exposes its 2 climbing frames");
  }
}

static void testVineClimbingControls() {
  Mario player;
  player.setPosition(128.0f, 128.0f);

  player.updateVineContact(true, 128.0f);
  CHECK(player.isClimbing(), "touching a V tile enters the climbing state");
  CHECK(player.getVelocity().x == 0.0f && player.getVelocity().y == 0.0f,
        "entering a vine stops the existing movement");

  const float startY = player.getPosition().y;
  player.climbUp(FIXED_DT);
  player.update(FIXED_DT);
  CHECK(player.getPosition().y < startY,
        "climb-up input moves the player upward without gravity");

  const float afterTapY = player.getPosition().y;
  player.update(FIXED_DT);
  CHECK(std::abs(player.getPosition().y - afterTapY) < 0.001f,
        "releasing climb input stops vertical movement immediately");

  const float climbedY = player.getPosition().y;
  player.climbDown(FIXED_DT);
  player.update(FIXED_DT);
  CHECK(player.getPosition().y > climbedY,
        "climb-down input moves the player downward");

  // The right key used to approach the vine can still be held when contact
  // begins. It must not detach until released and pressed again.
  player.setVineHorizontalInput(true);
  player.moveRight(FIXED_DT);
  CHECK(player.isClimbing(),
        "held approach direction does not instantly cancel vine climbing");
  player.setVineHorizontalInput(false);
  player.moveRight(FIXED_DT);
  CHECK(!player.isClimbing() && player.getVelocity().x > 0.0f,
        "right input detaches from the vine and starts horizontal movement");

  player.updateVineContact(true, 128.0f);
  CHECK(!player.isClimbing(),
        "a horizontal detach cannot immediately reattach while still overlapping");
  player.updateVineContact(false, 0.0f);
  player.updateVineContact(true, 128.0f);
  CHECK(player.isClimbing(),
        "leaving the vine tile re-enables climbing on the next contact");
}

static void testGifFramesFallsBackToOneFrameForMissingFile() {
  // getGifFrames() falls back to SFML's single-frame sf::Texture::loadFromFile
  // whenever the STB multi-frame decode doesn't produce any frames -- a
  // nonexistent path exercises that fallback without needing a crafted
  // malformed GIF fixture. It must degrade to a 1-frame (if empty) result,
  // never a crash or an empty vector.
  const std::string missingPath = "assets/gifs/does-not-exist.gif";
  CHECK(!std::filesystem::exists(missingPath),
        "setup: the path used for this test does not exist on disk");

  const auto &frames = AssetManager::getInstance().getGifFrames(missingPath);
  CHECK(frames.size() == 1,
        "a missing GIF file still returns exactly one (empty) fallback frame");
  CHECK(AssetManager::getInstance().getGifFrameCount(missingPath) == 1,
        "getGifFrameCount agrees with getGifFrames for the same fallback path");

  // A second call must hit the cache and return the same result, not
  // re-attempt the failed load.
  const auto &framesAgain = AssetManager::getInstance().getGifFrames(missingPath);
  CHECK(&framesAgain == &frames,
        "a repeated lookup for the same missing path returns the cached result");
}

static void testPiranhaFramesAndEmergenceStayStable() {
  const std::string &path = SpriteRegistry::piranhaPlantPath(0);
  CHECK(std::filesystem::exists(path),
        "Piranha animation uses a checked-in asset");

  const auto &frames = AssetManager::getInstance().getGifFrames(path);
  CHECK(SpriteRegistry::piranhaFrameCount() == 2 && frames.size() == 2,
        "Piranha animation exposes the two mouth poses from its GIF");

  for (int frame = 0; frame < SpriteRegistry::piranhaFrameCount(); ++frame) {
    const sf::IntRect rect = SpriteRegistry::piranhaFrameRect(frame);
    const sf::Vector2u size = frames[static_cast<size_t>(frame)].getSize();
    CHECK(rect.left == 0 && rect.top == 0 && rect.width == 16 &&
              rect.height == 24,
          "each Piranha pose is a fixed-size 16x24 source frame");
    CHECK(rect.left + rect.width <= static_cast<int>(size.x) &&
              rect.top + rect.height <= static_cast<int>(size.y),
          "every Piranha frame rectangle stays inside its texture");

    sf::Sprite sprite;
    SpriteRegistry::applyPiranhaFrame(
        sprite, frame, sf::FloatRect(0.0f, 0.0f, 32.0f, 48.0f));
    CHECK(sprite.getTextureRect() == rect,
          "Piranha rendering selects exactly one decoded GIF frame");
    CHECK(std::abs(sprite.getScale().x - 2.0f) < 0.001f &&
              std::abs(sprite.getScale().y - 2.0f) < 0.001f,
          "Piranha poses render at a stable NES 2x scale");
  }

  PiranhaPlant plant;
  plant.setPosition(64.0f, 64.0f);
  plant.update(0.0f); // capture the pipe-relative base position
  CHECK(!plant.usesTerrainCollisions(),
        "pipe-anchored Piranha opts out of walking-enemy terrain response");
  CHECK(plant.getBounds().width == 0.0f,
        "Piranha starts fully hidden inside the pipe");

  plant.update(2.0f); // hidden -> emerging
  float previousHeight = 0.0f;
  for (int step = 0; step < 6; ++step) {
    plant.update(0.1f);
    const float height = plant.getBounds().height;
    CHECK(height >= previousHeight && height <= 48.0f,
          "Piranha emergence grows monotonically to its fixed height");
    previousHeight = height;
  }
  CHECK(std::abs(previousHeight - 48.0f) < 0.001f &&
            std::abs(plant.getBounds().width - 28.0f) < 0.001f,
        "fully emerged Piranha has a stable 32x48 visual and inset hitbox");
  const sf::FloatRect emergedBounds = plant.getBounds();
  CHECK(std::abs(emergedBounds.left + emergedBounds.width / 2.0f - 96.0f) <
            0.001f,
        "Piranha stays centered over the two-tile pipe anchored at x=64");

  plant.update(1.0f);
  CHECK(std::abs(plant.getBounds().height - 48.0f) < 0.001f,
        "mouth animation does not move the plant while it waits");

  plant.update(1.5f); // waiting -> retracting
  plant.update(0.1f);
  CHECK(plant.getBounds().height < 48.0f,
        "Piranha retracts only through its movement state machine");

  for (int step = 0; step < 5; ++step) plant.update(0.1f);
  CHECK(plant.getBounds().width == 0.0f,
        "Piranha becomes fully hidden after retracting");

  plant.update(2.0f); // hidden -> emerging again
  plant.update(0.1f);
  CHECK(plant.getBounds().height > 0.0f &&
            std::abs(plant.getBounds().left + plant.getBounds().width / 2.0f -
                     96.0f) < 0.001f,
        "Piranha begins the next emergence cycle at the same pipe center");
}

// Regression tests for issue #22: every way the player can hit a Piranha
// Plant needs its own defined outcome. A stomp must never defeat it (it
// opts out through canBeStomped()), while onStomped() (the method star
// power uses) and a fireball hit must both actually kill it -- onStomped()
// used to do nothing, so a stomp or a star touch could report a defeat
// and award score without the plant ever dying.
static void testPiranhaPlantAttackTypes() {
  {
    PiranhaPlant plant;
    CHECK(!plant.canBeStomped(),
          "a Piranha Plant opts out of being defeated by an ordinary stomp");
  }
  {
    PiranhaPlant plant;
    plant.onStomped();
    CHECK(plant.isDead() && !plant.isActive(),
          "onStomped() (used by star power) fully defeats a Piranha Plant "
          "instead of doing nothing");
  }
  {
    PiranhaPlant plant;
    CHECK(plant.hitByFireball(),
          "a fireball hit reports a successful defeat");
    CHECK(plant.isDead() && !plant.isActive(),
          "a fireball hit fully defeats a Piranha Plant");
  }
}

// ── SpriteRegistry lookup-table coverage ──────────────────────────────────
// SpriteRegistry is ~50 pure lookup functions mapping (entity, theme, state,
// frame) to a texture path/rect. Before this batch only the flagpole-slide
// and Piranha paths were exercised. A copy-paste error swapping two themes'
// filenames, or a wrong frame-count constant, would previously go unnoticed
// until it visibly broke on screen.

static const std::vector<LevelTheme> kAllThemes = {
    LevelTheme::Overworld, LevelTheme::Underground, LevelTheme::Castle};

static void testGroundAndPipeTilePathsVaryDistinctlyByTheme() {
  std::vector<std::string> groundPaths;
  std::vector<std::string> pipePaths;
  for (LevelTheme theme : kAllThemes) {
    const std::string &ground = SpriteRegistry::tilePath(TileType::Ground, theme);
    const std::string &pipe = SpriteRegistry::tilePath(TileType::PipeTopLeft, theme);
    CHECK(std::filesystem::exists(ground),
          "ground tile art exists on disk for every theme");
    CHECK(std::filesystem::exists(pipe),
          "pipe tile art exists on disk for every theme");
    groundPaths.push_back(ground);
    pipePaths.push_back(pipe);
  }
  CHECK(groundPaths[0] != groundPaths[1] && groundPaths[1] != groundPaths[2] &&
            groundPaths[0] != groundPaths[2],
        "each theme uses a distinct ground tile file");
  CHECK(pipePaths[0] != pipePaths[1] && pipePaths[1] != pipePaths[2] &&
            pipePaths[0] != pipePaths[2],
        "each theme uses a distinct pipe tile file");

  // The four pipe-piece tile types share the same themed art (only the crop
  // rect differs, applied elsewhere by Tile itself).
  for (LevelTheme theme : kAllThemes) {
    const std::string &topLeft = SpriteRegistry::tilePath(TileType::PipeTopLeft, theme);
    CHECK(SpriteRegistry::tilePath(TileType::PipeTopRight, theme) == topLeft &&
              SpriteRegistry::tilePath(TileType::PipeBodyLeft, theme) == topLeft &&
              SpriteRegistry::tilePath(TileType::PipeBodyRight, theme) == topLeft,
          "all four pipe-piece tile types share one themed sheet");
  }

  // Non-themed decorative tiles are the same file across every theme.
  for (const std::string &path :
       {SpriteRegistry::tilePath(TileType::CastlePiece, LevelTheme::Overworld),
        SpriteRegistry::tilePath(TileType::WardPipePiece, LevelTheme::Overworld),
        SpriteRegistry::tilePath(TileType::VineTop, LevelTheme::Overworld)}) {
    CHECK(std::filesystem::exists(path),
          "non-themed decorative tile art exists on disk");
  }
  CHECK(SpriteRegistry::tilePath(TileType::CastlePiece, LevelTheme::Castle) ==
            SpriteRegistry::tilePath(TileType::CastlePiece, LevelTheme::Overworld),
        "castle-piece decoration ignores theme (one shared sheet)");
}

static void testBlockPathsVaryByThemeExceptUsedState() {
  std::vector<std::string> brickPaths;
  std::vector<std::string> questionPaths;
  for (LevelTheme theme : kAllThemes) {
    const std::string &brick = SpriteRegistry::blockPath(
        BlockType::Brick, theme, SpriteRegistry::BlockVisualState::Idle);
    const std::string &question = SpriteRegistry::blockPath(
        BlockType::Question, theme, SpriteRegistry::BlockVisualState::Idle);
    CHECK(std::filesystem::exists(brick) && std::filesystem::exists(question),
          "brick and question block art exist on disk for every theme");
    brickPaths.push_back(brick);
    questionPaths.push_back(question);

    // Brick art doesn't change between Idle/Hit/Used -- it either breaks
    // entirely or just bumps, with no separate flash/emptied sprite.
    CHECK(SpriteRegistry::blockPath(BlockType::Brick, theme,
                                    SpriteRegistry::BlockVisualState::Hit) == brick &&
              SpriteRegistry::blockPath(BlockType::Brick, theme,
                                        SpriteRegistry::BlockVisualState::Used) == brick,
          "brick block art is identical across every visual state");
  }
  CHECK(brickPaths[0] != brickPaths[1] && brickPaths[1] != brickPaths[2] &&
            brickPaths[0] != brickPaths[2],
        "each theme uses a distinct brick block file");
  CHECK(questionPaths[0] != questionPaths[1] && questionPaths[1] != questionPaths[2] &&
            questionPaths[0] != questionPaths[2],
        "each theme uses a distinct question block file");

  // A used Question block falls back to one generic emptied sprite, the same
  // for every theme -- there's no per-theme "emptied block" art in the pack.
  const std::string &usedOverworld = SpriteRegistry::blockPath(
      BlockType::Question, LevelTheme::Overworld,
      SpriteRegistry::BlockVisualState::Used);
  for (LevelTheme theme : kAllThemes) {
    CHECK(SpriteRegistry::blockPath(BlockType::Question, theme,
                                    SpriteRegistry::BlockVisualState::Used) ==
              usedOverworld,
          "a used question block shares one generic sprite across every theme");
  }
  CHECK(std::filesystem::exists(usedOverworld),
        "the generic used-block sprite exists on disk");
}

static void testCoinPathVariesByThemeWithDistinctGifs() {
  std::vector<std::string> coinPaths;
  for (LevelTheme theme : kAllThemes) {
    const std::string &coin = SpriteRegistry::coinPath(theme, 0);
    CHECK(std::filesystem::exists(coin),
          "coin shimmer GIF exists on disk for every theme");
    coinPaths.push_back(coin);
  }
  CHECK(coinPaths[0] != coinPaths[1] && coinPaths[1] != coinPaths[2] &&
            coinPaths[0] != coinPaths[2],
        "each theme uses a distinct coin shimmer GIF");
}

static void testEnemyAndPowerUpArtIsIntentionallyThemeIndependent() {
  // Documented, deliberate decisions (see SpriteRegistry.cpp comments): no
  // Castle Goomba/Koopa art survived, and no per-theme recolor exists for
  // Mushroom/Fire Flower/Star, so these intentionally return the same file
  // for every theme. A future partial per-theme fix that only updates one
  // theme should trip this guard.
  for (auto pathForTheme :
       {&SpriteRegistry::goombaPath, &SpriteRegistry::koopaWalkPath}) {
    const std::string &overworld = pathForTheme(LevelTheme::Overworld, 0);
    CHECK(std::filesystem::exists(overworld),
          "shared (non-themed) enemy walk art exists on disk");
    for (LevelTheme theme : kAllThemes) {
      CHECK(pathForTheme(theme, 0) == overworld,
            "enemy walk art is intentionally identical across every theme");
    }
  }

  const std::string &squish = SpriteRegistry::goombaSquishPath(LevelTheme::Overworld);
  const std::string &shell =
      SpriteRegistry::koopaShellPath(LevelTheme::Overworld, false);
  CHECK(std::filesystem::exists(squish) && std::filesystem::exists(shell),
        "shared squish/shell art exists on disk");
  for (LevelTheme theme : kAllThemes) {
    CHECK(SpriteRegistry::goombaSquishPath(theme) == squish,
          "Goomba squish art is intentionally identical across every theme");
    CHECK(SpriteRegistry::koopaShellPath(theme, false) == shell,
          "Koopa shell art is intentionally identical across every theme");
  }

  const std::string &mushroom = SpriteRegistry::mushroomPath(LevelTheme::Overworld);
  const std::string &fireFlower = SpriteRegistry::fireFlowerPath(LevelTheme::Overworld);
  const std::string &star = SpriteRegistry::starPath(LevelTheme::Overworld, 0);
  CHECK(std::filesystem::exists(mushroom) && std::filesystem::exists(fireFlower) &&
            std::filesystem::exists(star),
        "shared power-up art exists on disk");
  for (LevelTheme theme : kAllThemes) {
    CHECK(SpriteRegistry::mushroomPath(theme) == mushroom,
          "Mushroom art is intentionally identical across every theme");
    CHECK(SpriteRegistry::fireFlowerPath(theme) == fireFlower,
          "Fire Flower art is intentionally identical across every theme");
    CHECK(SpriteRegistry::starPath(theme, 0) == star,
          "Star art is intentionally identical across every theme");
  }
}

static void testFixedFrameCountsMatchTheirSpriteSheets() {
  CHECK(SpriteRegistry::goombaFrameCount() == 16,
        "Goomba.png is an 8x2 grid of 16 walk frames");
  CHECK(SpriteRegistry::koopaFrameCount() == 20,
        "Koopa.png is a 5x4 grid of 20 frames");
  CHECK(SpriteRegistry::troopaFrameCount() == 4,
        "Troopa.png holds four flight frames");
  CHECK(SpriteRegistry::bowserBreathFrameCount() == 6,
        "Bowser_Breath.png holds six unevenly-sized breath frames");
  CHECK(SpriteRegistry::bowserFireFrameCount() == 3,
        "Bowser_Fire.png holds three fireball frames");
  CHECK(SpriteRegistry::fireballFrameCount() == 4,
        "Fire_Ball.png is a 64x16 sheet of four contiguous 16x16 frames");
  CHECK(SpriteRegistry::flowersBuffFrameCount() == 4,
        "FlowersBuff.png holds four 16x16 frames spaced 2px apart");

  CHECK(std::filesystem::exists(SpriteRegistry::troopaPath()) &&
            std::filesystem::exists(SpriteRegistry::bowserPath()) &&
            std::filesystem::exists(SpriteRegistry::bowserBreathPath()) &&
            std::filesystem::exists(SpriteRegistry::bowserFirePath()) &&
            std::filesystem::exists(SpriteRegistry::fireballPath()) &&
            std::filesystem::exists(SpriteRegistry::flowersBuffPath()),
        "every fixed-frame-count sheet asset exists on disk");
}

static void testBowserBreathFrameRectMatchesSheetLayout() {
  constexpr int widths[] = {78, 78, 78, 91, 105, 103};
  constexpr int offsets[] = {0, 78, 156, 234, 325, 430};
  for (int frame = 0; frame < SpriteRegistry::bowserBreathFrameCount(); ++frame) {
    const sf::IntRect rect = SpriteRegistry::bowserBreathFrameRect(frame);
    CHECK(rect.left == offsets[frame] && rect.width == widths[frame] &&
              rect.top == 0 && rect.height == 60,
          "each Bowser breath frame matches its documented sheet offset/width");
  }

  const sf::IntRect lastFrame = SpriteRegistry::bowserBreathFrameRect(5);
  CHECK(SpriteRegistry::bowserBreathFrameRect(99) == lastFrame,
        "an out-of-range high frame clamps to the last breath frame");
  CHECK(SpriteRegistry::bowserBreathFrameRect(-5) ==
            SpriteRegistry::bowserBreathFrameRect(0),
        "a negative frame clamps to the first breath frame");
}

static void testPlayerFrameCountsMatchEachSheetDefinition() {
  struct Expectation {
    SpriteRegistry::PlayerAnim anim;
    int marioFrames;
    int luigiFrames;
  };
  const std::vector<Expectation> expectations = {
      {SpriteRegistry::PlayerAnim::Idle, 4, 3},
      {SpriteRegistry::PlayerAnim::Walk, 6, 8},
      {SpriteRegistry::PlayerAnim::Jump, 3, 5},
      {SpriteRegistry::PlayerAnim::Fire, 2, 1},
      {SpriteRegistry::PlayerAnim::Climb, 2, 4},
      // Skid has no dedicated pose for either character -- both fall back to
      // their Idle sheet and share its frame count.
      {SpriteRegistry::PlayerAnim::Skid, 4, 3},
  };
  for (const Expectation &expectation : expectations) {
    CHECK(SpriteRegistry::playerFrameCount(CharacterId::Mario, PowerUpState::Small,
                                           expectation.anim) == expectation.marioFrames,
          "Mario's sheet exposes the documented frame count for this animation");
    CHECK(SpriteRegistry::playerFrameCount(CharacterId::Luigi, PowerUpState::Small,
                                           expectation.anim) == expectation.luigiFrames,
          "Luigi's sheet exposes the documented frame count for this animation");
    // Power-up state no longer selects a different sheet for either
    // character -- the frame count must stay the same at every power level.
    CHECK(SpriteRegistry::playerFrameCount(CharacterId::Mario, PowerUpState::Fire,
                                           expectation.anim) == expectation.marioFrames &&
              SpriteRegistry::playerFrameCount(CharacterId::Luigi, PowerUpState::Big,
                                               expectation.anim) == expectation.luigiFrames,
          "player animation frame counts are independent of power-up state");

    CHECK(std::filesystem::exists(SpriteRegistry::playerPath(
              CharacterId::Mario, PowerUpState::Small, expectation.anim, 0)),
          "Mario's sheet for this animation exists on disk");
    CHECK(std::filesystem::exists(SpriteRegistry::playerPath(
              CharacterId::Luigi, PowerUpState::Small, expectation.anim, 0)),
          "Luigi's sheet for this animation exists on disk");
  }

  CHECK(std::filesystem::exists(SpriteRegistry::playerDeathPath(CharacterId::Mario)) &&
            std::filesystem::exists(SpriteRegistry::playerDeathPath(CharacterId::Luigi)),
        "each character's death sprite exists on disk");
  CHECK(SpriteRegistry::playerDeathPath(CharacterId::Mario) !=
            SpriteRegistry::playerDeathPath(CharacterId::Luigi),
        "Mario and Luigi use distinct death sprites");
}

static void testCoinAndStarGifFrameCountsAreStableAndPositive() {
  const int coinFrames = SpriteRegistry::coinFrameCount();
  const int starFrames = SpriteRegistry::starFrameCount();
  CHECK(coinFrames > 0, "the coin shimmer GIF decodes at least one frame");
  CHECK(starFrames > 0, "the Starman GIF decodes at least one frame");
  CHECK(SpriteRegistry::coinFrameCount() == coinFrames &&
            SpriteRegistry::starFrameCount() == starFrames,
        "repeated frame-count lookups agree (cached, not re-decoded differently)");
}

static void testFlagpoleSlideRectClampsNegativeFrame() {
  // playerFlagpoleSlideRect() used to index a 2-element array with a raw
  // signed frame % count, so a negative frame produced an out-of-bounds
  // read. A hand-edited save file can inject one via a corrupted
  // animationFrame, so this must degrade to frame 0 instead of reading OOB.
  const std::vector<CharacterId> characters = {CharacterId::Mario,
                                                CharacterId::Luigi};
  const std::vector<PowerUpState> powers = {
      PowerUpState::Small, PowerUpState::Big, PowerUpState::Fire};
  for (CharacterId character : characters) {
    for (PowerUpState power : powers) {
      const sf::IntRect zero =
          SpriteRegistry::playerFlagpoleSlideRect(character, power, 0);
      const sf::IntRect negativeOne =
          SpriteRegistry::playerFlagpoleSlideRect(character, power, -1);
      const sf::IntRect negativeLarge =
          SpriteRegistry::playerFlagpoleSlideRect(character, power, -99);
      CHECK(negativeOne == zero,
            "a negative frame clamps to frame 0 instead of reading out of "
            "bounds");
      CHECK(negativeLarge == zero,
            "a large negative frame also clamps to frame 0");
    }
  }
}

static void testFlagpoleSlideFramesAndCutscene() {
  const std::string& sheet = SpriteRegistry::playerFlagpoleSlideSheetPath();
  CHECK(std::filesystem::exists(sheet),
        "flagpole animation uses the checked-in player sheet");

  sf::Image sheetImage;
  CHECK(sheetImage.loadFromFile(sheet) && sheetImage.getSize().x == 584 &&
            sheetImage.getSize().y == 469,
        "flagpole animation source sheet decodes at its expected dimensions");
  sf::Image keyedSheet = sheetImage;
  keyedSheet.createMaskFromColor(sf::Color(146, 144, 255));

  const std::vector<CharacterId> characters = {
      CharacterId::Mario, CharacterId::Luigi};
  const std::vector<PowerUpState> powers = {
      PowerUpState::Small, PowerUpState::Big, PowerUpState::Fire};
  for (CharacterId character : characters) {
    for (PowerUpState power : powers) {
      CHECK(SpriteRegistry::playerFrameCount(
                character, power, SpriteRegistry::PlayerAnim::FlagpoleSlide) == 2,
            "flagpole slide exposes both climb frames for every player form");
      for (int frame = 0; frame < 2; ++frame) {
        const sf::IntRect rect = SpriteRegistry::playerFlagpoleSlideRect(
            character, power, frame);
        CHECK(rect.left >= 0 && rect.top >= 0 &&
                  rect.left + rect.width <= static_cast<int>(sheetImage.getSize().x) &&
                  rect.top + rect.height <= static_cast<int>(sheetImage.getSize().y),
              "flagpole slide source rectangle stays inside the character sheet");
        CHECK(rect.width == 16 &&
                  rect.height == (power == PowerUpState::Small ? 16 : 32),
              "flagpole slide selects the correct small or tall character cell");
        CHECK(keyedSheet.getPixel(static_cast<unsigned>(rect.left),
                                  static_cast<unsigned>(rect.top)).a == 0,
              "flagpole slide cell background becomes transparent after keying");
      }
    }
  }

  Flagpole pole(300.0f, 300.0f);
  CHECK(std::abs(pole.getSlideAnchorX() - 316.0f) < 0.001f &&
            std::abs(pole.getSlideEndY() - 268.0f) < 0.001f,
        "flagpole exposes a stable cutscene anchor and landing position");

  Mario mario;
  Luigi luigi;
  for (Player* player : {static_cast<Player*>(&mario), static_cast<Player*>(&luigi)}) {
    player->setPosition(100.0f, 100.0f);
    player->beginFlagpoleSlide(pole.getSlideAnchorX(), pole.getSlideEndY());
    CHECK(player->isFlagpoleCutsceneActive(),
          "starting the flagpole sequence locks the player into a cutscene");

    const float expectedX = pole.getSlideAnchorX() - (TILE_SIZE - 2.0f);
    const float startY = player->getPosition().y;
    player->setVelocity(-500.0f, 600.0f);
    player->update(FIXED_DT);
    CHECK(std::abs(player->getPosition().x - expectedX) < 0.001f,
          "flagpole slide holds the player against the pole horizontally");
    CHECK(player->getPosition().y > startY &&
              player->getPosition().y < pole.getSlideEndY(),
          "flagpole slide moves toward the landing point without gravity physics");
    CHECK(std::abs(player->getVelocity().x) < 0.001f &&
              std::abs(player->getVelocity().y) < 0.001f,
          "flagpole slide clears player velocity every cutscene frame");

    for (int frame = 0; frame < 120 && !player->isFlagpoleSlideComplete();
         ++frame) {
      player->update(FIXED_DT);
    }
    CHECK(player->isFlagpoleSlideComplete(),
          "flagpole slide reaches its exact landing point");
    CHECK(std::abs(player->getPosition().y - pole.getSlideEndY()) < 0.001f,
          "flagpole slide clamps to the pole base rather than overshooting");

    player->beginFlagpoleCastleWalk();
    player->setVelocity(400.0f, 400.0f);
    player->update(FIXED_DT);
    CHECK(player->isFlagpoleCutsceneActive() &&
              std::abs(player->getVelocity().x) < 0.001f &&
              std::abs(player->getVelocity().y) < 0.001f,
          "castle walk remains a physics-free completion cutscene");
  }
}

static void testPlayerDeathAnimationUsesFacingPoses() {
  const std::vector<CharacterId> characters = {
      CharacterId::Mario, CharacterId::Luigi};
  for (CharacterId character : characters) {
    const std::string &path = SpriteRegistry::playerDeathPath(character);
    CHECK(std::filesystem::exists(path),
          "death animation has a registered character-specific asset");

    sf::Image image;
    CHECK(image.loadFromFile(path) && image.getSize().x == 14 &&
              image.getSize().y == 14,
          "death animation asset is a loadable 14x14 pose facing the player");
  }

  Mario mario;
  Luigi luigi;
  for (Player *player : {static_cast<Player *>(&mario),
                         static_cast<Player *>(&luigi)}) {
    player->setPosition(100.0f, 200.0f);
    const float startY = player->getPosition().y;
    float previousY = startY;
    bool rose = false;
    bool fell = false;

    player->die();
    CHECK(player->isDead() && !player->isDeathAnimationComplete(),
          "death starts as an active animation instead of completing immediately");

    for (int frame = 0; frame < 120 && !player->isDeathAnimationComplete();
         ++frame) {
      player->update(FIXED_DT);
      const float y = player->getPosition().y;
      rose = rose || y < startY;
      fell = fell || (rose && y > previousY);
      previousY = y;
    }

    CHECK(rose, "death animation moves the player upward first");
    CHECK(fell, "death animation reverses into a downward drop");
    CHECK(player->isDeathAnimationComplete(),
          "death animation pauses briefly and then reports completion");
    CHECK(player->getPosition().y > startY &&
              std::abs(player->getVelocity().y) < 0.001f,
          "death animation stops below its starting position before respawn");
    CHECK(player->getBounds().top > static_cast<float>(WINDOW_HEIGHT),
          "death animation pauses only after the player falls out of the screen");
  }
}

// Regression tests for issue #19: a single gameplay incident (an enemy
// cluster overlapping the player, or several fixed-step sub-updates running
// within one rendered frame) must not publish more than one PlayerDied
// event or remove more than one life. Player::die() guards on m_dead as the
// single source of truth every damage/hazard/timer path funnels through.
static void testPlayerDeathIsIdempotentUnderEnemyCluster() {
  Mario player;
  player.setPosition(100.0f, 100.0f);

  int deathEvents = 0;
  auto sub = ScopedEventSubscription(
      EventType::PlayerDied,
      [&deathEvents](const GameEvent &) { ++deathEvents; });

  // Simulate a cluster of overlapping enemies each independently landing a
  // lethal hit within the same collision pass -- the exact scenario the
  // pre-fix per-enemy takeDamage() loop produced.
  for (int i = 0; i < 4; ++i) {
    player.takeDamage();
  }

  CHECK(deathEvents == 1, "an enemy cluster hitting the player in one pass "
                          "publishes exactly one PlayerDied event");
  CHECK(player.isDead(), "the player is dead after the cluster hit");

  // A later, independent death source (e.g. a lava tile checked after the
  // enemy loop) must also be swallowed once the player is already dead.
  player.die();
  player.die();
  CHECK(deathEvents == 1, "calling die() again after death does not "
                          "publish additional PlayerDied events");
}

static void testPlayerDeathSurvivesMultipleFixedUpdatesPerFrame() {
  Mario player;
  player.setPosition(100.0f, 100.0f);

  int deathEvents = 0;
  auto sub = ScopedEventSubscription(
      EventType::PlayerDied,
      [&deathEvents](const GameEvent &) { ++deathEvents; });

  player.die();
  CHECK(deathEvents == 1, "the first die() call publishes PlayerDied");

  // Several fixed sub-steps within one rendered frame, mirroring
  // PlayingState's fixed-step catch-up loop. A timer-expiry (or other
  // hazard) re-check that fires again partway through must not add another
  // event, and the death animation must keep advancing instead of
  // resetting.
  const float startY = player.getPosition().y;
  for (int step = 0; step < 3; ++step) {
    player.die(); // simulates a repeated timer-expiry / damage source
    player.update(FIXED_DT);
  }

  CHECK(deathEvents == 1,
        "repeated die() calls across several fixed updates in one frame "
        "still publish only one PlayerDied event");
  CHECK(player.isDead() && player.getPosition().y != startY,
        "the death animation keeps progressing across the repeated "
        "sub-steps instead of restarting");
}

static void testFireDamageFollowsBigThenSmallProgression() {
  Mario player;
  player.applyPowerUp(PowerUpState::Fire);

  player.takeDamage();
  CHECK(player.getPowerUpState() == PowerUpState::Big,
        "Fire Mario downgrades to Big on the first hit, not straight to Small");
  CHECK(!player.isDead(), "Fire Mario survives its first hit");

  player.update(INVINCIBILITY_DUR + 0.1f); // clear the post-hit invincibility window

  player.takeDamage();
  CHECK(player.getPowerUpState() == PowerUpState::Small,
        "Big Mario downgrades to Small on a second hit");
  CHECK(!player.isDead(), "Big Mario survives its second hit");

  player.update(INVINCIBILITY_DUR + 0.1f);

  player.takeDamage();
  CHECK(player.isDead(), "Small Mario dies on a third hit");
}

int main() {
  testReflectHelperPure();
  testPublishIsSafeWhenHandlersMutateSubscriptionsMidDispatch();
  testGoombaBouncesBothDirections();
  testMushroomReversesInsteadOfStopping();
  testStarBouncesOffFloor();
  testMovingItemCollidesWithBlocks();
  testFallingItemIsCulledBelowLevel();
  testShellIsCulledAfterFallingOffMap();
  testKoopaDyingAndRespawn();
  testWalkingKoopaHitboxMatchesSprite();
  testFlyingTroopa();
  testBowserBreathingCycle();
  testBowserFireballs();
  testStarPowerOnlyDefeatsEnemiesItActuallyKills();
  testEnemyScoreValuesMatchDifficultyTier();
  testHorizontalEscalaterMovement();
  testLevel2LavaTilesKillPlayer();
  testLevel2VineEntersClimbState();
  testGoombaStompDisablesCollisionImmediately();
  testEnlargedPlayersCanHitBlocksWithCompactBody();
  testBrickBreaksOnThirdHit();
  testMushroomBlockSpawnsMushroomItem();
  testFireFlowerBlockSpawnsFireFlowerItem();
  testCollectingMushroomGrowsPlayer();
  testCollectingFireFlowerGrantsFireState();
  testCoinCollectionPublishesConfiguredScore();
  testPowerUpCollectionPublishesFlatBonusScore();
  testLevel1PlacesReachableCoinBlocks();
  testLevel2PlacesAReachableCoinBlock();
  testShippedLevelsAdvertiseConfiguredEnemiesAndPowerUps();
  testResolveCollisionAloneStillZeroesVelocity();
  testTileGridExcludesDistantTiles();
  testUpwardEdgeHitResolvesAsWall();
  testSweptStompCatchesTunneling();
  testSprintDoesNotCompoundVelocity();
  testAllLuigiSpriteStatesLoad();
  testAllMarioSpriteStatesLoad();
  testVineClimbingControls();
  testGifFramesFallsBackToOneFrameForMissingFile();
  testPiranhaFramesAndEmergenceStayStable();
  testPiranhaPlantAttackTypes();
  testGroundAndPipeTilePathsVaryDistinctlyByTheme();
  testBlockPathsVaryByThemeExceptUsedState();
  testCoinPathVariesByThemeWithDistinctGifs();
  testEnemyAndPowerUpArtIsIntentionallyThemeIndependent();
  testFixedFrameCountsMatchTheirSpriteSheets();
  testBowserBreathFrameRectMatchesSheetLayout();
  testPlayerFrameCountsMatchEachSheetDefinition();
  testCoinAndStarGifFrameCountsAreStableAndPositive();
  testFlagpoleSlideRectClampsNegativeFrame();
  testFlagpoleSlideFramesAndCutscene();
  testPlayerDeathAnimationUsesFacingPoses();
  testPlayerDeathIsIdempotentUnderEnemyCluster();
  testPlayerDeathSurvivesMultipleFixedUpdatesPerFrame();
  testFireDamageFollowsBigThenSmallProgression();

  if (g_failures == 0) {
    std::cout << "All collision-resolution tests passed.\n";
    return 0;
  }
  std::cerr << g_failures << " test(s) failed.\n";
  return 1;
}
