#pragma once

#include "Physics/PhysicsConstants.hpp"
#include <algorithm>

namespace LevelCompletion {

/// Matches HUD::setTime() by truncating the remaining timer to a non-negative
/// whole-second value before any completion animation begins.
inline int displayedSeconds(float remainingTime) {
    return std::max(0, static_cast<int>(remainingTime));
}

inline int timeBonusForSeconds(int seconds) {
    return std::max(0, seconds) * TIME_BONUS_PER_SECOND;
}

/// Converts one second of the frozen completion timer into score. Returns
/// false when every remaining second has already been converted.
inline bool convertNextSecond(int& remainingSeconds, int& convertedScore) {
    if (remainingSeconds <= 0) {
        return false;
    }

    --remainingSeconds;
    convertedScore += TIME_BONUS_PER_SECOND;
    return true;
}

} // namespace LevelCompletion
