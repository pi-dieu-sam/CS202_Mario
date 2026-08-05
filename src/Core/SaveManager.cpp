#include "Core/SaveManager.hpp"
#include "Core/Game.hpp"
#include <fstream>
#include <iostream>
#include <filesystem>

bool SaveManager::saveGame(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "[SaveManager] Failed to open file for saving: " << filename << std::endl;
        return false;
    }

    PlayerProgress& progress = Game::getInstance().getProgress();
    file << "level="     << progress.getCurrentLevel()       << "\n";
    file << "score="     << progress.getScore()              << "\n";
    file << "lives="     << progress.getLives()               << "\n";
    file << "coins="     << progress.getCoins()              << "\n";
    file << "character=" << progress.getSelectedCharacter()  << "\n";

    file.close();
    std::cout << "[SaveManager] Game saved to " << filename << std::endl;
    return true;
}

bool SaveManager::loadGame(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "[SaveManager] No save file found: " << filename << std::endl;
        return false;
    }

    PlayerProgress& progress = Game::getInstance().getProgress();
    std::string line;

    while (std::getline(file, line)) {
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;

        std::string key   = line.substr(0, eq);
        std::string value = line.substr(eq + 1);

        if      (key == "level")     progress.setCurrentLevel(std::stoi(value));
        else if (key == "score")     progress.setScore(std::stoi(value));
        else if (key == "lives")     progress.setLives(std::stoi(value));
        else if (key == "coins")     progress.setCoins(std::stoi(value));
        else if (key == "character") progress.setSelectedCharacter(value);
    }

    file.close();
    std::cout << "[SaveManager] Game loaded from " << filename << std::endl;
    return true;
}

bool SaveManager::saveExists(const std::string& filename) {
    return std::filesystem::exists(filename);
}
