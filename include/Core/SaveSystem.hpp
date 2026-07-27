#ifndef SAVE_SYSTEM_HPP
#define SAVE_SYSTEM_HPP

#include <string>
#include <fstream>
#include <iostream>

struct SaveData {
    int currentLevel = 1;
    std::string characterName = "Mario";
    int score = 0;
    int coins = 0;
    int lives = 3;
    int powerState = 0; // 0: Small, 1: Super, 2: Fire, 3: Invincible
    bool hasSaveFile = false;
};

/**
 * @brief Singleton class to handle saving and loading game progress from disk.
 */
class SaveSystem {
public:
    static SaveSystem& getInstance() {
        static SaveSystem instance;
        return instance;
    }

    SaveSystem(const SaveSystem&) = delete;
    SaveSystem& operator=(const SaveSystem&) = delete;

    /**
     * @brief Save game data to file.
     */
    bool saveGame(const SaveData& data, const std::string& filename = "savegame.dat") {
        std::ofstream outFile(filename);
        if (!outFile.is_open()) {
            std::cerr << "[SaveSystem] Error: Unable to open file for writing: " << filename << std::endl;
            return false;
        }

        outFile << "LEVEL " << data.currentLevel << "\n";
        outFile << "CHARACTER " << data.characterName << "\n";
        outFile << "SCORE " << data.score << "\n";
        outFile << "COINS " << data.coins << "\n";
        outFile << "LIVES " << data.lives << "\n";
        outFile << "POWER " << data.powerState << "\n";

        outFile.close();
        std::cout << "[SaveSystem] Game saved successfully to " << filename << std::endl;
        return true;
    }

    /**
     * @brief Load game data from file.
     */
    bool loadGame(SaveData& outData, const std::string& filename = "savegame.dat") {
        std::ifstream inFile(filename);
        if (!inFile.is_open()) {
            std::cout << "[SaveSystem] Info: No save file found at " << filename << std::endl;
            outData.hasSaveFile = false;
            return false;
        }

        std::string key;
        while (inFile >> key) {
            if (key == "LEVEL") inFile >> outData.currentLevel;
            else if (key == "CHARACTER") inFile >> outData.characterName;
            else if (key == "SCORE") inFile >> outData.score;
            else if (key == "COINS") inFile >> outData.coins;
            else if (key == "LIVES") inFile >> outData.lives;
            else if (key == "POWER") inFile >> outData.powerState;
        }

        inFile.close();
        outData.hasSaveFile = true;
        std::cout << "[SaveSystem] Game loaded successfully from " << filename << std::endl;
        return true;
    }

    bool hasSaveFile(const std::string& filename = "savegame.dat") const {
        std::ifstream inFile(filename);
        return inFile.good();
    }

private:
    SaveSystem() = default;
    ~SaveSystem() = default;
};

#endif // SAVE_SYSTEM_HPP
