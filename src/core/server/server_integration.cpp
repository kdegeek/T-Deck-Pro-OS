#include "server_integration.h"
#include "../utils/logger.h"
#include "../storage/storage_manager.h"
#include "../communication/communication_manager.h"
#include "../ui/launcher.h"
#include <ArduinoJson.h>
#include <WiFi.h>

// Constants
#define TELEMETRY_INTERVAL 300000    // 5 minutes in milliseconds
#define HEARTBEAT_INTERVAL 60000     // 1 minute in milliseconds
#define RECONNECT_INTERVAL 30000     // 30 seconds in milliseconds
#define MAX_RECONNECT_ATTEMPTS 10    // Maximum reconnection attempts
#define FIRMWARE_VERSION "1.0.0"     // Firmware version

ServerIntegration* ServerIntegration::instance = nullptr;

ServerIntegration::ServerIntegration() :
    mqttClient(wifiClient),
    currentState(SERVER_STATE_DISCONNECTED),
    initialized(false),
    lastTelemetryTime(0),
    lastHeartbeatTime(0),
    lastReconnectAttempt(0),
    connectionStartTime(0),
    messagesReceived(0),
    messagesSent(0),
    reconnectCount(0),
    lastConnectionDuration(0),
    configUpdateHandler(nullptr),
    otaUpdateHandler(nullptr),
    appCommandHandler(nullptr) {
}

ServerIntegration::~ServerIntegration() {
    shutdown();
}

ServerIntegration* ServerIntegration::getInstance() {
    if (!instance) {
        instance = new ServerIntegration();
    }
    return instance;
}

bool ServerIntegration::initialize(const ServerConfig& serverConfig) {
    Logger::info("ServerIntegration", "Initializing server integration...");
    
    config = serverConfig;
    
    // Generate device ID if not provided
    if (config.deviceId.isEmpty()) {
        uint64_t chipId = ESP.getEfuseMac();
        config.deviceId = "tdeckpro_" + String((uint32_t)(chipId >> 32), HEX) + String((uint32_t)chipId, HEX);
    }
    
    Logger::info("ServerIntegration", "Device ID: " + config.deviceId);
    
    // Configure MQTT client
    mqttClient.setServer(config.brokerHost.c_str(), config.brokerPort);
    mqttClient.setCallback(mqttCallback);
    
    currentState = SERVER_STATE_DISCONNECTED;
    initialized = true;
    Logger::info("ServerIntegration", "Server integration initialized");
    return true;
}

void ServerIntegration::shutdown() {
    if (initialized) {
        disconnect();
        initialized = false;
        currentState = SERVER_STATE_DISCONNECTED;
        Logger::info("ServerIntegration", "Server integration shutdown");
    }
}

// Static callback wrapper
void ServerIntegration::mqttCallback(char* topic, byte* payload, unsigned int length) {
    if (instance) {
        instance->onMQTTMessage(topic, payload, length);
    }
}

bool ServerIntegration::connect() {
    if (!initialized) {
        Logger::error("ServerIntegration", "Server integration not initialized");
        return false;
    }
    
    // Check WiFi connection
    using namespace TDeckOS::Communication;
    CommunicationManager* comm = CommunicationManager::getInstance();
    if (!comm->isWiFiConnected()) {
        Logger::warning("ServerIntegration", "WiFi not connected, cannot connect to server");
        return false;
    }
    
    currentState = SERVER_STATE_CONNECTING;
    connectionStartTime = millis();
    
    Logger::info("ServerIntegration", "Connecting to MQTT server: " + config.brokerHost + ":" + String(config.brokerPort));
    
    return connectToMQTT();
}

bool ServerIntegration::connectToMQTT() {
    // Generate client ID
    String clientId = "TDeckPro_" + config.deviceId;
    
    // Connect to MQTT broker
    if (mqttClient.connect(clientId.c_str())) {
        currentState = SERVER_STATE_CONNECTED;
        reconnectCount = 0;
        
        Logger::info("ServerIntegration", "Connected to MQTT server");
        
        onMQTTConnect();
        return true;
    } else {
        currentState = SERVER_STATE_ERROR;
        reconnectCount++;
        
        Logger::error("ServerIntegration", "Failed to connect to MQTT server (attempt " + String(reconnectCount) + ")");
        return false;
    }
}

void ServerIntegration::onMQTTConnect() {
    // Subscribe to command topics
    String deviceId = config.deviceId;
    
    mqttClient.subscribe(("tdeckpro/" + deviceId + "/config").c_str());
    mqttClient.subscribe(("tdeckpro/" + deviceId + "/ota").c_str());
    mqttClient.subscribe(("tdeckpro/" + deviceId + "/apps").c_str());
    mqttClient.subscribe("tdeckpro/mesh/in");
    
    // Register device
    registerDevice();
    
    // Send initial telemetry
    sendTelemetry();
    
    Logger::info("ServerIntegration", "MQTT connection established and subscriptions active");
}

void ServerIntegration::onMQTTDisconnect() {
    currentState = SERVER_STATE_DISCONNECTED;
    lastConnectionDuration = millis() - connectionStartTime;
    Logger::warning("ServerIntegration", "MQTT connection lost");
}

void ServerIntegration::disconnect() {
    if (currentState != SERVER_STATE_DISCONNECTED) {
        mqttClient.disconnect();
        onMQTTDisconnect();
        Logger::info("ServerIntegration", "Disconnected from MQTT server");
    }
}

bool ServerIntegration::reconnect() {
    if (currentState == SERVER_STATE_CONNECTED) {
        return true;
    }
    
    disconnect();
    return connect();
}

server_state_t ServerIntegration::getState() const {
    return currentState;
}

bool ServerIntegration::isConnected() const {
    return currentState == SERVER_STATE_CONNECTED && const_cast<PubSubClient&>(mqttClient).connected();
}

bool ServerIntegration::registerDevice() {
    if (currentState != SERVER_STATE_CONNECTED) {
        return false;
    }
    
    Logger::info("ServerIntegration", "Registering device with server...");
    
    DynamicJsonDocument doc(1024);
    doc["device_id"] = config.deviceId;
    doc["device_type"] = config.deviceType.isEmpty() ? "T-Deck-Pro" : config.deviceType;
    doc["firmware_version"] = FIRMWARE_VERSION;
    doc["hardware_version"] = "1.0";
    doc["capabilities"] = JsonArray();
    doc["capabilities"].add("wifi");
    doc["capabilities"].add("cellular");
    doc["capabilities"].add("lora");
    doc["capabilities"].add("bluetooth");
    doc["capabilities"].add("gps");
    doc["timestamp"] = millis();
    
    // Add system information
    JsonObject sysInfo = doc.createNestedObject("system_info");
    sysInfo["free_heap"] = ESP.getFreeHeap();
    sysInfo["chip_model"] = ESP.getChipModel();
    sysInfo["chip_revision"] = ESP.getChipRevision();
    sysInfo["cpu_freq"] = ESP.getCpuFreqMHz();
    
    String payload;
    serializeJson(doc, payload);
    
    String registerTopic = "tdeckpro/" + config.deviceId + "/register";
    if (mqttClient.publish(registerTopic.c_str(), payload.c_str())) {
        currentState = SERVER_STATE_AUTHENTICATED;
        Logger::info("ServerIntegration", "Device registered successfully");
        return true;
    } else {
        Logger::error("ServerIntegration", "Failed to register device");
        return false;
    }
}

bool ServerIntegration::updateDeviceInfo() {
    return registerDevice(); // For now, same as registration
}

void ServerIntegration::collectTelemetryData(TelemetryData& telemetry) {
    // System data
    telemetry.uptime = millis();
    telemetry.freeHeap = ESP.getFreeHeap();
    telemetry.temperature = 0.0; // Placeholder for temperatureRead()
    telemetry.batteryPercent = getBatteryPercentage();
    
    // Storage data
    StorageManager& storage = StorageManager::getInstance();
    StorageStats storageStats = storage.getStorageStats();
    telemetry.flashUsed = storageStats.flashUsed;
    telemetry.sdUsed = storageStats.sdUsed;
    
    // Communication data
    using namespace TDeckOS::Communication;
    CommunicationManager* comm = CommunicationManager::getInstance();
    telemetry.wifiConnected = comm->isWiFiConnected();
    telemetry.cellularConnected = comm->isCellularConnected();
    telemetry.loraActive = comm->isLoRaActive();
    
    if (telemetry.wifiConnected) {
        telemetry.signalStrength = WiFi.RSSI();
    }
    
    // Application data
    telemetry.runningApps = storage.getInstalledApps();
    
    // GPS data (placeholder)
    telemetry.gpsLatitude = 0.0;
    telemetry.gpsLongitude = 0.0;
    
    // CPU usage (placeholder)
    telemetry.cpuUsage = 0.0;
    telemetry.memoryUsage = (float)(ESP.getHeapSize() - ESP.getFreeHeap()) / ESP.getHeapSize() * 100.0;
}

void ServerIntegration::sendTelemetry() {
    if (currentState != SERVER_STATE_AUTHENTICATED) {
        return;
    }
    
    unsigned long now = millis();
    if (now - lastTelemetryTime < config.telemetryInterval * 1000) {
        return;
    }
    
    Logger::debug("ServerIntegration", "Sending telemetry data...");
    
    TelemetryData telemetry;
    collectTelemetryData(telemetry);
    
    DynamicJsonDocument doc(2048);
    doc["device_id"] = config.deviceId;
    doc["timestamp"] = now;
    
    // System telemetry
    JsonObject system = doc.createNestedObject("system");
    system["uptime"] = telemetry.uptime;
    system["free_heap"] = telemetry.freeHeap;
    system["cpu_temp"] = telemetry.temperature;
    system["battery_percentage"] = telemetry.batteryPercent;
    system["cpu_usage"] = telemetry.cpuUsage;
    system["memory_usage"] = telemetry.memoryUsage;
    
    // Storage telemetry
    JsonObject storageObj = doc.createNestedObject("storage");
    storageObj["flash_used"] = telemetry.flashUsed;
    storageObj["sd_used"] = telemetry.sdUsed;
    
    // Communication telemetry
    JsonObject commObj = doc.createNestedObject("communication");
    commObj["wifi_connected"] = telemetry.wifiConnected;
    commObj["cellular_connected"] = telemetry.cellularConnected;
    commObj["lora_active"] = telemetry.loraActive;
    commObj["signal_strength"] = telemetry.signalStrength;
    
    // GPS telemetry
    JsonObject gpsObj = doc.createNestedObject("gps");
    gpsObj["latitude"] = telemetry.gpsLatitude;
    gpsObj["longitude"] = telemetry.gpsLongitude;
    
    // Application telemetry
    JsonArray apps = doc.createNestedArray("running_apps");
    for (const String& app : telemetry.runningApps) {
        apps.add(app);
    }
    
    String payload;
    serializeJson(doc, payload);
    
    String telemetryTopic = "tdeckpro/" + config.deviceId + "/telemetry";
    if (mqttClient.publish(telemetryTopic.c_str(), payload.c_str())) {
        lastTelemetryTime = now;
        messagesSent++;
        Logger::debug("ServerIntegration", "Telemetry sent successfully");
    } else {
        Logger::warning("ServerIntegration", "Failed to send telemetry");
    }
}

bool ServerIntegration::sendCurrentTelemetry() {
    sendTelemetry();
    return true;
}

void ServerIntegration::sendHeartbeat() {
    if (currentState != SERVER_STATE_AUTHENTICATED) {
        return;
    }
    
    unsigned long now = millis();
    if (now - lastHeartbeatTime < config.heartbeatInterval * 1000) {
        return;
    }
    
    DynamicJsonDocument doc(256);
    doc["device_id"] = config.deviceId;
    doc["timestamp"] = now;
    doc["status"] = "alive";
    doc["state"] = currentState;
    
    String payload;
    serializeJson(doc, payload);
    
    String heartbeatTopic = "tdeckpro/" + config.deviceId + "/heartbeat";
    if (mqttClient.publish(heartbeatTopic.c_str(), payload.c_str())) {
        lastHeartbeatTime = now;
        messagesSent++;
        Logger::debug("ServerIntegration", "Heartbeat sent");
    }
}

void ServerIntegration::sendStatus(const String& status, const String& reason) {
    if (currentState == SERVER_STATE_DISCONNECTED) {
        return;
    }
    
    DynamicJsonDocument doc(512);
    doc["device_id"] = config.deviceId;
    doc["timestamp"] = millis();
    doc["status"] = status;
    if (!reason.isEmpty()) {
        doc["reason"] = reason;
    }
    doc["state"] = currentState;
    
    String payload;
    serializeJson(doc, payload);
    
    String statusTopic = "tdeckpro/" + config.deviceId + "/status";
    mqttClient.publish(statusTopic.c_str(), payload.c_str());
    messagesSent++;
}

bool ServerIntegration::reportStatus(const String& status, const String& reason) {
    sendStatus(status, reason);
    return true;
}

bool ServerIntegration::sendCustomData(const String& dataType, const JsonObject& data) {
    if (currentState != SERVER_STATE_AUTHENTICATED) {
        return false;
    }
    
    DynamicJsonDocument doc(1024);
    doc["device_id"] = config.deviceId;
    doc["timestamp"] = millis();
    doc["data_type"] = dataType;
    doc["data"] = data;
    
    String payload;
    serializeJson(doc, payload);
    
    String customTopic = "tdeckpro/" + config.deviceId + "/custom/" + dataType;
    bool result = mqttClient.publish(customTopic.c_str(), payload.c_str());
    if (result) {
        messagesSent++;
    }
    return result;
}

void ServerIntegration::update() {
    if (!initialized) {
        return;
    }
    
    // Handle MQTT client
    if (currentState == SERVER_STATE_CONNECTED || currentState == SERVER_STATE_AUTHENTICATED) {
        if (!mqttClient.connected()) {
            onMQTTDisconnect();
        } else {
            mqttClient.loop();
            sendTelemetry();
            sendHeartbeat();
        }
    } else if (currentState == SERVER_STATE_DISCONNECTED && config.autoReconnect) {
        // Try to reconnect
        unsigned long now = millis();
        
        if (now - lastReconnectAttempt > config.reconnectInterval * 1000) {
            lastReconnectAttempt = now;
            if (reconnectCount < MAX_RECONNECT_ATTEMPTS) {
                Logger::info("ServerIntegration", "Attempting to reconnect to server...");
                connect();
            }
        }
    }
}

void ServerIntegration::onMQTTMessage(char* topic, byte* payload, unsigned int length) {
    String topicStr = String(topic);
    String message = "";
    
    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    
    messagesReceived++;
    Logger::info("ServerIntegration", "Received message on topic: " + topicStr);
    Logger::debug("ServerIntegration", "Message: " + message);
    
    // Parse JSON message
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, message);
    
    if (error) {
        Logger::error("ServerIntegration", "Failed to parse JSON message: " + String(error.c_str()));
        return;
    }
    
    // Handle different message types based on topic patterns
    String deviceId = config.deviceId;
    if (topicStr == "tdeckpro/" + deviceId + "/config") {
        handleConfigMessage(doc.as<JsonObject>());
    } else if (topicStr == "tdeckpro/" + deviceId + "/ota") {
        handleOTAMessage(doc.as<JsonObject>());
    } else if (topicStr == "tdeckpro/" + deviceId + "/apps") {
        handleAppMessage(doc.as<JsonObject>());
    } else if (topicStr == "tdeckpro/mesh/in") {
        // Handle mesh message
        Logger::info("ServerIntegration", "Handling mesh message");
    }
}

void ServerIntegration::handleConfigMessage(const JsonObject& config) {
    Logger::info("ServerIntegration", "Handling configuration message");
    
    // Call user handler if set
    if (configUpdateHandler) {
        configUpdateHandler(config);
    }
    
    // Update internal configuration
    if (config.containsKey("telemetry_interval")) {
        this->config.telemetryInterval = config["telemetry_interval"].as<int>();
        Logger::info("ServerIntegration", "Updated telemetry interval to " + String(this->config.telemetryInterval));
    }
    
    if (config.containsKey("heartbeat_interval")) {
        this->config.heartbeatInterval = config["heartbeat_interval"].as<int>();
        Logger::info("ServerIntegration", "Updated heartbeat interval to " + String(this->config.heartbeatInterval));
    }
    
    if (config.containsKey("auto_reconnect")) {
        this->config.autoReconnect = config["auto_reconnect"].as<bool>();
        Logger::info("ServerIntegration", "Updated auto reconnect to " + String(this->config.autoReconnect ? "true" : "false"));
    }
    
    // Send acknowledgment
    reportStatus("config_updated", "Configuration updated successfully");
}

void ServerIntegration::handleOTAMessage(const JsonObject& ota) {
    Logger::info("ServerIntegration", "Handling OTA message");
    
    OTAUpdate update;
    update.available = ota["available"] | false;
    update.type = ota["type"] | "firmware";
    update.version = ota["version"] | "";
    update.filename = ota["filename"] | "";
    update.checksum = ota["checksum"] | "";
    update.downloadUrl = ota["download_url"] | "";
    update.sizeBytes = ota["size_bytes"] | 0;
    update.releaseNotes = ota["release_notes"] | "";
    
    // Call user handler if set
    if (otaUpdateHandler) {
        otaUpdateHandler(update);
    } else {
        // Default behavior - start download
        if (update.available && !update.downloadUrl.isEmpty()) {
            Logger::info("ServerIntegration", "Starting OTA update to version: " + update.version);
            downloadUpdate(update);
        }
    }
}

void ServerIntegration::handleAppMessage(const JsonObject& app) {
    Logger::info("ServerIntegration", "Handling app message");
    
    AppCommand command;
    command.action = app["action"] | "";
    command.appId = app["app_id"] | "";
    command.appName = app["app_name"] | "";
    command.version = app["version"] | "";
    command.filename = app["filename"] | "";
    command.checksum = app["checksum"] | "";
    command.downloadUrl = app["download_url"] | "";
    
    // Parse dependencies array
    if (app.containsKey("dependencies")) {
        JsonArray deps = app["dependencies"];
        for (JsonVariant dep : deps) {
            command.dependencies.push_back(dep.as<String>());
        }
    }
    
    // Call user handler if set
    if (appCommandHandler) {
        appCommandHandler(command);
    } else {
        // Default behavior
        if (command.action == "install") {
            downloadApp(command);
        } else if (command.action == "remove") {
            removeApp(command.appId);
        }
    }
}

// Configuration methods
void ServerIntegration::setConfig(const ServerConfig& serverConfig) {
    config = serverConfig;
}

ServerConfig ServerIntegration::getConfig() const {
    return config;
}

void ServerIntegration::setTelemetryInterval(int intervalSeconds) {
    config.telemetryInterval = intervalSeconds;
}

void ServerIntegration::setHeartbeatInterval(int intervalSeconds) {
    config.heartbeatInterval = intervalSeconds;
}

// Event handler setters
void ServerIntegration::setConfigUpdateHandler(void (*handler)(const JsonObject& config)) {
    configUpdateHandler = handler;
}

void ServerIntegration::setOTAUpdateHandler(void (*handler)(const OTAUpdate& ota)) {
    otaUpdateHandler = handler;
}

void ServerIntegration::setAppCommandHandler(void (*handler)(const AppCommand& command)) {
    appCommandHandler = handler;
}

// Mesh integration
bool ServerIntegration::forwardMeshMessage(const String& fromNode, const String& toNode,
                                         const String& messageType, const JsonObject& payload) {
    if (currentState != SERVER_STATE_AUTHENTICATED) {
        return false;
    }
    
    DynamicJsonDocument doc(1024);
    doc["from_node"] = fromNode;
    doc["to_node"] = toNode;
    doc["message_type"] = messageType;
    doc["payload"] = payload;
    doc["timestamp"] = millis();
    
    String payloadStr;
    serializeJson(doc, payloadStr);
    
    String meshTopic = "tdeckpro/mesh/out";
    bool result = mqttClient.publish(meshTopic.c_str(), payloadStr.c_str());
    if (result) {
        messagesSent++;
    }
    return result;
}

// OTA management
bool ServerIntegration::checkForUpdates() {
    if (currentState != SERVER_STATE_AUTHENTICATED) {
        return false;
    }
    
    DynamicJsonDocument doc(256);
    doc["device_id"] = config.deviceId;
    doc["current_version"] = FIRMWARE_VERSION;
    doc["request"] = "check_updates";
    doc["timestamp"] = millis();
    
    String payload;
    serializeJson(doc, payload);
    
    String otaTopic = "tdeckpro/" + config.deviceId + "/ota/request";
    return mqttClient.publish(otaTopic.c_str(), payload.c_str());
}

bool ServerIntegration::downloadUpdate(const OTAUpdate& update) {
    Logger::info("ServerIntegration", "Starting OTA update download from: " + update.downloadUrl);
    
    // Add notification to launcher
    UILauncher* launcher = UILauncher::getInstance();
    launcher->addNotification("OTA Update", "Downloading firmware update...");
    
    // Implementation would use ESP32 OTA library
    // This is a placeholder for the actual OTA implementation
    
    return true; // Placeholder
}

bool ServerIntegration::installUpdate(const String& updatePath) {
    Logger::info("ServerIntegration", "Installing update from: " + updatePath);
    
    // Implementation would install the downloaded update
    // This is a placeholder for the actual installation
    
    return true; // Placeholder
}

// App management
bool ServerIntegration::requestAppList() {
    if (currentState != SERVER_STATE_AUTHENTICATED) {
        return false;
    }
    
    DynamicJsonDocument doc(256);
    doc["device_id"] = config.deviceId;
    doc["request"] = "app_list";
    doc["timestamp"] = millis();
    
    String payload;
    serializeJson(doc, payload);
    
    String appsTopic = "tdeckpro/" + config.deviceId + "/apps/request";
    return mqttClient.publish(appsTopic.c_str(), payload.c_str());
}

bool ServerIntegration::downloadApp(const AppCommand& command) {
    Logger::info("ServerIntegration", "Downloading app: " + command.appName + " from " + command.downloadUrl);
    
    // Add notification to launcher
    UILauncher* launcher = UILauncher::getInstance();
    launcher->addNotification("App Install", "Downloading " + command.appName + "...");
    
    // Implementation would download app binary and install via StorageManager
    // This is a placeholder for the actual download and install implementation
    
    return true; // Placeholder
}

bool ServerIntegration::installApp(const String& appPath) {
    Logger::info("ServerIntegration", "Installing app from: " + appPath);
    
    // Implementation would install the app via StorageManager
    // This is a placeholder
    
    return true; // Placeholder
}

bool ServerIntegration::removeApp(const String& appId) {
    Logger::info("ServerIntegration", "Removing app: " + appId);
    
    StorageManager& storage = StorageManager::getInstance();
    bool result = storage.uninstallApp(appId);
    
    if (result) {
        // Refresh launcher
        // Launcher* launcher = Launcher::getInstance();
        // This method does not exist on Launcher, need to implement a public refresh method
        // launcher->refreshAppGrid();
    }
    
    return result;
}

float ServerIntegration::getBatteryVoltage() {
    // Read battery voltage from ADC
    // This is a placeholder - actual implementation would read from battery ADC pin
    return 3.7; // Placeholder value
}

int ServerIntegration::getBatteryPercentage() {
    float voltage = getBatteryVoltage();
    
    // Convert voltage to percentage (rough approximation)
    if (voltage >= 4.1) return 100;
    if (voltage >= 3.9) return 80;
    if (voltage >= 3.7) return 60;
    if (voltage >= 3.5) return 40;
    if (voltage >= 3.3) return 20;
    return 0;
}

// Statistics methods
ServerIntegration::ServerStats ServerIntegration::getStatistics() const {
    ServerStats stats;
    stats.state = currentState;
    stats.uptime = millis();
    stats.messagesReceived = messagesReceived;
    stats.messagesSent = messagesSent;
    stats.reconnectCount = reconnectCount;
    stats.lastConnectionDuration = lastConnectionDuration;
    stats.lastError = ""; // Would be set during error conditions
    
    // Calculate connection quality based on reconnect frequency
    if (stats.uptime > 0) {
        stats.connectionQuality = 100.0 - (float(reconnectCount) / (stats.uptime / 3600000.0)) * 10.0;
        if (stats.connectionQuality < 0) stats.connectionQuality = 0;
        if (stats.connectionQuality > 100) stats.connectionQuality = 100;
    } else {
        stats.connectionQuality = 0;
    }
    
    return stats;
}

void ServerIntegration::resetStatistics() {
    messagesReceived = 0;
    messagesSent = 0;
    reconnectCount = 0;
    lastConnectionDuration = 0;
}

// Utility methods
String ServerIntegration::getDeviceId() const {
    return config.deviceId;
}

String ServerIntegration::getServerStatus() const {
    switch (currentState) {
        case SERVER_STATE_DISCONNECTED: return "Disconnected";
        case SERVER_STATE_CONNECTING: return "Connecting";
        case SERVER_STATE_CONNECTED: return "Connected";
        case SERVER_STATE_AUTHENTICATED: return "Authenticated";
        case SERVER_STATE_ERROR: return "Error";
        default: return "Unknown";
    }
}

bool ServerIntegration::testConnection() {
    if (currentState != SERVER_STATE_AUTHENTICATED) {
        return false;
    }
    
    // Send a test message
    DynamicJsonDocument doc(256);
    doc["device_id"] = config.deviceId;
    doc["test"] = true;
    doc["timestamp"] = millis();
    
    String payload;
    serializeJson(doc, payload);
    
    String testTopic = "tdeckpro/" + config.deviceId + "/test";
    return mqttClient.publish(testTopic.c_str(), payload.c_str());
}

void ServerIntegration::forceReconnect() {
    disconnect();
    reconnectCount = 0;
    lastReconnectAttempt = 0;
    connect();
}

// Message publishing helpers
bool ServerIntegration::publishMessage(const String& topic, const JsonObject& payload, bool retain) {
    if (currentState == SERVER_STATE_DISCONNECTED) {
        return false;
    }
    
    String payloadStr;
    serializeJson(payload, payloadStr);
    
    bool result = mqttClient.publish(topic.c_str(), payloadStr.c_str(), retain);
    if (result) {
        messagesSent++;
    }
    return result;
}

bool ServerIntegration::publishMessage(const String& topic, const String& payload, bool retain) {
    if (currentState == SERVER_STATE_DISCONNECTED) {
        return false;
    }
    
    bool result = mqttClient.publish(topic.c_str(), payload.c_str(), retain);
    if (result) {
        messagesSent++;
    }
    return result;
}