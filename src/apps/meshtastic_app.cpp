/**
 * @file meshtastic_app.cpp
 * @brief T-Deck-Pro Meshtastic App - LoRa Mesh Networking
 * @author T-Deck-Pro OS Team
 * @date 2025
 * @note Provides LoRa mesh networking with message routing and node discovery
 */

#include "meshtastic_app.h"
#include "../core/utils/logger.h"
#include "../core/storage/storage_manager.h"
#include <ArduinoJson.h>

// ===== MESHTASTIC APP IMPLEMENTATION =====

MeshtasticApp::MeshtasticApp() : initialized_(false), lora_initialized_(false),
                                  node_id_(0), channel_(0), encryption_enabled_(false),
                                  message_count_(0), last_message_time_(0), 
                                  connected_nodes_(0), max_nodes_(50) {
    
    // Initialize message queue
    messages_.reserve(100);
    nodes_.reserve(max_nodes_);
    
    // Initialize app info
    app_info_.name = "Meshtastic";
    app_info_.description = "LoRa mesh networking and messaging";
    app_info_.type = AppType::SYSTEM;
    app_info_.enabled = true;
    app_info_.memory_usage = 0;
    app_info_.last_used = 0;
}

MeshtasticApp::~MeshtasticApp() {
    if (initialized_) {
        cleanup();
    }
}

bool MeshtasticApp::init() {
    if (initialized_) {
        return true;
    }
    
    logInfo("MESHTASTIC", "Initializing Meshtastic app");
    
    // Initialize storage manager if not already done
    if (!getStorageManager()) {
        if (!initializeStorageManager()) {
            logError("MESHTASTIC", "Failed to initialize storage manager");
            return false;
        }
    }
    
    // Load configuration
    loadConfig();
    
    // Initialize LoRa
    if (!initLoRa()) {
        logError("MESHTASTIC", "Failed to initialize LoRa");
        return false;
    }
    
    // Initialize UI
    if (!initUI()) {
        logError("MESHTASTIC", "Failed to initialize UI");
        return false;
    }
    
    initialized_ = true;
    app_info_.last_used = millis();
    
    logInfo("MESHTASTIC", "Meshtastic app initialized successfully");
    return true;
}

void MeshtasticApp::run() {
    if (!initialized_) {
        if (!init()) {
            logError("MESHTASTIC", "Failed to initialize Meshtastic app");
            return;
        }
    }
    
    // Update app usage time
    app_info_.last_used = millis();
    
    // Process LoRa messages
    processLoRaMessages();
    
    // Process user input
    processInput();
    
    // Update display
    updateDisplay();
    
    // Periodic tasks
    processPeriodicTasks();
}

void MeshtasticApp::cleanup() {
    if (!initialized_) {
        return;
    }
    
    logInfo("MESHTASTIC", "Cleaning up Meshtastic app");
    
    // Save configuration
    saveConfig();
    
    // Clear messages and nodes
    messages_.clear();
    nodes_.clear();
    
    // Shutdown LoRa
    shutdownLoRa();
    
    initialized_ = false;
}

const char* MeshtasticApp::getName() {
    return "Meshtastic";
}

bool MeshtasticApp::initLoRa() {
    // Initialize LoRa hardware
    // This will be implemented when LoRa driver is available
    
    logInfo("MESHTASTIC", "LoRa initialized");
    lora_initialized_ = true;
    return true;
}

bool MeshtasticApp::initUI() {
    // Create main UI elements
    // This will be implemented when LVGL is available
    
    logInfo("MESHTASTIC", "UI initialized");
    return true;
}

void MeshtasticApp::loadConfig() {
    StorageManager* storage = getStorageManager();
    if (!storage) {
        return;
    }
    
    DynamicJsonDocument doc(1024);
    if (storage->loadJSON("/config/meshtastic.json", doc, StorageType::SPIFFS)) {
        // Load configuration from JSON
        if (doc.containsKey("node_id")) {
            node_id_ = doc["node_id"].as<uint32_t>();
        }
        if (doc.containsKey("channel")) {
            channel_ = doc["channel"].as<uint8_t>();
        }
        if (doc.containsKey("encryption_enabled")) {
            encryption_enabled_ = doc["encryption_enabled"].as<bool>();
        }
        if (doc.containsKey("max_nodes")) {
            max_nodes_ = doc["max_nodes"].as<uint8_t>();
        }
        
        logInfo("MESHTASTIC", "Configuration loaded");
    } else {
        logInfo("MESHTASTIC", "No configuration found, using defaults");
        generateNodeId();
    }
}

void MeshtasticApp::saveConfig() {
    StorageManager* storage = getStorageManager();
    if (!storage) {
        return;
    }
    
    DynamicJsonDocument doc(1024);
    doc["node_id"] = node_id_;
    doc["channel"] = channel_;
    doc["encryption_enabled"] = encryption_enabled_;
    doc["max_nodes"] = max_nodes_;
    
    if (storage->saveJSON("/config/meshtastic.json", doc, StorageType::SPIFFS)) {
        logInfo("MESHTASTIC", "Configuration saved");
    } else {
        logError("MESHTASTIC", "Failed to save configuration");
    }
}

void MeshtasticApp::generateNodeId() {
    // Generate a unique node ID based on hardware
    node_id_ = (uint32_t)random(0x10000000, 0xFFFFFFFF);
    logInfo("MESHTASTIC", "Generated node ID: " + String(node_id_, HEX));
}

void MeshtasticApp::processLoRaMessages() {
    if (!lora_initialized_) {
        return;
    }
    
    // Check for incoming messages
    // This will be implemented when LoRa driver is available
    
    // Process message queue
    processMessageQueue();
}

void MeshtasticApp::processMessageQueue() {
    for (auto it = messages_.begin(); it != messages_.end();) {
        MeshMessage& msg = *it;
        
        // Check if message should be retransmitted
        if (msg.retransmit_count < MAX_RETRANSMIT_COUNT && 
            (millis() - msg.timestamp) > RETRANSMIT_DELAY) {
            
            if (retransmitMessage(msg)) {
                msg.retransmit_count++;
                msg.timestamp = millis();
                ++it;
            } else {
                // Remove failed message
                it = messages_.erase(it);
            }
        } else if (msg.retransmit_count >= MAX_RETRANSMIT_COUNT) {
            // Remove expired message
            it = messages_.erase(it);
        } else {
            ++it;
        }
    }
}

void MeshtasticApp::processInput() {
    // Handle keyboard input
    // This will be implemented when keyboard input is available
    
    // Handle touch input
    // This will be implemented when touch input is available
}

void MeshtasticApp::updateDisplay() {
    // Update display with current mesh status
    // This will be implemented when display manager is available
    
    // For now, just log the current state
    logDebug("MESHTASTIC", "Display updated - " + String(connected_nodes_) + " nodes, " + 
             String(messages_.size()) + " messages");
}

void MeshtasticApp::processPeriodicTasks() {
    uint32_t now = millis();
    
    // Update node list every 30 seconds
    if (now - last_node_update_ > 30000) {
        updateNodeList();
        last_node_update_ = now;
    }
    
    // Clean up old messages every 60 seconds
    if (now - last_cleanup_ > 60000) {
        cleanupOldMessages();
        last_cleanup_ = now;
    }
}

bool MeshtasticApp::sendMessage(const String& message, uint32_t destination_id) {
    if (!lora_initialized_) {
        logError("MESHTASTIC", "LoRa not initialized");
        return false;
    }
    
    MeshMessage msg;
    msg.id = generateMessageId();
    msg.source_id = node_id_;
    msg.destination_id = destination_id;
    msg.message = message;
    msg.timestamp = millis();
    msg.retransmit_count = 0;
    msg.type = MessageType::TEXT;
    
    // Add to message queue
    messages_.push_back(msg);
    
    // Send immediately
    if (transmitMessage(msg)) {
        logInfo("MESHTASTIC", "Message sent: " + message);
        message_count_++;
        last_message_time_ = millis();
        return true;
    } else {
        logError("MESHTASTIC", "Failed to send message");
        return false;
    }
}

bool MeshtasticApp::sendBroadcast(const String& message) {
    return sendMessage(message, BROADCAST_ID);
}

bool MeshtasticApp::transmitMessage(const MeshMessage& message) {
    // This will be implemented when LoRa driver is available
    // For now, just simulate transmission
    
    logDebug("MESHTASTIC", "Transmitting message ID: " + String(message.id, HEX));
    return true;
}

bool MeshtasticApp::retransmitMessage(const MeshMessage& message) {
    // This will be implemented when LoRa driver is available
    // For now, just simulate retransmission
    
    logDebug("MESHTASTIC", "Retransmitting message ID: " + String(message.id, HEX));
    return true;
}

void MeshtasticApp::receiveMessage(const MeshMessage& message) {
    // Check if we've already seen this message
    for (const auto& existing : messages_) {
        if (existing.id == message.id) {
            return; // Already processed
        }
    }
    
    // Add to message list
    messages_.push_back(message);
    
    // Update node list
    updateNodeInfo(message.source_id);
    
    logInfo("MESHTASTIC", "Received message from " + String(message.source_id, HEX) + 
            ": " + message.message);
}

void MeshtasticApp::updateNodeInfo(uint32_t node_id) {
    // Check if node already exists
    for (auto& node : nodes_) {
        if (node.id == node_id) {
            node.last_seen = millis();
            return;
        }
    }
    
    // Add new node
    if (nodes_.size() < max_nodes_) {
        MeshNode node;
        node.id = node_id;
        node.last_seen = millis();
        node.message_count = 0;
        nodes_.push_back(node);
        connected_nodes_ = nodes_.size();
        
        logInfo("MESHTASTIC", "New node discovered: " + String(node_id, HEX));
    }
}

void MeshtasticApp::updateNodeList() {
    uint32_t now = millis();
    
    // Remove nodes that haven't been seen recently
    for (auto it = nodes_.begin(); it != nodes_.end();) {
        if ((now - it->last_seen) > NODE_TIMEOUT) {
            logInfo("MESHTASTIC", "Node timeout: " + String(it->id, HEX));
            it = nodes_.erase(it);
        } else {
            ++it;
        }
    }
    
    connected_nodes_ = nodes_.size();
}

void MeshtasticApp::cleanupOldMessages() {
    uint32_t now = millis();
    
    // Remove messages older than 1 hour
    for (auto it = messages_.begin(); it != messages_.end();) {
        if ((now - it->timestamp) > MESSAGE_TIMEOUT) {
            it = messages_.erase(it);
        } else {
            ++it;
        }
    }
}

uint32_t MeshtasticApp::generateMessageId() {
    static uint32_t message_counter = 0;
    return (node_id_ << 16) | (++message_counter & 0xFFFF);
}

void MeshtasticApp::shutdownLoRa() {
    if (lora_initialized_) {
        // Shutdown LoRa hardware
        // This will be implemented when LoRa driver is available
        
        logInfo("MESHTASTIC", "LoRa shutdown");
        lora_initialized_ = false;
    }
}

uint32_t MeshtasticApp::getNodeId() const {
    return node_id_;
}

void MeshtasticApp::setNodeId(uint32_t node_id) {
    node_id_ = node_id;
    logInfo("MESHTASTIC", "Node ID set to: " + String(node_id_, HEX));
}

uint8_t MeshtasticApp::getChannel() const {
    return channel_;
}

void MeshtasticApp::setChannel(uint8_t channel) {
    channel_ = channel;
    logInfo("MESHTASTIC", "Channel set to: " + String(channel_));
}

bool MeshtasticApp::isEncryptionEnabled() const {
    return encryption_enabled_;
}

void MeshtasticApp::setEncryptionEnabled(bool enabled) {
    encryption_enabled_ = enabled;
    logInfo("MESHTASTIC", "Encryption " + String(enabled ? "enabled" : "disabled"));
}

uint8_t MeshtasticApp::getConnectedNodes() const {
    return connected_nodes_;
}

uint32_t MeshtasticApp::getMessageCount() const {
    return message_count_;
}

std::vector<MeshMessage> MeshtasticApp::getMessages() const {
    return messages_;
}

std::vector<MeshNode> MeshtasticApp::getNodes() const {
    return nodes_;
}

String MeshtasticApp::getNetworkStatus() {
    DynamicJsonDocument doc(1024);
    
    doc["node_id"] = String(node_id_, HEX);
    doc["channel"] = channel_;
    doc["connected_nodes"] = connected_nodes_;
    doc["message_count"] = message_count_;
    doc["encryption"] = encryption_enabled_;
    doc["lora_initialized"] = lora_initialized_;
    
    String output;
    serializeJson(doc, output);
    return output;
}

// ===== GLOBAL MESHTASTIC APP INSTANCE =====
MeshtasticApp* g_meshtastic_app = nullptr;

// ===== MESHTASTIC APP UTILITY FUNCTIONS =====

bool initializeMeshtasticApp() {
    if (g_meshtastic_app) {
        return true;
    }
    
    g_meshtastic_app = new MeshtasticApp();
    if (!g_meshtastic_app) {
        return false;
    }
    
    return g_meshtastic_app->init();
}

MeshtasticApp* getMeshtasticApp() {
    return g_meshtastic_app;
} 