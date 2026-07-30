#pragma once
#include <vector>
#include <memory>

class GameObject;

/// CollisionDetector — AABB collision detection between game objects.
/// Checks player against tiles, enemies, items, and blocks.
class CollisionDetector {
public:
    /// Collision side indicators.
    enum class Side { None, Top, Bottom, Left, Right };

    /// Result of a collision check.
    struct CollisionResult {
        bool  collided = false;
        Side  side     = Side::None;
        float overlap  = 0.0f;
    };

    /// Check AABB overlap between two objects.
    static CollisionResult checkCollision(const GameObject& a, const GameObject& b);

    /// Resolve player-tile collision (push player out of tile).
    static void resolveCollision(GameObject& movable, const GameObject& solid, const CollisionResult& result);
};