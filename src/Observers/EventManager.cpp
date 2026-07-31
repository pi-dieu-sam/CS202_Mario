#include "Observers/EventManager.hpp"

EventManager& EventManager::getInstance() {
    static EventManager instance;
    return instance;
}

void EventManager::subscribe(EventType type, Callback callback) {
    m_subscribers[type].push_back(std::move(callback));
}

void EventManager::publish(const GameEvent& event) {
    auto it = m_subscribers.find(event.type);
    if (it != m_subscribers.end()) {
        for (auto& callback : it->second) {
            callback(event);
        }
    }
}

void EventManager::clearAll() {
    m_subscribers.clear();
}
