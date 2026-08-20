#include "Graphics/SpriteRegistry.hpp"
#include "Core/AssetManager.hpp"
#include "Entities/Block.hpp"
#include "Entities/Player.hpp"
#include "Entities/Tile.hpp"
#include <algorithm>
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
      "assets/textures/block_lava.png",              // Underground
      "assets/textures/SMB_Ground_Castle.png",       // Castle
  };
  return paths[themeIndex(theme)];
}

const std::string &pipePath(LevelTheme theme) {
  static const std::string paths[THEME_COUNT] = {
      "assets/textures/Warp_Pipe_SMB.png",          // Overworld
      "assets/textures/Warp_Pipe_Gray_SMB.png",     // Underground
      "assets/textures/Warp_Pipe_Orange_SMB.png",   // Castle
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

const std::string &castlePiecePath() {
  // 4x2 sheet of 16x16 castle tiles with 1px gaps. Each cell is cropped by
  // Tile (CastlePiece) and scaled to one 32x32 tile.
  static const std::string p = "assets/textures/items/Castle_piece.png";
  return p;
}

const std::string &wardPipePiecePath() {
  // 3x2 sheet of 16x16 ward-pipe tiles with 1px gaps. Each cell is cropped by
  // Tile (WardPipePiece) and scaled to one 32x32 tile.
  static const std::string p = "assets/textures/items/WardPipe_piece.png";
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
  case TileType::CastlePiece:
    return castlePiecePath();
  case TileType::WardPipePiece:
    return wardPipePiecePath();
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

const std::string &SpriteRegistry::goombaPath(LevelTheme /*theme*/, int /*walkFrame*/) {
  static const std::string sheet = "assets/textures/Character/Goomba.png";
  return sheet;
}

const std::string &SpriteRegistry::goombaSquishPath(LevelTheme /*theme*/) {
  static const std::string die = "assets/textures/Character/Goomba_Die.png";
  return die;
}

int SpriteRegistry::goombaFrameCount() {
  return 16;
}

void SpriteRegistry::applyGoombaFrame(sf::Sprite &sprite, int frame,
                                      const sf::FloatRect &box, bool flip) {
  static const std::string path = "assets/textures/Character/Goomba.png";
  sf::Texture &texture = AssetManager::getInstance().getTexture(path);
  sf::Vector2u size = texture.getSize();
  if (size.x == 0 || size.y == 0) return;

  constexpr int cols = 8;
  constexpr int rows = 2;
  int cellW = static_cast<int>(size.x) / cols;
  int cellH = static_cast<int>(size.y) / rows;

  int f = frame % (cols * rows);
  int col = f % cols;
  int row = f / cols;

  applyFrame(sprite, texture,
             sf::IntRect(col * cellW, row * cellH, cellW, cellH), box, flip);
}

const std::string &SpriteRegistry::koopaWalkPath(LevelTheme /*theme*/,
                                                   int /*walkFrame*/) {
  static const std::string sheet = "assets/textures/Character/Koopa.png";
  return sheet;
}

const std::string &SpriteRegistry::koopaShellPath(LevelTheme /*theme*/,
                                                    bool /*spinning*/) {
  static const std::string shell = "assets/textures/SMB_Greenshell.png";
  return shell;
}

const std::string &SpriteRegistry::koopaDiePath() {
  static const std::string die = "assets/textures/Character/Koopa_Die.png";
  return die;
}

int SpriteRegistry::koopaFrameCount() {
  return 20;
}

void SpriteRegistry::applyKoopaFrame(sf::Sprite &sprite, int frame,
                                     const sf::FloatRect &box, bool flip) {
  static const std::string path = "assets/textures/Character/Koopa.png";
  sf::Texture &texture = AssetManager::getInstance().getTexture(path);
  sf::Vector2u size = texture.getSize();
  if (size.x == 0 || size.y == 0) return;

  constexpr int cols = 5;
  constexpr int rows = 4;
  int cellW = static_cast<int>(size.x) / cols;
  int cellH = static_cast<int>(size.y) / rows;

  int f = frame % (cols * rows);
  int col = f % cols;
  int row = f / cols;

  applyFrame(sprite, texture,
             sf::IntRect(col * cellW, row * cellH, cellW, cellH), box, flip);
}

const std::string &SpriteRegistry::piranhaPlantPath(int /*frame*/) {
  // This GIF contains the two classic, fixed-size mouth poses. Vertical
  // emergence is controlled by PiranhaPlant itself; the old 95-frame strip
  // also encoded rising/falling and therefore fought the entity movement.
  static const std::string gif = "assets/textures/SMB_PI~1.GIF";
  return gif;
}

int SpriteRegistry::piranhaFrameCount() {
  return AssetManager::getInstance().getGifFrameCount(piranhaPlantPath(0));
}

sf::IntRect SpriteRegistry::piranhaFrameRect(int frame) {
  const auto &frames =
      AssetManager::getInstance().getGifFrames(piranhaPlantPath(frame));
  if (frames.empty()) return sf::IntRect();

  const sf::Texture &texture =
      frames[static_cast<size_t>(std::max(frame, 0)) % frames.size()];
  const sf::Vector2u size = texture.getSize();
  return sf::IntRect(0, 0, static_cast<int>(size.x),
                     static_cast<int>(size.y));
}

void SpriteRegistry::applyPiranhaFrame(sf::Sprite &sprite, int frame,
                                       const sf::FloatRect &box, bool flip) {
  applyGifFrame(sprite, piranhaPlantPath(frame), frame, box, flip);
}

namespace {
// ── New Mario art ──────────────────────────────────────────────────────
// Mario's new look lives in assets/textures/Character/ as horizontal sheets.
// Each sheet is a fixed grid of frame cells separated by transparent gaps
// (only the art inside a cell is unevenly wide), so every frame is picked
// with an sf::IntRect via applySheetFrame(): cell x = frame * frameWidth.
// Stand/Walk/Jump use 32px cells, Fire uses 48px cells.
struct MarioSheet {
  const std::string path;
  int frameWidth;
  int frameCount;
};
const MarioSheet &marioSheet(SpriteRegistry::PlayerAnim anim) {
  static const MarioSheet idle = {"assets/textures/Character/Mario_Stand.png",
                                  32, 4};
  static const MarioSheet walk = {"assets/textures/Character/Mario_Walk.png",
                                  32, 6};
  static const MarioSheet jump = {"assets/textures/Character/Mario_Jump.png",
                                  32, 3};
  static const MarioSheet fire = {"assets/textures/Character/Mario_Fire.png",
                                  48, 2};
  switch (anim) {
  case SpriteRegistry::PlayerAnim::Walk:
    return walk;
  case SpriteRegistry::PlayerAnim::Jump:
    return jump;
  case SpriteRegistry::PlayerAnim::Fire:
    return fire;
  case SpriteRegistry::PlayerAnim::Idle:
  case SpriteRegistry::PlayerAnim::Skid:
  default:
    // Skid has no dedicated pose — reuse the idle sheet (same as before).
    return idle;
  }
}

// ── New Luigi art ──────────────────────────────────────────────────────
// Luigi's new look lives in assets/textures/Character/ as horizontal sheets,
// but unlike Mario's sheets the poses are NOT on a uniform grid — each frame's
// art block is unevenly wide and unevenly offset, so every frame is cropped
// with its own explicit sf::IntRect (the art's tight bounds) instead of the
// frame * frameWidth formula applySheetFrame() uses for Mario.
struct LuigiSheet {
  const std::string path;
  const sf::IntRect *frames; // frameCount entries, tight art bounds
  int frameCount;
};

const sf::IntRect LUIGI_STAND_FRAMES[3] = {
    sf::IntRect(0, 0, 21, 38),
    sf::IntRect(24, 0, 22, 38),
    sf::IntRect(49, 1, 22, 37),
};

const sf::IntRect LUIGI_WALK_FRAMES[8] = {
    sf::IntRect(2, 1, 19, 37),
    sf::IntRect(25, 2, 25, 36),
    sf::IntRect(56, 3, 27, 35),
    sf::IntRect(88, 2, 21, 36),
    sf::IntRect(116, 1, 21, 37),
    sf::IntRect(140, 2, 27, 36),
    sf::IntRect(171, 3, 28, 35),
    sf::IntRect(203, 2, 21, 36),
};

const sf::IntRect LUIGI_JUMP_FRAMES[5] = {
    sf::IntRect(0, 1, 21, 39),
    sf::IntRect(24, 0, 25, 39),
    sf::IntRect(52, 1, 28, 39),
    sf::IntRect(85, 2, 37, 38),
    sf::IntRect(124, 1, 37, 39),
};

const sf::IntRect LUIGI_FIRE_FRAMES[1] = {
    sf::IntRect(0, 0, 41, 36),
};

const LuigiSheet &luigiSheet(SpriteRegistry::PlayerAnim anim) {
  static const LuigiSheet idle = {
      "assets/textures/Character/Luigi_Stand.png", LUIGI_STAND_FRAMES, 3};
  static const LuigiSheet walk = {
      "assets/textures/Character/Luigi_Walk.png", LUIGI_WALK_FRAMES, 8};
  static const LuigiSheet jump = {
      "assets/textures/Character/Luigi_Jump.png", LUIGI_JUMP_FRAMES, 5};
  static const LuigiSheet fire = {
      "assets/textures/Character/Luigi_Fire.png", LUIGI_FIRE_FRAMES, 1};
  switch (anim) {
  case SpriteRegistry::PlayerAnim::Walk:
    return walk;
  case SpriteRegistry::PlayerAnim::Jump:
    return jump;
  case SpriteRegistry::PlayerAnim::Fire:
    return fire;
  case SpriteRegistry::PlayerAnim::Idle:
  case SpriteRegistry::PlayerAnim::Skid:
  default:
    // Skid has no dedicated pose — reuse the idle sheet (same as before).
    return idle;
  }
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

  if (!luigi) {
    // Mario uses the new Character/ sheets for every animation (Skid falls
    // back to the Stand sheet). The caller must render through
    // applyPlayerFrame() so only the current frame cell is cropped.
    return marioSheet(anim).path;
  }

  // Luigi uses the new Character/ sheets for every animation too (Skid falls
  // back to the Stand sheet). Unlike Mario, each frame is a tight art-bounds
  // crop rather than a uniform grid cell — applyPlayerFrame() handles that.
  // Power-up state no longer picks a different file, matching Mario.
  return luigiSheet(anim).path;
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
  if (character == CharacterId::Mario) {
    // Mario animates with the new Character/ sheets (Skid reuses the idle
    // sheet, so it shares its frame count).
    return marioSheet(anim).frameCount;
  }
  // Luigi animates with the new Character/ sheets too; each animation exposes
  // every pose its sheet holds.
  return luigiSheet(anim).frameCount;
}

void SpriteRegistry::applyPlayerFrame(sf::Sprite &sprite, CharacterId character,
                                      PowerUpState power, PlayerAnim anim,
                                      int frame, const sf::FloatRect &box,
                                      bool flip) {
  if (anim == PlayerAnim::FlagpoleSlide) {
    applyPlayerFlagpoleSlideFrame(sprite, character, power, frame, box, flip);
    return;
  }

  if (character == CharacterId::Mario) {
    // Mario's new sheets are fixed grids of frame cells (see marioSheet), so
    // each frame is cropped with an sf::IntRect and the sheet's own cell
    // width — this is what lets the unequal-width poses line up correctly.
    const MarioSheet &sheet = marioSheet(anim);
    applySheetFrame(sprite, sheet.path, frame, sheet.frameWidth, 0, box, flip);
    return;
  }

  // Luigi's new sheets are not uniform grids (see luigiSheet), so each frame
  // is cropped with its own tight-art-bounds rect.
  const LuigiSheet &sheet = luigiSheet(anim);
  sf::Texture &texture = AssetManager::getInstance().getTexture(sheet.path);
  applyFrame(sprite, texture,
             sheet.frames[frame % sheet.frameCount], box, flip);
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

const std::string &SpriteRegistry::fireballPath() {
  static const std::string p = "assets/textures/Character/Fire_Ball.png";
  return p;
}

int SpriteRegistry::fireballFrameCount() {
  // Fire_Ball.png is a 64x16 sheet of four contiguous 16x16 frames.
  return 4;
}

const std::string &SpriteRegistry::flowersBuffPath() {
  static const std::string p = "assets/textures/items/FlowersBuff.png";
  return p;
}

int SpriteRegistry::flowersBuffFrameCount() {
  // FlowersBuff.png is a 70x16 sheet: four 16x16 frames spaced 2px apart.
  return 4;
}

const std::string &SpriteRegistry::escalaterPath() {
  static const std::string p = "assets/textures/items/Escalater.png";
  return p;
}

const std::string &SpriteRegistry::lavaPath() {
  static const std::string p = "assets/textures/lava.png";
  return p;
}

const std::string &SpriteRegistry::flamePath() {
  static const std::string p = "assets/textures/items/flame.png";
  return p;
}

const std::string &SpriteRegistry::fireBarBlockPath() {
  static const std::string p = "assets/textures/items/blockbarfire.png";
  return p;
}

const std::string &SpriteRegistry::fireBarSegmentPath() {
  static const std::string p = "assets/textures/items/FireBarWithoutBlock.png";
  return p;
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

void SpriteRegistry::applySheetFrame(sf::Sprite &sprite,
                                     const std::string &path, int frame,
                                     int frameWidth, int gap,
                                     const sf::FloatRect &box, bool flip) {
  sf::Texture &texture = AssetManager::getInstance().getTexture(path);
  sf::Vector2u size = texture.getSize();
  if (size.x == 0 || frameWidth <= 0) return;

  int left = frame * (frameWidth + gap);
  int width = frameWidth;
  if (left >= static_cast<int>(size.x)) {
    // Clamp defensively so an out-of-range frame still draws the last cell.
    int cellWidth = frameWidth + gap;
    int lastCell = static_cast<int>(size.x) / cellWidth - 1;
    left = std::max(0, lastCell * cellWidth);
  }

  applyFrame(sprite, texture,
             sf::IntRect(left, 0, width, static_cast<int>(size.y)), box,
             flip);
}
