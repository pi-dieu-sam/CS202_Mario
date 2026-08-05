#pragma once
#include "GameEvents.hpp"
#include <cstdint>
#include <functional>
#include <optional>
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

    using Callback       = std::function<void(const GameEvent&)>;
    using SubscriptionId = std::uint64_t;

    /// Opaque handle returned by subscribe(), needed to unsubscribe later.
    struct SubscriptionHandle {
        EventType      type;
        SubscriptionId id = 0;
    };

    /// Subscribe to an event type. Returns a handle that can be passed to
    /// unsubscribe() to remove just this callback.
    SubscriptionHandle subscribe(EventType type, Callback callback);

    /// Remove a single subscription. No-op if it was already removed.
    void unsubscribe(const SubscriptionHandle& handle);

    /// Publish an event to all subscribers of that type.
    void publish(const GameEvent& event);

    /// Remove every subscriber of every event type. Prefer unsubscribe()
    /// (or ScopedEventSubscription) for per-owner cleanup; this is a blunt
    /// reset intended for full teardown scenarios.
    void clearAll();

private:
    EventManager() = default;

    struct Entry {
        SubscriptionId id;
        Callback       callback;
    };

    std::unordered_map<EventType, std::vector<Entry>> m_subscribers;
    SubscriptionId m_nextId = 1;
};

/// RAII wrapper around an EventManager subscription. Subscribes on
/// construction and unsubscribes on destruction (or an explicit reset()),
/// so an owner (a GameState, or any object) can hold one as a member and
/// have its subscription automatically cleaned up on the owner's own
/// lifetime, with no risk of a dangling callback.
class ScopedEventSubscription {
public:
    ScopedEventSubscription() = default;
    ScopedEventSubscription(EventType type, EventManager::Callback callback);
    ~ScopedEventSubscription();

    ScopedEventSubscription(const ScopedEventSubscription&)            = delete;
    ScopedEventSubscription& operator=(const ScopedEventSubscription&) = delete;

    ScopedEventSubscription(ScopedEventSubscription&& other) noexcept;
    ScopedEventSubscription& operator=(ScopedEventSubscription&& other) noexcept;

    /// Unsubscribe early. Safe to call multiple times.
    void reset();

private:
    std::optional<EventManager::SubscriptionHandle> m_handle;
};
