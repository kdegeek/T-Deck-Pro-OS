/**
 * @file mqtt_manager.h
 * @brief T-Deck-Pro MQTT Manager - Communication and notification system
 * @author T-Deck-Pro OS Team
 * @date 2025
 */

#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <queue>
#include "../config/os_config.h"

/**
 * @brief MQTT message structure
 */
struct MQTTMessage {
    String topic;
    String payload;
    uint32_t timestamp;
    bool retained;
    int qos;
};

/**
 * @brief App launch request from MQTT
 */
struct AppLaunchRequest {
    String app_name;
    String parameters;
    String sender;
    uint32_t timestamp;
    bool processed;
};

/**
 * @brief Notification message
 */
struct NotificationMessage {
    String app_name;
    String title;
    String message;
    String icon;
    int priority; // 1=low, 2=normal, 3=high, 4=urgent
    uint32_t timestamp;
    bool persistent;
    bool shown;
};

/**
 * @brief MQTT connection status
 */
enum class MQTTStatus {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    ERROR,
    MQTT_DISABLED
};

/**
 * @brief MQTT Manager handles all MQTT communication
 */
class MQTTManager {
public:
    MQTTManager();
    ~MQTTManager();
    
    /**
     * @brief Initialize MQTT manager
     * @return true if successful
     */
    bool initialize();
    
    /**
     * @brief Update MQTT manager (call periodically)
     */
    void update();
    
    /**
     * @brief Connect to MQTT broker
     * @param broker_host MQTT broker hostname/IP
     * @param broker_port MQTT broker port
     * @param username Username (optional)
     * @param password Password (optional)
     * @return true if connection initiated
     */
    bool connect(const String& broker_host, int broker_port = 1883, 
                const String& username = "", const String& password = "");
    
    /**
     * @brief Disconnect from MQTT broker
     */
    void disconnect();
    
    /**
     * @brief Check if connected to MQTT broker
     * @return true if connected
     */
    bool is_connected() const;
    
    /**
     * @brief Get current MQTT status
     * @return MQTT status
     */
    MQTTStatus get_status() const;
    
    /**
     * @brief Publish message to MQTT topic
     * @param topic Topic to publish to
     * @param payload Message payload
     * @param retained Retain message
     * @param qos Quality of service (0, 1, or 2)
     * @return true if published successfully
     */
    bool publish(const String& topic, const String& payload, bool retained = false, int qos = 0);
    
    /**
     * @brief Subscribe to MQTT topic
     * @param topic Topic to subscribe to
     * @param qos Quality of service
     * @return true if subscribed successfully
     */
    bool subscribe(const String& topic, int qos = 0);
    
    /**
     * @brief Unsubscribe from MQTT topic
     * @param topic Topic to unsubscribe from
     * @return true if unsubscribed successfully
     */
    bool unsubscribe(const String& topic);
    
    /**
     * @brief Send device telemetry
     * @param telemetry_data JSON telemetry data
     * @return true if sent successfully
     */
    bool send_telemetry(const String& telemetry_data);
    
    /**
     * @brief Send device status update
     * @param status_data JSON status data
     * @return true if sent successfully
     */
    bool send_status_update(const String& status_data);
    
    /**
     * @brief Send app launch result
     * @param app_name App that was launched
     * @param success Whether launch was successful
     * @param error_message Error message if failed
     * @return true if sent successfully
     */
    bool send_app_launch_result(const String& app_name, bool success, const String& error_message = "");
    
    /**
     * @brief Get pending app launch request
     * @return App name to launch, or empty string if none
     */
    String get_pending_app_request();
    
    /**
     * @brief Get pending notifications
     * @return Vector of pending notifications
     */
    std::vector<NotificationMessage> get_pending_notifications();
    
    /**
     * @brief Mark notification as shown
     * @param notification_id Notification ID
     */
    void mark_notification_shown(const String& notification_id);
    
    /**
     * @brief Clear all notifications for an app
     * @param app_name App name
     */
    void clear_app_notifications(const String& app_name);
    
    /**
     * @brief Get notification count for an app
     * @param app_name App name
     * @return Number of unread notifications
     */
    int get_app_notification_count(const String& app_name);
    
    /**
     * @brief Set device ID for MQTT topics
     * @param device_id Unique device identifier
     */
    void set_device_id(const String& device_id);
    
    /**
     * @brief Get device ID
     * @return Device ID
     */
    String get_device_id() const;
    
    /**
     * @brief Get MQTT statistics
     * @return JSON string with MQTT statistics
     */
    String get_statistics() const;
    
private:
    /**
     * @brief MQTT message callback
     * @param topic Message topic
     * @param payload Message payload
     * @param length Payload length
     */
    static void mqtt_callback(char* topic, byte* payload, unsigned int length);
    
    /**
     * @brief Handle incoming MQTT message
     * @param topic Message topic
     * @param payload Message payload
     */
    void handle_message(const String& topic, const String& payload);
    
    /**
     * @brief Handle app launch command
     * @param payload JSON payload with app launch request
     */
    void handle_app_launch_command(const String& payload);
    
    /**
     * @brief Handle notification message
     * @param payload JSON payload with notification
     */
    void handle_notification(const String& payload);
    
    /**
     * @brief Handle system command
     * @param payload JSON payload with system command
     */
    void handle_system_command(const String& payload);
    
    /**
     * @brief Subscribe to default topics
     */
    void subscribe_to_default_topics();
    
    /**
     * @brief Generate device-specific topic
     * @param base_topic Base topic name
     * @return Full topic with device ID
     */
    String get_device_topic(const String& base_topic) const;
    
    /**
     * @brief Reconnect to MQTT broker
     * @return true if reconnection successful
     */
    bool reconnect();
    
    /**
     * @brief Process message queue
     */
    void process_message_queue();
    
    /**
     * @brief Clean up old messages and notifications
     */
    void cleanup_old_data();
    
    // MQTT client
    WiFiClient wifi_client;
    PubSubClient mqtt_client;
    
    // Connection settings
    String broker_host;
    int broker_port;
    String username;
    String password;
    String device_id;
    
    // Status
    MQTTStatus status;
    uint32_t last_connect_attempt;
    uint32_t last_heartbeat;
    uint32_t connect_retry_count;
    
    // Message queues
    std::queue<MQTTMessage> outgoing_messages;
    std::queue<AppLaunchRequest> app_launch_requests;
    std::vector<NotificationMessage> notifications;
    
    // Statistics
    struct MQTTStats {
        uint32_t messages_sent;
        uint32_t messages_received;
        uint32_t connection_attempts;
        uint32_t successful_connections;
        uint32_t last_disconnect_time;
        String last_error;
    } stats;
    
    // Configuration
    static const uint32_t RECONNECT_INTERVAL = 5000;  // 5 seconds
    static const uint32_t HEARTBEAT_INTERVAL = 30000; // 30 seconds
    static const uint32_t MESSAGE_TIMEOUT = 300000;   // 5 minutes
    static const int MAX_RECONNECT_ATTEMPTS = 10;
    static const int MAX_OUTGOING_MESSAGES = 50;
    static const int MAX_NOTIFICATIONS = 100;
    
    // Static instance for callback
    static MQTTManager* instance;
};

#endif // MQTT_MANAGER_H