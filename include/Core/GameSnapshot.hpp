#pragma once

#include <cstdint>
#include <string>
#include <vector>

// Data-only representation of a resumable single-player run. These structs
// deliberately contain no SFML resource types or owning pointers so a save can
// be decoded and validated before it touches the running game.
namespace SaveData {

constexpr std::uint32_t SNAPSHOT_FORMAT_VERSION = 1;
constexpr int SAVE_SLOT_COUNT = 5;

struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;
};

struct ObjectState {
    Vec2 position;
    Vec2 velocity;
    bool active = true;
};

struct CharacterState {
    ObjectState object;
    float speed = 0.0f;
    float jumpForce = 0.0f;
    int health = 1;
    bool grounded = false;
    bool facingRight = true;
    bool dead = false;
    bool skidding = false;
    float animationTimer = 0.0f;
    int animationFrame = 0;
    int animationFrames = 1;
    float animationSpeed = 0.1f;
};

struct PlayerState {
    CharacterState character;
    int powerUp = 0;
    int lives = 3;
    bool sprinting = false;
    bool wantsToShoot = false;
    bool climbing = false;
    bool climbMoving = false;
    bool vineReattachLocked = false;
    bool vineHorizontalReleaseRequired = false;
    bool hasVineAnchor = false;
    Vec2 lastVineAnchor;
    int playerId = 1;
    float invincibilityTimer = 0.0f;
    bool invincible = false;
    float starTimer = 0.0f;
    bool starPower = false;
    float sizeScale = 1.0f;
    bool growing = false;
    float growTimer = 0.0f;
    float buffTimer = 0.0f;
    float blinkTimer = 0.0f;
    bool visible = true;
    int characterId = 0;
    int currentAnimation = 0;
    float shootAnimationTimer = 0.0f;
};

enum class EnemyKind : std::int32_t {
    Goomba,
    Koopa,
    Troopa,
    Bowser,
    PiranhaPlant,
};

struct EnemyState {
    EnemyKind kind = EnemyKind::Goomba;
    CharacterState character;
    int scoreValue = 0;
    int theme = 0;

    // Goomba
    float goombaDeathTimer = 0.0f;
    bool goombaSquished = false;

    // Koopa
    int koopaState = 0;
    bool koopaSliding = false;
    bool koopaBrakingAfterWall = false;
    float koopaDieTimer = 0.0f;
    float koopaShellSpeed = 0.0f;

    // Bowser
    int bowserState = 0;
    float bowserStateTimer = 0.0f;
    int bowserBreathFrame = 0;
    int bowserFireballHits = 0;
    Vec2 bowserPlayerPosition;
    bool bowserHasPlayerPosition = false;
    float bowserNextFireTime = 0.0f;
    int bowserPendingFireballs = 0;

    // Piranha Plant
    int piranhaState = 0;
    int piranhaCurrentFrame = 0;
    float piranhaFrameTimer = 0.0f;
    float piranhaHideTimer = 0.0f;
    float piranhaWaitTimer = 0.0f;
    Vec2 piranhaBasePosition;
    bool piranhaBaseCaptured = false;
};

enum class ItemKind : std::int32_t {
    Coin,
    Mushroom,
    FireFlower,
    Star,
    FlowersBuff,
};

struct ItemState {
    ItemKind kind = ItemKind::Coin;
    ObjectState object;
    bool moving = false;
    bool collected = false;
    int theme = 0;
    float animationTimer = 0.0f;
    int animationFrame = 0;
};

struct BlockState {
    ObjectState object;
    int blockType = 0;
    int containedItem = 0;
    bool used = false;
    int theme = 0;
    float bumpOffset = 0.0f;
    float bumpTimer = 0.0f;
    bool bumping = false;
};

struct FireballState {
    ObjectState object;
    float lifetime = 0.0f;
    int direction = 1;
    float animationTimer = 0.0f;
    int animationFrame = 0;
    int surfaceHits = 0;
};

struct BowserFireballState {
    ObjectState object;
    float lifetime = 0.0f;
    float animationTimer = 0.0f;
    int animationFrame = 0;
};

struct EscalaterState {
    ObjectState object;
    int axis = 0;
    Vec2 size;
    Vec2 renderSize;
    float centerX = 0.0f;
    float centerY = 0.0f;
    float range = 0.0f;
    float speed = 0.0f;
    float direction = 0.0f;
    float mapLeft = 0.0f;
    float mapRight = 0.0f;
    float mapTop = 0.0f;
    float mapBottom = 0.0f;
};

struct FireBarState {
    ObjectState object;
    float angle = 0.0f;
    float angularSpeed = 0.0f;
    float fireballRotationDegrees = 0.0f;
    float fireballSpinSpeed = 0.0f;
    float animationTimer = 0.0f;
    int animationFrame = 0;
    int segmentCount = 0;
    float segmentSpacing = 0.0f;
};

struct LavaFireballState {
    ObjectState object;
    Vec2 launchPosition;
    float launchSpeed = 0.0f;
    float totalFlightTime = 0.0f;
    float flightTimer = 0.0f;
    float cooldownTimer = 0.0f;
    int animationFrame = 0;
    bool visible = true;
};

struct FlagpoleState {
    bool present = false;
    ObjectState object;
    Vec2 flagPosition;
    bool reached = false;
    float flagDropY = 0.0f;
};

struct LevelState {
    PlayerState player;
    std::vector<BlockState> blocks;
    std::vector<EnemyState> enemies;
    std::vector<ItemState> items;
    std::vector<FireballState> fireballs;
    std::vector<BowserFireballState> bowserFireballs;
    std::vector<EscalaterState> escalaters;
    std::vector<FireBarState> fireBars;
    std::vector<LavaFireballState> lavaFireballs;
    FlagpoleState flagpole;
};

struct ProgressState {
    int level = 1;
    int score = 0;
    int lives = 3;
    int coins = 0;
    std::string character = "Mario";
};

struct GameSnapshot {
    std::uint32_t formatVersion = SNAPSHOT_FORMAT_VERSION;
    std::uint64_t savedAtEpochSeconds = 0;
    // Snapshot slots are deliberately single-player only. The persisted value
    // is retained so invalid/multiplayer files can be rejected.
    int gameMode = 0;
    ProgressState progress;
    float levelTimer = 0.0f;
    int mainLevelNumber = 1;
    bool inSecretRoom = false;
    Vec2 pipeReturnPosition;
    int pipeReturnPowerUp = 0;
    LevelState level;
};

} // namespace SaveData
