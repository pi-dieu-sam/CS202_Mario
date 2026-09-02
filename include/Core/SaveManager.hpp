#pragma once
#include <string>

/// SaveManager — handles saving and loading game progress to/from text files.
class SaveManager {
public:
    /// Save current game state to a file.
    static bool saveGame(const std::string& filename = "savegame.sav");

    /// Load game state from a file. Returns true if successful.
    static bool loadGame(const std::string& filename = "savegame.sav");

    /// Check if a save file exists.
    static bool saveExists(const std::string& filename = "savegame.sav");
};
