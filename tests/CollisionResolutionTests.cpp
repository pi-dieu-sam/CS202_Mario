// Regression tests for issue #23: wall collisions must reverse horizontal
// velocity reliably instead of destroying it. See CollisionDetector.hpp's
// reflectHorizontalVelocity() and Level::handleCollisions()'s three
// wall-reversal call sites (Enemy vs Tiles, Enemy vs Blocks, Items vs Tiles).
//
// Uses a small always-on CHECK macro instead of assert(), since Release
// builds (as used in CI) define NDEBUG and would silently strip assert().

#include "Physics/CollisionDetector.hpp"
#include "Physics/PhysicsConstants.hpp"
#include "Level/TileGrid.hpp"
#include "Entities/Goomba.hpp"
#include "Entities/Koopa.hpp"
#include "Entities/Mario.hpp"
#include "Entities/Mushroom.hpp"
#include "Entities/Tile.hpp"
#include <cmath>
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
static void placeWallRightOf(const GameObject &obj, Tile &wall) {
  sf::FloatRect b = obj.getBounds();
  wall.setPosition(b.left + b.width - 1.0f, b.top);
}

// Mirror of placeWallRightOf: overlaps obj's left edge by 1px -> Side::Left.
static void placeWallLeftOf(const GameObject &obj, Tile &wall) {
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

static void testSlidingKoopaShellBouncesBothDirections() {
  {
    Koopa koopa;
    koopa.onStomped(); // Walking -> Shell
    koopa.onStomped(); // Shell -> Sliding (kickShell(1), vel.x = +300)
    CHECK(koopa.getKoopaState() == KoopaState::Sliding,
          "Koopa reaches Sliding state after two stomps");
    Tile wall;
    placeWallRightOf(koopa, wall);
    bounceOnce(koopa, wall, koopa.getSpeed());
    CHECK(koopa.getVelocity().x == -300.0f,
          "Sliding shell bounces off right wall at -300 (not stopping)");
  }
  {
    Koopa koopa;
    koopa.onStomped();
    koopa.onStomped();
    koopa.kickShell(-1); // force leftward slide, vel.x = -300
    Tile wall;
    placeWallLeftOf(koopa, wall);
    bounceOnce(koopa, wall, koopa.getSpeed());
    CHECK(koopa.getVelocity().x == 300.0f,
          "Sliding shell bounces off left wall at +300 (not stopping)");
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

int main() {
  testReflectHelperPure();
  testGoombaBouncesBothDirections();
  testMushroomReversesInsteadOfStopping();
  testSlidingKoopaShellBouncesBothDirections();
  testResolveCollisionAloneStillZeroesVelocity();
  testTileGridExcludesDistantTiles();
  testUpwardEdgeHitResolvesAsWall();
  testSweptStompCatchesTunneling();
  testSprintDoesNotCompoundVelocity();

  if (g_failures == 0) {
    std::cout << "All collision-resolution tests passed.\n";
    return 0;
  }
  std::cerr << g_failures << " test(s) failed.\n";
  return 1;
}
