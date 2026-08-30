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
#include "Entities/Goomba.hpp"
#include "Entities/Koopa.hpp"
#include "Entities/Troopa.hpp"
#include "Entities/Bowser.hpp"
#include "Entities/Block.hpp"
#include "Entities/Escalater.hpp"
#include "Entities/PiranhaPlant.hpp"
#include "Entities/Luigi.hpp"
#include "Entities/Mario.hpp"
#include "Entities/Mushroom.hpp"
#include "Entities/Flagpole.hpp"
#include "Entities/Tile.hpp"
#include <cmath>
#include <filesystem>
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

static void testLevel1VineEntersClimbState() {
  // level1.txt places a vertical V vine at map column 7, rows 6-11. Its
  // first cell lands at world row 11 after the five-row display offset.
  Level level;
  CHECK(level.loadFromFile("assets/levels/level1.txt", "Mario",
                           LevelTheme::Overworld),
        "level 1 loads for vine climbing test");
  Player *player = level.getPlayer();
  CHECK(player != nullptr, "level 1 creates a player for vine climbing test");
  if (!player) return;

  // Player position is a two-tile anchor; its small-form body begins 32px
  // below it. Position that body inside the first V tile at (7, 11).
  player->setPosition(7.0f * TILE_SIZE - 26.0f,
                      11.0f * TILE_SIZE - TILE_SIZE);
  level.update(0.0f);
  CHECK(player->isClimbing(),
        "touching a V map tile enters the climb state without terrain resolve");
  CHECK(std::abs(player->getPosition().x - 7.0f * TILE_SIZE) < 0.001f,
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

int main() {
  testReflectHelperPure();
  testGoombaBouncesBothDirections();
  testMushroomReversesInsteadOfStopping();
  testKoopaDyingAndRespawn();
  testFlyingTroopa();
  testBowserBreathingCycle();
  testHorizontalEscalaterMovement();
  testLevel2LavaTilesKillPlayer();
  testLevel1VineEntersClimbState();
  testGoombaStompDisablesCollisionImmediately();
  testEnlargedPlayersCanHitBlocksWithCompactBody();
  testResolveCollisionAloneStillZeroesVelocity();
  testTileGridExcludesDistantTiles();
  testUpwardEdgeHitResolvesAsWall();
  testSweptStompCatchesTunneling();
  testSprintDoesNotCompoundVelocity();
  testAllLuigiSpriteStatesLoad();
  testAllMarioSpriteStatesLoad();
  testVineClimbingControls();
  testPiranhaFramesAndEmergenceStayStable();
  testFlagpoleSlideFramesAndCutscene();
  testPlayerDeathAnimationUsesFacingPoses();

  if (g_failures == 0) {
    std::cout << "All collision-resolution tests passed.\n";
    return 0;
  }
  std::cerr << g_failures << " test(s) failed.\n";
  return 1;
}
