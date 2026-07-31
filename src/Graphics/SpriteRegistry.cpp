#include "Graphics/SpriteRegistry.hpp"
#include "Core/AssetManager.hpp"
#include "Entities/Block.hpp"
#include "Entities/Player.hpp"
#include "Entities/Tile.hpp"

// ─────────────────────────────────────────────────────────────────────────
// Every path below points at a single-sprite file under assets/textures/,
// sourced from the Mario Wiki / Wikipedia. Where a theme or animation frame
// has no working dedicated asset, it deliberately falls back to the closest
// available art (documented inline) rather than being left broken — see
// asset-triage.md at the repo root for the full survey this was built from.
// ─────────────────────────────────────────────────────────────────────────

namespace {
constexpr int THEME_COUNT = 3; // Overworld, Underground, Castle
int themeIndex(LevelTheme theme) { return static_cast<int>(theme); }

const std::string &groundPath(LevelTheme theme) {
  static const std::string paths[THEME_COUNT] = {
      "assets/textures/SMB_Ground.png",             // Overworld
      "assets/textures/SMB_Ground_Underground.png", // Underground
      "assets/textures/SMB_Ground_Castle.png",       // Castle
  };
  return paths[themeIndex(theme)];
}

const std::string &pipePath(LevelTheme theme) {
  // Each theme's pipe is now one assembled image rather than 4 sliced
  // quarter-tiles, so all 4 TileType pipe positions reuse it (see tilePath).
  static const std::string paths[THEME_COUNT] = {
      "assets/textures/Warp_Pipe_SMB.png",        // Overworld
      "assets/textures/Warp_Pipe_Gray_SMB.png",   // Underground
      "assets/textures/Warp_Pipe_Orange_SMB.png", // Castle
  };
  return paths[themeIndex(theme)];
}

const std::string &brickPath(LevelTheme theme) {
  static const std::string paths[THEME_COUNT] = {
      "assets/textures/SMB_Brick_Block_Sprite.png",
      "assets/textures/SMB_Underground_Brick_Block.png",
      "assets/textures/SMB_Castle_Brick_Block.png",
  };
  return paths[themeIndex(theme)];
}

const std::string &questionIdlePath(LevelTheme theme) {
  static const std::string paths[THEME_COUNT] = {
      "assets/textures/SMB_Qblock.png",
      "assets/textures/SMB_Underground_Question_Block.png",
      "assets/textures/SMB_Castle_Question_Block.png",
  };
  return paths[themeIndex(theme)];
}
} // namespace

const std::string &SpriteRegistry::tilePath(TileType type, LevelTheme theme) {
  switch (type) {
  case TileType::PipeTopLeft:
  case TileType::PipeTopRight:
  case TileType::PipeBodyLeft:
  case TileType::PipeBodyRight:
    return pipePath(theme);
  case TileType::Ground:
  case TileType::Underground:
  case TileType::CastleBlock:
  case TileType::Brick:
  default:
    return groundPath(theme);
  }
}

const std::string &SpriteRegistry::blockPath(BlockType type, LevelTheme theme,
                                              BlockVisualState state) {
  if (type == BlockType::Brick) {
    // Bricks either break entirely or just bump without visibly changing —
    // state doesn't affect which file is shown.
    return brickPath(theme);
  }

  if (state == BlockVisualState::Used) {
    // No per-theme "emptied block" art in the pack — one generic used-block
    // sprite covers all themes.
    static const std::string used = "assets/textures/SMB1_Empty_Block.png";
    return used;
  }

  // Idle and Hit share the same art — there's no separate "flash" frame
  // asset, and Hit is only shown for a fraction of a second during the bump.
  return questionIdlePath(theme);
}

namespace {
// No dedicated Castle Goomba/Koopa art survived (broken WebP or missing
// entirely) — reuse the Overworld variant there, a deliberate call, not a
// gap. Underground Goomba does have its own grey recolor.
LevelTheme normalizeGoombaTheme(LevelTheme theme) {
  return theme == LevelTheme::Underground ? LevelTheme::Underground
                                           : LevelTheme::Overworld;
}
} // namespace

const std::string &SpriteRegistry::goombaPath(LevelTheme theme, int walkFrame) {
  if (normalizeGoombaTheme(theme) == LevelTheme::Underground) {
    // Only one usable Underground Goomba file exists — both walk frames
    // point at it, so the walk cycle is a static pose rather than missing.
    static const std::string grey = "assets/textures/GoombaSMBGrey.gif";
    return grey;
  }
  static const std::string frames[2] = {
      "assets/textures/Goomba_SMB.png",
      "assets/textures/SMB_Goomba_Sprite.gif",
  };
  return frames[walkFrame % 2];
}

const std::string &SpriteRegistry::goombaSquishPath(LevelTheme theme) {
  static const std::string overworld = "assets/textures/SMB_Dead_Goomba.png";
  static const std::string underground =
      "assets/textures/SMB_Goomba_Underground_Dead.png";
  return normalizeGoombaTheme(theme) == LevelTheme::Underground ? underground
                                                                 : overworld;
}

const std::string &SpriteRegistry::koopaWalkPath(LevelTheme /*theme*/,
                                                   int /*walkFrame*/) {
  // Underground/Castle Koopa art is broken WebP or missing — every theme
  // reuses the Overworld Koopa (deliberate, see goombaPath note above). Only
  // one usable file exists, so the walk cycle is a static pose.
  static const std::string walking = "assets/textures/Green_Koopa_Troopa_SMB.gif";
  return walking;
}

const std::string &SpriteRegistry::koopaShellPath(LevelTheme /*theme*/,
                                                    bool /*spinning*/) {
  static const std::string shell = "assets/textures/SMB_Greenshell.png";
  return shell;
}

const std::string &SpriteRegistry::piranhaPlantPath(int frame) {
  static const std::string frames[2] = {
      "assets/textures/Piranha_Plant_Underwater.png",
      "assets/textures/SMB_PI~1.GIF",
  };
  return frames[frame % 2];
}

namespace {
// Wherever Luigi-specific art is missing or broken, fall back to the
// equivalent Mario pose — no usable Luigi source art exists for these
// (confirmed during asset triage; see asset-triage.md).
const std::string &marioSmallIdle() {
  static const std::string p = "assets/textures/SMB_Smallmario.png";
  return p;
}
const std::string &marioSmallJump() {
  static const std::string p = "assets/textures/SMB_Small_Mario_Jumping_Sprite.png";
  return p;
}
const std::string &marioBigIdle() {
  static const std::string p = "assets/textures/SMB_Super_Mario_Sprite.png";
  return p;
}
const std::string &marioBigJump() {
  static const std::string p = "assets/textures/SMB_Super_Mario_Jumping.png";
  return p;
}
const std::string &marioFireIdle() {
  static const std::string p = "assets/textures/SMB_Fire_Mario_Sprite.png";
  return p;
}
const std::string &marioFireJump() {
  static const std::string p = "assets/textures/SMB_Fire_Mario_Jumping.png";
  return p;
}
} // namespace

const std::string &SpriteRegistry::playerPath(CharacterId character,
                                               PowerUpState power,
                                               PlayerAnim anim, int frame) {
  bool luigi = (character == CharacterId::Luigi);

  // Walk needs a small per-combo frame list; everything else is one pose.
  if (anim == PlayerAnim::Walk) {
    if (power == PowerUpState::Small) {
      if (luigi) {
        static const std::string f = "assets/textures/Small_Luigi_Walk_SMB.gif";
        return f; // only one usable Luigi small-walk file
      }
      static const std::string frames[2] = {
          "assets/textures/SMB_Mario_walking_sprite.png",
          "assets/textures/SMB_NES_Mario_Walking_Sprite.gif",
      };
      return frames[frame % 2];
    }
    if (power == PowerUpState::Big) {
      if (luigi) {
        static const std::string f = "assets/textures/SMB_NES_Luigi_Running.gif";
        return f; // only one usable Luigi big-walk file
      }
      static const std::string frames[2] = {
          "assets/textures/SMB_Mario_walking_sprite.png",
          "assets/textures/SMB_Super_Mario_Sprite.png",
      };
      return frames[frame % 2];
    }
    // Fire — Luigi has no Fire art at all, falls back to Fire Mario.
    static const std::string frames[2] = {
        "assets/textures/SMB_Fire_Mario_Walking.gif",
        "assets/textures/SMB_Fire_Mario_Sprite.png",
    };
    return frames[frame % 2];
  }

  if (anim == PlayerAnim::Jump) {
    if (power == PowerUpState::Small) {
      if (luigi) {
        static const std::string p = "assets/textures/SMB_Small_Luigi_Jumping.png";
        return p;
      }
      return marioSmallJump();
    }
    if (power == PowerUpState::Big) {
      if (luigi) {
        static const std::string p = "assets/textures/SMB_Super_Luigi_Jumping.png";
        return p;
      }
      return marioBigJump();
    }
    return marioFireJump(); // no Fire Luigi art — fall back to Fire Mario
  }

  // Idle and Skid: no dedicated skid pose in the pack, so Skid falls back
  // to Idle for every character/power combo (easy to replace once skid art
  // is sourced — this is the only place that decision needs to change).
  if (power == PowerUpState::Small) {
    // Luigi's small idle files are all broken WebP — fall back to Mario.
    return marioSmallIdle();
  }
  if (power == PowerUpState::Big) {
    // Luigi's big idle files are all broken WebP — fall back to Mario.
    return marioBigIdle();
  }
  return marioFireIdle(); // no Fire Luigi art — fall back to Fire Mario
}

int SpriteRegistry::playerFrameCount(CharacterId character, PowerUpState power,
                                      PlayerAnim anim) {
  if (anim != PlayerAnim::Walk) {
    return 1;
  }
  bool luigi = (character == CharacterId::Luigi);
  if (luigi && power != PowerUpState::Fire) {
    return 1; // Luigi small/big walk only has one usable file each
  }
  return 2;
}

const std::string &SpriteRegistry::coinPath(LevelTheme theme, int /*frame*/) {
  // 5-frame shimmer GIFs exist per theme, but SFML's loader only ever reads
  // frame 0 of an animated GIF — the shimmer is a static pose per theme.
  static const std::string paths[THEME_COUNT] = {
      "assets/textures/SMBCoin.gif",
      "assets/textures/SMB_CoinUnderground.gif",
      "assets/textures/SMB_CoinCastle.gif",
  };
  return paths[themeIndex(theme)];
}

int SpriteRegistry::coinFrameCount() { return 1; }

const std::string &SpriteRegistry::mushroomPath(LevelTheme /*theme*/) {
  // No per-theme mushroom recolor in the pack — reused across themes, same
  // as before.
  static const std::string p = "assets/textures/SMB_Supermushroom.png";
  return p;
}

const std::string &SpriteRegistry::fireFlowerPath(LevelTheme /*theme*/) {
  static const std::string p = "assets/textures/Fire_Flower_SMB.gif";
  return p;
}

const std::string &SpriteRegistry::starPath(LevelTheme /*theme*/, int /*frame*/) {
  static const std::string p = "assets/textures/Starman.gif";
  return p;
}

int SpriteRegistry::starFrameCount() { return 1; }

const std::string &SpriteRegistry::fireballPath(int /*frame*/) {
  static const std::string p = "assets/textures/SMBFireBall.gif";
  return p;
}

int SpriteRegistry::fireballFrameCount() { return 1; }

void SpriteRegistry::applyFrame(sf::Sprite &sprite, const std::string &path,
                                 const sf::FloatRect &box, bool flip) {
  sf::Texture &texture = AssetManager::getInstance().getTexture(path);
  sf::Vector2u size = texture.getSize();
  applyFrame(sprite, texture, sf::IntRect(0, 0, static_cast<int>(size.x),
                                           static_cast<int>(size.y)),
             box, flip);
}

void SpriteRegistry::applyFrame(sf::Sprite &sprite, sf::Texture &texture,
                                 const sf::IntRect &cropRect,
                                 const sf::FloatRect &box, bool flip) {
  sprite.setTexture(texture);
  sprite.setTextureRect(cropRect);

  float scale = 1.0f;
  if (cropRect.height != 0) {
    scale = box.height / static_cast<float>(cropRect.height);
  }

  sprite.setOrigin(cropRect.width / 2.0f, static_cast<float>(cropRect.height));
  sprite.setScale(flip ? -scale : scale, scale);
  sprite.setPosition(box.left + box.width / 2.0f, box.top + box.height);
}
