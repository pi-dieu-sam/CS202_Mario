#include "Graphics/SpriteRegistry.hpp"
#include "Core/AssetManager.hpp"
#include "Entities/Block.hpp"
#include "Entities/Player.hpp"
#include "Entities/Tile.hpp"
#include <iostream>

// ─────────────────────────────────────────────────────────────────────────
// Every ordinary path below points at a single-sprite file under
// assets/textures/, sourced from the Mario Wiki / Wikipedia. The flag-pole
// climb poses are the deliberate exception: they are cropped from the shared
// playable-character sheet because that is where both Mario and Luigi poses
// are available. Where a theme or animation frame has no working dedicated
// asset, it deliberately falls back to the closest available art (documented
// inline) rather than being left broken — see asset-triage.md at the repo root
// for the full survey this was built from.
// ─────────────────────────────────────────────────────────────────────────

namespace {
constexpr int THEME_COUNT = 3; // Overworld, Underground, Castle
constexpr char PLAYER_FLAGPOLE_SHEET_PATH[] =
    "assets/textures/NES - Super Mario Bros. - Playable Characters - Mario & Luigi.png";
const sf::Color PLAYER_SHEET_CELL_BACKGROUND(146, 144, 255);

/// The character sheet is an annotated reference image rather than a normal
/// transparent sprite atlas.  Its climb cells are surrounded by this exact
/// lavender colour, so make only that colour transparent once before using
/// the known frame rectangles below.
sf::Texture &flagpoleSlideSheetTexture() {
  static sf::Texture texture;
  static bool attemptedLoad = false;
  if (attemptedLoad) {
    return texture;
  }
  attemptedLoad = true;

  sf::Image image;
  if (!image.loadFromFile(PLAYER_FLAGPOLE_SHEET_PATH)) {
    std::cerr << "[SpriteRegistry] Failed to load flagpole player sheet: "
              << PLAYER_FLAGPOLE_SHEET_PATH << "\n";
    return texture;
  }

  image.createMaskFromColor(PLAYER_SHEET_CELL_BACKGROUND);
  if (!texture.loadFromImage(image)) {
    std::cerr << "[SpriteRegistry] Failed to create flagpole player texture: "
              << PLAYER_FLAGPOLE_SHEET_PATH << "\n";
  }
  return texture;
}

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

const std::string &forkedPipePath() {
  // Single 64x64 fork/warp-pipe sprite shared across all themes.
  static const std::string p = "assets/textures/SMBWarpPipeForked.png";
  return p;
}

const std::string &castlePiecePath() {
  // 4x2 sheet of 16x16 castle tiles with 1px gaps. Each cell is cropped by
  // Tile (CastlePiece) and scaled to one 32x32 tile.
  static const std::string p = "assets/textures/items/Castle_piece.png";
  return p;
}
} // namespace

const std::string &SpriteRegistry::tilePath(TileType type, LevelTheme theme) {
  switch (type) {
  case TileType::PipeTopLeft:
  case TileType::PipeTopRight:
  case TileType::PipeBodyLeft:
  case TileType::PipeBodyRight:
    return pipePath(theme);
  case TileType::ForkedPipeHead:
  case TileType::ForkedPipeBase:
    return forkedPipePath();
  case TileType::CastlePiece:
    return castlePiecePath();
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

  if (anim == PlayerAnim::FlagpoleSlide) {
    // The caller must use applyPlayerFlagpoleSlideFrame() so only the
    // selected source cell is shown. Returning the real sheet path here keeps
    // this logical animation queryable like every other PlayerAnim.
    return playerFlagpoleSlideSheetPath();
  }

  // Walk needs a small per-combo frame list; everything else is one pose.
  if (anim == PlayerAnim::Walk) {
    if (power == PowerUpState::Small) {
      if (luigi) {
        static const std::string frames[2] = {
            "assets/textures/SMB_Luigi_Walking_0.png",
            "assets/textures/SMB_Luigi_Walking_1.png",
        };
        return frames[frame % 2];
      }
      static const std::string frames[2] = {
          "assets/textures/SMB_Mario_walking_sprite.png",
          "assets/textures/SMB_NES_Mario_Walking_Sprite.gif",
      };
      return frames[frame % 2];
    }
    if (power == PowerUpState::Big) {
      if (luigi) {
        static const std::string frames[2] = {
            "assets/textures/SMB_Luigi_Walking_0.png",
            "assets/textures/SMB_Luigi_Walking_1.png",
        };
        return frames[frame % 2];
      }
      static const std::string frames[2] = {
          "assets/textures/SMB_Mario_walking_sprite.png",
          "assets/textures/SMB_Super_Mario_Sprite.png",
      };
      return frames[frame % 2];
    }
    if (luigi) {
      static const std::string frames[2] = {
          "assets/textures/SMB_Fire_Luigi_Walking_0.png",
          "assets/textures/SMB_Fire_Luigi_Walking_1.png",
      };
      return frames[frame % 2];
    }
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
    if (luigi) {
      static const std::string p = "assets/textures/SMB_Fire_Luigi_Jumping.png";
      return p;
    }
    return marioFireJump();
  }

  // Idle and Skid: no dedicated skid pose in the pack, so Skid falls back
  // to Idle for every character/power combo (easy to replace once skid art
  // is sourced — this is the only place that decision needs to change).
  if (power == PowerUpState::Small) {
    if (luigi) {
      static const std::string p = "assets/textures/SMB_Small_Luigi_Idle.png";
      return p;
    }
    return marioSmallIdle();
  }
  if (power == PowerUpState::Big) {
    if (luigi) {
      static const std::string p = "assets/textures/SMB_Super_Luigi_Idle.png";
      return p;
    }
    return marioBigIdle();
  }
  if (luigi) {
    static const std::string p = "assets/textures/SMB_Fire_Luigi_Idle.png";
    return p;
  }
  return marioFireIdle();
}

const std::string &SpriteRegistry::playerDeathPath(CharacterId character) {
  static const std::string mario =
      "assets/textures/SMB_Mario_Death_Sprite.png";
  static const std::string luigi =
      "assets/textures/SMB_Luigi_Death_Sprite.png";
  return character == CharacterId::Luigi ? luigi : mario;
}

int SpriteRegistry::playerFrameCount(CharacterId character, PowerUpState power,
                                      PlayerAnim anim) {
  if (anim == PlayerAnim::FlagpoleSlide) {
    return playerFlagpoleSlideFrameCount(character, power);
  }
  if (anim != PlayerAnim::Walk) {
    return 1;
  }
  return 2;
}

const std::string &SpriteRegistry::playerFlagpoleSlideSheetPath() {
  static const std::string path = PLAYER_FLAGPOLE_SHEET_PATH;
  return path;
}

int SpriteRegistry::playerFlagpoleSlideFrameCount(CharacterId /*character*/,
                                                    PowerUpState /*power*/) {
  return 2;
}

sf::IntRect SpriteRegistry::playerFlagpoleSlideRect(CharacterId character,
                                                     PowerUpState power,
                                                     int frame) {
  // These two cells are the alternating pole-climb poses in the shared NES
  // character sheet. The Luigi half begins exactly 288 pixels to the right.
  constexpr int marioFrameX[2] = {136, 154};
  constexpr int luigiOffsetX = 288;
  constexpr int smallFrameY = 8;
  constexpr int tallFrameY = 31;
  constexpr int frameWidth = 16;
  constexpr int smallFrameHeight = 16;
  constexpr int tallFrameHeight = 32;

  const int normalizedFrame =
      frame % playerFlagpoleSlideFrameCount(character, power);
  const int x = marioFrameX[normalizedFrame] +
                (character == CharacterId::Luigi ? luigiOffsetX : 0);
  const bool small = power == PowerUpState::Small;
  return sf::IntRect(x, small ? smallFrameY : tallFrameY, frameWidth,
                     small ? smallFrameHeight : tallFrameHeight);
}

void SpriteRegistry::applyPlayerFlagpoleSlideFrame(
    sf::Sprite &sprite, CharacterId character, PowerUpState power, int frame,
    const sf::FloatRect &box, bool flip) {
  sf::Texture &texture = flagpoleSlideSheetTexture();
  if (texture.getSize().x == 0 || texture.getSize().y == 0) {
    return;
  }
  applyFrame(sprite, texture, playerFlagpoleSlideRect(character, power, frame),
             box, flip);
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

int SpriteRegistry::coinFrameCount() {
  // Return the actual decoded GIF frame count so the entity can cycle.
  // All three theme coin GIFs have the same number of frames — use Overworld.
  return AssetManager::getInstance().getGifFrameCount("assets/textures/SMBCoin.gif");
}

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

int SpriteRegistry::starFrameCount() {
  return AssetManager::getInstance().getGifFrameCount("assets/textures/Starman.gif");
}

const std::string &SpriteRegistry::fireballPath(int /*frame*/) {
  static const std::string p = "assets/textures/SMBFireBall.gif";
  return p;
}

int SpriteRegistry::fireballFrameCount() {
  return AssetManager::getInstance().getGifFrameCount("assets/textures/SMBFireBall.gif");
}

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

void SpriteRegistry::applyGifFrame(sf::Sprite &sprite,
                                    const std::string &gifPath, int frame,
                                    const sf::FloatRect &box, bool flip) {
  const auto &frames = AssetManager::getInstance().getGifFrames(gifPath);
  if (frames.empty()) return;

  // Clamp frame index defensively (frame count can change on first load).
  const sf::Texture &tex = frames[static_cast<size_t>(frame) % frames.size()];
  sf::Vector2u size = tex.getSize();

  // Cast away const to pass to setTexture — sf::Sprite takes non-const ref
  // but never modifies the texture; this is an SFML API limitation.
  applyFrame(sprite, const_cast<sf::Texture &>(tex),
             sf::IntRect(0, 0, static_cast<int>(size.x),
                         static_cast<int>(size.y)),
             box, flip);
}
