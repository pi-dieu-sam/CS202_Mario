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

/// SaveManager — owns the five on-disk single-player snapshot slots.
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
