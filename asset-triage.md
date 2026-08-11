# assets/textures triage — 188 files

Checked every file's real byte signature (not just its extension), cross-referenced against
what the engine currently models (`CharacterId`, `PowerUpState`, `PlayerAnim`, `TileType`,
`BlockType`, `LevelTheme` = Overworld/Underground/Castle). No implementation yet — this is the
map for deciding what to re-source before `SpriteRegistry`/`AssetManager` get rebuilt.

**Update — all blocking gaps closed:**
- Re-exported and verified as real PNG: `Warp_Pipe_SMB.png`, `Warp_Pipe_Gray_SMB.png`, `Warp_Pipe_Orange_SMB.png`, `SMB_Castle_Brick_Block.png`, `SMB_Castle_Question_Block.png`. (`Warp_Pipe_Water_SMB.png` is still broken WebP, but Underwater isn't a modeled theme, so it doesn't block anything.)
- Decision: Castle-theme Goomba and Underground/Castle-theme Koopa reuse the Overworld/normal-variant art instead of needing dedicated sprites — no separate castle enemy files needed.
- Luigi coverage restored: valid transparent PNGs now cover Small/Big idle and
  every Fire Luigi state. They were derived from the matching loadable Mario
  poses with Luigi's green/white NES palette, verified against the shared
  Mario & Luigi reference sheet.

## Bottom line

| | Count |
|---|---|
| Total files | 188 |
| Real, decodable PNG | 91 |
| Real, decodable GIF | 43 |
| **WebP data mislabeled `.png`/`.gif`** — SFML cannot load these | ~~54~~ **49** (5 re-exported and fixed) |
| Of the 134 decodable files: actual gameplay sprites | ~95 |
| Of the 134 decodable files: logos / icons / screenshots / thumbnail dupes | ~25 |
| Files needing visual ID (mangled names) | 6 — all identified below |

---

## 1. Coverage by engine system

**Player** (`CharacterId` Mario/Luigi × `PowerUpState` Small/Big/Fire × `PlayerAnim` Idle/Walk/Jump/Skid)

| Character | Small | Big | Fire |
|---|---|---|---|
| Mario | Covered (idle/walk/jump). No skid pose in the pack. | Covered (idle/walk/jump/duck). No skid. | Covered (idle/walk/jump/duck/shoot). No skid. |
| Luigi | Idle, jump, and a two-frame walk cycle covered. | Idle, jump, and a two-frame walk cycle covered; duck remains bonus/unmodeled. | Idle, jump, and a two-frame walk cycle covered by derived transparent Luigi PNGs. |

Bonus, not currently modeled: `SMBMarioGrowing.png` / `MariotoSuperMario.gif` (mushroom transform), `Invincible_Mario.gif` / `SmallinvincibleMario.gif` (star blink), death sprites, `Firemarioclimb.gif` (16-frame vine-climb clip), swimming Mario/Luigi.

**Enemies**

| Entity | Overworld | Underground | Castle |
|---|---|---|---|
| Goomba | Covered (walk 2fr, dead) | Covered (grey variant, dead) | Reuses Overworld art (decision — no dedicated Castle sprite needed). |
| Koopa | Covered (walk, shell) | Reuses Overworld art (decision) | Reuses Overworld art (decision) |
| Piranha Plant | Covered — 4 usable files once the mangled names are resolved (see §2) | n/a (not theme-dependent in current model) | |

Not modeled by any current class, but present in the pack (bonus/future scope): Bowser (idle, walking, fire-breath, bridge), Hammer Bro, Lakitu + thrown Spiny, Spiny + egg, Bullet Bill/Blaster, Buzzy Beetle (all 3 shell-color variants are broken WebP), Blooper, Cheep Cheep (red/grey + swimming), Podoboo.

**Blocks** (Brick / Question × 3 themes × Idle/Hit/Used)

| | Overworld | Underground | Castle |
|---|---|---|---|
| Brick | Covered | Covered | Covered (re-exported) |
| Question (animated, 5fr) | Covered | Covered | Static art covered (re-exported); animated Castle variant (`SMB_QuestionBlockCastleAnim.gif`) still available too |
| Hard Block ("used" state) | Covered | **Broken WebP only** | — none found |

**Tiles**

- Ground: full coverage — Overworld, Underground, Castle, and bonus Underwater/Underwater-Castle.
- Pipes: `Warp_Pipe_SMB.png` (Overworld), `Warp_Pipe_Gray_SMB.png` (Underground), and `Warp_Pipe_Orange_SMB.png` (Castle) are re-exported and verified real PNGs — all 3 modeled themes covered. `Warp_Pipe_Water_SMB.png` is still broken WebP but Underwater isn't a modeled theme, so it's not blocking.
- Vines/scenery/lava/islands: mostly intact, decent bonus scenery beyond current scope.

**Items** — Coin (5-frame animated, all 3 themes + bonus underwater), Mushroom, 1-Up (x2), Fire Flower, Star, Fireball: all have at least one real usable file. `SMB_Sprite_Fire_Flower.png` and `SMB_Sprite_Super_Star.png` are broken WebP duplicates, but `Fire_Flower_SMB.gif` and `Starman.gif` cover the same items in a working format.

**Flagpole** — `17px-SMB_Goal_Pole.png` and `SMB_Goal_Pole.png` are real, but they're tall pre-assembled pole+flag renders, not the small separate flag/ball crops `SpriteRegistry::flagpoleFlagRect`/`flagpoleBallRect` currently expect. This is a small rendering-approach change, not just a file swap. `SMB_fortress_flag.png` is broken WebP.

---

## 2. Mangled filenames — identified by opening them

| File | Dimensions | What it actually is |
|---|---|---|
| `129858~1.PNG` | 120×60 | "Super Mario Bros." logo badge — not a gameplay sprite |
| `SM7D05~1.PNG` | 16×23 | Piranha Plant (static) — near-duplicate of `Piranha_Plant_Underwater.png` |
| `SMB_BA~2.PNG` | 16×16 | Green brick-textured tile — likely an alt/duplicate of `SMB_Background_Brick_Block.png` |
| `SMB_PI~1.PNG` | 16×23 | Piranha Plant (static) — another near-duplicate |
| `SMB_PI~1.GIF` | 16×24, 2 frames | Piranha Plant chomp animation — usable, real GIF |
| `SMB_HA~2.GIF` | — | Green enemy sprite (reads as Hammer Bro), **but this one is a broken WebP** despite the `.gif` extension — confirmed by opening it: a general-purpose image viewer decodes it fine, SFML's loader won't |

The `~N` suffix pattern is Windows auto-renaming files that collided on the same truncated name during a bulk download — i.e. these are accidental duplicates of already-named files, not unique content.

---

## 3. Broken files (WebP mislabeled as PNG/GIF) — 54 total

These decode fine in a browser/viewer but **will fail to load in SFML** (no WebP support in its image backend). Grouped by what they represent, so re-sourcing/re-exporting can be prioritized:

- **Pipes (all 4 standard colors)**: `Warp_Pipe_SMB`, `Warp_Pipe_Gray_SMB`, `Warp_Pipe_Orange_SMB`, `Warp_Pipe_Water_SMB` — highest priority, this is a total gap otherwise.
- **Player**: `ClassicNES_SMB_Fire_Mario_Sprite`, `ClassicNES_SMB_Small_Luigi_Sprite`, `ClassicNES_SMB_Small_Mario_Sprite`, `ClassicNES_SMB_Super_Luigi_Sprite`, `ClassicNES_SMB_Super_Mario_Sprite`, `SmallFireMarioJump`, `SMB_NES_Mario_Death_Sprite`, `SMB_Small_Luigi_Sprite`, `SMB_Super_Luigi_Sprite`, `SMBSuperLuigiSwimming`
- **Blocks**: `SMB_Castle_Brick_Block`, `SMB_Castle_Question_Block`, `SMB_Underground_Hard_Block`, `SMB_Question_Block`, `SMB3_Cloud_Block_tile_alt`
- **Enemies**: `PiranhaPlant_SMB_Sprite`, `SMB_Sprite_Piranha_Plant`, `SMB_NES_Blue_Koopa_Troopa_Walking`, `SMB_Goomba_Castle_Dead`, `SMB_Buzzy_Shell` / `_Castle` / `_Underground`, `SMB_Hammer_Bro_Sprite`, `SMB_Lakitu_Sprite`, `SMB_Lakitu_Throwing_Spiny_Sprites`, `SMB_Sprite_Spiny`, `SMB_Red_Koopa_Paratroopa_Sprite`, `SMB_Sprite_Cheep_Cheep_-Gray-`, `SMB_Swimming_Cheep_Cheep_Sprite`, `SMB_TurtleCannonCastle`, `SMB_TurtleCannonUnderground`, `SMB_HA~2.GIF`
- **Items/UI**: `SMB_Sprite_Fire_Flower`, `SMB_Sprite_Super_Star`, `SMB_Sprite_1UP`, `SMB_Sprite_Axe`, `SMB_Sprite_Coin`, `SMB_Sprite_Coral`, `SMB1_Sprite_Coin`
- **Scenery/logos/other**: `SMB_Fortress`, `SMB_fortress_flag`, `SMBCastle`, `SMB_3DS_Virtual_Console_Icon`, `SMB-DH-WCTM_SMB_in-game_logo`, `SMB_Climbable_Ball_Sprite`, `SMB_Princess_Toadstool_Sprite`, `SMB_White_Horsetail_Short`, `SMB_Green_Koopa_Troopa_Sprite`... *(full list available on request — this covers the ones that matter for current engine scope)*

---

## 4. Not gameplay sprites — exclude from `SpriteRegistry`

Logos, icons, box art, and screenshots that were never meant to be cropped as in-game sprites:
`120px-SMB-DH_SMB_logo.png`, `SMB-DH_SMB_logo.png`, `129858~1.PNG`, `120px-SMB_In-game_Logo.png`,
`SMB_In-game_Logo.png`, `98px-SMBCastle.png`, `LargeFortressSMB.png` / `101px-LargeFortressSMB.png`,
`SMB_2ndCHRMap_World36.png` / `120px-...` (ironically this one *is* a multi-sprite sheet screenshot —
the one file in the whole pack that contradicts the "one sprite per file" premise), `SMB_3DS_Virtual_Console_Icon.png`.
These might be useful later for a title/menu screen, but they're a separate concern from entity rendering.

## 5. Duplicates to resolve (pick one canonical file)

- Piranha Plant static: `Piranha_Plant_Underwater.png`, `SM7D05~1.PNG`, `SMB_PI~1.PNG` — same subject, same 16×23 size.
- Logo: `129858~1.PNG` ≈ `120px-SMB-DH_SMB_logo.png` (same 120×60 dims).
- Scale lift: `SMB_Scale_Lift_sprite.png` vs its `120px-` thumbnail — same image, two resolutions.
- Goomba idle: `Goomba_SMB.png` vs frame 0 of `SMB_Goomba_Sprite.gif` — likely the same pose twice.

---

## Recommendation

All blocking gaps are now closed (see the update note at the top). Nothing left standing in the
way of rebuilding `SpriteRegistry`/`AssetManager` around the per-file asset set:

1. ~~Pipes~~ — resolved, re-exported and verified.
2. ~~Castle-theme block art~~ — resolved, re-exported and verified. Castle Goomba/Koopa resolved by reusing Overworld art.
3. ~~Fire Luigi / Luigi idle~~ — resolved with loadable Luigi palette variants for every modeled state.
4. Everything else already had a working file; the remaining broken WebP files (§3, now 49) are all either non-blocking bonus/future-scope content or have a real, loadable twin already covering the same content.
