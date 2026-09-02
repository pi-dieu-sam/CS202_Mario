#include "Observers/EventManager.hpp"
#include <algorithm>

EventManager& EventManager::getInstance() {
    static EventManager instance;
    return instance;
}

EventManager::SubscriptionHandle EventManager::subscribe(EventType type, Callback callback) {
    SubscriptionId id = m_nextId++;
    m_subscribers[type].push_back({id, std::move(callback)});
    return { type, id };
}

void EventManager::unsubscribe(const SubscriptionHandle& handle) {
    auto it = m_subscribers.find(handle.type);
    if (it == m_subscribers.end()) return;

    auto& entries = it->second;
    entries.erase(
        std::remove_if(entries.begin(), entries.end(),
            [&](const Entry& entry) { return entry.id == handle.id; }),
        entries.end());
}

void EventManager::publish(const GameEvent& event) {
    auto it = m_subscribers.find(event.type);
    if (it != m_subscribers.end()) {
        // Dispatch over a copy: a callback that subscribes or unsubscribes
        // (including its own ScopedEventSubscription being destroyed as a
        // side effect of handling this very event) must not invalidate the
        // iterator we're currently walking.
        auto entries = it->second;
        for (auto& entry : entries) {
            if (entry.callback) {
                entry.callback(event);
            }
        }
    }
}

void EventManager::clearAll() {
    m_subscribers.clear();
}

// ── ScopedEventSubscription ──

ScopedEventSubscription::ScopedEventSubscription(EventType type, EventManager::Callback callback)
    : m_handle(EventManager::getInstance().subscribe(type, std::move(callback))) {}

ScopedEventSubscription::~ScopedEventSubscription() {
    reset();
}

ScopedEventSubscription::ScopedEventSubscription(ScopedEventSubscription&& other) noexcept
    : m_handle(other.m_handle) {
    other.m_handle.reset();
}

ScopedEventSubscription& ScopedEventSubscription::operator=(ScopedEventSubscription&& other) noexcept {
    if (this != &other) {
        reset();
        m_handle = other.m_handle;
        other.m_handle.reset();
    }
    return *this;
}

void ScopedEventSubscription::reset() {
    if (m_handle) {
        EventManager::getInstance().unsubscribe(*m_handle);
        m_handle.reset();
    }
}
