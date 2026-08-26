#pragma once
#include "States/ScreenFlow.hpp"

class StateManager;

/// Navigator — translates a ScreenFlow::Transition into actual
/// StateManager calls. This is the only place that turns a
/// ScreenFlow::Screen into a concrete GameState subclass, so individual
/// States never need to include each other's headers just to navigate —
/// they only depend on ScreenFlow (policy) and Navigator (execution).
namespace Navigator {

/// Apply `transition` against `sm`. `mode` is only consulted for
/// Op::ResetTo, to build the right ScreenFlow::canonicalStack(); pass the
/// current PlayerProgress::getGameMode().
void apply(const ScreenFlow::Transition& transition, StateManager& sm, GameMode mode);

} // namespace Navigator
