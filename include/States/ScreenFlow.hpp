#pragma once
#include "Core/PlayerProgress.hpp"
#include <vector>

/// ScreenFlow — pure navigation policy for issue #18's back-navigation fix.
///
/// Deliberately has no SFML/window/StateManager dependency (same spirit as
/// Core/LevelCompletion.hpp) so the "which screen comes after/before which"
/// decisions can be unit-tested headless, without a render window. The
/// States/ classes are responsible for two things only: gathering the input
/// (which menu option, which key) and translating the Transition this
/// namespace returns into actual StateManager calls (see Navigator.hpp).
///
/// Navigation contract this namespace encodes:
///   - Push   — enter a screen that has a way back (has a Screen beneath it
///              on the stack).
///   - Back   — leave the current screen for whatever is beneath it. Never
///              empties the stack (StateManager::popState() already
///              refuses to pop the last state).
///   - Replace — swap the current screen for a same-depth one (e.g. level
///              N -> level N+1). Not modeled here; States call
///              StateManager::changeState() directly for these, since they
///              don't affect back-navigation.
///   - ResetTo — clear the stack and rebuild it as canonicalStack(target),
///              used whenever gameplay starts or the player backs all the
///              way out to a fresh root (e.g. "Quit to Menu").
///   - QuitApp — the only way the application is allowed to close outside
///              the window's own close button; see Game::requestExit().
namespace ScreenFlow {

enum class Screen {
    MainMenu,
    CharacterSelect,
    LevelSelect,
    Playing,
    Pause,
    GameOver,
};

/// Mirrors GameOverState's result — lives here (rather than
/// GameOverState.hpp) so the post-game-over navigation decision in
/// onGameOverPrimary() can be tested without pulling in SFML.
enum class GameResult {
    Lost,
    Won,
    P1Won,
    P2Won,
};

enum class Op {
    None,     ///< No navigation should happen.
    Push,     ///< StateManager::pushState(target).
    Back,     ///< StateManager::popState().
    Replace,  ///< StateManager::changeState(target) — not emitted by this
              ///< namespace today; declared for contract-completeness (see
              ///< the header comment above).
    ResetTo,  ///< StateManager::clearStates() + push canonicalStack(target).
    QuitApp,  ///< Game::requestExit().
};

struct Transition {
    Op     op     = Op::None;
    Screen target = Screen::MainMenu;
};

/// Main menu option selected (mouse click or Enter/Space), matching the
/// index order of MenuState::m_options: 0 = 1 Player, 1 = 2P Co-op,
/// 2 = 2P PvP, 3 = Load Game, 4 = Exit. `saveLoaded` is the result of
/// having already called SaveManager::loadGame() for option 3; it's
/// irrelevant for every other option.
inline Transition onMenuOption(int option, bool saveLoaded) {
    switch (option) {
        case 0: return {Op::Push, Screen::CharacterSelect};
        case 1: return {Op::Push, Screen::LevelSelect};
        case 2: return {Op::ResetTo, Screen::Playing};
        case 3: return saveLoaded ? Transition{Op::ResetTo, Screen::Playing}
                                   : Transition{Op::None, Screen::MainMenu};
        case 4: return {Op::QuitApp, Screen::MainMenu};
        default: return {Op::None, Screen::MainMenu};
    }
}

/// The player confirmed their choice on a selection screen (character or
/// level). `mode` is unused today — both selection screens always advance
/// to the same next screen regardless of game mode — but is accepted for
/// symmetry with onBack() and in case a mode-specific confirm target is
/// ever needed.
inline Transition onConfirm(Screen current, GameMode /*mode*/) {
    switch (current) {
        case Screen::CharacterSelect: return {Op::Push, Screen::LevelSelect};
        case Screen::LevelSelect:     return {Op::ResetTo, Screen::Playing};
        default:                      return {Op::None, current};
    }
}

/// Escape pressed on a selection screen. Always just "go back" — which
/// screen that resolves to depends on what's actually beneath it on the
/// stack (Navigator/StateManager handle that), not on game mode. This is
/// what makes back-navigation correct for both the SinglePlayer flow
/// (Menu -> CharacterSelect -> LevelSelect) and the Co-op flow
/// (Menu -> LevelSelect, skipping CharacterSelect) without hardcoding
/// either one.
inline Transition onBack(Screen current) {
    switch (current) {
        case Screen::CharacterSelect:
        case Screen::LevelSelect:
        case Screen::Pause:
            return {Op::Back, current};
        default:
            return {Op::None, current};
    }
}

/// GameOverState's primary action (top option — "New Game" / "Play Again" /
/// "Retry" depending on result). The secondary action ("Main Menu") is
/// always ResetTo(MainMenu) regardless of result, so it doesn't need a
/// policy function of its own.
inline Transition onGameOverPrimary(GameResult result) {
    switch (result) {
        case GameResult::Won:            return {Op::ResetTo, Screen::CharacterSelect};
        case GameResult::P1Won:
        case GameResult::P2Won:          return {Op::ResetTo, Screen::Playing};
        case GameResult::Lost:           return {Op::ResetTo, Screen::Playing};
        default:                         return {Op::ResetTo, Screen::MainMenu};
    }
}

/// PauseState option selected: 0 = Resume, 1 = Quit to Menu.
inline Transition onPauseOption(int option) {
    switch (option) {
        case 0: return {Op::Back, Screen::Pause};
        case 1: return {Op::ResetTo, Screen::MainMenu};
        default: return {Op::None, Screen::Pause};
    }
}

/// The stack that should exist once `screen` is the active/topmost state,
/// reached from a fresh start (i.e. what ResetTo(screen) should rebuild).
/// This is the single source of truth back-navigation is checked against:
/// popping one screen off canonicalStack(current, mode) must land on
/// canonicalStack's entry for whatever's actually beneath it.
inline std::vector<Screen> canonicalStack(Screen screen, GameMode mode) {
    switch (screen) {
        case Screen::MainMenu:
            return {Screen::MainMenu};
        case Screen::CharacterSelect:
            return {Screen::MainMenu, Screen::CharacterSelect};
        case Screen::LevelSelect:
            if (mode == GameMode::Coop) {
                return {Screen::MainMenu, Screen::LevelSelect};
            }
            return {Screen::MainMenu, Screen::CharacterSelect, Screen::LevelSelect};
        case Screen::Playing:
            // Gameplay is always a fresh root: no selection screen is ever
            // left underneath it, so nothing can leak when the run ends.
            return {Screen::Playing};
        case Screen::Pause:
        case Screen::GameOver:
        default:
            // Never a ResetTo target in their own right — Pause is pushed
            // on top of an already-running Playing state, and GameOver is
            // always followed by a ResetTo to some other screen.
            return {Screen::MainMenu};
    }
}

} // namespace ScreenFlow
