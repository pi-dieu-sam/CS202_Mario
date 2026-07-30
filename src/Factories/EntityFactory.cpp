#include "Factories/EntityFactory.hpp"

#include <algorithm>
#include <cctype>

namespace {
	std::string normalizeType(std::string value) {
		std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
			return static_cast<char>(std::tolower(c));
		});
		return value;
	}
}

std::unique_ptr<Player> EntityFactory::createPlayer(const std::string& characterName, float x, float y) {
	if (normalizeType(characterName) == "luigi") {
		return std::make_unique<Player>("Luigi", x, y);
	}

	if (normalizeType(characterName) == "mario" || characterName.empty()) {
		return std::make_unique<Player>("Mario", x, y);
	}

	return std::make_unique<Player>(characterName, x, y);
}

std::unique_ptr<Item> EntityFactory::createItem(const std::string& itemType, float x, float y) {
	const std::string type = normalizeType(itemType);
	if (type == "mushroom") {
		return std::make_unique<Item>("Mushroom", x, y);
	}
	if (type == "fireflower" || type == "fire_flower") {
		return std::make_unique<Item>("FireFlower", x, y);
	}
	if (type == "coin" || itemType.empty()) {
		return std::make_unique<Item>("Coin", x, y);
	}

	return std::make_unique<Item>(itemType, x, y);
}

std::unique_ptr<Level> EntityFactory::createLevel(int levelIndex) {
	return std::make_unique<Level>(levelIndex);
}
