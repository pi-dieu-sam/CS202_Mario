#pragma once
#include "GameEvents.hpp"
#include <functional>
#include <unordered_map>
#include <vector>

/// EventManager — Observer pattern.
/// Components subscribe to event types and get notified when those events are published.
/// This decouples gameplay logic from UI, sound, and score tracking.
class EventManager {
public:
    static EventManager& getInstance();

    EventManager(const EventManager&)            = delete;
    EventManager& operator=(const EventManager&) = delete;

    using Callback = std::function<void(const GameEvent&)>;

    /// Subscribe to an event type.
    void subscribe(EventType type, Callback callback);

    /// Publish an event to all subscribers of that type.
    void publish(const GameEvent& event);

    /// Clear all subscriptions (useful on state change).
    void clearAll();

private:
    EventManager() = default;

    std::unordered_map<EventType, std::vector<Callback>> m_subscribers;
};
