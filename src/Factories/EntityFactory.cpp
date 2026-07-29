#include "Factories/EntityFactory.hpp"

std::unique_ptr<Player> EntityFactory::createPlayer(const std::string& characterName, float x, float y) {
	return std::make_unique<Player>(characterName, x, y);
}

std::unique_ptr<Level> EntityFactory::createLevel(int levelIndex) {
	return std::make_unique<Level>(levelIndex);
}
