#ifndef ENTITY_FACTORY_HPP
#define ENTITY_FACTORY_HPP

#include "Entities/Player.hpp"
#include "Entities/Enemy.hpp"
#include "Entities/Item.hpp"
#include "World/Level.hpp"
#include <memory>
#include <string>

/**
 * @brief Factory for creating game entities and levels.
 */
class EntityFactory {
public:
	EntityFactory() = delete;
	~EntityFactory() = delete;

	static std::unique_ptr<Player> createPlayer(const std::string& characterName, float x = 100.0f, float y = 400.0f);
	static std::unique_ptr<Enemy> createEnemy(const std::string& enemyType, float x = 0.0f, float y = 0.0f);
	static std::unique_ptr<Item> createItem(const std::string& itemType, float x = 0.0f, float y = 0.0f);
	static std::unique_ptr<Level> createLevel(int levelIndex = 1);
};

#endif // ENTITY_FACTORY_HPP
