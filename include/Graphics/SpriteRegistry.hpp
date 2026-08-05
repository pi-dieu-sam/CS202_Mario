#pragma once
#include "../Level/LevelTheme.hpp"
#include <SFML/Graphics.hpp>
#include <string>

// Forward declarations — kept light since this header is pulled in by
// nearly every entity .cpp file.
enum class TileType;
enum class BlockType;
enum class CharacterId;
enum class PowerUpState;

/// SpriteRegistry — static lookup table mapping (entity type, theme, state,
/// frame) to a texture file under assets/textures/, where each file is a
/// single, already-cropped sprite (no shared sheets, no pixel-rect slicing).
/// Every hardcoded sprite filename in the game lives in SpriteRegistry.cpp,
/// never scattered across entity files, so swapping an asset only ever means
/// touching one file.
///
/// Never instantiated — same "static-method utility class" convention as
/// EntityFactory / CollisionDetector / LevelLoader / SaveManager.
class SpriteRegistry {
public:
  enum class BlockVisualState { Idle, Hit, Used };
  enum class PlayerAnim { Idle, Walk, Jump, Skid };

  // ── Tiles ──
  static const std::string &tilePath(TileType type, LevelTheme theme);

  // ── Blocks ──
  static const std::string &blockPath(BlockType type, LevelTheme theme,
                                       BlockVisualState state);

  // ── Enemies ──
  static const std::string &goombaPath(LevelTheme theme, int walkFrame);
  static const std::string &goombaSquishPath(LevelTheme theme);
  static const std::string &koopaWalkPath(LevelTheme theme, int walkFrame);
  static const std::string &koopaShellPath(LevelTheme theme, bool spinning);
  static const std::string &piranhaPlantPath(int frame);

  // ── Player ──
  static const std::string &playerPath(CharacterId character, PowerUpState power,
                                        PlayerAnim anim, int frame);
  static int playerFrameCount(CharacterId character, PowerUpState power,
                               PlayerAnim anim);

  // ── Items ──
  static const std::string &coinPath(LevelTheme theme, int frame);
  static int coinFrameCount();
  static const std::string &mushroomPath(LevelTheme theme);
  static const std::string &fireFlowerPath(LevelTheme theme);
  static const std::string &starPath(LevelTheme theme, int frame);
  static int starFrameCount();
  static const std::string &fireballPath(int frame);
  static int fireballFrameCount();

  /// Shared draw-transform helper: loads `path` through AssetManager, binds
  /// it to the sprite, pivots on bottom-center (so horizontal flip mirrors
  /// in place instead of jumping sideways), derives scale from the
  /// texture's own height so no per-file scale constant is ever needed, and
  /// positions the sprite so its bottom-center lands at the box's
  /// bottom-center.
  static void applyFrame(sf::Sprite &sprite, const std::string &path,
                          const sf::FloatRect &box, bool flip = false);

  /// Lower-level overload for the rare case where only part of a texture
  /// should be shown (e.g. a Piranha Plant emerging from a pipe) — this is
  /// cropping a single already-correct sprite, not slicing a shared sheet.
  static void applyFrame(sf::Sprite &sprite, sf::Texture &texture,
                          const sf::IntRect &cropRect, const sf::FloatRect &box,
                          bool flip = false);

  /// Apply a specific GIF animation frame: fetches frame `frame` of the GIF
  /// at `gifPath` from AssetManager's GIF cache and applies it identically
  /// to the single-frame applyFrame overload above.
  static void applyGifFrame(sf::Sprite &sprite, const std::string &gifPath,
                             int frame, const sf::FloatRect &box,
                             bool flip = false);
};
