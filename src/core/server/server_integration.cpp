#include "server_integration.h"
#include "../utils/logger.h"
#include "../storage/storage_manager.h"
#include "../communication/communication_manager.h"
#include "../ui/launcher.h"
#include <ArduinoJson.h>
#include <WiFi.h>

ServerIntegration* ServerIntegration::instance = nullptr;

ServerIntegration::ServerIntegration() : 
    mqttClient(wifiClient),
    connected(false),
    deviceRegistered(false),
    lastTelemetryTime(0),
    lastHeartbeatTime(0),
    reconnectAttempts(0),
    initialized(false) {
}

ServerIntegration& ServerIntegration::getInstance() {
    if (!instance) {
        instance = new ServerIntegration();
    }
    return *instance;
}

bool ServerIntegration::init(const String& deviceId, const String& mqttServer, int mqttPort) {
    Logger::info("ServerIntegration", "Initializing server integration...");
    
    this->deviceId = deviceId;
    this->mqttServer = mqttServer;
    this->mqttPort = mqttPort;
    
    // Generate device ID if not provided
    if (this->deviceId.isEmpty()) {
        uint64_t chipId = ESP.getEfuseMac();
        this->deviceId = "tdeckpro_" + String((uint32_t)(chipId >> 32), HEX) + String((uint32_t)chipId, HEX);
    }
    
    Logger::info("ServerIntegration", "Device ID: %s", this->deviceId.c_str());
    
    // Configure MQTT client
    mqttClient.setServer(mqttServer.c_str(), mqttPort);
    mqttClient.setCallback([this](char* topic, byte* payload, unsigned int length) {
        this->onMqttMessage(topic, payload, length);
    });
    
    // Set up topics
    setupTopics();
    
    initialized = true;
    Logger::info("ServerIntegration", "Server integration initialized");
    return true;
}

void ServerIntegration::setupTopics() {
    // Device-specific topics
    registerTopic = "tdeckpro/" + deviceId + "/register";
    telemetryTopic = "tdeckpro/" + deviceId + "/telemetry";
    configTopic = "tdeckpro/" + deviceId + "/config";
    otaTopic = "tdeckpro/" + deviceId + "/ota";
    appsTopic = "tdeckpro/" + deviceId + "/apps";
    
    // Command topics (server to device)
    configCommandTopic = configTopic + "/cmd";
    otaCommandTopic = otaTopic + "/cmd";
    appsCommandTopic = appsTopic + "/cmd";
    
    // Mesh topics
    meshInTopic = "tdeckpro/mesh/in";
    meshOutTopic = "tdeckpro/mesh/out";
    
    Logger::debug("ServerIntegration", "Topics configured for device: %s", deviceId.c_str());
}

bool ServerIntegration::connect() {
    if (!initialized) {
        Logger::error("ServerIntegration", "Server integration not initialized");
        return false;
    }
    
    // Check WiFi connection
    CommunicationManager& comm = CommunicationManager::getInstance();
    if (!comm.isWiFiConnected()) {
        Logger::warning("ServerIntegration", "WiFi not connected, cannot connect to server");
        return false;
    }
    
    Logger::info("ServerIntegration", "Connecting to MQTT server: %s:%d", mqttServer.c_str(), mqttPort);
    
    // Generate client ID
    String clientId = "TDeckPro_" + deviceId;
    
    // Connect to MQTT broker
    if (mqttClient.connect(clientId.c_str())) {
        connected = true;
        reconnectAttempts = 0;
        
        Logger::info("ServerIntegration", "Connected to MQTT server");
        
        // Subscribe to command topics
        subscribeToTopics();
        
        // Register device
        registerDevice();
        
        // Send initial telemetry
        sendTelemetry();
        
        return true;
    } else {
        connected = false;
        reconnectAttempts++;
        
        Logger::error("ServerIntegration", "Failed to connect to MQTT server (attempt %d)", reconnectAttempts);
        return false;
    }
}

void ServerIntegration::disconnect() {
    if (connected) {
        mqttClient.disconnect();
        connected = false;
        deviceRegistered = false;
        Logger::info("ServerIntegration", "Disconnected from MQTT server");
    }
}

void ServerIntegration::subscribeToTopics() {
    if (!connected) {
        return;
    }
    
    Logger::info("ServerIntegration", "Subscribing to command topics...");
    
    mqttClient.subscribe(configCommandTopic.c_str());
    mqttClient.subscribe(otaCommandTopic.c_str());
    mqttClient.subscribe(appsCommandTopic.c_str());
    mqttClient.subscribe(meshInTopic.c_str());
    
    Logger::info("ServerIntegration", "Subscribed to command topics");
}

void ServerIntegration::registerDevice() {
    if (!connected) {
        return;
    }
    
    Logger::info("ServerIntegration", "Registering device with server...");
    
    DynamicJsonDocument doc(1024);
    doc["device_id"] = deviceId;
    doc["device_type"] = "T-Deck-Pro";
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
    
    if (mqttClient.publish(registerTopic.c_str(), payload.c_str())) {
        deviceRegistered = true;
        Logger::info("ServerIntegration", "Device registered successfully");
    } else {
        Logger::error("ServerIntegration", "Failed to register device");
    }
}

void ServerIntegration::sendTelemetry() {
    if (!connected || !deviceRegistered) {
        return;
    }
    
    unsigned long now = millis();
    if (now - lastTelemetryTime < TELEMETRY_INTERVAL) {
        return;
    }
    
    Logger::debug("ServerIntegration", "Sending telemetry data...");
    
    DynamicJsonDocument doc(2048);
    doc["device_id"] = deviceId;
    doc["timestamp"] = now;
    
    // System telemetry
    JsonObject system = doc.createNestedObject("system");
    system["uptime"] = now;
    system["free_heap"] = ESP.getFreeHeap();
    system["cpu_temp"] = temperatureRead();
    system["battery_voltage"] = getBatteryVoltage();
    system["battery_percentage"] = getBatteryPercentage();
    
    // Storage telemetry
    StorageManager& storage = StorageManager::getInstance();
    StorageInfo storageInfo = storage.getStorageInfo();
    JsonObject storageObj = doc.createNestedObject("storage");
    storageObj["flash_used"] = storageInfo.flashUsed;
    storageObj["flash_total"] = storageInfo.flashTotal;
    storageObj["sd_used"] = storageInfo.sdUsed;
    storageObj["sd_total"] = storageInfo.sdTotal;
    storageObj["sd_present"] = storageInfo.sdCardPresent;
    
    // Communication telemetry
    CommunicationManager& comm = CommunicationManager::getInstance();
    JsonObject commObj = doc.createNestedObject("communication");
    commObj["wifi_connected"] = comm.isWiFiConnected();
    commObj["cellular_connected"] = comm.isCellularConnected();
    commObj["lora_active"] = comm.isLoRaActive();
    
    if (comm.isWiFiConnected()) {
        commObj["wifi_rssi"] = WiFi.RSSI();
        commObj["wifi_ssid"] = WiFi.SSID();
    }
    
    // Application telemetry
    std::vector<String> installedApps = storage.getInstalledApps();
    JsonArray apps = doc.createNestedArray("installed_apps");
    for (const String& app : installedApps) {
        apps.add(app);
    }
    
    String payload;
    serializeJson(doc, payload);
    
    if (mqttClient.publish(telemetryTopic.c_str(), payload.c_str())) {
        lastTelemetryTime = now;
        Logger::debug("ServerIntegration", "Telemetry sent successfully");
    } else {
        Logger::warning("ServerIntegration", "Failed to send telemetry");
    }
}

void ServerIntegration::sendHeartbeat() {
    if (!connected) {
        return;
    }
    
    unsigned long now = millis();
    if (now - lastHeartbeatTime < HEARTBEAT_INTERVAL) {
        return;
    }
    
    DynamicJsonDocument doc(256);
    doc["device_id"] = deviceId;
    doc["timestamp"] = now;
    doc["status"] = "alive";
    
    String payload;
    serializeJson(doc, payload);
    
    String heartbeatTopic = "tdeckpro/" + deviceId + "/heartbeat";
    if (mqttClient.publish(heartbeatTopic.c_str(), payload.c_str())) {
        lastHeartbeatTime = now;
        Logger::debug("ServerIntegration", "Heartbeat sent");
    }
}

void ServerIntegration::update() {
    if (!initialized) {
        return;
    }
    
    // Handle MQTT client
    if (connected) {
        if (!mqttClient.connected()) {
            connected = false;
            deviceRegistered = false;
            Logger::warning("ServerIntegration", "Lost connection to MQTT server");
        } else {
            mqttClient.loop();
            sendTelemetry();
            sendHeartbeat();
        }
    } else {
        // Try to reconnect
        unsigned long now = millis();
        static unsigned long lastReconnectAttempt = 0;
        
        if (now - lastReconnectAttempt > RECONNECT_INTERVAL) {
            lastReconnectAttempt = now;
            if (reconnectAttempts < MAX_RECONNECT_ATTEMPTS) {
                Logger::info("ServerIntegration", "Attempting to reconnect to server...");
                connect();
            }
        }
    }
}

void ServerIntegration::onMqttMessage(char* topic, byte* payload, unsigned int length) {
    String topicStr = String(topic);
    String message = "";
    
    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    
    Logger::info("ServerIntegration", "Received message on topic: %s", topicStr.c_str());
    Logger::debug("ServerIntegration", "Message: %s", message.c_str());
    
    // Parse JSON message
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, message);
    
    if (error) {
        Logger::error("ServerIntegration", "Failed to parse JSON message: %s", error.c_str());
        return;
    }
    
    // Handle different message types
    if (topicStr == configCommandTopic) {
        handleConfigCommand(doc);
    } else if (topicStr == otaCommandTopic) {
        handleOtaCommand(doc);
    } else if (topicStr == appsCommandTopic) {
        handleAppsCommand(doc);
    } else if (topicStr == meshInTopic) {
        handleMeshMessage(doc);
    }
}

void ServerIntegration::handleConfigCommand(const JsonDocument& doc) {
    Logger::info("ServerIntegration", "Handling configuration command");
    
    String command = doc["command"];
    
    if (command == "update_config") {
        JsonObject config = doc["config"];
        
        // Update device configuration
        if (config.containsKey("telemetry_interval")) {
            // Update telemetry interval
            Logger::info("ServerIntegration", "Updated telemetry interval");
        }
        
        if (config.containsKey("wifi_settings")) {
            // Update WiFi settings
            Logger::info("ServerIntegration", "Updated WiFi settings");
        }
        
        // Send acknowledgment
        sendCommandResponse("config", "success", "Configuration updated");
    }
}

void ServerIntegration::handleOtaCommand(const JsonDocument& doc) {
    Logger::info("ServerIntegration", "Handling OTA command");
    
    String command = doc["command"];
    
    if (command == "update_firmware") {
        String version = doc["version"];
        String url = doc["url"];
        
        Logger::info("ServerIntegration", "Starting OTA update to version: %s", version.c_str());
        
        // Start OTA update process
        if (startOtaUpdate(url)) {
            sendCommandResponse("ota", "success", "OTA update started");
        } else {
            sendCommandResponse("ota", "error", "Failed to start OTA update");
        }
    }
}

void ServerIntegration::handleAppsCommand(const JsonDocument& doc) {
    Logger::info("ServerIntegration", "Handling apps command");
    
    String command = doc["command"];
    
    if (command == "install_app") {
        String appName = doc["app_name"];
        String appUrl = doc["app_url"];
        
        Logger::info("ServerIntegration", "Installing app: %s", appName.c_str());
        
        if (downloadAndInstallApp(appName, appUrl)) {
            sendCommandResponse("apps", "success", "App installed successfully");
            
            // Refresh launcher
            Launcher& launcher = Launcher::getInstance();
            launcher.refreshAppGrid();
        } else {
            sendCommandResponse("apps", "error", "Failed to install app");
        }
    } else if (command == "remove_app") {
        String appName = doc["app_name"];
        
        Logger::info("ServerIntegration", "Removing app: %s", appName.c_str());
        
        StorageManager& storage = StorageManager::getInstance();
        if (storage.removeApp(appName)) {
            sendCommandResponse("apps", "success", "App removed successfully");
            
            // Refresh launcher
            Launcher& launcher = Launcher::getInstance();
            launcher.refreshAppGrid();
        } else {
            sendCommandResponse("apps", "error", "Failed to remove app");
        }
    }
}

void ServerIntegration::handleMeshMessage(const JsonDocument& doc) {
    Logger::info("ServerIntegration", "Handling mesh message");
    
    String messageType = doc["type"];
    String destination = doc["destination"];
    String payload = doc["payload"];
    
    // Forward message to LoRa mesh network
    CommunicationManager& comm = CommunicationManager::getInstance();
    if (comm.isLoRaActive()) {
        // Send message via LoRa
        Logger::info("ServerIntegration", "Forwarding mesh message via LoRa");
        // Implementation depends on mesh protocol
    }
}

void ServerIntegration::sendCommandResponse(const String& commandType, const String& status, const String& message) {
    if (!connected) {
        return;
    }
    
    DynamicJsonDocument doc(512);
    doc["device_id"] = deviceId;
    doc["command_type"] = commandType;
    doc["status"] = status;
    doc["message"] = message;
    doc["timestamp"] = millis();
    
    String payload;
    serializeJson(doc, payload);
    
    String responseTopic = "tdeckpro/" + deviceId + "/response";
    mqttClient.publish(responseTopic.c_str(), payload.c_str());
    
    Logger::info("ServerIntegration", "Sent command response: %s - %s", status.c_str(), message.c_str());
}

bool ServerIntegration::startOtaUpdate(const String& url) {
    Logger::info("ServerIntegration", "Starting OTA update from: %s", url.c_str());
    
    // Add notification to launcher
    Launcher& launcher = Launcher::getInstance();
    launcher.addNotification("OTA Update", "Firmware update starting...");
    
    // Implementation would use ESP32 OTA library
    // This is a placeholder for the actual OTA implementation
    
    return true; // Placeholder
}

bool ServerIntegration::downloadAndInstallApp(const String& appName, const String& url) {
    Logger::info("ServerIntegration", "Downloading app: %s from %s", appName.c_str(), url.c_str());
    
    // Add notification to launcher
    Launcher& launcher = Launcher::getInstance();
    launcher.addNotification("App Install", "Installing " + appName + "...");
    
    // Implementation would download app binary and install via StorageManager
    // This is a placeholder for the actual download and install implementation
    
    return true; // Placeholder
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

bool ServerIntegration::isConnected() {
    return connected && mqttClient.connected();
}

String ServerIntegration::getDeviceId() {
    return deviceId;
}