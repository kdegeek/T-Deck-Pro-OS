/**
 * @file      event_bridge.h
 * @author    T-Deck-Pro OS Team
 * @license   MIT
 * @copyright Copyright (c) 2025
 * @date      2025-01-11
 * @brief     Event bridge system for T-Deck-Pro OS communication
 */

#ifndef EVENT_BRIDGE_H
#define EVENT_BRIDGE_H

// Only compile if integration layer is enabled
#ifdef INTEGRATION_LAYER_ENABLED

#include <Arduino.h>
#include <vector>
#include <map>
#include <functional>
#include <memory>
#include "simple_logger.h"
#include "service_container.h"

// Forward declarations
class EventBridge;
class Event;

/**
 * @brief Event Types
 * 
 * Enumeration of all event types in the system
 */
enum class EventType {
    // Hardware events
    HARDWARE_INITIALIZED,
    HARDWARE_ERROR,
    DISPLAY_READY,
    DISPLAY_ERROR,
    TOUCH_PRESSED,
    TOUCH_RELEASED,
    WIFI_CONNECTED,
    WIFI_DISCONNECTED,
    SD_CARD_INSERTED,
    SD_CARD_REMOVED,
    BATTERY_LOW,
    BATTERY_CHARGING,
    
    // System events
    SYSTEM_STARTUP,
    SYSTEM_SHUTDOWN,
    SERVICE_STARTED,
    SERVICE_STOPPED,
    SERVICE_ERROR,
    
    // Application events
    APP_LAUNCHED,
    APP_CLOSED,
    APP_ERROR,
    
    // Communication events
    MQTT_CONNECTED,
    MQTT_DISCONNECTED,
    MQTT_MESSAGE_RECEIVED,
    LORA_MESSAGE_RECEIVED,
    GPS_LOCATION_UPDATE,
    
    // User events
    USER_INPUT,
    MENU_SELECTED,
    BUTTON_PRESSED,
    
    // Custom events (for extensibility)
    CUSTOM_EVENT_BASE = 1000
};

/**
 * @brief Event Priority Levels
 */
enum class EventPriority {
    EVENT_LOW = 0,
    NORMAL = 1,
    EVENT_HIGH = 2,
    CRITICAL = 3
};

/**
 * @brief Event Data Base Class
 * 
 * Base class for all event data payloads
 */
class EventData {
public:
    virtual ~EventData() = default;
    virtual String toString() const { return "EventData"; }
};

/**
 * @brief Generic Event Data Template
 * 
 * Template for strongly-typed event data
 */
template<typename T>
class TypedEventData : public EventData {
private:
    T data;
    
public:
    TypedEventData(const T& d) : data(d) {}
    
    const T& getData() const { return data; }
    T& getData() { return data; }
    
    String toString() const override {
        return "TypedEventData<" + String(typeid(T).name()) + ">";
    }
};

/**
 * @brief Event Class
 * 
 * Represents a single event in the system
 */
class Event {
private:
    EventType type;
    EventPriority priority;
    String source;
    uint32_t timestamp;
    std::shared_ptr<EventData> data;
    bool handled;
    
public:
    Event(EventType t, const String& src, EventPriority p = EventPriority::NORMAL);
    Event(EventType t, const String& src, std::shared_ptr<EventData> d, EventPriority p = EventPriority::NORMAL);
    
    // Getters
    EventType getType() const { return type; }
    EventPriority getPriority() const { return priority; }
    const String& getSource() const { return source; }
    uint32_t getTimestamp() const { return timestamp; }
    std::shared_ptr<EventData> getData() const { return data; }
    bool isHandled() const { return handled; }
    
    // Setters
    void setHandled(bool h = true) { handled = h; }
    
    // Utility methods
    template<typename T>
    std::shared_ptr<TypedEventData<T>> getTypedData() const;
    
    String toString() const;
};

/**
 * @brief Event Handler Function Type
 */
typedef std::function<void(const Event&)> EventHandler;

/**
 * @brief Event Subscription Information
 */
struct EventSubscription {
    String subscriber_id;
    EventType event_type;
    EventHandler handler;
    EventPriority min_priority;
    bool active;
    
    EventSubscription(const String& id, EventType type, EventHandler h, EventPriority min_p = EventPriority::EVENT_LOW)
        : subscriber_id(id), event_type(type), handler(h), min_priority(min_p), active(true) {}
};

/**
 * @brief Event Bridge Class
 * 
 * Central event management system for T-Deck-Pro OS
 * Implements publish/subscribe pattern for loose coupling
 */
class EventBridge : public IService {
private:
    std::map<EventType, std::vector<EventSubscription>> subscriptions;
    std::vector<Event> event_queue;
    std::vector<Event> event_history;
    
    bool initialized;
    size_t max_queue_size;
    size_t max_history_size;
    uint32_t events_processed;
    uint32_t events_dropped;
    
    // Processing control
    bool processing_enabled;
    uint32_t last_process_time;
    uint32_t process_interval_ms;
    
public:
    EventBridge();
    ~EventBridge() override;
    
    // IService implementation
    bool initialize() override;
    void shutdown() override;
    const char* getServiceName() const override { return "EventBridge"; }
    bool isInitialized() const override { return initialized; }
    
    // Event publishing
    void publishEvent(const Event& event);
    void publishEvent(EventType type, const String& source, EventPriority priority = EventPriority::NORMAL);
    void publishEvent(EventType type, const String& source, std::shared_ptr<EventData> data, EventPriority priority = EventPriority::NORMAL);
    
    // Template for easy typed event publishing
    template<typename T>
    void publishTypedEvent(EventType type, const String& source, const T& data, EventPriority priority = EventPriority::NORMAL);
    
    // Event subscription
    bool subscribe(const String& subscriber_id, EventType event_type, EventHandler handler, EventPriority min_priority = EventPriority::EVENT_LOW);
    bool unsubscribe(const String& subscriber_id, EventType event_type);
    bool unsubscribeAll(const String& subscriber_id);
    
    // Event processing
    void processEvents();
    void processEvent(const Event& event);
    void update(); // Called from main loop

    // Configuration
    void setMaxQueueSize(size_t size) { max_queue_size = size; }
    void setMaxHistorySize(size_t size) { max_history_size = size; }
    void setProcessInterval(uint32_t interval_ms) { process_interval_ms = interval_ms; }
    void setProcessingEnabled(bool enabled) { processing_enabled = enabled; }
    
    // Status and diagnostics
    size_t getQueueSize() const { return event_queue.size(); }
    size_t getHistorySize() const { return event_history.size(); }
    size_t getSubscriptionCount() const;
    size_t getSubscriptionCount(EventType event_type) const;
    uint32_t getEventsProcessed() const { return events_processed; }
    uint32_t getEventsDropped() const { return events_dropped; }
    
    void printStatus() const;
    void printSubscriptions() const;
    
    // Utility methods
    static String eventTypeToString(EventType type);
    static String eventPriorityToString(EventPriority priority);
    
private:
    void addToHistory(const Event& event);
    void trimQueue();
    void trimHistory();
};

/**
 * @brief Global Event Bridge Instance
 */
extern EventBridge* GlobalEventBridge;

// Convenience macros for event operations
#define PUBLISH_EVENT(type, source) \
    if (GlobalEventBridge) GlobalEventBridge->publishEvent(type, source)

#define PUBLISH_EVENT_WITH_PRIORITY(type, source, priority) \
    if (GlobalEventBridge) GlobalEventBridge->publishEvent(type, source, priority)

#define PUBLISH_TYPED_EVENT(type, source, data) \
    if (GlobalEventBridge) GlobalEventBridge->publishTypedEvent(type, source, data)

#define SUBSCRIBE_TO_EVENT(subscriber, event_type, handler) \
    (GlobalEventBridge ? GlobalEventBridge->subscribe(subscriber, event_type, handler) : false)

#define UNSUBSCRIBE_FROM_EVENT(subscriber, event_type) \
    (GlobalEventBridge ? GlobalEventBridge->unsubscribe(subscriber, event_type) : false)

// Template implementations
template<typename T>
std::shared_ptr<TypedEventData<T>> Event::getTypedData() const {
    return std::dynamic_pointer_cast<TypedEventData<T>>(data);
}

template<typename T>
void EventBridge::publishTypedEvent(EventType type, const String& source, const T& data, EventPriority priority) {
    auto typed_data = std::make_shared<TypedEventData<T>>(data);
    publishEvent(Event(type, source, typed_data, priority));
}

#endif // INTEGRATION_LAYER_ENABLED

#endif // EVENT_BRIDGE_H
