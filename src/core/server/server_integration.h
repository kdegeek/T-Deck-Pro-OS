// server_integration.h
// T-Deck-Pro OS Server Integration Manager
#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "core/utils/logger.h"
#include "core/communication/communication_manager.h"
#include "core/apps/app_manager.h"
#include "core/storage/storage_manager.h"

// Server connection states
typedef enum {
    SERVER_STATE_DISCONNECTED,
    SERVER_STATE_CONNECTING,
    SERVER_STATE_CONNECTED,
    SERVER_STATE_AUTHENTICATED,
    SERVER_STATE_ERROR
} server_state_t;

// Message types
typedef enum {
    SERVER_MSG_REGISTER,
    SERVER_MSG_TELEMETRY,
    SERVER_MSG_STATUS,
    SERVER_MSG_CONFIG,
    SERVER_MSG_OTA,
    SERVER_MSG_APP_MANAGEMENT,
    SERVER_MSG_MESH_FORWARD
} server_message_type_t;

// Server configuration
struct ServerConfig {
    String brokerHost;
    int brokerPort;
    String deviceId;
    String deviceType;
    bool autoReconnect;
    int reconnectInterval;
    int telemetryInterval;
    int heartbeatInterval;
    bool enableOTA;
    bool enableAppManagement;
    bool enableMeshForwarding;
};

// Telemetry data structure
struct TelemetryData {
    float batteryPercent;
    float temperature;
    float cpuUsage;
    float memoryUsage;
    int signalStrength;
    float gpsLatitude;
    float gpsLongitude;
    bool wifiConnected;
    bool loraActive;
    bool cellularConnected;
    std::vector<String> runningApps;
    uint32_t uptime;
    uint32_t freeHeap;
    uint32_t flashUsed;
    uint32_t sdUsed;
};

// OTA update information
struct OTAUpdate {
    bool available;
    String type;        // "firmware" or "app"
    String version;
    String filename;
    String checksum;
    String downloadUrl;
    size_t sizeBytes;
    String releaseNotes;
};

// App management command
struct AppCommand {
    String action;      // "install", "remove", "update", "list"
    String appId;
    String appName;
    String version;
    String filename;
    String checksum;
    String downloadUrl;
    std::vector<String> dependencies;
};

class ServerIntegration {
private:
    static ServerIntegration* instance;
    
    // MQTT client
    WiFiClient wifiClient;
    PubSubClient mqttClient;
    
    // Configuration
    ServerConfig config;
    server_state_t currentState;
    bool initialized;
    
    // Timing
    uint32_t lastTelemetryTime;
    uint32_t lastHeartbeatTime;
    uint32_t lastReconnectAttempt;
    uint32_t connectionStartTime;
    
    // Statistics
    uint32_t messagesReceived;
    uint32_t messagesSent;
    uint32_t reconnectCount;
    uint32_t lastConnectionDuration;
    
    // Message handlers
    void (*configUpdateHandler)(const JsonObject& config);
    void (*otaUpdateHandler)(const OTAUpdate& ota);
    void (*appCommandHandler)(const AppCommand& command);
    
    // Internal methods
    bool connectToMQTT();
    void onMQTTConnect();
    void onMQTTDisconnect();
    void onMQTTMessage(char* topic, byte* payload, unsigned int length);
    
    void handleConfigMessage(const JsonObject& config);
    void handleOTAMessage(const JsonObject& ota);
    void handleAppMessage(const JsonObject& app);
    
    bool publishMessage(const String& topic, const JsonObject& payload, bool retain = false);
    bool publishMessage(const String& topic, const String& payload, bool retain = false);
    
    void collectTelemetryData(TelemetryData& telemetry);
    void sendTelemetry();
    void sendHeartbeat();
    void sendStatus(const String& status, const String& reason = "");
    
    // Static callback wrapper
    static void mqttCallback(char* topic, byte* payload, unsigned int length);
    
public:
    ServerIntegration();
    ~ServerIntegration();
    
    // Singleton access
    static ServerIntegration* getInstance();
    
    // Lifecycle
    bool initialize(const ServerConfig& serverConfig);
    void shutdown();
    void update();
    
    // Connection management
    bool connect();
    void disconnect();
    bool reconnect();
    server_state_t getState() const;
    bool isConnected() const;
    
    // Device registration
    bool registerDevice();
    bool updateDeviceInfo();
    
    // Telemetry and status
    bool sendCurrentTelemetry();
    bool reportStatus(const String& status, const String& reason = "");
    bool sendCustomData(const String& dataType, const JsonObject& data);
    
    // Mesh integration
    bool forwardMeshMessage(const String& fromNode, const String& toNode, 
                           const String& messageType, const JsonObject& payload);
    
    // OTA management
    bool checkForUpdates();
    bool downloadUpdate(const OTAUpdate& update);
    bool installUpdate(const String& updatePath);
    
    // App management
    bool requestAppList();
    bool downloadApp(const AppCommand& command);
    bool installApp(const String& appPath);
    bool removeApp(const String& appId);
    
    // Configuration
    void setConfig(const ServerConfig& serverConfig);
    ServerConfig getConfig() const;
    void setTelemetryInterval(int intervalSeconds);
    void setHeartbeatInterval(int intervalSeconds);
    
    // Event handlers
    void setConfigUpdateHandler(void (*handler)(const JsonObject& config));
    void setOTAUpdateHandler(void (*handler)(const OTAUpdate& ota));
    void setAppCommandHandler(void (*handler)(const AppCommand& command));
    
    // Statistics
    struct ServerStats {
        server_state_t state;
        uint32_t uptime;
        uint32_t messagesReceived;
        uint32_t messagesSent;
        uint32_t reconnectCount;
        uint32_t lastConnectionDuration;
        String lastError;
        float connectionQuality;
    };
    
    ServerStats getStatistics() const;
    void resetStatistics();
    
    // Utility
    String getDeviceId() const;
    String getServerStatus() const;
    bool testConnection();
    void forceReconnect();
    
    // Battery monitoring
    float getBatteryVoltage();
    int getBatteryPercentage();
};

// Convenience macros
#define SERVER_MGR ServerIntegration::getInstance()

// MQTT topic structure
#define MQTT_TOPIC_REGISTER "tdeckpro/%s/register"
#define MQTT_TOPIC_TELEMETRY "tdeckpro/%s/telemetry"
#define MQTT_TOPIC_STATUS "tdeckpro/%s/status"
#define MQTT_TOPIC_CONFIG "tdeckpro/%s/config"
#define MQTT_TOPIC_OTA "tdeckpro/%s/ota"
#define MQTT_TOPIC_APPS "tdeckpro/%s/apps"
#define MQTT_TOPIC_MESH "tdeckpro/mesh/%s"
#define MQTT_TOPIC_HEARTBEAT "tdeckpro/%s/heartbeat"

// Default configuration values
#define DEFAULT_BROKER_HOST "localhost"
#define DEFAULT_BROKER_PORT 1883
#define DEFAULT_TELEMETRY_INTERVAL 300  // 5 minutes
#define DEFAULT_HEARTBEAT_INTERVAL 60   // 1 minute
#define DEFAULT_RECONNECT_INTERVAL 30   // 30 seconds

// Server integration events
typedef enum {
    SERVER_EVENT_CONNECTED,
    SERVER_EVENT_DISCONNECTED,
    SERVER_EVENT_CONFIG_UPDATED,
    SERVER_EVENT_OTA_AVAILABLE,
    SERVER_EVENT_APP_COMMAND,
    SERVER_EVENT_ERROR
} server_event_t;

// Event callback type
typedef void (*server_event_callback_t)(server_event_t event, void* data);