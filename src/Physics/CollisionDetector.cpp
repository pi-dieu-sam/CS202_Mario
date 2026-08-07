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

    // Of the two overlaps on each axis, only the one consistent with A's
    // actual direction of travel is physically possible this frame -- e.g.
    // while falling (vel.y > 0), "A's bottom hit B's top" (overlapTop) can
    // happen, but "A's top hit B's bottom" (overlapBottom) cannot, so it
    // must never win the axis choice just for having a smaller raw number.
    // (vel == 0 falls back to the old either-side comparison, e.g. a
    // stationary object being pushed into from outside.)
    sf::Vector2f vel = a.getVelocity();
    float verticalOverlap   = (vel.y > 0.0f)  ? overlapTop
                             : (vel.y < 0.0f) ? overlapBottom
                             : std::min(overlapTop, overlapBottom);
    float horizontalOverlap = (vel.x > 0.0f)  ? overlapLeft
                             : (vel.x < 0.0f) ? overlapRight
                             : std::min(overlapLeft, overlapRight);

    // At a seam between two flush tiles (e.g. walking across flat ground),
    // floating-point rounding can make the horizontal overlap come out
    // marginally smaller than the vertical one, which would wrongly resolve
    // a landing as a sideways push ("snagging"). Only trust the horizontal
    // resolution when it's clearly smaller than the vertical alternative.
    const float VERTICAL_BIAS = 4.0f;
    bool horizontalWins = horizontalOverlap < verticalOverlap - VERTICAL_BIAS;

    if (horizontalWins) {
        if (horizontalOverlap == overlapLeft) {
            result.side    = Side::Right; // A's right hit B's left
            result.overlap = overlapLeft;
        } else {
            result.side    = Side::Left; // A's left hit B's right
            result.overlap = overlapRight;
        }
    } else {
        if (verticalOverlap == overlapTop) {
            result.side    = Side::Bottom; // A's bottom hit B's top
            result.overlap = overlapTop;
        } else {
            result.side    = Side::Top; // A's top hit B's bottom
            result.overlap = overlapBottom;
        }
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