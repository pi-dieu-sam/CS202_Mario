#include "States/Navigator.hpp"
#include "States/StateManager.hpp"
#include "States/MenuState.hpp"
#include "States/CharacterSelectState.hpp"
#include "States/LevelSelectState.hpp"
#include "States/PlayingState.hpp"
#include "States/SaveSlotState.hpp"
#include "Core/Game.hpp"
#include <memory>

namespace {

/// Screens ResetTo/Push actually construct. Pause and GameOver are
/// deliberately excluded — Pause is always pushed directly by PlayingState
/// (it isn't reached through this factory), and GameOver states carry
/// constructor arguments (result, winner name) only the state that creates
/// them knows, so they're never a Navigator target either.
std::unique_ptr<GameState> createScreen(ScreenFlow::Screen screen) {
    switch (screen) {
        case ScreenFlow::Screen::MainMenu:
            return std::make_unique<MenuState>();
        case ScreenFlow::Screen::SaveSlots:
            return std::make_unique<SaveSlotState>(SaveSlotMode::Load);
        case ScreenFlow::Screen::CharacterSelect:
            return std::make_unique<CharacterSelectState>();
        case ScreenFlow::Screen::LevelSelect:
            return std::make_unique<LevelSelectState>();
        case ScreenFlow::Screen::Playing:
            return std::make_unique<PlayingState>();
        case ScreenFlow::Screen::Pause:
        case ScreenFlow::Screen::GameOver:
        default:
            return nullptr;
    }
}

} // namespace

void Navigator::apply(const ScreenFlow::Transition& transition, StateManager& sm, GameMode mode) {
    switch (transition.op) {
        case ScreenFlow::Op::None:
            break;

        case ScreenFlow::Op::Push:
            if (auto state = createScreen(transition.target)) {
                sm.pushState(std::move(state));
            }
            break;

        case ScreenFlow::Op::Back:
            sm.popState();
            break;

        case ScreenFlow::Op::Replace:
            if (auto state = createScreen(transition.target)) {
                sm.changeState(std::move(state));
            }
            break;

        case ScreenFlow::Op::ResetTo:
            sm.clearStates();
            for (ScreenFlow::Screen screen : ScreenFlow::canonicalStack(transition.target, mode)) {
                if (auto state = createScreen(screen)) {
                    sm.pushState(std::move(state));
                }
            }
            break;

        case ScreenFlow::Op::QuitApp:
            Game::getInstance().requestExit();
            break;
    }
}
