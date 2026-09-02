#include "UI/MenuAttractTimeline.hpp"

#include <cmath>
#include <iostream>

static int g_failures = 0;

#define CHECK(cond, msg)                                                     \
  do {                                                                       \
    if (!(cond)) {                                                           \
      std::cerr << "FAIL: " << msg << " (" << __FILE__ << ":" << __LINE__    \
                << ")\n";                                                   \
      ++g_failures;                                                         \
    }                                                                        \
  } while (0)

int main() {
    using namespace MenuAttractTimeline;

    const Frame opening = evaluate(0.0f);
    CHECK(opening.scene == Scene::MarioChase && opening.sceneTime == 0.0f,
          "the title screen opens on Mario's chase vignette");
    CHECK(evaluate(2.8f).marioJumping,
          "Mario jumps during the pipe-clearing window");
    CHECK(!evaluate(2.2f).marioJumping,
          "Mario stays grounded before the pipe-clearing window");

    const Frame fireScene = evaluate(5.0f);
    CHECK(fireScene.scene == Scene::LuigiFire && std::fabs(fireScene.sceneTime) < 0.001f,
          "Luigi's fire vignette follows Mario's chase without a gap");
    CHECK(evaluate(7.5f).luigiFiring,
          "Luigi fires during the planned fireball window");
    CHECK(!evaluate(8.5f).luigiFiring,
          "Luigi returns to running after the fireball window");

    const Frame looped = evaluate(CYCLE_DURATION);
    CHECK(looped.scene == Scene::MarioChase && std::fabs(looped.sceneTime) < 0.001f,
          "the attract loop restarts deterministically after ten seconds");

    if (g_failures == 0) {
        std::cout << "All menu animation tests passed.\n";
        return 0;
    }
    std::cerr << g_failures << " test(s) failed.\n";
    return 1;
}
