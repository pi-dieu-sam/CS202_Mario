#include "Core/SaveManager.hpp"

#include <chrono>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <system_error>

#ifdef _WIN32
#include <windows.h>
#endif

namespace {

constexpr char FILE_MAGIC[] = "SMARIO_SNAPSHOT";
constexpr std::uint32_t MAX_COLLECTION_SIZE = 10000;
constexpr std::uint32_t MAX_STRING_SIZE = 1024;

std::optional<std::filesystem::path> g_testSaveDirectory;

bool isValidSlot(int slot) {
    return slot >= 1 && slot <= SaveManager::SLOT_COUNT;
}

bool finite(float value) {
    return std::isfinite(value);
}

class BinaryWriter {
public:
    explicit BinaryWriter(std::ostream& stream) : m_stream(stream) {}

    template <typename T>
    bool value(const T& item) {
        m_stream.write(reinterpret_cast<const char*>(&item), sizeof(T));
        return static_cast<bool>(m_stream);
    }

    bool boolean(bool item) {
        const std::uint8_t byte = item ? 1 : 0;
        return value(byte);
    }

    bool string(const std::string& item) {
        if (item.size() > MAX_STRING_SIZE) return false;
        const auto size = static_cast<std::uint32_t>(item.size());
        if (!value(size)) return false;
        m_stream.write(item.data(), static_cast<std::streamsize>(item.size()));
        return static_cast<bool>(m_stream);
    }

private:
    std::ostream& m_stream;
};

class BinaryReader {
public:
    explicit BinaryReader(std::istream& stream) : m_stream(stream) {}

    template <typename T>
    bool value(T& item) {
        m_stream.read(reinterpret_cast<char*>(&item), sizeof(T));
        return static_cast<bool>(m_stream);
    }

    bool boolean(bool& item) {
        std::uint8_t byte = 0;
        if (!value(byte) || byte > 1) return false;
        item = byte != 0;
        return true;
    }

    bool string(std::string& item) {
        std::uint32_t size = 0;
        if (!value(size) || size > MAX_STRING_SIZE) return false;
        item.resize(size);
        m_stream.read(item.data(), static_cast<std::streamsize>(size));
        return static_cast<bool>(m_stream);
    }

private:
    std::istream& m_stream;
};

bool writeVec2(BinaryWriter& out, const SaveData::Vec2& state) {
    return out.value(state.x) && out.value(state.y);
}

bool readVec2(BinaryReader& in, SaveData::Vec2& state) {
    return in.value(state.x) && in.value(state.y) && finite(state.x) && finite(state.y);
}

bool writeObject(BinaryWriter& out, const SaveData::ObjectState& state) {
    return writeVec2(out, state.position) && writeVec2(out, state.velocity) &&
           out.boolean(state.active);
}

bool readObject(BinaryReader& in, SaveData::ObjectState& state) {
    return readVec2(in, state.position) && readVec2(in, state.velocity) &&
           in.boolean(state.active);
}

bool writeCharacter(BinaryWriter& out, const SaveData::CharacterState& state) {
    return writeObject(out, state.object) && out.value(state.speed) &&
           out.value(state.jumpForce) && out.value(state.health) &&
           out.boolean(state.grounded) && out.boolean(state.facingRight) &&
           out.boolean(state.dead) && out.boolean(state.skidding) &&
           out.value(state.animationTimer) && out.value(state.animationFrame) &&
           out.value(state.animationFrames) && out.value(state.animationSpeed);
}

bool readCharacter(BinaryReader& in, SaveData::CharacterState& state) {
    return readObject(in, state.object) && in.value(state.speed) &&
           in.value(state.jumpForce) && in.value(state.health) &&
           in.boolean(state.grounded) && in.boolean(state.facingRight) &&
           in.boolean(state.dead) && in.boolean(state.skidding) &&
           in.value(state.animationTimer) && in.value(state.animationFrame) &&
           in.value(state.animationFrames) && in.value(state.animationSpeed) &&
           finite(state.speed) && finite(state.jumpForce) &&
           finite(state.animationTimer) && finite(state.animationSpeed);
}

bool writePlayer(BinaryWriter& out, const SaveData::PlayerState& state) {
    return writeCharacter(out, state.character) && out.value(state.powerUp) &&
           out.value(state.lives) && out.boolean(state.sprinting) &&
           out.boolean(state.wantsToShoot) && out.boolean(state.climbing) &&
           out.boolean(state.climbMoving) && out.boolean(state.vineReattachLocked) &&
           out.boolean(state.vineHorizontalReleaseRequired) &&
           out.boolean(state.hasVineAnchor) && writeVec2(out, state.lastVineAnchor) &&
           out.value(state.playerId) && out.value(state.invincibilityTimer) &&
           out.boolean(state.invincible) && out.value(state.starTimer) &&
           out.boolean(state.starPower) && out.value(state.sizeScale) &&
           out.boolean(state.growing) && out.value(state.growTimer) &&
           out.value(state.buffTimer) && out.value(state.blinkTimer) &&
           out.boolean(state.visible) && out.value(state.characterId) &&
           out.value(state.currentAnimation) && out.value(state.shootAnimationTimer);
}

bool readPlayer(BinaryReader& in, SaveData::PlayerState& state) {
    return readCharacter(in, state.character) && in.value(state.powerUp) &&
           in.value(state.lives) && in.boolean(state.sprinting) &&
           in.boolean(state.wantsToShoot) && in.boolean(state.climbing) &&
           in.boolean(state.climbMoving) && in.boolean(state.vineReattachLocked) &&
           in.boolean(state.vineHorizontalReleaseRequired) &&
           in.boolean(state.hasVineAnchor) && readVec2(in, state.lastVineAnchor) &&
           in.value(state.playerId) && in.value(state.invincibilityTimer) &&
           in.boolean(state.invincible) && in.value(state.starTimer) &&
           in.boolean(state.starPower) && in.value(state.sizeScale) &&
           in.boolean(state.growing) && in.value(state.growTimer) &&
           in.value(state.buffTimer) && in.value(state.blinkTimer) &&
           in.boolean(state.visible) && in.value(state.characterId) &&
           in.value(state.currentAnimation) && in.value(state.shootAnimationTimer) &&
           finite(state.invincibilityTimer) && finite(state.starTimer) &&
           finite(state.sizeScale) && finite(state.growTimer) &&
           finite(state.buffTimer) && finite(state.blinkTimer) &&
           finite(state.shootAnimationTimer);
}

bool writeEnemy(BinaryWriter& out, const SaveData::EnemyState& state) {
    const auto kind = static_cast<std::int32_t>(state.kind);
    return out.value(kind) && writeCharacter(out, state.character) &&
           out.value(state.scoreValue) && out.value(state.theme) &&
           out.value(state.goombaDeathTimer) && out.boolean(state.goombaSquished) &&
           out.value(state.koopaState) && out.boolean(state.koopaSliding) &&
           out.boolean(state.koopaBrakingAfterWall) && out.value(state.koopaDieTimer) &&
           out.value(state.koopaShellSpeed) && out.value(state.bowserState) &&
           out.value(state.bowserStateTimer) && out.value(state.bowserBreathFrame) &&
           out.value(state.bowserFireballHits) && writeVec2(out, state.bowserPlayerPosition) &&
           out.boolean(state.bowserHasPlayerPosition) && out.value(state.bowserNextFireTime) &&
           out.value(state.bowserPendingFireballs) && out.value(state.piranhaState) &&
           out.value(state.piranhaCurrentFrame) && out.value(state.piranhaFrameTimer) &&
           out.value(state.piranhaHideTimer) && out.value(state.piranhaWaitTimer) &&
           writeVec2(out, state.piranhaBasePosition) && out.boolean(state.piranhaBaseCaptured);
}

bool readEnemy(BinaryReader& in, SaveData::EnemyState& state) {
    std::int32_t kind = 0;
    if (!in.value(kind) || kind < static_cast<std::int32_t>(SaveData::EnemyKind::Goomba) ||
        kind > static_cast<std::int32_t>(SaveData::EnemyKind::PiranhaPlant)) return false;
    state.kind = static_cast<SaveData::EnemyKind>(kind);
    return readCharacter(in, state.character) && in.value(state.scoreValue) &&
           in.value(state.theme) && in.value(state.goombaDeathTimer) &&
           in.boolean(state.goombaSquished) && in.value(state.koopaState) &&
           in.boolean(state.koopaSliding) && in.boolean(state.koopaBrakingAfterWall) &&
           in.value(state.koopaDieTimer) && in.value(state.koopaShellSpeed) &&
           in.value(state.bowserState) && in.value(state.bowserStateTimer) &&
           in.value(state.bowserBreathFrame) && in.value(state.bowserFireballHits) &&
           readVec2(in, state.bowserPlayerPosition) && in.boolean(state.bowserHasPlayerPosition) &&
           in.value(state.bowserNextFireTime) && in.value(state.bowserPendingFireballs) &&
           in.value(state.piranhaState) && in.value(state.piranhaCurrentFrame) &&
           in.value(state.piranhaFrameTimer) && in.value(state.piranhaHideTimer) &&
           in.value(state.piranhaWaitTimer) && readVec2(in, state.piranhaBasePosition) &&
           in.boolean(state.piranhaBaseCaptured) && finite(state.goombaDeathTimer) &&
           finite(state.koopaDieTimer) && finite(state.koopaShellSpeed) &&
           finite(state.bowserStateTimer) && finite(state.bowserNextFireTime) &&
           finite(state.piranhaFrameTimer) && finite(state.piranhaHideTimer) &&
           finite(state.piranhaWaitTimer);
}

bool writeItem(BinaryWriter& out, const SaveData::ItemState& state) {
    const auto kind = static_cast<std::int32_t>(state.kind);
    return out.value(kind) && writeObject(out, state.object) && out.boolean(state.moving) &&
           out.boolean(state.collected) && out.value(state.theme) &&
           out.value(state.animationTimer) && out.value(state.animationFrame);
}

bool readItem(BinaryReader& in, SaveData::ItemState& state) {
    std::int32_t kind = 0;
    if (!in.value(kind) || kind < static_cast<std::int32_t>(SaveData::ItemKind::Coin) ||
        kind > static_cast<std::int32_t>(SaveData::ItemKind::FlowersBuff)) return false;
    state.kind = static_cast<SaveData::ItemKind>(kind);
    return readObject(in, state.object) && in.boolean(state.moving) &&
           in.boolean(state.collected) && in.value(state.theme) &&
           in.value(state.animationTimer) && in.value(state.animationFrame) &&
           finite(state.animationTimer);
}

bool writeBlock(BinaryWriter& out, const SaveData::BlockState& state) {
    return writeObject(out, state.object) && out.value(state.blockType) &&
           out.value(state.containedItem) && out.boolean(state.used) &&
           out.value(state.theme) && out.value(state.bumpOffset) &&
           out.value(state.bumpTimer) && out.boolean(state.bumping);
}

bool readBlock(BinaryReader& in, SaveData::BlockState& state) {
    return readObject(in, state.object) && in.value(state.blockType) &&
           in.value(state.containedItem) && in.boolean(state.used) &&
           in.value(state.theme) && in.value(state.bumpOffset) &&
           in.value(state.bumpTimer) && in.boolean(state.bumping) &&
           finite(state.bumpOffset) && finite(state.bumpTimer);
}

bool writeFireball(BinaryWriter& out, const SaveData::FireballState& state) {
    return writeObject(out, state.object) && out.value(state.lifetime) &&
           out.value(state.direction) && out.value(state.animationTimer) &&
           out.value(state.animationFrame) && out.value(state.surfaceHits);
}

bool readFireball(BinaryReader& in, SaveData::FireballState& state) {
    return readObject(in, state.object) && in.value(state.lifetime) &&
           in.value(state.direction) && in.value(state.animationTimer) &&
           in.value(state.animationFrame) && in.value(state.surfaceHits) &&
           finite(state.lifetime) && finite(state.animationTimer);
}

bool writeBowserFireball(BinaryWriter& out, const SaveData::BowserFireballState& state) {
    return writeObject(out, state.object) && out.value(state.lifetime) &&
           out.value(state.animationTimer) && out.value(state.animationFrame);
}

bool readBowserFireball(BinaryReader& in, SaveData::BowserFireballState& state) {
    return readObject(in, state.object) && in.value(state.lifetime) &&
           in.value(state.animationTimer) && in.value(state.animationFrame) &&
           finite(state.lifetime) && finite(state.animationTimer);
}

bool writeEscalater(BinaryWriter& out, const SaveData::EscalaterState& state) {
    return writeObject(out, state.object) && out.value(state.axis) &&
           writeVec2(out, state.size) && writeVec2(out, state.renderSize) &&
           out.value(state.centerX) && out.value(state.centerY) && out.value(state.range) &&
           out.value(state.speed) && out.value(state.direction) && out.value(state.mapLeft) &&
           out.value(state.mapRight) && out.value(state.mapTop) && out.value(state.mapBottom);
}

bool readEscalater(BinaryReader& in, SaveData::EscalaterState& state) {
    return readObject(in, state.object) && in.value(state.axis) && readVec2(in, state.size) &&
           readVec2(in, state.renderSize) && in.value(state.centerX) && in.value(state.centerY) &&
           in.value(state.range) && in.value(state.speed) && in.value(state.direction) &&
           in.value(state.mapLeft) && in.value(state.mapRight) && in.value(state.mapTop) &&
           in.value(state.mapBottom) && finite(state.centerX) && finite(state.centerY) &&
           finite(state.range) && finite(state.speed) && finite(state.direction) &&
           finite(state.mapLeft) && finite(state.mapRight) && finite(state.mapTop) &&
           finite(state.mapBottom);
}

bool writeFireBar(BinaryWriter& out, const SaveData::FireBarState& state) {
    return writeObject(out, state.object) && out.value(state.angle) &&
           out.value(state.angularSpeed) && out.value(state.fireballRotationDegrees) &&
           out.value(state.fireballSpinSpeed) && out.value(state.animationTimer) &&
           out.value(state.animationFrame) && out.value(state.segmentCount) &&
           out.value(state.segmentSpacing);
}

bool readFireBar(BinaryReader& in, SaveData::FireBarState& state) {
    return readObject(in, state.object) && in.value(state.angle) &&
           in.value(state.angularSpeed) && in.value(state.fireballRotationDegrees) &&
           in.value(state.fireballSpinSpeed) && in.value(state.animationTimer) &&
           in.value(state.animationFrame) && in.value(state.segmentCount) &&
           in.value(state.segmentSpacing) && finite(state.angle) && finite(state.angularSpeed) &&
           finite(state.fireballRotationDegrees) && finite(state.fireballSpinSpeed) &&
           finite(state.animationTimer) && finite(state.segmentSpacing);
}

bool writeLavaFireball(BinaryWriter& out, const SaveData::LavaFireballState& state) {
    return writeObject(out, state.object) && writeVec2(out, state.launchPosition) &&
           out.value(state.launchSpeed) && out.value(state.totalFlightTime) &&
           out.value(state.flightTimer) && out.value(state.cooldownTimer) &&
           out.value(state.animationFrame) && out.boolean(state.visible);
}

bool readLavaFireball(BinaryReader& in, SaveData::LavaFireballState& state) {
    return readObject(in, state.object) && readVec2(in, state.launchPosition) &&
           in.value(state.launchSpeed) && in.value(state.totalFlightTime) &&
           in.value(state.flightTimer) && in.value(state.cooldownTimer) &&
           in.value(state.animationFrame) && in.boolean(state.visible) &&
           finite(state.launchSpeed) && finite(state.totalFlightTime) &&
           finite(state.flightTimer) && finite(state.cooldownTimer);
}

bool writeFlagpole(BinaryWriter& out, const SaveData::FlagpoleState& state) {
    return out.boolean(state.present) && writeObject(out, state.object) &&
           writeVec2(out, state.flagPosition) && out.boolean(state.reached) &&
           out.value(state.flagDropY);
}

bool readFlagpole(BinaryReader& in, SaveData::FlagpoleState& state) {
    return in.boolean(state.present) && readObject(in, state.object) &&
           readVec2(in, state.flagPosition) && in.boolean(state.reached) &&
           in.value(state.flagDropY) && finite(state.flagDropY);
}

template <typename T, typename Writer>
bool writeVector(BinaryWriter& out, const std::vector<T>& items, Writer writer) {
    if (items.size() > MAX_COLLECTION_SIZE) return false;
    const auto count = static_cast<std::uint32_t>(items.size());
    if (!out.value(count)) return false;
    for (const auto& item : items) {
        if (!writer(out, item)) return false;
    }
    return true;
}

template <typename T, typename Reader>
bool readVector(BinaryReader& in, std::vector<T>& items, Reader reader) {
    std::uint32_t count = 0;
    if (!in.value(count) || count > MAX_COLLECTION_SIZE) return false;
    items.clear();
    items.reserve(count);
    for (std::uint32_t i = 0; i < count; ++i) {
        T item;
        if (!reader(in, item)) return false;
        items.push_back(std::move(item));
    }
    return true;
}

bool writeSnapshot(BinaryWriter& out, const SaveData::GameSnapshot& snapshot) {
    const std::uint32_t magicSize = static_cast<std::uint32_t>(sizeof(FILE_MAGIC) - 1);
    if (!out.value(magicSize)) return false;
    for (std::uint32_t i = 0; i < magicSize; ++i) {
        if (!out.value(FILE_MAGIC[i])) return false;
    }

    return out.value(snapshot.formatVersion) && out.value(snapshot.savedAtEpochSeconds) &&
           out.value(snapshot.gameMode) && out.value(snapshot.progress.level) &&
           out.value(snapshot.progress.score) && out.value(snapshot.progress.lives) &&
           out.value(snapshot.progress.coins) && out.string(snapshot.progress.character) &&
           out.value(snapshot.levelTimer) && out.value(snapshot.mainLevelNumber) &&
           out.boolean(snapshot.inSecretRoom) && writeVec2(out, snapshot.pipeReturnPosition) &&
           out.value(snapshot.pipeReturnPowerUp) && writePlayer(out, snapshot.level.player) &&
           writeVector(out, snapshot.level.blocks, writeBlock) &&
           writeVector(out, snapshot.level.enemies, writeEnemy) &&
           writeVector(out, snapshot.level.items, writeItem) &&
           writeVector(out, snapshot.level.fireballs, writeFireball) &&
           writeVector(out, snapshot.level.bowserFireballs, writeBowserFireball) &&
           writeVector(out, snapshot.level.escalaters, writeEscalater) &&
           writeVector(out, snapshot.level.fireBars, writeFireBar) &&
           writeVector(out, snapshot.level.lavaFireballs, writeLavaFireball) &&
           writeFlagpole(out, snapshot.level.flagpole);
}

bool readSnapshot(BinaryReader& in, SaveData::GameSnapshot& snapshot) {
    std::uint32_t magicSize = 0;
    if (!in.value(magicSize) || magicSize != sizeof(FILE_MAGIC) - 1) return false;
    for (std::uint32_t i = 0; i < magicSize; ++i) {
        char value = '\0';
        if (!in.value(value) || value != FILE_MAGIC[i]) return false;
    }

    return in.value(snapshot.formatVersion) && in.value(snapshot.savedAtEpochSeconds) &&
           in.value(snapshot.gameMode) && in.value(snapshot.progress.level) &&
           in.value(snapshot.progress.score) && in.value(snapshot.progress.lives) &&
           in.value(snapshot.progress.coins) && in.string(snapshot.progress.character) &&
           in.value(snapshot.levelTimer) && in.value(snapshot.mainLevelNumber) &&
           in.boolean(snapshot.inSecretRoom) && readVec2(in, snapshot.pipeReturnPosition) &&
           in.value(snapshot.pipeReturnPowerUp) && readPlayer(in, snapshot.level.player) &&
           readVector(in, snapshot.level.blocks, readBlock) &&
           readVector(in, snapshot.level.enemies, readEnemy) &&
           readVector(in, snapshot.level.items, readItem) &&
           readVector(in, snapshot.level.fireballs, readFireball) &&
           readVector(in, snapshot.level.bowserFireballs, readBowserFireball) &&
           readVector(in, snapshot.level.escalaters, readEscalater) &&
           readVector(in, snapshot.level.fireBars, readFireBar) &&
           readVector(in, snapshot.level.lavaFireballs, readLavaFireball) &&
           readFlagpole(in, snapshot.level.flagpole) && finite(snapshot.levelTimer);
}

bool validateSnapshot(const SaveData::GameSnapshot& snapshot, std::string* error) {
    auto fail = [&](const char* message) {
        if (error) *error = message;
        return false;
    };

    if (snapshot.formatVersion != SaveData::SNAPSHOT_FORMAT_VERSION)
        return fail("Unsupported save version");
    if (snapshot.gameMode != 0)
        return fail("Only single-player snapshots can be loaded");
    if (snapshot.progress.level < 1 || snapshot.progress.level > 3 ||
        snapshot.mainLevelNumber < 1 || snapshot.mainLevelNumber > 3)
        return fail("Save references an unavailable level");
    if (snapshot.progress.lives < 1 || snapshot.progress.lives > 99 ||
        snapshot.progress.coins < 0 || snapshot.progress.coins > 9999 ||
        snapshot.progress.score < 0 || snapshot.progress.character.empty() ||
        snapshot.progress.character.size() > 64)
        return fail("Save contains invalid progress values");
    if (snapshot.levelTimer < 0.0f || snapshot.levelTimer > 3600.0f ||
        snapshot.level.player.character.dead)
        return fail("Save was captured during an unsupported game state");
    return true;
}

std::uint64_t nowEpochSeconds() {
    return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
}

bool replaceFile(const std::filesystem::path& temporary,
                 const std::filesystem::path& destination,
                 std::string* error) {
#ifdef _WIN32
    if (MoveFileExW(temporary.c_str(), destination.c_str(),
                    MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        return true;
    }
    if (error) *error = "Could not replace the save slot";
    return false;
#else
    std::error_code ec;
    std::filesystem::rename(temporary, destination, ec);
    if (!ec) return true;
    if (error) *error = "Could not replace the save slot";
    return false;
#endif
}

} // namespace

std::filesystem::path SaveManager::saveDirectory() {
    if (g_testSaveDirectory) return *g_testSaveDirectory;

#ifdef _WIN32
    if (const char* localAppData = std::getenv("LOCALAPPDATA")) {
        return std::filesystem::path(localAppData) / "SuperMario" / "saves";
    }
#endif
    return std::filesystem::current_path() / "saves";
}

std::filesystem::path SaveManager::slotPath(int slot) {
    if (!isValidSlot(slot)) return {};
    return saveDirectory() / ("slot-" + std::to_string(slot) + ".snapshot");
}

void SaveManager::setSaveDirectoryForTesting(
    const std::optional<std::filesystem::path>& directory) {
    g_testSaveDirectory = directory;
}

bool SaveManager::saveSlot(int slot, const SaveData::GameSnapshot& input,
                           std::string* error) {
    if (!isValidSlot(slot)) {
        if (error) *error = "Invalid save slot";
        return false;
    }

    SaveData::GameSnapshot snapshot = input;
    if (!validateSnapshot(snapshot, error)) return false;
    snapshot.savedAtEpochSeconds = nowEpochSeconds();

    std::error_code ec;
    const auto directory = saveDirectory();
    std::filesystem::create_directories(directory, ec);
    if (ec) {
        if (error) *error = "Could not create the save directory";
        return false;
    }

    const auto destination = slotPath(slot);
    const std::filesystem::path temporary = destination.string() + ".tmp";
    {
        std::ofstream file(temporary, std::ios::binary | std::ios::trunc);
        if (!file.is_open()) {
            if (error) *error = "Could not open the temporary save file";
            return false;
        }
        BinaryWriter writer(file);
        if (!writeSnapshot(writer, snapshot)) {
            file.close();
            std::filesystem::remove(temporary, ec);
            if (error) *error = "Could not write the save snapshot";
            return false;
        }
        file.flush();
        if (!file.good()) {
            file.close();
            std::filesystem::remove(temporary, ec);
            if (error) *error = "Could not finish writing the save snapshot";
            return false;
        }
    }

    if (!replaceFile(temporary, destination, error)) {
        std::filesystem::remove(temporary, ec);
        return false;
    }
    return true;
}

std::optional<SaveData::GameSnapshot> SaveManager::loadSlot(int slot,
                                                              std::string* error) {
    if (!isValidSlot(slot)) {
        if (error) *error = "Invalid save slot";
        return std::nullopt;
    }

    const auto path = slotPath(slot);
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        if (error) *error = "This slot is empty";
        return std::nullopt;
    }

    SaveData::GameSnapshot snapshot;
    BinaryReader reader(file);
    if (!readSnapshot(reader, snapshot) || !validateSnapshot(snapshot, error)) {
        if (error && error->empty()) *error = "Save file is corrupted";
        return std::nullopt;
    }
    return snapshot;
}

std::array<SaveSlotInfo, SaveManager::SLOT_COUNT> SaveManager::listSlots() {
    std::array<SaveSlotInfo, SLOT_COUNT> slots{};
    for (int index = 0; index < SLOT_COUNT; ++index) {
        SaveSlotInfo& info = slots[static_cast<std::size_t>(index)];
        info.slot = index + 1;

        std::error_code ec;
        if (!std::filesystem::exists(slotPath(info.slot), ec)) {
            info.status = SaveSlotStatus::Empty;
            continue;
        }

        std::string error;
        const auto snapshot = loadSlot(info.slot, &error);
        if (!snapshot) {
            info.status = SaveSlotStatus::Corrupt;
            info.error = error.empty() ? "Save file is corrupted" : error;
            continue;
        }

        info.status = SaveSlotStatus::Occupied;
        info.character = snapshot->progress.character;
        info.level = snapshot->progress.level;
        info.score = snapshot->progress.score;
        info.lives = snapshot->progress.lives;
        info.coins = snapshot->progress.coins;
        info.remainingSeconds = static_cast<int>(snapshot->levelTimer);
        info.savedAtEpochSeconds = snapshot->savedAtEpochSeconds;
    }
    return slots;
}
