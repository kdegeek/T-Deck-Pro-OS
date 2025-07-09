/**
 * @file mqtt_manager.cpp
 * @brief T-Deck-Pro MQTT Manager Implementation
 * @author T-Deck-Pro OS Team
 * @date 2025
 */

#include "mqtt_manager.h"
#include <WiFi.h>
#include <esp_system.h>

// Static instance for callback
MQTTManager* MQTTManager::instance = nullptr;

MQTTManager::MQTTManager() : 
    mqtt_client(wifi_client),
    broker_port(1883),
    status(MQTTStatus::DISCONNECTED),
    last_connect_attempt(0),
    last_heartbeat(0),
    connect_retry_count(0) {
    
    // Set static instance for callback
    instance = this;
    
    // Initialize statistics
    memset(&stats, 0, sizeof(stats));
    
    // Generate device ID if not set
    if (device_id.isEmpty()) {
        uint64_t chipid = ESP.getEfuseMac();
        device_id = "tdeck-" + String((uint32_t)(chipid >> 32), HEX) + String((uint32_t)chipid, HEX);
    }
}

MQTTManager::~MQTTManager() {
    disconnect();
    instance = nullptr;
}

bool MQTTManager::initialize() {
    Serial.println("[MQTT] Initializing MQTT Manager");
    
    // Set MQTT callback
    mqtt_client.setCallback(mqtt_callback);
    
    // Set buffer size for larger messages
    mqtt_client.setBufferSize(2048);
    
    Serial.printf("[MQTT] Device ID: %s\n", device_id.c_str());
    
    return true;
}

void MQTTManager::update() {
    uint32_t now = millis();
    
    // Handle MQTT client loop
    if (mqtt_client.connected()) {
        mqtt_client.loop();
        
        // Send heartbeat
        if (now - last_heartbeat > HEARTBEAT_INTERVAL) {
            send_status_update("{\"status\":\"online\",\"uptime\":" + String(now) + "}");
            last_heartbeat = now;
        }
    } else if (status == MQTTStatus::CONNECTED) {
        // Connection lost
        status = MQTTStatus::DISCONNECTED;
        Serial.println("[MQTT] Connection lost");
        stats.last_disconnect_time = now;
    }
    
    // Auto-reconnect
    if (status == MQTTStatus::DISCONNECTED && 
        !broker_host.isEmpty() && 
        now - last_connect_attempt > RECONNECT_INTERVAL &&
        connect_retry_count < MAX_RECONNECT_ATTEMPTS) {
        
        Serial.println("[MQTT] Attempting to reconnect...");
        reconnect();
    }
    
    // Process outgoing message queue
    process_message_queue();
    
    // Clean up old data
    if (now % 60000 == 0) { // Every minute
        cleanup_old_data();
    }
}

bool MQTTManager::connect(const String& broker_host, int broker_port, 
                         const String& username, const String& password) {
    
    this->broker_host = broker_host;
    this->broker_port = broker_port;
    this->username = username;
    this->password = password;
    
    return reconnect();
}

bool MQTTManager::reconnect() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[MQTT] WiFi not connected");
        return false;
    }
    
    last_connect_attempt = millis();
    stats.connection_attempts++;
    
    Serial.printf("[MQTT] Connecting to %s:%d\n", broker_host.c_str(), broker_port);
    
    mqtt_client.setServer(broker_host.c_str(), broker_port);
    
    String client_id = device_id + "-" + String(random(0xffff), HEX);
    
    bool connected = false;
    if (username.isEmpty()) {
        connected = mqtt_client.connect(client_id.c_str());
    } else {
        connected = mqtt_client.connect(client_id.c_str(), username.c_str(), password.c_str());
    }
    
    if (connected) {
        status = MQTTStatus::CONNECTED;
        connect_retry_count = 0;
        stats.successful_connections++;
        
        Serial.println("[MQTT] Connected successfully");
        
        // Subscribe to default topics
        subscribe_to_default_topics();
        
        // Send online status
        send_status_update("{\"status\":\"online\",\"device_id\":\"" + device_id + "\"}");
        
        return true;
    } else {
        status = MQTTStatus::ERROR;
        connect_retry_count++;
        
        String error = "Connection failed, rc=" + String(mqtt_client.state());
        stats.last_error = error;
        Serial.println("[MQTT] " + error);
        
        return false;
    }
}

void MQTTManager::disconnect() {
    if (mqtt_client.connected()) {
        // Send offline status
        send_status_update("{\"status\":\"offline\"}");
        mqtt_client.disconnect();
    }
    status = MQTTStatus::DISCONNECTED;
    Serial.println("[MQTT] Disconnected");
}

bool MQTTManager::is_connected() const {
    return const_cast<PubSubClient&>(mqtt_client).connected();
}

MQTTStatus MQTTManager::get_status() const {
    return status;
}

bool MQTTManager::publish(const String& topic, const String& payload, bool retained, int qos) {
    if (!mqtt_client.connected()) {
        // Queue message for later
        if (outgoing_messages.size() < MAX_OUTGOING_MESSAGES) {
            MQTTMessage msg = {topic, payload, millis(), retained, qos};
            outgoing_messages.push(msg);
        }
        return false;
    }
    
    bool result = mqtt_client.publish(topic.c_str(), payload.c_str(), retained);
    if (result) {
        stats.messages_sent++;
        Serial.printf("[MQTT] Published to %s: %s\n", topic.c_str(), payload.c_str());
    } else {
        Serial.printf("[MQTT] Failed to publish to %s\n", topic.c_str());
    }
    
    return result;
}

bool MQTTManager::subscribe(const String& topic, int qos) {
    if (!mqtt_client.connected()) {
        return false;
    }
    
    bool result = mqtt_client.subscribe(topic.c_str(), qos);
    if (result) {
        Serial.printf("[MQTT] Subscribed to %s\n", topic.c_str());
    } else {
        Serial.printf("[MQTT] Failed to subscribe to %s\n", topic.c_str());
    }
    
    return result;
}

bool MQTTManager::unsubscribe(const String& topic) {
    if (!mqtt_client.connected()) {
        return false;
    }
    
    bool result = mqtt_client.unsubscribe(topic.c_str());
    if (result) {
        Serial.printf("[MQTT] Unsubscribed from %s\n", topic.c_str());
    }
    
    return result;
}

bool MQTTManager::send_telemetry(const String& telemetry_data) {
    String topic = get_device_topic(MQTT_TOPIC_TELEMETRY);
    return publish(topic, telemetry_data);
}

bool MQTTManager::send_status_update(const String& status_data) {
    String topic = get_device_topic(MQTT_TOPIC_STATUS);
    return publish(topic, status_data, true); // Retained
}

bool MQTTManager::send_app_launch_result(const String& app_name, bool success, const String& error_message) {
    DynamicJsonDocument doc(512);
    doc["app"] = app_name;
    doc["success"] = success;
    doc["timestamp"] = millis();
    
    if (!success && !error_message.isEmpty()) {
        doc["error"] = error_message;
    }
    
    String payload;
    serializeJson(doc, payload);
    
    String topic = get_device_topic(MQTT_TOPIC_APP_RESULT);
    return publish(topic, payload);
}

String MQTTManager::get_pending_app_request() {
    if (app_launch_requests.empty()) {
        return "";
    }
    
    AppLaunchRequest request = app_launch_requests.front();
    app_launch_requests.pop();
    
    return request.app_name;
}

std::vector<NotificationMessage> MQTTManager::get_pending_notifications() {
    std::vector<NotificationMessage> pending;
    
    for (const auto& notification : notifications) {
        if (!notification.shown) {
            pending.push_back(notification);
        }
    }
    
    return pending;
}

void MQTTManager::mark_notification_shown(const String& notification_id) {
    for (auto& notification : notifications) {
        if (notification.app_name == notification_id) {
            notification.shown = true;
            break;
        }
    }
}

void MQTTManager::clear_app_notifications(const String& app_name) {
    notifications.erase(
        std::remove_if(notifications.begin(), notifications.end(),
            [&app_name](const NotificationMessage& n) {
                return n.app_name == app_name;
            }),
        notifications.end());
}

int MQTTManager::get_app_notification_count(const String& app_name) {
    int count = 0;
    for (const auto& notification : notifications) {
        if (notification.app_name == app_name && !notification.shown) {
            count++;
        }
    }
    return count;
}

void MQTTManager::set_device_id(const String& device_id) {
    this->device_id = device_id;
}

String MQTTManager::get_device_id() const {
    return device_id;
}

String MQTTManager::get_statistics() const {
    DynamicJsonDocument doc(512);
    doc["status"] = (int)status;
    doc["connected"] = const_cast<PubSubClient&>(mqtt_client).connected();
    doc["messages_sent"] = stats.messages_sent;
    doc["messages_received"] = stats.messages_received;
    doc["connection_attempts"] = stats.connection_attempts;
    doc["successful_connections"] = stats.successful_connections;
    doc["last_disconnect"] = stats.last_disconnect_time;
    doc["last_error"] = stats.last_error;
    doc["pending_notifications"] = notifications.size();
    doc["pending_app_requests"] = app_launch_requests.size();
    
    String result;
    serializeJson(doc, result);
    return result;
}

void MQTTManager::mqtt_callback(char* topic, byte* payload, unsigned int length) {
    if (instance) {
        String topic_str = String(topic);
        String payload_str = "";
        
        for (unsigned int i = 0; i < length; i++) {
            payload_str += (char)payload[i];
        }
        
        instance->handle_message(topic_str, payload_str);
    }
}

void MQTTManager::handle_message(const String& topic, const String& payload) {
    stats.messages_received++;
    
    Serial.printf("[MQTT] Received: %s -> %s\n", topic.c_str(), payload.c_str());
    
    // Route message based on topic
    if (topic.endsWith("/app/launch")) {
        handle_app_launch_command(payload);
    } else if (topic.endsWith("/notification")) {
        handle_notification(payload);
    } else if (topic.endsWith("/system/command")) {
        handle_system_command(payload);
    }
}

void MQTTManager::handle_app_launch_command(const String& payload) {
    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, payload);
    
    if (error) {
        Serial.println("[MQTT] Failed to parse app launch command");
        return;
    }
    
    String app_name = doc["app"].as<String>();
    String parameters = doc["parameters"].as<String>();
    String sender = doc["sender"].as<String>();
    
    if (!app_name.isEmpty()) {
        AppLaunchRequest request = {
            app_name,
            parameters,
            sender,
            millis(),
            false
        };
        
        app_launch_requests.push(request);
        Serial.printf("[MQTT] App launch request: %s\n", app_name.c_str());
    }
}

void MQTTManager::handle_notification(const String& payload) {
    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, payload);
    
    if (error) {
        Serial.println("[MQTT] Failed to parse notification");
        return;
    }
    
    NotificationMessage notification = {
        doc["app"].as<String>(),
        doc["title"].as<String>(),
        doc["message"].as<String>(),
        doc["icon"].as<String>(),
        doc["priority"].as<int>(),
        millis(),
        doc["persistent"].as<bool>(),
        false
    };
    
    // Limit notification count
    if (notifications.size() >= MAX_NOTIFICATIONS) {
        notifications.erase(notifications.begin());
    }
    
    notifications.push_back(notification);
    Serial.printf("[MQTT] Notification: %s - %s\n", 
                 notification.app_name.c_str(), notification.title.c_str());
}

void MQTTManager::handle_system_command(const String& payload) {
    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, payload);
    
    if (error) {
        Serial.println("[MQTT] Failed to parse system command");
        return;
    }
    
    String command = doc["command"].as<String>();
    
    if (command == "restart") {
        Serial.println("[MQTT] Restart command received");
        ESP.restart();
    } else if (command == "status") {
        send_status_update(get_statistics());
    }
}

void MQTTManager::subscribe_to_default_topics() {
    // Subscribe to device-specific topics
    subscribe(get_device_topic(MQTT_TOPIC_APP_LAUNCH));
    subscribe(get_device_topic(MQTT_TOPIC_NOTIFICATION));
    subscribe(get_device_topic(MQTT_TOPIC_SYSTEM_CMD));
    
    // Subscribe to broadcast topics
    subscribe(MQTT_TOPIC_BROADCAST_NOTIFICATION);
    subscribe(MQTT_TOPIC_BROADCAST_SYSTEM);
}

String MQTTManager::get_device_topic(const String& base_topic) const {
    return "tdeck/" + device_id + "/" + base_topic;
}

void MQTTManager::process_message_queue() {
    if (!mqtt_client.connected() || outgoing_messages.empty()) {
        return;
    }
    
    // Process one message per update cycle
    MQTTMessage msg = outgoing_messages.front();
    outgoing_messages.pop();
    
    publish(msg.topic, msg.payload, msg.retained, msg.qos);
}

void MQTTManager::cleanup_old_data() {
    uint32_t now = millis();
    
    // Remove old notifications (older than 24 hours)
    notifications.erase(
        std::remove_if(notifications.begin(), notifications.end(),
            [now](const NotificationMessage& n) {
                return !n.persistent && (now - n.timestamp > 86400000);
            }),
        notifications.end());
    
    // Clear old app launch requests
    while (!app_launch_requests.empty()) {
        AppLaunchRequest request = app_launch_requests.front();
        if (now - request.timestamp > MESSAGE_TIMEOUT) {
            app_launch_requests.pop();
        } else {
            break;
        }
    }
}