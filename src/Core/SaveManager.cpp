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

    Game& game = Game::getInstance();
    file << "level="     << game.getCurrentLevel()       << "\n";
    file << "score="     << game.getScore()              << "\n";
    file << "lives="     << game.getLives()               << "\n";
    file << "coins="     << game.getCoins()              << "\n";
    file << "character=" << game.getSelectedCharacter()  << "\n";

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

    Game& game = Game::getInstance();
    std::string line;

    while (std::getline(file, line)) {
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;

        std::string key   = line.substr(0, eq);
        std::string value = line.substr(eq + 1);

        if      (key == "level")     game.setCurrentLevel(std::stoi(value));
        else if (key == "score")     game.setScore(std::stoi(value));
        else if (key == "lives")     game.setLives(std::stoi(value));
        else if (key == "coins")     game.setCoins(std::stoi(value));
        else if (key == "character") game.setSelectedCharacter(value);
    }

    file.close();
    std::cout << "[SaveManager] Game loaded from " << filename << std::endl;
    return true;
}

bool SaveManager::saveExists(const std::string& filename) {
    return std::filesystem::exists(filename);
}
