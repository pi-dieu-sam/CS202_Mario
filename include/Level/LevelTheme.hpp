#pragma once

/// LevelTheme — which environment palette a level uses.
/// Drives which themed region of the shared spritesheets entities/tiles/
/// backgrounds pull from. Baked into entities once at level-load time.
enum class LevelTheme {
  Overworld,
  Underground,
  Castle
};