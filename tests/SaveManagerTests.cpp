// Regression tests for the five-slot single-player snapshot repository.
// These exercise only the data/persistence boundary: no render window,
// assets, or running Game singleton are required.

#include "Core/SaveManager.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <optional>
#include <string>

static int g_failures = 0;

#define CHECK(cond, msg)                                                     \
  do {                                                                       \
    if (!(cond)) {                                                           \
      std::cerr << "FAIL: " << msg << " (" << __FILE__ << ":" << __LINE__    \
                << ")\n";                                                   \
      ++g_failures;                                                          \
    }                                                                        \
  } while (0)

namespace {

class ScopedSaveDirectory {
public:
  ScopedSaveDirectory() {
    const auto tick = static_cast<unsigned long long>(
        std::chrono::high_resolution_clock::now().time_since_epoch().count());
    m_directory = std::filesystem::temp_directory_path() /
                  ("cs202-mario-save-slot-test-" + std::to_string(tick));

    std::error_code ec;
    std::filesystem::create_directories(m_directory, ec);
    if (ec) {
      std::cerr << "Could not create test save directory: " << ec.message() << "\n";
      ++g_failures;
    }
    SaveManager::setSaveDirectoryForTesting(m_directory);
  }

  ~ScopedSaveDirectory() {
    SaveManager::setSaveDirectoryForTesting(std::nullopt);
    std::error_code ec;
    std::filesystem::remove_all(m_directory, ec);
  }

private:
  std::filesystem::path m_directory;
};

SaveData::GameSnapshot makeSnapshot() {
  SaveData::GameSnapshot snapshot;
  snapshot.progress.level = 2;
  snapshot.progress.score = 12345;
  snapshot.progress.lives = 2;
  snapshot.progress.coins = 37;
  snapshot.progress.character = "Luigi";
  snapshot.levelTimer = 217.5f;
  snapshot.mainLevelNumber = 2;
  snapshot.inSecretRoom = true;
  snapshot.pipeReturnPosition = {432.0f, 128.0f};
  snapshot.pipeReturnPowerUp = 2;

  auto& player = snapshot.level.player;
  player.character.object.position = {345.0f, 220.0f};
  player.character.object.velocity = {-72.0f, 14.0f};
  player.character.grounded = true;
  player.character.facingRight = false;
  player.character.animationTimer = 0.07f;
  player.character.animationFrame = 2;
  player.character.animationFrames = 4;
  player.powerUp = 2;
  player.lives = 2;
  player.sprinting = true;
  player.hasVineAnchor = true;
  player.lastVineAnchor = {320.0f, 160.0f};
  player.invincibilityTimer = 0.8f;
  player.sizeScale = 1.25f;
  player.growing = true;
  player.growTimer = 0.3f;
  player.currentAnimation = 4;

  SaveData::BlockState block;
  block.object.position = {128.0f, 160.0f};
  block.blockType = 1;
  block.containedItem = 8;
  block.used = true;
  block.theme = 1;
  block.bumpOffset = -6.0f;
  block.bumpTimer = 0.04f;
  block.bumping = true;
  snapshot.level.blocks.push_back(block);

  SaveData::EnemyState enemy;
  enemy.kind = SaveData::EnemyKind::PiranhaPlant;
  enemy.character.object.position = {512.0f, 288.0f};
  enemy.character.object.velocity = {0.0f, -20.0f};
  enemy.theme = 2;
  enemy.piranhaState = 2;
  enemy.piranhaCurrentFrame = 1;
  enemy.piranhaFrameTimer = 0.11f;
  enemy.piranhaHideTimer = 0.42f;
  enemy.piranhaWaitTimer = 0.63f;
  enemy.piranhaBasePosition = {512.0f, 320.0f};
  enemy.piranhaBaseCaptured = true;
  snapshot.level.enemies.push_back(enemy);

  SaveData::ItemState item;
  item.kind = SaveData::ItemKind::FlowersBuff;
  item.object.position = {250.0f, 130.0f};
  item.object.velocity = {40.0f, 0.0f};
  item.moving = true;
  item.theme = 0;
  item.animationTimer = 0.2f;
  item.animationFrame = 3;
  snapshot.level.items.push_back(item);

  SaveData::FireballState fireball;
  fireball.object.position = {360.0f, 250.0f};
  fireball.object.velocity = {180.0f, -10.0f};
  fireball.lifetime = 1.7f;
  fireball.direction = 1;
  fireball.animationTimer = 0.09f;
  fireball.animationFrame = 2;
  fireball.surfaceHits = 1;
  snapshot.level.fireballs.push_back(fireball);

  SaveData::BowserFireballState bowserFireball;
  bowserFireball.object.position = {700.0f, 190.0f};
  bowserFireball.object.velocity = {-180.0f, 0.0f};
  bowserFireball.lifetime = 1.2f;
  bowserFireball.animationTimer = 0.05f;
  bowserFireball.animationFrame = 1;
  snapshot.level.bowserFireballs.push_back(bowserFireball);

  SaveData::EscalaterState escalater;
  escalater.object.position = {288.0f, 384.0f};
  escalater.axis = 1;
  escalater.size = {32.0f, 32.0f};
  escalater.renderSize = {32.0f, 16.0f};
  escalater.centerX = 288.0f;
  escalater.centerY = 384.0f;
  escalater.range = 96.0f;
  escalater.speed = 60.0f;
  escalater.direction = 1.0f;
  escalater.mapRight = 2000.0f;
  escalater.mapBottom = 640.0f;
  snapshot.level.escalaters.push_back(escalater);

  SaveData::FireBarState fireBar;
  fireBar.object.position = {800.0f, 280.0f};
  fireBar.angle = 1.25f;
  fireBar.angularSpeed = 1.5f;
  fireBar.fireballRotationDegrees = 45.0f;
  fireBar.fireballSpinSpeed = 540.0f;
  fireBar.animationTimer = 0.03f;
  fireBar.animationFrame = 1;
  fireBar.segmentCount = 6;
  fireBar.segmentSpacing = 16.0f;
  snapshot.level.fireBars.push_back(fireBar);

  SaveData::LavaFireballState lava;
  lava.object.position = {960.0f, 360.0f};
  lava.launchPosition = {960.0f, 480.0f};
  lava.launchSpeed = 310.0f;
  lava.totalFlightTime = 1.3f;
  lava.flightTimer = 0.4f;
  lava.cooldownTimer = 0.0f;
  lava.animationFrame = 2;
  lava.visible = true;
  snapshot.level.lavaFireballs.push_back(lava);

  snapshot.level.flagpole.present = true;
  snapshot.level.flagpole.object.position = {1400.0f, 200.0f};
  snapshot.level.flagpole.flagPosition = {1412.0f, 242.0f};
  snapshot.level.flagpole.flagDropY = 432.0f;
  return snapshot;
}

void testInitialSlotsAreEmpty() {
  const auto slots = SaveManager::listSlots();
  for (int index = 0; index < SaveManager::SLOT_COUNT; ++index) {
    CHECK(slots[static_cast<std::size_t>(index)].slot == index + 1,
          "every visible slot keeps its one-based number");
    CHECK(slots[static_cast<std::size_t>(index)].status == SaveSlotStatus::Empty,
          "a new save directory exposes every slot as empty");
  }
}

void testSnapshotRoundTripAndSlotLimit() {
  const SaveData::GameSnapshot source = makeSnapshot();
  std::string error;

  CHECK(SaveManager::saveSlot(1, source, &error),
        "a valid single-player snapshot writes to slot 1");
  CHECK(std::filesystem::exists(SaveManager::slotPath(1)),
        "saving creates the numbered slot file");

  const auto loaded = SaveManager::loadSlot(1, &error);
  CHECK(loaded.has_value(), "a saved slot can be decoded again");
  if (!loaded) return;

  CHECK(loaded->savedAtEpochSeconds > 0, "the repository stamps a successful save");
  CHECK(loaded->progress.character == "Luigi" && loaded->progress.level == 2 &&
            loaded->progress.score == 12345 && loaded->progress.lives == 2 &&
            loaded->progress.coins == 37,
        "progress metadata survives a save/load round trip");
  CHECK(loaded->levelTimer == 217.5f && loaded->inSecretRoom &&
            loaded->pipeReturnPosition.x == 432.0f && loaded->pipeReturnPowerUp == 2,
        "level and pipe state survive a save/load round trip");
  CHECK(loaded->level.player.character.object.position.x == 345.0f &&
            loaded->level.player.powerUp == 2 && loaded->level.player.currentAnimation == 4,
        "the mutable player state survives a save/load round trip");
  CHECK(loaded->level.blocks.size() == 1 && loaded->level.blocks[0].used &&
            loaded->level.blocks[0].bumpOffset == -6.0f,
        "changed block state survives a save/load round trip");
  CHECK(loaded->level.enemies.size() == 1 &&
            loaded->level.enemies[0].kind == SaveData::EnemyKind::PiranhaPlant &&
            loaded->level.enemies[0].piranhaWaitTimer == 0.63f,
        "enemy subtype state survives a save/load round trip");
  CHECK(loaded->level.items.size() == 1 &&
            loaded->level.items[0].kind == SaveData::ItemKind::FlowersBuff &&
            loaded->level.items[0].moving,
        "item state survives a save/load round trip");
  CHECK(loaded->level.fireballs.size() == 1 && loaded->level.bowserFireballs.size() == 1 &&
            loaded->level.escalaters.size() == 1 && loaded->level.fireBars.size() == 1 &&
            loaded->level.lavaFireballs.size() == 1,
        "all active projectile and hazard collections survive the round trip");
  CHECK(loaded->level.flagpole.present && loaded->level.flagpole.flagDropY == 432.0f,
        "flagpole completion state survives the round trip");

  for (int slot = 2; slot <= SaveManager::SLOT_COUNT; ++slot) {
    SaveData::GameSnapshot varied = source;
    varied.progress.score += slot;
    CHECK(SaveManager::saveSlot(slot, varied, &error),
          "each of the remaining numbered slots can be saved independently");
  }

  const auto slots = SaveManager::listSlots();
  for (const SaveSlotInfo& info : slots) {
    CHECK(info.status == SaveSlotStatus::Occupied,
          "five valid saves occupy exactly the five displayed slots");
  }

  CHECK(!SaveManager::saveSlot(0, source, &error), "slot zero is rejected");
  CHECK(!SaveManager::saveSlot(SaveManager::SLOT_COUNT + 1, source, &error),
        "a sixth slot is rejected");
}

void testLoadingAnEmptySlotFailsGracefully() {
  std::string error;
  const auto loaded = SaveManager::loadSlot(2, &error);

  CHECK(!loaded.has_value(),
        "loading a slot that was never saved returns no snapshot");
  CHECK(error == "This slot is empty",
        "an empty slot reports its dedicated error message, not a generic one");
}

void testCorruptAndMultiplayerSavesAreRejectedSafely() {
  const std::filesystem::path corruptPath = SaveManager::slotPath(3);
  {
    std::ofstream corruptFile(corruptPath, std::ios::binary | std::ios::trunc);
    corruptFile << "not a Super Mario snapshot";
  }

  const auto slots = SaveManager::listSlots();
  CHECK(slots[2].status == SaveSlotStatus::Corrupt,
        "a malformed file is shown as corrupt instead of as a loadable slot");

  std::string error;
  CHECK(!SaveManager::loadSlot(3, &error).has_value(),
        "a malformed slot cannot produce a snapshot");

  SaveData::GameSnapshot multiplayer = makeSnapshot();
  multiplayer.gameMode = 1;
  CHECK(!SaveManager::saveSlot(3, multiplayer, &error),
        "multiplayer snapshots are rejected before they reach a slot");

  const SaveData::GameSnapshot valid = makeSnapshot();
  CHECK(SaveManager::saveSlot(3, valid, &error),
        "a valid snapshot can deliberately replace a corrupt slot");
  CHECK(SaveManager::listSlots()[2].status == SaveSlotStatus::Occupied,
        "replacing the corrupt file restores a normal occupied slot");
}

void testValidateSnapshotRejectsOutOfRangeProgressValues() {
  std::string error;

  auto rejects = [&](const char *reason,
                      const std::function<void(SaveData::GameSnapshot &)> &mutate) {
    SaveData::GameSnapshot snapshot = makeSnapshot();
    mutate(snapshot);
    CHECK(!SaveManager::saveSlot(5, snapshot, &error), reason);
  };

  rejects("level 0 is below the valid range",
          [](SaveData::GameSnapshot &s) { s.progress.level = 0; });
  rejects("level 4 is above the valid range (only 3 levels exist)",
          [](SaveData::GameSnapshot &s) { s.progress.level = 4; });
  rejects("zero lives is below the valid range",
          [](SaveData::GameSnapshot &s) { s.progress.lives = 0; });
  rejects("100 lives is above the valid range",
          [](SaveData::GameSnapshot &s) { s.progress.lives = 100; });
  rejects("negative coins are invalid",
          [](SaveData::GameSnapshot &s) { s.progress.coins = -1; });
  rejects("10000 coins is above the valid range",
          [](SaveData::GameSnapshot &s) { s.progress.coins = 10000; });
  rejects("negative score is invalid",
          [](SaveData::GameSnapshot &s) { s.progress.score = -1; });
  rejects("an empty character name is invalid",
          [](SaveData::GameSnapshot &s) { s.progress.character = ""; });
  rejects("a character name over 64 characters is invalid",
          [](SaveData::GameSnapshot &s) {
            s.progress.character = std::string(65, 'x');
          });

  const SaveData::GameSnapshot valid = makeSnapshot();
  CHECK(SaveManager::saveSlot(5, valid, &error),
        "a snapshot with every value back in range is accepted again");
}

void testTruncatedSaveFileFailsGracefullyWithoutCrashing() {
  const SaveData::GameSnapshot source = makeSnapshot();
  std::string error;
  CHECK(SaveManager::saveSlot(4, source, &error),
        "setup: a valid snapshot writes to slot 4");

  const std::filesystem::path path = SaveManager::slotPath(4);
  const auto fullSize = std::filesystem::file_size(path);

  std::error_code ec;
  std::filesystem::resize_file(path, fullSize / 2, ec);
  CHECK(!ec, "setup: the slot file can be truncated to half its size");

  const auto loaded = SaveManager::loadSlot(4, &error);
  CHECK(!loaded.has_value(),
        "a snapshot truncated mid-record cannot be decoded");
  CHECK(!error.empty(),
        "a truncated file reports an error instead of leaving it blank");
}

} // namespace

int main() {
  ScopedSaveDirectory testDirectory;

  testInitialSlotsAreEmpty();
  testLoadingAnEmptySlotFailsGracefully();
  testSnapshotRoundTripAndSlotLimit();
  testCorruptAndMultiplayerSavesAreRejectedSafely();
  testValidateSnapshotRejectsOutOfRangeProgressValues();
  testTruncatedSaveFileFailsGracefullyWithoutCrashing();

  if (g_failures == 0) {
    std::cout << "All save manager tests passed.\n";
    return 0;
  }

  std::cerr << g_failures << " test(s) failed.\n";
  return 1;
}
