#pragma once

#include <algorithm>
#include <cmath>

/// Pure timing model for MenuState's presentation-only attract loop. Keeping
/// this independent from SFML makes the action sequence regression-testable.
namespace MenuAttractTimeline {

enum class Scene {
    MarioChase,
    LuigiFire,
};

struct Frame {
    Scene scene = Scene::MarioChase;
    float sceneTime = 0.0f;
    bool marioJumping = false;
    bool luigiFiring = false;
};

constexpr float CYCLE_DURATION = 10.0f;
constexpr float SCENE_DURATION = 5.0f;

inline Frame evaluate(float elapsedSeconds) {
    const float elapsed = std::max(0.0f, elapsedSeconds);
    const float cycleTime = std::fmod(elapsed, CYCLE_DURATION);
    if (cycleTime < SCENE_DURATION) {
        const bool jumping = cycleTime > 2.55f && cycleTime < 3.45f;
        return {Scene::MarioChase, cycleTime, jumping, false};
    }
    const float sceneTime = cycleTime - SCENE_DURATION;
    const bool firing = sceneTime > 2.15f && sceneTime < 3.1f;
    return {Scene::LuigiFire, sceneTime, false, firing};
}

} // namespace MenuAttractTimeline
