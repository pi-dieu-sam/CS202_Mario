#include "Physics/CollisionDetector.hpp"
#include "Entities/GameObject.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace {

struct SweepResult {
    bool hit = false;
    CollisionDetector::Side side = CollisionDetector::Side::None;
    float timeOfImpact = 1.0f;
};

sf::FloatRect previousBounds(const GameObject& object,
                             const sf::FloatRect& currentBounds, float dt) {
    sf::Vector2f velocity = object.getVelocity();
    return {currentBounds.left - velocity.x * dt,
            currentBounds.top - velocity.y * dt,
            currentBounds.width,
            currentBounds.height};
}

SweepResult sweepAabb(const sf::FloatRect& aPrevious,
                      const sf::FloatRect& bPrevious,
                      const sf::Vector2f& relativeDelta) {
    constexpr float infinity = std::numeric_limits<float>::infinity();

    float xEntry = -infinity;
    float xExit = infinity;
    if (relativeDelta.x > 0.0f) {
        xEntry = (bPrevious.left - (aPrevious.left + aPrevious.width)) /
                 relativeDelta.x;
        xExit = ((bPrevious.left + bPrevious.width) - aPrevious.left) /
                relativeDelta.x;
    } else if (relativeDelta.x < 0.0f) {
        xEntry = ((bPrevious.left + bPrevious.width) - aPrevious.left) /
                 relativeDelta.x;
        xExit = (bPrevious.left - (aPrevious.left + aPrevious.width)) /
                relativeDelta.x;
    } else if (aPrevious.left + aPrevious.width <= bPrevious.left ||
               aPrevious.left >= bPrevious.left + bPrevious.width) {
        return {};
    }

    float yEntry = -infinity;
    float yExit = infinity;
    if (relativeDelta.y > 0.0f) {
        yEntry = (bPrevious.top - (aPrevious.top + aPrevious.height)) /
                 relativeDelta.y;
        yExit = ((bPrevious.top + bPrevious.height) - aPrevious.top) /
                relativeDelta.y;
    } else if (relativeDelta.y < 0.0f) {
        yEntry = ((bPrevious.top + bPrevious.height) - aPrevious.top) /
                 relativeDelta.y;
        yExit = (bPrevious.top - (aPrevious.top + aPrevious.height)) /
                relativeDelta.y;
    } else if (aPrevious.top + aPrevious.height <= bPrevious.top ||
               aPrevious.top >= bPrevious.top + bPrevious.height) {
        return {};
    }

    const float entryTime = std::max(xEntry, yEntry);
    const float exitTime = std::min(xExit, yExit);
    if (entryTime > exitTime || entryTime < 0.0f || entryTime > 1.0f) {
        return {};
    }

    SweepResult result;
    result.hit = true;
    result.timeOfImpact = entryTime;
    if (xEntry > yEntry) {
        result.side = relativeDelta.x > 0.0f
                          ? CollisionDetector::Side::Right
                          : CollisionDetector::Side::Left;
    } else {
        result.side = relativeDelta.y > 0.0f
                          ? CollisionDetector::Side::Bottom
                          : CollisionDetector::Side::Top;
    }
    return result;
}

} // namespace

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

    // A support contact while falling may be a fraction of a pixel deep, so
    // retain a small tolerance only in that direction. Applying it while
    // rising was the source of edge-jump snagging: a shallow side overlap was
    // incorrectly turned into a vertical head/landing collision.
    const float fallingSupportBias = vel.y > 0.0f ? 4.0f : 0.0f;
    bool horizontalWins = horizontalOverlap < verticalOverlap - fallingSupportBias;

    // An upward-moving player can clip a block's lower corner by only a few
    // pixels while travelling beside it. The tiny vertical penetration in
    // that case is not a head-hit: resolving it vertically zeroes vy and
    // produces the visible jump snag. Treat this narrow edge band as a wall.
    // A wider overlap still behaves as a normal hit from below.
    constexpr float upwardEdgeOverlap = 6.0f;
    if (vel.y < 0.0f && horizontalOverlap <= upwardEdgeOverlap) {
        horizontalWins = true;
    }

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

CollisionDetector::CollisionResult CollisionDetector::checkSweptCollision(
    const GameObject& a, const GameObject& b, float dt) {
    CollisionResult result;
    if (dt <= 0.0f) {
        return result;
    }

    const sf::FloatRect currentA = a.getBounds();
    const sf::FloatRect currentB = b.getBounds();
    const sf::FloatRect previousA = previousBounds(a, currentA, dt);
    const sf::FloatRect previousB = previousBounds(b, currentB, dt);

    // A swept test describes a new impact. Persistent overlap is handled by
    // the regular discrete resolver so we do not repeatedly report an old
    // contact as a new one.
    if (previousA.intersects(previousB)) {
        return result;
    }

    const sf::Vector2f relativeDelta{
        (currentA.left - previousA.left) - (currentB.left - previousB.left),
        (currentA.top - previousA.top) - (currentB.top - previousB.top)};
    const SweepResult sweep = sweepAabb(previousA, previousB, relativeDelta);
    if (!sweep.hit) {
        return result;
    }

    result.collided = true;
    result.side = sweep.side;
    result.swept = true;
    result.timeOfImpact = sweep.timeOfImpact;
    const sf::Vector2f velocity = a.getVelocity();
    result.impactPosition =
        a.getPosition() - velocity * (dt * (1.0f - sweep.timeOfImpact));
    return result;
}

void CollisionDetector::moveToImpact(GameObject& movable,
                                     const CollisionResult& result) {
    if (result.collided && result.swept) {
        movable.setPosition(result.impactPosition);
    }
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
