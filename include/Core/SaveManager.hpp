#pragma once
#include "Core/GameSnapshot.hpp"
#include <array>
#include <filesystem>
#include <optional>
#include <string>

enum class SaveSlotStatus {
    Empty,
    Occupied,
    Corrupt,
};

/// Summary used by the slot-selection UI. It deliberately exposes only
/// display metadata; loading is a separate validated operation.
struct SaveSlotInfo {
    int slot = 0;
    SaveSlotStatus status = SaveSlotStatus::Empty;
    std::string character;
    int level = 1;
    int score = 0;
    int lives = 3;
    int coins = 0;
    int remainingSeconds = 0;
    std::uint64_t savedAtEpochSeconds = 0;
    std::string error;
};

/**
 * @class SaveManager
 * @brief Manages the persistence of game state through binary snapshots.
 *
 * This subsystem handles the creation, validation, and retrieval of save files
 * using a robust binary serialization format. It ensures that only fully intact
 * and valid single-player snapshots can be written to or read from the disk.
 * 
 * Key architectural features:
 * - **Atomic Saves:** Writes are performed via temporary files (`.tmp`) and then
 *   atomically renamed, preventing corrupted states if the game crashes mid-save.
 * - **Integrity Validation:** Every loaded snapshot is rigorously validated against
 *   version mismatch, out-of-bounds parameters, and corrupted data streams.
 * - **Testability:** Provides an isolated `setSaveDirectoryForTesting` interface
 *   to avoid polluting user `AppData` during unit test execution.
 *
 * @note This manager does not mutate the active `Game` instance; it merely
 * decodes file contents into `SaveData::GameSnapshot` objects for external use.
 */
class SaveManager {
public:
    static constexpr int SLOT_COUNT = SaveData::SAVE_SLOT_COUNT;

    /// Read all five slots. Invalid files are surfaced as Corrupt rather
    /// than being allowed to crash the game when the player selects them.
    static std::array<SaveSlotInfo, SLOT_COUNT> listSlots();

    /// Write a fully captured single-player snapshot to a numbered slot.
    /// Returns false and fills error on invalid input or I/O failure.
    static bool saveSlot(int slot, const SaveData::GameSnapshot& snapshot,
                         std::string* error = nullptr);

    /// Decode and validate one slot without mutating the running game.
    static std::optional<SaveData::GameSnapshot> loadSlot(
        int slot, std::string* error = nullptr);

    /// Public so the UI/tests can state where the new slot files live, but
    /// callers must use slotPath() rather than construct arbitrary paths.
    static std::filesystem::path saveDirectory();
    static std::filesystem::path slotPath(int slot);

    /// Test-only override keeps automated tests out of the player's app-data
    /// directory. Passing std::nullopt restores the production path.
    static void setSaveDirectoryForTesting(
        const std::optional<std::filesystem::path>& directory);
};
