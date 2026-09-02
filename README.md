# Super Mario 2D Platformer (C++ / SFML 2.6)

A 2D Super Mario clone built using C++17 and SFML 2.6, implementing strong Object-Oriented Programming (OOP) principles and 6 design patterns.

## Features
- **Design Patterns**: Singleton, Factory, Observer, Command, State, Strategy
- **Characters**: Selectable Mario and Luigi with distinct stats
- **Levels**: 3 levels (Grassland, Underground, Castle) with increasing difficulty
- **Enemies**: Goombas, Koopas (with shell mechanics), Piranha Plants
- **Power-ups**: Mushrooms (grow), Fire Flowers (fireballs), Stars (invincibility)
- **UI & Menu**: Mouse & Keyboard support, HUD overlay, Pause state, Game Over state
- **Save/Load**: Save game state and progress to file

## Level Map Format (Bảng ký tự map)

Mỗi level là một file text trong `assets/levels/` (e.g. `level1.txt`).
Mỗi ký tự đại diện cho **một ô vuông 32×32 pixel** trong map.

| Ký tự | Vật thể |
|---|---|
| `-` | Ô trống |
| `X` | Đất (Ground) |
| `<` `>` | Miệng ống (top-left / top-right) |
| `[` `]` | Thân ống (body-left / body-right) |
| `(` `{` `\` | Ống Ward (ward pipe) — **mảnh hàng trên** (xếp chồng lên mảnh dưới) |
| `)` `}` `/` | Ống Ward (ward pipe) — **mảnh hàng dưới** |
| `S` | Gạch vỡ sau 3 lần nhảy đụng từ phía dưới |
| `?` | Block có chứa xu (Coin) |
| `M` | Block có chứa Nấm (Mushroom) |
| `F` | Block có chứa Hoa lửa (Fire Flower) |
| `E` | Escalater
| `O` | Fire Bar: block 32px và 8 cầu lửa liên tục quay xuyên map (lửa chạm vào sẽ chết) |
| `i` | Lớp mặt dung nham có cầu lửa: bắn lên, rơi xuống, ẩn 3 giây rồi lặp lại |
| `s` | Block có chứa Sao (Star) |
| `W` | Block có chứa FlowersBuff (buff kích thước/tốc độ/nhảy) |
| `Q` | Block đã dùng hết (cứng, không còn item) |
| `o` | Xu (Coin) |
| `G` | Goomba |
| `K` | Koopa |
| `T` | Troopa bay (4 frame, không chịu trọng lực; đổi hướng khi chạm vật cản) |
| `B` | Bowser cố định, chiếm vùng 2×2 ô tính từ ô `B`; đứng yên 2 giây rồi thở 3 giây |
| `P` | Piranha Plant (đặt ngay phía trên nửa trái miệng ống `<`) |
| `@` | Điểm spawn Player 1 |
| `9` | Điểm spawn Player 2 (PvP / Co-op) |
| `f` | Cột cờ (đích của level) |
|`Q` `2` `3` `4`| Castle
|`6` `S` `7` `5`|
|`L`| Lava;
|`l`| lava wave
|`V`| VineTop


**Lưu ý:**
- Ống (`<>` / `[]`) cần xếp thành khối 2 ô ngang × 2 ô dọc để hiển thị đúng (xem ví dụ trong `level1.txt`).
- Cây Piranha: dùng `P` ở ô **ngay trên** ký tự `<`. `P` không phải tile
  rắn; nó là điểm neo để cây xuất phát bên trong miệng ống, trồi lên, chờ,
  rồi chui xuống. Ví dụ ống cao 3 hàng:

  ```text
  --P--
  --<>-
  --[]-
  --[]-
  ```

  Không đặt `P` giữa `<` và `>`: cây sẽ lấy tâm của cả miệng ống từ vị trí
  `P`, nên phải luôn canh thẳng cột với `<`.
- Lâu đài (`c` / `C`): đặt ký tự **ngay hàng mặt đất** — ký tự là ô góc dưới-trái, lâu đài tự mọc lên trên (`c` = 3×3 ô, `C` = 5×6 ô). Là khối đặc; kích thước chỉnh bằng `CASTLE_*_TILES` trong `include/Physics/PhysicsConstants.hpp`.
- Flagpole tự động: khi level có lâu đài, cột cờ tự đặt **3 ô trước lâu đài** (`c`/`C`); không có lâu đài thì mới đặt gần mép phải như cũ. Có thể ghi đè bằng ký tự `f`.
- Map VGLC chuẩn 14 dòng; LevelLoader tự căn level xuống đáy màn hình (19 dòng), 5 dòng trên cùng là trời.
- **Không** chèn dòng comment vào file `.txt` — LevelLoader parse mọi dòng thành một hàng của map, dòng thừa sẽ làm lệch level.
- Chi tiết parser và các ký tự mở rộng xem `include/Level/LevelLoader.hpp`.

## Build Instructions (VS Code / CMake)

### Using VS Code CMake Tools Extension:
1. Open this project folder in VS Code.
2. Select your C++ Kit (e.g. GCC/MinGW or MSVC) when prompted by CMake Tools.
3. Click **Build** (`F7`) or **Debug/Run** (`Ctrl+F5`) on the CMake Tools status bar.

### Using Command Line:
```bash
cmake -B build -S .
cmake --build build
```

Run the executable from `build/bin/SuperMario.exe`.
