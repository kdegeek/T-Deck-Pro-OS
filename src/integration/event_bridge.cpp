/**
 * @file      event_bridge.cpp
 * @author    T-Deck-Pro OS Team
 * @license   MIT
 * @copyright Copyright (c) 2025
 * @date      2025-01-11
 * @brief     Event bridge system implementation
 */

#include "event_bridge.h"

// Global event bridge instance
EventBridge* GlobalEventBridge = nullptr;

// Event class implementation
Event::Event(EventType t, const String& src, EventPriority p)
    : type(t), priority(p), source(src), timestamp(millis()), handled(false) {
}

Event::Event(EventType t, const String& src, std::shared_ptr<EventData> d, EventPriority p)
    : type(t), priority(p), source(src), data(d), timestamp(millis()), handled(false) {
}

String Event::toString() const {
    String result = "Event{";
    result += "type=" + EventBridge::eventTypeToString(type);
    result += ", source=" + source;
    result += ", priority=" + EventBridge::eventPriorityToString(priority);
    result += ", timestamp=" + String(timestamp);
    result += ", handled=" + String(handled ? "true" : "false");
    if (data) {
        result += ", data=" + data->toString();
    }
    result += "}";
    return result;
}

// EventBridge class implementation
EventBridge::EventBridge()
    : initialized(false), max_queue_size(100), max_history_size(50),
      events_processed(0), events_dropped(0), processing_enabled(true),
      last_process_time(0), process_interval_ms(10) {
    // Don't log during static initialization - logger may not be ready
}

EventBridge::~EventBridge() {
    shutdown();
    LOG_INFO("EventBridge", "Event bridge destroyed");
}

bool EventBridge::initialize() {
    LOG_INFO("EventBridge", "Initializing event bridge...");
    
    if (initialized) {
        LOG_WARN("EventBridge", "Event bridge already initialized");
        return true;
    }
    
    // Clear any existing state
    subscriptions.clear();
    event_queue.clear();
    event_history.clear();
    events_processed = 0;
    events_dropped = 0;
    
    // Set global instance
    if (!GlobalEventBridge) {
        GlobalEventBridge = this;
    }
    
    initialized = true;
    LOG_INFO("EventBridge", "Event bridge initialized successfully");
    
    // Publish system startup event
    publishEvent(EventType::SYSTEM_STARTUP, "EventBridge", EventPriority::EVENT_HIGH);
    
    return true;
}

void EventBridge::shutdown() {
    if (!initialized) {
        return;
    }
    
    LOG_INFO("EventBridge", "Shutting down event bridge...");
    
    // Publish system shutdown event
    publishEvent(EventType::SYSTEM_SHUTDOWN, "EventBridge", EventPriority::EVENT_HIGH);
    
    // Process any remaining events
    processEvents();
    
    // Clear all subscriptions and events
    subscriptions.clear();
    event_queue.clear();
    event_history.clear();
    
    initialized = false;
    
    // Clear global instance if it's this bridge
    if (GlobalEventBridge == this) {
        GlobalEventBridge = nullptr;
    }
    
    LOG_INFO("EventBridge", "Event bridge shut down");
}

void EventBridge::publishEvent(const Event& event) {
    if (!initialized || !processing_enabled) {
        return;
    }
    
    // Check queue size limit
    if (event_queue.size() >= max_queue_size) {
        LOG_WARNF("EventBridge", "Event queue full, dropping event: %s", 
                  eventTypeToString(event.getType()).c_str());
        events_dropped++;
        return;
    }
    
    event_queue.push_back(event);
    
    LOG_DEBUGF("EventBridge", "Event published: %s from %s", 
               eventTypeToString(event.getType()).c_str(), 
               event.getSource().c_str());
}

void EventBridge::publishEvent(EventType type, const String& source, EventPriority priority) {
    publishEvent(Event(type, source, priority));
}

void EventBridge::publishEvent(EventType type, const String& source, std::shared_ptr<EventData> data, EventPriority priority) {
    publishEvent(Event(type, source, data, priority));
}

bool EventBridge::subscribe(const String& subscriber_id, EventType event_type, EventHandler handler, EventPriority min_priority) {
    if (!initialized) {
        LOG_ERROR("EventBridge", "Cannot subscribe: event bridge not initialized");
        return false;
    }
    
    LOG_INFOF("EventBridge", "Subscribing %s to event %s", 
              subscriber_id.c_str(), eventTypeToString(event_type).c_str());
    
    subscriptions[event_type].emplace_back(subscriber_id, event_type, handler, min_priority);
    return true;
}

bool EventBridge::unsubscribe(const String& subscriber_id, EventType event_type) {
    auto it = subscriptions.find(event_type);
    if (it == subscriptions.end()) {
        return false;
    }
    
    auto& subs = it->second;
    auto sub_it = std::remove_if(subs.begin(), subs.end(),
        [&subscriber_id](const EventSubscription& sub) {
            return sub.subscriber_id == subscriber_id;
        });
    
    if (sub_it != subs.end()) {
        subs.erase(sub_it, subs.end());
        LOG_INFOF("EventBridge", "Unsubscribed %s from event %s", 
                  subscriber_id.c_str(), eventTypeToString(event_type).c_str());
        return true;
    }
    
    return false;
}

bool EventBridge::unsubscribeAll(const String& subscriber_id) {
    bool found = false;
    
    for (auto& pair : subscriptions) {
        auto& subs = pair.second;
        auto sub_it = std::remove_if(subs.begin(), subs.end(),
            [&subscriber_id](const EventSubscription& sub) {
                return sub.subscriber_id == subscriber_id;
            });
        
        if (sub_it != subs.end()) {
            subs.erase(sub_it, subs.end());
            found = true;
        }
    }
    
    if (found) {
        LOG_INFOF("EventBridge", "Unsubscribed %s from all events", subscriber_id.c_str());
    }
    
    return found;
}

void EventBridge::update() {
    if (!initialized || !processing_enabled) {
        return;
    }
    
    uint32_t current_time = millis();
    if (current_time - last_process_time >= process_interval_ms) {
        processEvents();
        last_process_time = current_time;
    }
}

void EventBridge::processEvents() {
    if (!initialized || event_queue.empty()) {
        return;
    }
    
    // Process events in priority order (higher priority first)
    std::sort(event_queue.begin(), event_queue.end(),
        [](const Event& a, const Event& b) {
            return static_cast<int>(a.getPriority()) > static_cast<int>(b.getPriority());
        });
    
    // Process all events in queue
    while (!event_queue.empty()) {
        Event event = event_queue.front();
        event_queue.erase(event_queue.begin());
        
        processEvent(event);
        addToHistory(event);
        events_processed++;
    }
    
    trimHistory();
}

void EventBridge::processEvent(const Event& event) {
    auto it = subscriptions.find(event.getType());
    if (it == subscriptions.end()) {
        return; // No subscribers for this event type
    }
    
    LOG_DEBUGF("EventBridge", "Processing event: %s", eventTypeToString(event.getType()).c_str());
    
    for (const auto& subscription : it->second) {
        if (!subscription.active) {
            continue;
        }
        
        // Check priority filter
        if (static_cast<int>(event.getPriority()) < static_cast<int>(subscription.min_priority)) {
            continue;
        }
        
        try {
            subscription.handler(event);
        } catch (const std::exception& e) {
            LOG_ERRORF("EventBridge", "Exception in event handler for %s: %s", 
                       subscription.subscriber_id.c_str(), e.what());
        } catch (...) {
            LOG_ERRORF("EventBridge", "Unknown exception in event handler for %s", 
                       subscription.subscriber_id.c_str());
        }
    }
}

void EventBridge::addToHistory(const Event& event) {
    event_history.push_back(event);
    trimHistory();
}

void EventBridge::trimHistory() {
    while (event_history.size() > max_history_size) {
        event_history.erase(event_history.begin());
    }
}

size_t EventBridge::getSubscriptionCount() const {
    size_t count = 0;
    for (const auto& pair : subscriptions) {
        count += pair.second.size();
    }
    return count;
}

size_t EventBridge::getSubscriptionCount(EventType event_type) const {
    auto it = subscriptions.find(event_type);
    return it != subscriptions.end() ? it->second.size() : 0;
}

void EventBridge::printStatus() const {
    LOG_INFO("EventBridge", "=== Event Bridge Status ===");
    LOG_INFOF("EventBridge", "Initialized: %s", initialized ? "true" : "false");
    LOG_INFOF("EventBridge", "Processing enabled: %s", processing_enabled ? "true" : "false");
    LOG_INFOF("EventBridge", "Queue size: %d/%d", event_queue.size(), max_queue_size);
    LOG_INFOF("EventBridge", "History size: %d/%d", event_history.size(), max_history_size);
    LOG_INFOF("EventBridge", "Total subscriptions: %d", getSubscriptionCount());
    LOG_INFOF("EventBridge", "Events processed: %lu", events_processed);
    LOG_INFOF("EventBridge", "Events dropped: %lu", events_dropped);
}

String EventBridge::eventTypeToString(EventType type) {
    switch (type) {
        case EventType::HARDWARE_INITIALIZED: return "HARDWARE_INITIALIZED";
        case EventType::HARDWARE_ERROR: return "HARDWARE_ERROR";
        case EventType::DISPLAY_READY: return "DISPLAY_READY";
        case EventType::DISPLAY_ERROR: return "DISPLAY_ERROR";
        case EventType::TOUCH_PRESSED: return "TOUCH_PRESSED";
        case EventType::TOUCH_RELEASED: return "TOUCH_RELEASED";
        case EventType::WIFI_CONNECTED: return "WIFI_CONNECTED";
        case EventType::WIFI_DISCONNECTED: return "WIFI_DISCONNECTED";
        case EventType::SD_CARD_INSERTED: return "SD_CARD_INSERTED";
        case EventType::SD_CARD_REMOVED: return "SD_CARD_REMOVED";
        case EventType::BATTERY_LOW: return "BATTERY_LOW";
        case EventType::BATTERY_CHARGING: return "BATTERY_CHARGING";
        case EventType::SYSTEM_STARTUP: return "SYSTEM_STARTUP";
        case EventType::SYSTEM_SHUTDOWN: return "SYSTEM_SHUTDOWN";
        case EventType::SERVICE_STARTED: return "SERVICE_STARTED";
        case EventType::SERVICE_STOPPED: return "SERVICE_STOPPED";
        case EventType::SERVICE_ERROR: return "SERVICE_ERROR";
        case EventType::APP_LAUNCHED: return "APP_LAUNCHED";
        case EventType::APP_CLOSED: return "APP_CLOSED";
        case EventType::APP_ERROR: return "APP_ERROR";
        case EventType::MQTT_CONNECTED: return "MQTT_CONNECTED";
        case EventType::MQTT_DISCONNECTED: return "MQTT_DISCONNECTED";
        case EventType::MQTT_MESSAGE_RECEIVED: return "MQTT_MESSAGE_RECEIVED";
        case EventType::LORA_MESSAGE_RECEIVED: return "LORA_MESSAGE_RECEIVED";
        case EventType::GPS_LOCATION_UPDATE: return "GPS_LOCATION_UPDATE";
        case EventType::USER_INPUT: return "USER_INPUT";
        case EventType::MENU_SELECTED: return "MENU_SELECTED";
        case EventType::BUTTON_PRESSED: return "BUTTON_PRESSED";
        default: return "UNKNOWN_EVENT";
    }
}

String EventBridge::eventPriorityToString(EventPriority priority) {
    switch (priority) {
        case EventPriority::EVENT_LOW: return "LOW";
        case EventPriority::NORMAL: return "NORMAL";
        case EventPriority::EVENT_HIGH: return "HIGH";
        case EventPriority::CRITICAL: return "CRITICAL";
        default: return "UNKNOWN";
    }
}
