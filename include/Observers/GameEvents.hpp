#pragma once
#include <SFML/System/Vector2.hpp>
#include <functional>
#include <unordered_map>
#include <vector>
#include <string>
#include <any>

/// Event types that can be published/subscribed.
enum class EventType {
    CoinCollected,
    EnemyDefeated,
    PowerUpCollected,
    PlayerDied,
    LevelCompleted,
    BlockHit,
    PlayerDamaged
};

/// GameEvent — data payload for an event.
struct GameEvent {
    EventType   type;
    int         intData   = 0;    // e.g., score amount, enemy type
    float       floatData = 0.0f; // e.g., position
    std::string strData;          // e.g., item name
    sf::Vector2f worldPosition = {0.0f, 0.0f};
};
