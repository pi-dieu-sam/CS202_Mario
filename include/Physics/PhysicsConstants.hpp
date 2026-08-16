#pragma once

// ── Physics ──
constexpr float GRAVITY          = 980.0f;   // pixels/s²
constexpr float MAX_FALL_SPEED   = 600.0f;   // pixels/s
constexpr float GROUND_FRICTION  = 0.85f;    // horizontal deceleration factor

// ── Tile grid ──
constexpr float TILE_SIZE        = 32.0f;    // pixels per tile

// ── Forked/warp pipe ('P') ──
// The 64x64 sprite is scaled up so one 'P' occupies (2*scale) x (2*scale)
// tiles — 2x matches the height of a regular pipe built from 2 stacked
// '<>' '[]' sets. Bump this number to make the pipe bigger.
constexpr float FORKED_PIPE_SCALE = 2.0f;

// ── Player defaults ──
constexpr float PLAYER_SPEED     = 200.0f;   // pixels/s
constexpr float PLAYER_JUMP      = -420.0f;  // initial jump velocity (negative = up)
constexpr float PLAYER_SPRINT    = 1.5f;     // speed multiplier when sprinting
constexpr float PLAYER_FALL_DEATH_MARGIN = TILE_SIZE * 3.0f; // allow brief fall below map before death

// ── Window ──
constexpr unsigned WINDOW_WIDTH  = 800;
constexpr unsigned WINDOW_HEIGHT = 608;      // 19 tiles tall (19*32)
constexpr float    FIXED_DT      = 1.0f / 60.0f;  // fixed timestep

// ── Game rules ──
constexpr int   STARTING_LIVES    = 3;
constexpr float LEVEL_TIME        = 300.0f;  // seconds per level
constexpr float INVINCIBILITY_DUR = 2.0f;    // seconds after taking damage
constexpr int   COIN_SCORE        = 100;
constexpr int   ENEMY_SCORE       = 200;
constexpr int   TIME_BONUS_PER_SECOND = 10;
constexpr int   TOTAL_LEVELS      = 3;
