#include "States/StateManager.hpp"

void StateManager::pushState(std::unique_ptr<GameState> state) {
    m_pending.push_back({Action::Push, std::move(state)});
}

void StateManager::popState() {
    m_pending.push_back({Action::Pop, nullptr});
}

void StateManager::changeState(std::unique_ptr<GameState> state) {
    m_pending.push_back({Action::Change, std::move(state)});
}

void StateManager::clearStates() {
    m_pending.push_back({Action::Clear, nullptr});
}

void StateManager::processPending() {
    if (m_pending.empty()) return;

    // Swap to a local buffer before iterating: onEnter()/onExit()/onPause()
    // run mid-loop below, and if a future hook ever enqueues another pending
    // change from inside one of those callbacks, mutating m_pending while
    // this range-for holds it would be undefined behavior.
    std::vector<PendingChange> pending;
    pending.swap(m_pending);

    for (auto& change : pending) {
        switch (change.action) {
            case Action::Push:
                if (!m_states.empty()) {
                    // The state being covered stays alive underneath — let
                    // it know it's no longer the active/visible state.
                    m_states.back()->onPause();
                }
                change.state->onEnter();
                m_states.push_back(std::move(change.state));
                break;

            case Action::Pop:
                // Never pop the last state: an empty stack isn't a valid
                // "go back" target, it's the app having nothing left to
                // show. See StateManager.hpp's popState() doc.
                if (m_states.size() > 1) {
                    m_states.back()->onExit();
                    m_states.pop_back();
                    m_states.back()->onResume();
                }
                break;

            case Action::Change:
                if (!m_states.empty()) {
                    m_states.back()->onExit();
                    m_states.pop_back();
                }
                change.state->onEnter();
                m_states.push_back(std::move(change.state));
                break;

            case Action::Clear:
                for (auto& s : m_states) {
                    s->onExit();
                }
                m_states.clear();
                break;
        }
    }
}

void StateManager::handleEvent(const sf::Event& event) {
    if (!m_states.empty()) {
        m_states.back()->handleEvent(event);
    }
}

void StateManager::update(float dt) {
    if (!m_states.empty()) {
        m_states.back()->update(dt);
    }
}

std::size_t StateManager::renderStartIndex() const {
    // Find the topmost opaque state — everything below it is fully covered
    // and doesn't need to render. States above it (e.g. a transparent pause
    // overlay) are drawn afterwards, bottom to top, so they can see what's
    // beneath them. Kept separate from render() so this selection logic can
    // be unit-tested without needing a real render window (see
    // tests/NavigationTests.cpp).
    for (std::size_t i = m_states.size(); i-- > 0;) {
        if (!m_states[i]->isTransparent()) {
            return i;
        }
    }
    return 0;
}

void StateManager::render(sf::RenderWindow& window) {
    if (m_states.empty()) return;

    for (std::size_t i = renderStartIndex(); i < m_states.size(); ++i) {
        m_states[i]->render(window);
    }
}

bool StateManager::isEmpty() const {
    return m_states.empty() && m_pending.empty();
}

GameState* StateManager::currentState() const {
    if (m_states.empty()) return nullptr;
    return m_states.back().get();
}

std::size_t StateManager::depth() const {
    return m_states.size();
}
