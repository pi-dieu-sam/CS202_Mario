#pragma once
#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Vector2.hpp>
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
        bool         collided = false;
        Side         side = Side::None;
        float        overlap = 0.0f;
        bool         swept = false;
        float        timeOfImpact = 1.0f;
        sf::Vector2f impactPosition{};
    };

    /// Check AABB overlap between two objects.
    static CollisionResult checkCollision(const GameObject& a, const GameObject& b);

    /// Check AABB overlap for explicit collision bodies. This is useful when
    /// an object has a specialized interaction body, such as the player's
    /// compact block-hit body.
    static CollisionResult checkCollision(const sf::FloatRect& boundsA,
                                          const sf::Vector2f& velocityA,
                                          const sf::FloatRect& boundsB,
                                          const sf::Vector2f& velocityB);

    /// Detect the first AABB contact during the most recent simulation step.
    /// This catches objects that pass through each other between two frames and
    /// reports the side at the actual time of impact instead of inferring it
    /// from a deep final overlap.
    static CollisionResult checkSweptCollision(const GameObject& a,
                                               const GameObject& b,
                                               float dt);

    /// Move an object back to the contact point returned by
    /// checkSweptCollision(). This is useful before applying a response such
    /// as a stomp bounce.
    static void moveToImpact(GameObject& movable, const CollisionResult& result);

    /// Resolve player-tile collision (push player out of tile).
    static void resolveCollision(GameObject& movable, const GameObject& solid, const CollisionResult& result);

    /// Given the horizontal velocity an object had going INTO a Left/Right wall
    /// collision (captured by the caller BEFORE calling resolveCollision(), since
    /// resolveCollision() zeroes it), compute what it should be immediately after.
    /// incomingVx != 0: mirror bounce, -incomingVx.
    /// incomingVx == 0: deterministic push away from the wall, derived from
    /// `side` alone (never from any AI strategy's own default direction), at
    /// |fallbackSpeed|.
    static float reflectHorizontalVelocity(float incomingVx, Side side, float fallbackSpeed);
};
