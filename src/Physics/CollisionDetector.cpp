#include "Physics/CollisionDetector.hpp"
#include "Entities/GameObject.hpp"
#include <algorithm>
#include <cmath>

CollisionDetector::CollisionResult CollisionDetector::checkCollision(
    const GameObject& a, const GameObject& b)
{
    CollisionResult result;

    sf::FloatRect boundsA = a.getBounds();
    sf::FloatRect boundsB = b.getBounds();

    if (!boundsA.intersects(boundsB)) {
        return result; // No collision
    }

    result.collided = true;

    // Calculate overlap on each side
    float overlapLeft   = (boundsA.left + boundsA.width)  - boundsB.left;
    float overlapRight  = (boundsB.left + boundsB.width)  - boundsA.left;
    float overlapTop    = (boundsA.top  + boundsA.height) - boundsB.top;
    float overlapBottom = (boundsB.top  + boundsB.height) - boundsA.top;

    // Find the smallest overlap to determine collision side
    float minOverlap = std::min({overlapLeft, overlapRight, overlapTop, overlapBottom});

    // At a seam between two flush tiles (e.g. walking across flat ground),
    // floating-point rounding can make a horizontal overlap come out
    // marginally smaller than the vertical one, which would wrongly resolve
    // a landing as a sideways push ("snagging"). Only trust a horizontal
    // resolution when it's clearly smaller than the vertical alternative.
    const float VERTICAL_BIAS = 4.0f;
    if (minOverlap == overlapLeft || minOverlap == overlapRight) {
        float verticalMin = std::min(overlapTop, overlapBottom);
        if (verticalMin <= minOverlap + VERTICAL_BIAS) {
            minOverlap = verticalMin;
        }
    }

    if (minOverlap == overlapTop) {
        result.side    = Side::Bottom; // A's bottom hit B's top
        result.overlap = overlapTop;
    } else if (minOverlap == overlapBottom) {
        result.side    = Side::Top; // A's top hit B's bottom
        result.overlap = overlapBottom;
    } else if (minOverlap == overlapLeft) {
        result.side    = Side::Right; // A's right hit B's left
        result.overlap = overlapLeft;
    } else {
        result.side    = Side::Left; // A's left hit B's right
        result.overlap = overlapRight;
    }

    return result;
}

void CollisionDetector::resolveCollision(
    GameObject& movable, const GameObject& solid, const CollisionResult& result)
{
    if (!result.collided) return;

    sf::Vector2f pos = movable.getPosition();
    sf::Vector2f vel = movable.getVelocity();

    switch (result.side) {
        case Side::Bottom: // Landing on top of solid
            pos.y -= result.overlap;
            vel.y = 0.0f;
            break;
        case Side::Top: // Hitting head on bottom of solid
            pos.y += result.overlap;
            vel.y = 0.0f;
            break;
        case Side::Right: // Hitting right wall
            pos.x -= result.overlap;
            vel.x = 0.0f;
            break;
        case Side::Left: // Hitting left wall
            pos.x += result.overlap;
            vel.x = 0.0f;
            break;
        default:
            break;
    }

    movable.setPosition(pos);
    movable.setVelocity(vel);
}

float CollisionDetector::reflectHorizontalVelocity(float incomingVx, Side side, float fallbackSpeed) {
    if (incomingVx != 0.0f) {
        return -incomingVx;
    }
    // Side::Right means the object's right edge hit a solid (wall to its
    // right) -> push left. Side::Left is the mirror case.
    float awayFromWall = (side == Side::Right) ? -1.0f : 1.0f;
    return awayFromWall * std::abs(fallbackSpeed);
}