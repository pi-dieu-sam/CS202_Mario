// Regression tests for issue #18: back navigation must never empty the
// state stack, and the forward/back screen matrix must hold for every game
// mode. Two independent things are covered:
//
//   - StateManager (push/pop/change/clear, the pop-guard, onPause/onResume
//     ordering, and the isTransparent() render-start selection) using a
//     DummyState that touches no SFML resource beyond the sf::Event/
//     sf::RenderWindow types GameState's interface requires.
//   - ScreenFlow (the pure navigation policy) across SinglePlayer, Co-op,
//     and PvP.
//
// Neither ever constructs an sf::RenderWindow, sf::Texture, or sf::Font, so
// (unlike CollisionResolutionTests) this suite doesn't need a display and
// runs on headless CI.
//
// Uses a small always-on CHECK macro instead of assert(), since Release
// builds (as used in CI) define NDEBUG and would silently strip assert().

#include "States/ScreenFlow.hpp"
#include "States/StateManager.hpp"
#include <iostream>
#include <memory>
#include <string>
#include <vector>

static int g_failures = 0;

#define CHECK(cond, msg)                                                     \
  do {                                                                       \
    if (!(cond)) {                                                           \
      std::cerr << "FAIL: " << msg << " (" << __FILE__ << ":" << __LINE__    \
                << ")\n";                                                    \
      ++g_failures;                                                         \
    }                                                                        \
  } while (0)

namespace {

/// GameState double that just logs which lifecycle hooks fired, in order.
/// render()/handleEvent()/update() are no-ops — this suite never calls them
/// (renderStartIndex() lets the isTransparent() logic be checked without
/// ever invoking render()), they only exist to satisfy GameState's pure
/// virtuals.
class DummyState : public GameState {
public:
    DummyState(std::string name, std::vector<std::string>& log, bool transparent = false)
        : m_name(std::move(name)), m_log(log), m_transparent(transparent) {}

    void onEnter() override { m_log.push_back(m_name + ":onEnter"); }
    void onExit() override { m_log.push_back(m_name + ":onExit"); }
    void onPause() override { m_log.push_back(m_name + ":onPause"); }
    void onResume() override { m_log.push_back(m_name + ":onResume"); }
    bool isTransparent() const override { return m_transparent; }
    void handleEvent(const sf::Event&) override {}
    void update(float) override {}
    void render(sf::RenderWindow&) override {}

private:
    std::string               m_name;
    std::vector<std::string>& m_log;
    bool                      m_transparent;
};

std::unique_ptr<DummyState> dummy(const char* name, std::vector<std::string>& log,
                                   bool transparent = false) {
    return std::make_unique<DummyState>(name, log, transparent);
}

} // namespace

// ── StateManager ────────────────────────────────────────────────────────

static void testPushPopOrder() {
    std::vector<std::string> log;
    StateManager sm;

    sm.pushState(dummy("A", log));
    sm.processPending();
    CHECK((log == std::vector<std::string>{"A:onEnter"}), "pushing onto an empty stack just enters the new state");
    log.clear();

    sm.pushState(dummy("B", log));
    sm.processPending();
    CHECK((log == std::vector<std::string>{"A:onPause", "B:onEnter"}),
          "pushing over a state pauses it, then enters the new top");
    log.clear();

    sm.popState();
    sm.processPending();
    CHECK((log == std::vector<std::string>{"B:onExit", "A:onResume"}),
          "popping exits the top state and resumes whatever's beneath it");
}

static void testChangeStateReplacesTop() {
    std::vector<std::string> log;
    StateManager sm;
    sm.pushState(dummy("A", log));
    sm.processPending();
    log.clear();

    sm.changeState(dummy("B", log));
    sm.processPending();
    CHECK((log == std::vector<std::string>{"A:onExit", "B:onEnter"}),
          "changeState exits the old top and enters the replacement, not pause/resume");
    CHECK(sm.depth() == 1, "changeState keeps the stack at the same depth");
}

static void testClearStatesEmptiesStack() {
    std::vector<std::string> log;
    StateManager sm;
    sm.pushState(dummy("A", log));
    sm.pushState(dummy("B", log));
    sm.processPending();
    log.clear();

    sm.clearStates();
    sm.processPending();
    CHECK(sm.depth() == 0, "clearStates empties the stack");
    CHECK((log == std::vector<std::string>{"A:onExit", "B:onExit"}), "clearStates exits every remaining state");
}

static void testPopGuardKeepsLastState() {
    // This is the guard that directly fixes issue #18: Escape on a
    // selection screen must never be able to close the app by emptying
    // the stack.
    std::vector<std::string> log;
    StateManager sm;
    sm.pushState(dummy("Root", log));
    sm.processPending();
    CHECK(sm.depth() == 1, "sanity: one state on the stack");
    log.clear();

    sm.popState();
    sm.processPending();
    CHECK(sm.depth() == 1, "popState() is a no-op when only one state remains");
    CHECK(log.empty(), "no lifecycle hooks fire for a refused pop");
    CHECK(!sm.isEmpty(), "the stack is never left empty by popState() alone");
}

static void testPopOnEmptyStackIsSafe() {
    StateManager sm;
    sm.popState(); // nothing was ever pushed
    sm.processPending();
    CHECK(sm.depth() == 0, "popping an empty stack stays empty");
    CHECK(sm.isEmpty(), "...and doesn't crash doing it");
}

static void testRepeatedPushPopDoesNotLeak() {
    std::vector<std::string> log;
    StateManager sm;
    sm.pushState(dummy("Root", log));
    sm.processPending();
    const std::size_t baseline = sm.depth();

    for (int i = 0; i < 20; ++i) {
        sm.pushState(dummy("Temp", log));
        sm.processPending();
        sm.popState();
        sm.processPending();
    }

    CHECK(sm.depth() == baseline,
          "20 push/pop round trips return to the same depth — forward/back never leaks a stray state");
}

static void testRenderStartIndexSkipsCoveredOpaqueStates() {
    std::vector<std::string> log;
    StateManager sm;

    sm.pushState(dummy("Menu", log)); // opaque
    sm.processPending();
    CHECK(sm.renderStartIndex() == 0, "a single opaque state renders from itself");

    sm.pushState(dummy("CharSelect", log)); // opaque, fully covers Menu
    sm.processPending();
    CHECK(sm.renderStartIndex() == 1, "an opaque state on top hides everything beneath it");

    sm.pushState(dummy("Overlay", log, /*transparent=*/true));
    sm.processPending();
    CHECK(sm.renderStartIndex() == 1,
          "a transparent overlay (e.g. Pause) still renders the opaque state beneath it");
}

// ── ScreenFlow ──────────────────────────────────────────────────────────

static void testMenuOptionsMapToPushOrResetOrQuit() {
    using namespace ScreenFlow;

    CHECK(onMenuOption(0, false).op == Op::Push && onMenuOption(0, false).target == Screen::CharacterSelect,
          "1 Player pushes Character Select");
    CHECK(onMenuOption(1, false).op == Op::Push && onMenuOption(1, false).target == Screen::LevelSelect,
          "Co-op pushes Level Select directly, skipping Character Select");
    CHECK(onMenuOption(2, false).op == Op::ResetTo && onMenuOption(2, false).target == Screen::Playing,
          "PvP resets straight into gameplay");
    CHECK(onMenuOption(3, true).op == Op::ResetTo && onMenuOption(3, true).target == Screen::Playing,
          "Load Game resets into gameplay once the save has loaded");
    CHECK(onMenuOption(3, false).op == Op::None, "Load Game is a no-op when there's no save to load");
    CHECK(onMenuOption(4, false).op == Op::QuitApp, "Exit is the only menu option that requests quitting the app");
}

static void testSinglePlayerForwardBackMatchesCanonicalStack() {
    using namespace ScreenFlow;
    const GameMode mode = GameMode::SinglePlayer;

    CHECK(onConfirm(Screen::CharacterSelect, mode).op == Op::Push &&
              onConfirm(Screen::CharacterSelect, mode).target == Screen::LevelSelect,
          "confirming a character pushes Level Select");
    CHECK(onConfirm(Screen::LevelSelect, mode).op == Op::ResetTo &&
              onConfirm(Screen::LevelSelect, mode).target == Screen::Playing,
          "confirming a level resets into gameplay");

    CHECK(onBack(Screen::CharacterSelect).op == Op::Back, "Escape at Character Select goes back (-> Main Menu)");
    CHECK(onBack(Screen::LevelSelect).op == Op::Back, "Escape at Level Select goes back (-> Character Select)");

    const std::vector<Screen> expected = {Screen::MainMenu, Screen::CharacterSelect, Screen::LevelSelect};
    CHECK(canonicalStack(Screen::LevelSelect, mode) == expected,
          "SinglePlayer's canonical stack is Main Menu -> Character Select -> Level Select");
}

static void testCoopBackSkipsCharacterSelect() {
    using namespace ScreenFlow;
    const GameMode mode = GameMode::Coop;

    const std::vector<Screen> stack = canonicalStack(Screen::LevelSelect, mode);
    const std::vector<Screen> expected = {Screen::MainMenu, Screen::LevelSelect};
    CHECK(stack == expected, "Co-op's canonical stack skips Character Select entirely");

    // The invariant that makes Back correct without hardcoding "the screen
    // before this one": popping one screen off canonicalStack(current, mode)
    // must land on exactly canonicalStack(whatever's now on top, mode).
    const std::vector<Screen> afterBack(stack.begin(), stack.end() - 1);
    CHECK(afterBack == canonicalStack(Screen::MainMenu, mode),
          "back from Level Select in Co-op lands on Main Menu, not Character Select");
}

static void testPlayingIsAlwaysAFreshRoot() {
    using namespace ScreenFlow;
    const GameMode modes[] = {GameMode::SinglePlayer, GameMode::Coop, GameMode::PvP};
    const std::vector<Screen> expected = {Screen::Playing};

    for (GameMode mode : modes) {
        CHECK(canonicalStack(Screen::Playing, mode) == expected,
              "gameplay is always its own root regardless of game mode, so no selection screen leaks beneath it");
    }
}

static void testGameOverPrimaryTargetsMatchResult() {
    using namespace ScreenFlow;

    CHECK(onGameOverPrimary(GameResult::Won).op == Op::ResetTo &&
              onGameOverPrimary(GameResult::Won).target == Screen::CharacterSelect,
          "a SinglePlayer win resets to Character Select, so Escape still works afterwards");
    CHECK(onGameOverPrimary(GameResult::Lost).op == Op::ResetTo &&
              onGameOverPrimary(GameResult::Lost).target == Screen::Playing,
          "a loss retries by resetting straight into gameplay");
    CHECK(onGameOverPrimary(GameResult::P1Won).target == Screen::Playing &&
              onGameOverPrimary(GameResult::P2Won).target == Screen::Playing,
          "a PvP result replays the arena by resetting into gameplay");
}

static void testPauseOptionsMapToBackOrMainMenu() {
    using namespace ScreenFlow;

    CHECK(onPauseOption(0).op == Op::Back, "Resume goes back to the paused gameplay");
    CHECK(onPauseOption(1).op == Op::ResetTo && onPauseOption(1).target == Screen::MainMenu,
          "Quit to Menu resets to Main Menu");
}

static void testMainMenuHasNoBackTarget() {
    CHECK(ScreenFlow::onBack(ScreenFlow::Screen::MainMenu).op == ScreenFlow::Op::None,
          "Main Menu isn't reachable via Back — Escape there does nothing (only Exit/window-close quit)");
}

int main() {
    testPushPopOrder();
    testChangeStateReplacesTop();
    testClearStatesEmptiesStack();
    testPopGuardKeepsLastState();
    testPopOnEmptyStackIsSafe();
    testRepeatedPushPopDoesNotLeak();
    testRenderStartIndexSkipsCoveredOpaqueStates();

    testMenuOptionsMapToPushOrResetOrQuit();
    testSinglePlayerForwardBackMatchesCanonicalStack();
    testCoopBackSkipsCharacterSelect();
    testPlayingIsAlwaysAFreshRoot();
    testGameOverPrimaryTargetsMatchResult();
    testPauseOptionsMapToBackOrMainMenu();
    testMainMenuHasNoBackTarget();

    if (g_failures == 0) {
        std::cout << "All navigation tests passed.\n";
        return 0;
    }

    std::cerr << g_failures << " test(s) failed.\n";
    return 1;
}
