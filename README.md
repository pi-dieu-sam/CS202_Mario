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