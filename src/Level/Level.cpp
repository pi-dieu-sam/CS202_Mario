#include "Level/Level.hpp"
#include "AI/ChaseStrategy.hpp"
#include "Entities/Koopa.hpp"
#include "Entities/Bowser.hpp"
#include "Entities/BowserFireball.hpp"
#include "Entities/Goomba.hpp"
#include "Entities/Troopa.hpp"
#include "Entities/PiranhaPlant.hpp"
#include "Entities/Coin.hpp"
#include "Entities/Mushroom.hpp"
#include "Entities/FireFlower.hpp"
#include "Entities/Star.hpp"
#include "Entities/FlowersBuff.hpp"
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

class SnapshotAccess {
public:
  static SaveData::LevelState capture(const Level &level) {
    SaveData::LevelState snapshot;
    if (level.m_player) {
      capturePlayer(*level.m_player, snapshot.player);
    }

    snapshot.blocks.reserve(level.m_blocks.size());
    for (const auto &block : level.m_blocks) {
      snapshot.blocks.push_back(captureBlock(*block));
    }
    snapshot.enemies.reserve(level.m_enemies.size());
    for (const auto &enemy : level.m_enemies) {
      snapshot.enemies.push_back(captureEnemy(*enemy));
    }
    snapshot.items.reserve(level.m_items.size());
    for (const auto &item : level.m_items) {
      snapshot.items.push_back(captureItem(*item));
    }
    snapshot.fireballs.reserve(level.m_fireballs.size());
    for (const auto &fireball : level.m_fireballs) {
      snapshot.fireballs.push_back(captureFireball(*fireball));
    }
    snapshot.bowserFireballs.reserve(level.m_bowserFireballs.size());
    for (const auto &fireball : level.m_bowserFireballs) {
      snapshot.bowserFireballs.push_back(captureBowserFireball(*fireball));
    }
    snapshot.escalaters.reserve(level.m_escalaters.size());
    for (const auto &escalater : level.m_escalaters) {
      snapshot.escalaters.push_back(captureEscalater(*escalater));
    }
    snapshot.fireBars.reserve(level.m_fireBars.size());
    for (const auto &fireBar : level.m_fireBars) {
      snapshot.fireBars.push_back(captureFireBar(*fireBar));
    }
    snapshot.lavaFireballs.reserve(level.m_lavaFireballs.size());
    for (const auto &lavaFireball : level.m_lavaFireballs) {
      snapshot.lavaFireballs.push_back(captureLavaFireball(*lavaFireball));
    }
    if (level.m_flagpole) {
      snapshot.flagpole.present = true;
      snapshot.flagpole.object = captureObject(*level.m_flagpole);
      snapshot.flagpole.flagPosition = captureVec(level.m_flagpole->m_flagPos);
      snapshot.flagpole.reached = level.m_flagpole->m_reached;
      snapshot.flagpole.flagDropY = level.m_flagpole->m_flagDropY;
    }
    return snapshot;
  }

  static bool restore(Level &level, const SaveData::LevelState &snapshot) {
    if (!level.m_player || snapshot.player.character.dead) return false;

    // The terrain grid is immutable and was loaded from a trusted map file.
    // Every mutable collection is replaced with the saved contents so defeated
    // enemies, broken blocks, collected items, and active projectiles survive.
    restorePlayer(*level.m_player, snapshot.player);
    level.m_player2.reset();

    level.m_blocks.clear();
    for (const auto &state : snapshot.blocks) {
      auto block = std::make_unique<Block>(
          static_cast<BlockType>(state.blockType), state.object.position.x,
          state.object.position.y, static_cast<LevelTheme>(state.theme), state.used);
      restoreBlock(*block, state);
      level.m_blocks.push_back(std::move(block));
    }

    level.m_enemies.clear();
    for (const auto &state : snapshot.enemies) {
      auto enemy = makeEnemy(state);
      if (!enemy) return false;
      level.m_enemies.push_back(std::move(enemy));
    }

    level.m_items.clear();
    for (const auto &state : snapshot.items) {
      auto item = makeItem(state);
      if (!item) return false;
      level.m_items.push_back(std::move(item));
    }

    level.m_fireballs.clear();
    for (const auto &state : snapshot.fireballs) {
      auto fireball = std::make_unique<Fireball>(
          state.object.position.x, state.object.position.y, state.direction);
      restoreFireball(*fireball, state);
      level.m_fireballs.push_back(std::move(fireball));
    }

    level.m_bowserFireballs.clear();
    for (const auto &state : snapshot.bowserFireballs) {
      const int direction = state.object.velocity.x < 0.0f ? -1 : 1;
      auto fireball = std::make_unique<BowserFireball>(
          state.object.position.x, state.object.position.y, direction);
      restoreBowserFireball(*fireball, state);
      level.m_bowserFireballs.push_back(std::move(fireball));
    }

    level.m_escalaters.clear();
    for (const auto &state : snapshot.escalaters) {
      auto escalater = std::make_unique<Escalater>(
          state.object.position.x, state.object.position.y,
          static_cast<Escalater::MovementAxis>(state.axis));
      restoreEscalater(*escalater, state);
      level.m_escalaters.push_back(std::move(escalater));
    }

    level.m_fireBars.clear();
    for (const auto &state : snapshot.fireBars) {
      auto fireBar = std::make_unique<FireBar>(state.object.position.x,
                                               state.object.position.y,
                                               state.segmentCount);
      restoreFireBar(*fireBar, state);
      level.m_fireBars.push_back(std::move(fireBar));
    }

    level.m_lavaFireballs.clear();
    for (const auto &state : snapshot.lavaFireballs) {
      auto lavaFireball = std::make_unique<LavaFireball>(
          state.launchPosition.x, state.launchPosition.y);
      restoreLavaFireball(*lavaFireball, state);
      level.m_lavaFireballs.push_back(std::move(lavaFireball));
    }

    if (snapshot.flagpole.present) {
      if (!level.m_flagpole) {
        level.m_flagpole = std::make_unique<Flagpole>(
            snapshot.flagpole.object.position.x, snapshot.flagpole.object.position.y);
      }
      restoreObject(*level.m_flagpole, snapshot.flagpole.object);
      level.m_flagpole->m_flagPos = restoreVec(snapshot.flagpole.flagPosition);
      level.m_flagpole->m_reached = snapshot.flagpole.reached;
      level.m_flagpole->m_flagDropY = snapshot.flagpole.flagDropY;
    } else {
      level.m_flagpole.reset();
    }
    return true;
  }

private:
  static SaveData::Vec2 captureVec(const sf::Vector2f &value) {
    return {value.x, value.y};
  }

  static sf::Vector2f restoreVec(const SaveData::Vec2 &value) {
    return {value.x, value.y};
  }

  static SaveData::ObjectState captureObject(const GameObject &object) {
    SaveData::ObjectState state;
    state.position = captureVec(object.m_position);
    state.velocity = captureVec(object.m_velocity);
    state.active = object.m_active;
    return state;
  }

  static void restoreObject(GameObject &object, const SaveData::ObjectState &state) {
    object.m_position = restoreVec(state.position);
    object.m_velocity = restoreVec(state.velocity);
    object.m_active = state.active;
  }

  static SaveData::CharacterState captureCharacter(const Character &character) {
    SaveData::CharacterState state;
    state.object = captureObject(character);
    state.speed = character.m_speed;
    state.jumpForce = character.m_jumpForce;
    state.health = character.m_health;
    state.grounded = character.m_grounded;
    state.facingRight = character.m_facingRight;
    state.dead = character.m_dead;
    state.skidding = character.m_skidding;
    state.animationTimer = character.m_animTimer;
    state.animationFrame = character.m_animFrame;
    state.animationFrames = character.m_animFrames;
    state.animationSpeed = character.m_animSpeed;
    return state;
  }

  static void restoreCharacter(Character &character,
                               const SaveData::CharacterState &state) {
    restoreObject(character, state.object);
    character.m_speed = state.speed;
    character.m_jumpForce = state.jumpForce;
    character.m_health = state.health;
    character.m_grounded = state.grounded;
    character.m_facingRight = state.facingRight;
    character.m_dead = state.dead;
    character.m_skidding = state.skidding;
    character.m_animTimer = state.animationTimer;
    character.m_animFrame = state.animationFrame;
    character.m_animFrames = state.animationFrames;
    character.m_animSpeed = state.animationSpeed;
  }

  static void capturePlayer(const Player &player, SaveData::PlayerState &state) {
    state.character = captureCharacter(player);
    state.powerUp = static_cast<int>(player.m_powerUp);
    state.lives = player.m_lives;
    state.sprinting = player.m_sprinting;
    state.wantsToShoot = player.m_wantsToShoot;
    state.climbing = player.m_climbing;
    state.climbMoving = player.m_climbMoving;
    state.vineReattachLocked = player.m_vineReattachLocked;
    state.vineHorizontalReleaseRequired = player.m_vineHorizontalReleaseRequired;
    state.hasVineAnchor = player.m_hasVineAnchor;
    state.lastVineAnchor = captureVec(player.m_lastVineAnchor);
    state.playerId = player.m_playerId;
    state.invincibilityTimer = player.m_invincibleTimer;
    state.invincible = player.m_invincible;
    state.starTimer = player.m_starTimer;
    state.starPower = player.m_starPower;
    state.sizeScale = player.m_sizeScale;
    state.growing = player.m_growing;
    state.growTimer = player.m_growTimer;
    state.buffTimer = player.m_buffTimer;
    state.blinkTimer = player.m_blinkTimer;
    state.visible = player.m_visible;
    state.characterId = static_cast<int>(player.m_characterId);
    state.currentAnimation = static_cast<int>(player.m_currentAnim);
    state.shootAnimationTimer = player.m_shootAnimTimer;
  }

  static void restorePlayer(Player &player, const SaveData::PlayerState &state) {
    restoreCharacter(player, state.character);
    player.m_powerUp = static_cast<PowerUpState>(state.powerUp);
    player.m_lives = state.lives;
    player.m_sprinting = state.sprinting;
    player.m_wantsToShoot = state.wantsToShoot;
    // Input state is intentionally reset on a load; preserving a physical
    // held key can make the player move before the user regains control.
    player.m_jumpHeld = false;
    player.m_climbing = state.climbing;
    player.m_climbMoving = state.climbMoving;
    player.m_vineReattachLocked = state.vineReattachLocked;
    player.m_vineHorizontalReleaseRequired = state.vineHorizontalReleaseRequired;
    player.m_hasVineAnchor = state.hasVineAnchor;
    player.m_lastVineAnchor = restoreVec(state.lastVineAnchor);
    player.m_playerId = state.playerId;
    player.m_invincibleTimer = state.invincibilityTimer;
    player.m_invincible = state.invincible;
    player.m_starTimer = state.starTimer;
    player.m_starPower = state.starPower;
    player.m_sizeScale = state.sizeScale;
    player.m_growing = state.growing;
    player.m_growTimer = state.growTimer;
    player.m_buffTimer = state.buffTimer;
    player.m_blinkTimer = state.blinkTimer;
    player.m_visible = state.visible;
    player.m_characterId = static_cast<CharacterId>(state.characterId);
    player.m_currentAnim = static_cast<SpriteRegistry::PlayerAnim>(state.currentAnimation);
    player.m_shootAnimTimer = state.shootAnimationTimer;
  }

  static SaveData::EnemyState captureEnemy(const Enemy &enemy) {
    SaveData::EnemyState state;
    state.character = captureCharacter(enemy);
    state.scoreValue = enemy.m_scoreValue;
    state.theme = static_cast<int>(enemy.m_theme);
    if (const auto *goomba = dynamic_cast<const Goomba *>(&enemy)) {
      state.kind = SaveData::EnemyKind::Goomba;
      state.goombaDeathTimer = goomba->m_deathTimer;
      state.goombaSquished = goomba->m_squished;
    } else if (const auto *koopa = dynamic_cast<const Koopa *>(&enemy)) {
      state.kind = SaveData::EnemyKind::Koopa;
      state.koopaState = static_cast<int>(koopa->m_koopaState);
      state.koopaSliding = koopa->m_sliding;
      state.koopaBrakingAfterWall = koopa->m_brakingAfterWall;
      state.koopaDieTimer = koopa->m_dieTimer;
      state.koopaShellSpeed = koopa->m_shellSpeed;
    } else if (const auto *troopa = dynamic_cast<const Troopa *>(&enemy)) {
      (void)troopa;
      state.kind = SaveData::EnemyKind::Troopa;
    } else if (const auto *bowser = dynamic_cast<const Bowser *>(&enemy)) {
      state.kind = SaveData::EnemyKind::Bowser;
      state.bowserState = static_cast<int>(bowser->m_state);
      state.bowserStateTimer = bowser->m_stateTimer;
      state.bowserBreathFrame = bowser->m_breathFrame;
      state.bowserFireballHits = bowser->m_fireballHits;
      state.bowserPlayerPosition = captureVec(bowser->m_playerPosition);
      state.bowserHasPlayerPosition = bowser->m_hasPlayerPosition;
      state.bowserNextFireTime = bowser->m_nextFireTime;
      state.bowserPendingFireballs = bowser->m_pendingFireballs;
    } else if (const auto *plant = dynamic_cast<const PiranhaPlant *>(&enemy)) {
      state.kind = SaveData::EnemyKind::PiranhaPlant;
      state.piranhaState = static_cast<int>(plant->m_state);
      state.piranhaCurrentFrame = plant->m_currentFrame;
      state.piranhaFrameTimer = plant->m_frameTimer;
      state.piranhaHideTimer = plant->m_hideTimer;
      state.piranhaWaitTimer = plant->m_waitTimer;
      state.piranhaBasePosition = captureVec(plant->m_basePosition);
      state.piranhaBaseCaptured = plant->m_baseCaptured;
    }
    return state;
  }

  static void restoreEnemy(Enemy &enemy, const SaveData::EnemyState &state) {
    restoreCharacter(enemy, state.character);
    enemy.m_scoreValue = state.scoreValue;
    enemy.m_theme = static_cast<LevelTheme>(state.theme);
    if (auto *goomba = dynamic_cast<Goomba *>(&enemy)) {
      goomba->m_deathTimer = state.goombaDeathTimer;
      goomba->m_squished = state.goombaSquished;
    } else if (auto *koopa = dynamic_cast<Koopa *>(&enemy)) {
      koopa->m_koopaState = static_cast<KoopaState>(state.koopaState);
      koopa->m_sliding = state.koopaSliding;
      koopa->m_brakingAfterWall = state.koopaBrakingAfterWall;
      koopa->m_dieTimer = state.koopaDieTimer;
      koopa->m_shellSpeed = state.koopaShellSpeed;
      if (koopa->m_koopaState == KoopaState::Shell) {
        koopa->m_strategy.reset();
      }
    } else if (auto *bowser = dynamic_cast<Bowser *>(&enemy)) {
      bowser->m_state = static_cast<Bowser::State>(state.bowserState);
      bowser->m_stateTimer = state.bowserStateTimer;
      bowser->m_breathFrame = state.bowserBreathFrame;
      bowser->m_fireballHits = state.bowserFireballHits;
      bowser->m_playerPosition = restoreVec(state.bowserPlayerPosition);
      bowser->m_hasPlayerPosition = state.bowserHasPlayerPosition;
      bowser->m_nextFireTime = state.bowserNextFireTime;
      bowser->m_pendingFireballs = state.bowserPendingFireballs;
    } else if (auto *plant = dynamic_cast<PiranhaPlant *>(&enemy)) {
      plant->m_state = static_cast<PiranhaPlant::State>(state.piranhaState);
      plant->m_currentFrame = state.piranhaCurrentFrame;
      plant->m_frameTimer = state.piranhaFrameTimer;
      plant->m_hideTimer = state.piranhaHideTimer;
      plant->m_waitTimer = state.piranhaWaitTimer;
      plant->m_basePosition = restoreVec(state.piranhaBasePosition);
      plant->m_baseCaptured = state.piranhaBaseCaptured;
    }
  }

  static std::unique_ptr<Enemy> makeEnemy(const SaveData::EnemyState &state) {
    EnemyType type = EnemyType::Goomba;
    switch (state.kind) {
    case SaveData::EnemyKind::Goomba: type = EnemyType::Goomba; break;
    case SaveData::EnemyKind::Koopa: type = EnemyType::Koopa; break;
    case SaveData::EnemyKind::Troopa: type = EnemyType::Troopa; break;
    case SaveData::EnemyKind::Bowser: type = EnemyType::Bowser; break;
    case SaveData::EnemyKind::PiranhaPlant: type = EnemyType::PiranhaPlant; break;
    }
    auto enemy = EntityFactory::createEnemy(
        type, restoreVec(state.character.object.position),
        static_cast<LevelTheme>(state.theme));
    if (enemy) restoreEnemy(*enemy, state);
    return enemy;
  }

  static SaveData::ItemState captureItem(const Item &item) {
    SaveData::ItemState state;
    state.object = captureObject(item);
    state.moving = item.m_moving;
    state.collected = item.m_collected;
    state.theme = static_cast<int>(item.m_theme);
    state.animationTimer = item.m_animTimer;
    state.animationFrame = item.m_animFrame;
    switch (item.m_type) {
    case ObjectType::Coin: state.kind = SaveData::ItemKind::Coin; break;
    case ObjectType::Mushroom: state.kind = SaveData::ItemKind::Mushroom; break;
    case ObjectType::FireFlower: state.kind = SaveData::ItemKind::FireFlower; break;
    case ObjectType::Star: state.kind = SaveData::ItemKind::Star; break;
    case ObjectType::FlowersBuff: state.kind = SaveData::ItemKind::FlowersBuff; break;
    default: state.kind = SaveData::ItemKind::Coin; break;
    }
    return state;
  }

  static void restoreItem(Item &item, const SaveData::ItemState &state) {
    restoreObject(item, state.object);
    item.m_moving = state.moving;
    item.m_collected = state.collected;
    item.m_theme = static_cast<LevelTheme>(state.theme);
    item.m_animTimer = state.animationTimer;
    item.m_animFrame = state.animationFrame;
    item.refreshSprite();
  }

  static std::unique_ptr<Item> makeItem(const SaveData::ItemState &state) {
    ItemType type = ItemType::Coin;
    switch (state.kind) {
    case SaveData::ItemKind::Coin: type = ItemType::Coin; break;
    case SaveData::ItemKind::Mushroom: type = ItemType::Mushroom; break;
    case SaveData::ItemKind::FireFlower: type = ItemType::FireFlower; break;
    case SaveData::ItemKind::Star: type = ItemType::Star; break;
    case SaveData::ItemKind::FlowersBuff: type = ItemType::FlowersBuff; break;
    }
    auto item = EntityFactory::createItem(
        type, restoreVec(state.object.position), static_cast<LevelTheme>(state.theme));
    if (item) restoreItem(*item, state);
    return item;
  }

  static SaveData::BlockState captureBlock(const Block &block) {
    SaveData::BlockState state;
    state.object = captureObject(block);
    state.blockType = static_cast<int>(block.m_blockType);
    state.containedItem = static_cast<int>(block.m_containedItem);
    state.used = block.m_used;
    state.theme = static_cast<int>(block.m_theme);
    state.bumpOffset = block.m_bumpOffset;
    state.bumpTimer = block.m_bumpTimer;
    state.bumping = block.m_bumping;
    return state;
  }

  static void restoreBlock(Block &block, const SaveData::BlockState &state) {
    restoreObject(block, state.object);
    block.m_blockType = static_cast<BlockType>(state.blockType);
    block.m_containedItem = static_cast<ObjectType>(state.containedItem);
    block.m_used = state.used;
    block.m_theme = static_cast<LevelTheme>(state.theme);
    block.m_bumpOffset = state.bumpOffset;
    block.m_bumpTimer = state.bumpTimer;
    block.m_bumping = state.bumping;
  }

  static SaveData::FireballState captureFireball(const Fireball &fireball) {
    SaveData::FireballState state;
    state.object = captureObject(fireball);
    state.lifetime = fireball.m_lifetime;
    state.direction = fireball.m_direction;
    state.animationTimer = fireball.m_animTimer;
    state.animationFrame = fireball.m_animFrame;
    state.surfaceHits = fireball.m_surfaceHits;
    return state;
  }

  static void restoreFireball(Fireball &fireball, const SaveData::FireballState &state) {
    restoreObject(fireball, state.object);
    fireball.m_lifetime = state.lifetime;
    fireball.m_direction = state.direction;
    fireball.m_animTimer = state.animationTimer;
    fireball.m_animFrame = state.animationFrame;
    fireball.m_surfaceHits = state.surfaceHits;
  }

  static SaveData::BowserFireballState captureBowserFireball(
      const BowserFireball &fireball) {
    SaveData::BowserFireballState state;
    state.object = captureObject(fireball);
    state.lifetime = fireball.m_lifetime;
    state.animationTimer = fireball.m_animTimer;
    state.animationFrame = fireball.m_animFrame;
    return state;
  }

  static void restoreBowserFireball(BowserFireball &fireball,
                                    const SaveData::BowserFireballState &state) {
    restoreObject(fireball, state.object);
    fireball.m_lifetime = state.lifetime;
    fireball.m_animTimer = state.animationTimer;
    fireball.m_animFrame = state.animationFrame;
  }

  static SaveData::EscalaterState captureEscalater(const Escalater &escalater) {
    SaveData::EscalaterState state;
    state.object = captureObject(escalater);
    state.axis = static_cast<int>(escalater.m_axis);
    state.size = captureVec(escalater.m_size);
    state.renderSize = captureVec(escalater.m_renderSize);
    state.centerX = escalater.m_centerX;
    state.centerY = escalater.m_centerY;
    state.range = escalater.m_range;
    state.speed = escalater.m_speed;
    state.direction = escalater.m_direction;
    state.mapLeft = escalater.m_mapLeft;
    state.mapRight = escalater.m_mapRight;
    state.mapTop = escalater.m_mapTop;
    state.mapBottom = escalater.m_mapBottom;
    return state;
  }

  static void restoreEscalater(Escalater &escalater,
                               const SaveData::EscalaterState &state) {
    restoreObject(escalater, state.object);
    escalater.m_axis = static_cast<Escalater::MovementAxis>(state.axis);
    escalater.m_size = restoreVec(state.size);
    escalater.m_renderSize = restoreVec(state.renderSize);
    escalater.m_centerX = state.centerX;
    escalater.m_centerY = state.centerY;
    escalater.m_range = state.range;
    escalater.m_speed = state.speed;
    escalater.m_direction = state.direction;
    escalater.m_mapLeft = state.mapLeft;
    escalater.m_mapRight = state.mapRight;
    escalater.m_mapTop = state.mapTop;
    escalater.m_mapBottom = state.mapBottom;
  }

  static SaveData::FireBarState captureFireBar(const FireBar &fireBar) {
    SaveData::FireBarState state;
    state.object = captureObject(fireBar);
    state.angle = fireBar.m_angle;
    state.angularSpeed = fireBar.m_angularSpeed;
    state.fireballRotationDegrees = fireBar.m_fireballRotationDegrees;
    state.fireballSpinSpeed = fireBar.m_fireballSpinSpeed;
    state.animationTimer = fireBar.m_animationTimer;
    state.animationFrame = fireBar.m_animationFrame;
    state.segmentCount = fireBar.m_segmentCount;
    state.segmentSpacing = fireBar.m_segmentSpacing;
    return state;
  }

  static void restoreFireBar(FireBar &fireBar, const SaveData::FireBarState &state) {
    restoreObject(fireBar, state.object);
    fireBar.m_angle = state.angle;
    fireBar.m_angularSpeed = state.angularSpeed;
    fireBar.m_fireballRotationDegrees = state.fireballRotationDegrees;
    fireBar.m_fireballSpinSpeed = state.fireballSpinSpeed;
    fireBar.m_animationTimer = state.animationTimer;
    fireBar.m_animationFrame = state.animationFrame;
    fireBar.m_segmentCount = state.segmentCount;
    fireBar.m_segmentSpacing = state.segmentSpacing;
  }

  static SaveData::LavaFireballState captureLavaFireball(
      const LavaFireball &lavaFireball) {
    SaveData::LavaFireballState state;
    state.object = captureObject(lavaFireball);
    state.launchPosition = captureVec(lavaFireball.m_launchPosition);
    state.launchSpeed = lavaFireball.m_launchSpeed;
    state.totalFlightTime = lavaFireball.m_totalFlightTime;
    state.flightTimer = lavaFireball.m_flightTimer;
    state.cooldownTimer = lavaFireball.m_cooldownTimer;
    state.animationFrame = lavaFireball.m_animationFrame;
    state.visible = lavaFireball.m_visible;
    return state;
  }

  static void restoreLavaFireball(LavaFireball &lavaFireball,
                                  const SaveData::LavaFireballState &state) {
    restoreObject(lavaFireball, state.object);
    lavaFireball.m_launchPosition = restoreVec(state.launchPosition);
    lavaFireball.m_launchSpeed = state.launchSpeed;
    lavaFireball.m_totalFlightTime = state.totalFlightTime;
    lavaFireball.m_flightTimer = state.flightTimer;
    lavaFireball.m_cooldownTimer = state.cooldownTimer;
    lavaFireball.m_animationFrame = state.animationFrame;
    lavaFireball.m_visible = state.visible;
  }
};

Level::Level() {}
Level::~Level() {}

bool Level::loadFromFile(const std::string &filename,
                         const std::string &characterName, LevelTheme theme,
                         bool autoPlaceFlagpole) {
  auto data = LevelLoader::loadLevel(filename, theme, autoPlaceFlagpole);
  if (!data.loaded) {
    return false;
  }

  m_tiles = std::move(data.tiles);
  m_tileGrid.build(m_tiles);
  m_blocks = std::move(data.blocks);
  m_enemies = std::move(data.enemies);
  m_items = std::move(data.items);
  m_escalaters = std::move(data.escalaters);
  m_fireBars = std::move(data.fireBars);
  m_lavaFireballs = std::move(data.lavaFireballs);
  m_flagpole = std::move(data.flagpole);
  m_width = data.width;
  m_height = data.height;

  // Set map bounds on escalaters so they reverse at level edges
  for (auto &esc : m_escalaters) {
    esc->setMapBounds(0.0f, m_width, 0.0f, m_height);
  }

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

  m_background.load(theme, m_width, filename == "assets/levels/level2.txt");

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
    if (auto *bowser = dynamic_cast<Bowser *>(enemy.get())) {
      for (int shot = bowser->takePendingFireballs(); shot > 0; --shot) {
        const sf::FloatRect bounds = bowser->getBounds();
        m_bowserFireballs.push_back(std::make_unique<BowserFireball>(
            bounds.left - 20.0f, bounds.top + bounds.height * 0.42f, -1));
      }
    }
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

  // Update tiles (flame animation)
  for (auto &tile : m_tiles) {
    if (tile->isActive())
      tile->update(dt);
  }

  // Update fire bars (rotating hazards)
  for (auto &fb : m_fireBars) {
    if (fb->isActive())
      fb->update(dt);
  }

  // Update lava fireballs (vertical hazards)
  for (auto &lavaFireball : m_lavaFireballs) {
    if (lavaFireball->isActive())
      lavaFireball->update(dt);
  }

  // Escalater vs Tile collisions — reverse direction on contact
  for (auto &esc : m_escalaters) {
    if (!esc->isActive()) continue;
    auto nearTiles = m_tileGrid.query(esc->getBounds());
    for (auto *tile : nearTiles) {
      if (!tile->isActive()) continue;
      auto result = CollisionDetector::checkCollision(*esc, *tile);
      if (result.collided) {
        esc->reverseDirection();
        break;
      }
    }
  }

  // Horizontal `e` platforms turn around when they meet another escalater.
  // Vertical `E` platforms retain their existing independent movement.
  for (size_t i = 0; i < m_escalaters.size(); ++i) {
    auto &first = m_escalaters[i];
    if (!first || !first->isActive()) continue;
    for (size_t j = i + 1; j < m_escalaters.size(); ++j) {
      auto &second = m_escalaters[j];
      if (!second || !second->isActive()) continue;
      if (!CollisionDetector::checkCollision(*first, *second).collided) continue;
      if (first->movesHorizontally()) first->reverseDirection();
      if (second->movesHorizontally()) second->reverseDirection();
    }
  }

  // Update fireballs
  for (auto &fb : m_fireballs) {
    if (fb->isActive())
      fb->update(dt);
  }

  for (auto &fb : m_bowserFireballs) {
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

void Level::render(sf::RenderWindow &window, float cameraCenterX,
                   bool playersBehindTerrain) {
  m_background.render(window, cameraCenterX);

  // During a pipe transition the player must be behind its terrain. Drawing
  // them before the tiles lets the pipe mouth/body conceal the overlapping
  // portion exactly as Mario moves into or out of it.
  if (playersBehindTerrain) {
    if (m_player)
      m_player->draw(window);
    if (m_player2)
      m_player2->draw(window);
  }

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

  for (auto &fb : m_bowserFireballs) {
    if (fb->isActive())
      fb->draw(window);
  }

  // Draw flagpole
  if (m_flagpole)
    m_flagpole->draw(window);

  // Fire bars are drawn above the map. Their fire passes through every map
  // object visually and physically; only a player touching a fire segment is
  // affected.
  for (auto &fb : m_fireBars) {
    if (fb->isActive())
      fb->draw(window);
  }

  for (auto &lavaFireball : m_lavaFireballs) {
    if (lavaFireball->isActive())
      lavaFireball->draw(window);
  }

  // Draw players on top during normal play. Pipe transitions already drew
  // them below terrain above so the pipe remains visually in front.
  if (!playersBehindTerrain) {
    if (m_player)
      m_player->draw(window);
    if (m_player2)
      m_player2->draw(window);
  }
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

std::optional<sf::FloatRect>
Level::getHorizontalPipeEntranceBounds(const Player &player) const {
  const sf::FloatRect playerBounds = player.getBounds();

  auto hasWardPiece = [&](float x, float y) {
    for (const auto &tile : m_tiles) {
      if (tile->getTileType() != TileType::WardPipePiece)
        continue;
      const sf::FloatRect bounds = tile->getBounds();
      if (std::abs(bounds.left - x) < 0.5f &&
          std::abs(bounds.top - y) < 0.5f) {
        return true;
      }
    }
    return false;
  };

  for (const auto &tile : m_tiles) {
    if (tile->getTileType() != TileType::WardPipePiece)
      continue;

    const sf::FloatRect topLeft = tile->getBounds();
    const float x = topLeft.left;
    const float y = topLeft.top;

    // A horizontal pipe is authored as a 3x2 WardPipe_piece rectangle.
    if (!hasWardPiece(x + TILE_SIZE, y) ||
        !hasWardPiece(x + TILE_SIZE * 2.0f, y) ||
        !hasWardPiece(x, y + TILE_SIZE) ||
        !hasWardPiece(x + TILE_SIZE, y + TILE_SIZE) ||
        !hasWardPiece(x + TILE_SIZE * 2.0f, y + TILE_SIZE)) {
      continue;
    }

    // Give the entrance a small lip on the left so the player can activate it
    // while standing against the solid first pipe tile.
    const sf::FloatRect entrance(x - 8.0f, y, TILE_SIZE * 3.0f + 8.0f,
                                 TILE_SIZE * 2.0f);
    if (playerBounds.intersects(entrance)) {
      return entrance;
    }
  }

  return std::nullopt;
}

std::optional<sf::FloatRect> Level::getPipeBoundsAtColumn(int column) const {
  const float expectedX = static_cast<float>(column) * TILE_SIZE;

  auto hasPipePart = [&](float x, float y, TileType type) {
    for (const auto &tile : m_tiles) {
      if (tile->getTileType() != type)
        continue;
      const sf::FloatRect bounds = tile->getBounds();
      if (std::abs(bounds.left - x) < 0.5f &&
          std::abs(bounds.top - y) < 0.5f) {
        return true;
      }
    }
    return false;
  };

  for (const auto &tile : m_tiles) {
    if (tile->getTileType() != TileType::PipeTopLeft)
      continue;

    const sf::FloatRect topLeft = tile->getBounds();
    if (std::abs(topLeft.left - expectedX) >= 0.5f)
      continue;

    const float x = topLeft.left;
    const float y = topLeft.top;
    if (hasPipePart(x + TILE_SIZE, y, TileType::PipeTopRight) &&
        hasPipePart(x, y + TILE_SIZE, TileType::PipeBodyLeft) &&
        hasPipePart(x + TILE_SIZE, y + TILE_SIZE, TileType::PipeBodyRight)) {
      return sf::FloatRect(x, y, TILE_SIZE * 2.0f, TILE_SIZE * 2.0f);
    }
  }

  return std::nullopt;
}

std::optional<sf::Vector2f> Level::getCastleDoorEntryPosition() const {
  for (const auto &tile : m_tiles) {
    // `5` is the lower-right castle-door cell. The player's position anchor
    // is centred on it, while their feet stay on the ground beneath it.
    if (tile->getTileType() == TileType::CastlePiece &&
        tile->getSubIndex() == 7) {
      const sf::FloatRect door = tile->getBounds();
      return sf::Vector2f(door.left, door.top - TILE_SIZE);
    }
  }
  return std::nullopt;
}

bool Level::isComplete() const { return m_flagpole && m_flagpole->isReached(); }

SaveData::LevelState Level::captureSnapshot() const {
  return SnapshotAccess::capture(*this);
}

bool Level::restoreSnapshot(const SaveData::LevelState &snapshot) {
  return SnapshotAccess::restore(*this, snapshot);
}

void Level::handlePlayerCollisions(Player* player, float dt) {
  if (!player || player->isDead()) return;

  player->setGrounded(false);
  bool touchingVine = false;
  float vineX = 0.0f;

  const auto nearbyTiles = m_tileGrid.query(player->getBounds());

  // Detect the vine before resolving terrain. This lets a vine take priority
  // over a neighbouring ceiling/floor tile in the same frame.
  for (Tile *tile : nearbyTiles) {
    if (tile->getTileType() != TileType::VineTop) continue;
    if (CollisionDetector::checkCollision(*player, *tile).collided) {
      touchingVine = true;
      vineX = tile->getBounds().left;
      break;
    }
  }
  player->updateVineContact(touchingVine, vineX);

  // Player vs Tiles
  for (Tile *tile : nearbyTiles) {
    auto result = CollisionDetector::checkCollision(*player, *tile);
    if (result.collided) {
      // Lava and flame are lethal hazards — kill on contact, no physics resolve.
      TileType tt = tile->getTileType();
      if (tt == TileType::Lava || tt == TileType::Flame) {
        player->die();
        return;
      }
      // Vines are non-solid interaction tiles, handled in the pre-pass.
      if (tt == TileType::VineTop) {
        continue;
      }
      CollisionDetector::resolveCollision(*player, *tile, result);
      if (result.side == CollisionDetector::Side::Bottom) {
        player->setGrounded(true);
      }
    }
  }
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
    // Vines are always non-solid. Every other tile stays solid, including
    // while climbing, so S/X/blocks stop upward or downward movement.
    if (tile->getTileType() == TileType::VineTop) {
      continue;
    }
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
        // Carry a standing player with either direction of platform motion.
        player->setVelocity(player->getVelocity().x, esc->getVelocity().y);
        if (esc->movesHorizontally()) {
          player->setPosition(player->getPosition().x +
                                  esc->getVelocity().x * dt,
                              player->getPosition().y);
        }
      }
    }
  }

  // Player vs FireBars (lethal rotating hazard)
  for (auto &fb : m_fireBars) {
    if (!fb->isActive()) continue;

    for (int i = 0; i < fb->getSegmentCount(); ++i) {
      sf::FloatRect segBounds = fb->getSegmentBounds(i);
      if (player->getBounds().intersects(segBounds)) {
        player->die();
        return;
      }
    }
  }

  // Lava fireballs pass through level geometry and kill only on player
  // contact while their launch/fall animation is visible.
  for (auto &lavaFireball : m_lavaFireballs) {
    if (lavaFireball->isActive() && lavaFireball->isVisible() &&
        player->getBounds().intersects(lavaFireball->getBounds())) {
      player->die();
      return;
    }
  }

  // Bowser's fire disappears after a solid hit and is lethal on player
  // contact. This is checked before enemies so it cannot be stomped or kicked.
  for (auto &fb : m_bowserFireballs) {
    if (fb->isActive() && player->getBounds().intersects(fb->getBounds())) {
      fb->setActive(false);
      player->die();
      return;
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
      // onStomped() may only change state (Koopa -> Shell) or do nothing
      // (Bowser, which cannot be defeated by contact). Only a real kill
      // should report a defeat and award points.
      if (enemy->isDead()) {
        publishEnemyDefeated(*enemy, scorePosition);
      }
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
          if (dynamic_cast<Bowser *>(enemy.get())) {
            EventManager::getInstance().publish({EventType::EnemyHitByFireball});
          }
          if (enemy->hitByFireball()) {
            publishEnemyDefeated(*enemy, scorePosition);
          }
        }
        fb->setActive(false);
        break;
      }
    }
  }

  // Bowser fireballs do not bounce: contact with any tile or block removes
  // them immediately.
  for (auto &fb : m_bowserFireballs) {
    if (!fb->isActive())
      continue;

    bool hitSolid = false;
    for (Tile *tile : m_tileGrid.query(fb->getBounds())) {
      if (CollisionDetector::checkCollision(*fb, *tile).collided) {
        hitSolid = true;
        break;
      }
    }
    if (!hitSolid) {
      for (auto &block : m_blocks) {
        if (block->isActive() &&
            CollisionDetector::checkCollision(*fb, *block).collided) {
          hitSolid = true;
          break;
        }
      }
    }
    if (hitSolid)
      fb->setActive(false);
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

  m_bowserFireballs.erase(
      std::remove_if(m_bowserFireballs.begin(), m_bowserFireballs.end(),
                     [](const auto &f) { return !f->isActive(); }),
      m_bowserFireballs.end());

  m_blocks.erase(std::remove_if(m_blocks.begin(), m_blocks.end(),
                                [](const auto &b) { return !b->isActive(); }),
                 m_blocks.end());
}
