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
    for (auto& change : m_pending) {
        switch (change.action) {
            case Action::Push:
                if (!m_states.empty()) {
                    // Don't call onExit for pushed-over state (it's still alive underneath)
                }
                change.state->onEnter();
                m_states.push_back(std::move(change.state));
                break;

            case Action::Pop:
                if (!m_states.empty()) {
                    m_states.back()->onExit();
                    m_states.pop_back();
                    if (!m_states.empty()) {
                        m_states.back()->onResume();
                    }
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
    m_pending.clear();
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

void StateManager::render(sf::RenderWindow& window) {
    // Render all states from bottom to top (for overlay support)
    for (auto& state : m_states) {
        state->render(window);
    }
}

bool StateManager::isEmpty() const {
    return m_states.empty() && m_pending.empty();
}

GameState* StateManager::currentState() const {
    if (m_states.empty()) return nullptr;
    return m_states.back().get();
}
