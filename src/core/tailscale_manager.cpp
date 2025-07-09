/**
 * @file tailscale_manager.cpp
 * @brief T-Deck-Pro Tailscale Manager Implementation
 * @author T-Deck-Pro OS Team
 * @date 2025
 */

#include "tailscale_manager.h"
#include <SPIFFS.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <lwip/sockets.h>
#include <lwip/netdb.h>

// Static file paths
const String TailscaleManager::CONFIG_FILE = "/tailscale/config.json";
const String TailscaleManager::STATE_FILE = "/tailscale/state.json";
const String TailscaleManager::LOG_FILE = "/tailscale/tailscale.log";

TailscaleManager::TailscaleManager() :
    status(TailscaleStatus::TAILSCALE_DISABLED),
    enabled(false),
    ssh_enabled(false),
    last_status_check(0),
    last_peer_update(0),
    last_stats_update(0),
    connection_start_time(0) {
    
    // Initialize configuration with defaults
    config.auto_connect = true;
    config.accept_routes = true;
    config.accept_dns = true;
    config.keepalive_interval = 25;
    config.shields_up = false;
    
    // Initialize statistics
    memset(&stats, 0, sizeof(stats));
    
    // Generate default hostname
    if (hostname.isEmpty()) {
        uint64_t chipid = ESP.getEfuseMac();
        hostname = "tdeck-" + String((uint32_t)(chipid >> 32), HEX) + String((uint32_t)chipid, HEX);
    }
}

TailscaleManager::~TailscaleManager() {
    disconnect();
}

bool TailscaleManager::initialize() {
    Serial.println("[Tailscale] Initializing Tailscale Manager");
    
    // Load saved configuration
    load_config();
    
    // Initialize daemon if enabled
    if (enabled) {
        return init_daemon();
    }
    
    return true;
}

void TailscaleManager::update() {
    if (!enabled || status == TailscaleStatus::TAILSCALE_DISABLED) {
        return;
    }
    
    uint32_t now = millis();
    
    // Check daemon status periodically
    if (now - last_status_check > STATUS_CHECK_INTERVAL) {
        check_daemon_status();
        last_status_check = now;
    }
    
    // Update peer list
    if (status == TailscaleStatus::CONNECTED && 
        now - last_peer_update > PEER_UPDATE_INTERVAL) {
        update_peers();
        last_peer_update = now;
    }
    
    // Update statistics
    if (status == TailscaleStatus::CONNECTED && 
        now - last_stats_update > STATS_UPDATE_INTERVAL) {
        update_statistics();
        last_stats_update = now;
    }
}

bool TailscaleManager::connect(const String& auth_key, const String& hostname) {
    if (!enabled) {
        enabled = true;
        save_config();
    }
    
    this->auth_key = auth_key;
    if (!hostname.isEmpty()) {
        this->hostname = hostname;
    }
    
    Serial.printf("[Tailscale] Connecting with hostname: %s\n", this->hostname.c_str());
    
    // Initialize daemon
    if (!init_daemon()) {
        return false;
    }
    
    // Start daemon
    if (!start_daemon()) {
        return false;
    }
    
    // Authenticate
    if (!auth_key.isEmpty()) {
        return authenticate(auth_key);
    }
    
    return true;
}

void TailscaleManager::disconnect() {
    if (status != TailscaleStatus::TAILSCALE_DISABLED) {
        Serial.println("[Tailscale] Disconnecting");
        stop_daemon();
        cleanup_network_interface();
        status = TailscaleStatus::DISCONNECTED;
    }
}

bool TailscaleManager::is_connected() const {
    return status == TailscaleStatus::CONNECTED;
}

TailscaleStatus TailscaleManager::get_status() const {
    return status;
}

String TailscaleManager::get_tailscale_ip() const {
    return tailscale_ip;
}

String TailscaleManager::get_hostname() const {
    return hostname;
}

std::vector<TailscalePeer> TailscaleManager::get_peers() const {
    return peers;
}

TailscaleStats TailscaleManager::get_statistics() const {
    return stats;
}

bool TailscaleManager::set_exit_node(const String& node_id) {
    if (status != TailscaleStatus::CONNECTED) {
        return false;
    }
    
    String command = "set";
    String args = node_id.isEmpty() ? "--exit-node=" : "--exit-node=" + node_id;
    
    String result = execute_command(command, args);
    if (!result.isEmpty() && !result.startsWith("Error")) {
        current_exit_node = node_id;
        Serial.printf("[Tailscale] Exit node set to: %s\n", node_id.c_str());
        return true;
    }
    
    return false;
}

bool TailscaleManager::set_subnet_routing(bool enabled, const String& subnets) {
    if (status != TailscaleStatus::CONNECTED) {
        return false;
    }
    
    String command = "set";
    String args = enabled ? "--advertise-routes=" + subnets : "--advertise-routes=";
    
    String result = execute_command(command, args);
    if (!result.isEmpty() && !result.startsWith("Error")) {
        advertised_subnets = enabled ? subnets : "";
        Serial.printf("[Tailscale] Subnet routing: %s\n", enabled ? "enabled" : "disabled");
        return true;
    }
    
    return false;
}

bool TailscaleManager::set_ssh_enabled(bool enabled) {
    if (status != TailscaleStatus::CONNECTED) {
        return false;
    }
    
    String command = "set";
    String args = enabled ? "--ssh" : "--ssh=false";
    
    String result = execute_command(command, args);
    if (!result.isEmpty() && !result.startsWith("Error")) {
        ssh_enabled = enabled;
        Serial.printf("[Tailscale] SSH: %s\n", enabled ? "enabled" : "disabled");
        return true;
    }
    
    return false;
}

String TailscaleManager::get_auth_url() const {
    return auth_url;
}

bool TailscaleManager::needs_authentication() const {
    return status == TailscaleStatus::AUTHENTICATING && !auth_url.isEmpty();
}

String TailscaleManager::get_config_json() const {
    DynamicJsonDocument doc(1024);
    
    doc["enabled"] = enabled;
    doc["hostname"] = hostname;
    doc["auto_connect"] = config.auto_connect;
    doc["accept_routes"] = config.accept_routes;
    doc["accept_dns"] = config.accept_dns;
    doc["login_server"] = config.login_server;
    doc["control_url"] = config.control_url;
    doc["keepalive_interval"] = config.keepalive_interval;
    doc["shields_up"] = config.shields_up;
    doc["ssh_enabled"] = ssh_enabled;
    doc["exit_node"] = current_exit_node;
    doc["advertised_subnets"] = advertised_subnets;
    
    String result;
    serializeJson(doc, result);
    return result;
}

bool TailscaleManager::set_config_json(const String& config_json) {
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, config_json);
    
    if (error) {
        last_error = "Invalid JSON configuration";
        return false;
    }
    
    enabled = doc["enabled"].as<bool>();
    hostname = doc["hostname"].as<String>();
    config.auto_connect = doc["auto_connect"].as<bool>();
    config.accept_routes = doc["accept_routes"].as<bool>();
    config.accept_dns = doc["accept_dns"].as<bool>();
    config.login_server = doc["login_server"].as<String>();
    config.control_url = doc["control_url"].as<String>();
    config.keepalive_interval = doc["keepalive_interval"].as<int>();
    config.shields_up = doc["shields_up"].as<bool>();
    ssh_enabled = doc["ssh_enabled"].as<bool>();
    current_exit_node = doc["exit_node"].as<String>();
    advertised_subnets = doc["advertised_subnets"].as<String>();
    
    save_config();
    return true;
}

void TailscaleManager::set_enabled(bool enabled) {
    this->enabled = enabled;
    if (!enabled && status != TailscaleStatus::TAILSCALE_DISABLED) {
        disconnect();
        status = TailscaleStatus::TAILSCALE_DISABLED;
    }
    save_config();
}

bool TailscaleManager::is_enabled() const {
    return enabled;
}

String TailscaleManager::get_last_error() const {
    return last_error;
}

int TailscaleManager::ping_peer(const String& peer_ip, int timeout_ms) {
    // Simple ping implementation using socket
    struct sockaddr_in addr;
    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    
    if (sock < 0) {
        return -1;
    }
    
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, peer_ip.c_str(), &addr.sin_addr);
    
    uint32_t start_time = millis();
    
    // Send ICMP echo request (simplified)
    uint8_t packet[64];
    memset(packet, 0, sizeof(packet));
    packet[0] = 8; // ICMP Echo Request
    
    if (sendto(sock, packet, sizeof(packet), 0, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(sock);
        return -1;
    }
    
    // Wait for response (simplified)
    fd_set readfds;
    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    
    FD_ZERO(&readfds);
    FD_SET(sock, &readfds);
    
    if (select(sock + 1, &readfds, NULL, NULL, &tv) > 0) {
        uint32_t ping_time = millis() - start_time;
        close(sock);
        return ping_time;
    }
    
    close(sock);
    return -1;
}

bool TailscaleManager::init_daemon() {
    Serial.println("[Tailscale] Initializing daemon");
    
    status = TailscaleStatus::INITIALIZING;
    
    // Setup network interface
    if (!setup_network_interface()) {
        last_error = "Failed to setup network interface";
        status = TailscaleStatus::ERROR;
        return false;
    }
    
    return true;
}

bool TailscaleManager::start_daemon() {
    Serial.println("[Tailscale] Starting daemon");
    
    // Note: On ESP32, we simulate Tailscale functionality
    // In a real implementation, this would start the actual Tailscale daemon
    
    status = TailscaleStatus::CONNECTING;
    connection_start_time = millis();
    
    return true;
}

bool TailscaleManager::stop_daemon() {
    Serial.println("[Tailscale] Stopping daemon");
    
    // Cleanup resources
    peers.clear();
    tailscale_ip = "";
    auth_url = "";
    
    return true;
}

bool TailscaleManager::authenticate(const String& auth_key) {
    Serial.println("[Tailscale] Authenticating");
    
    status = TailscaleStatus::AUTHENTICATING;
    
    // Simulate authentication process
    // In real implementation, this would use the Tailscale API
    
    if (auth_key.length() > 10) { // Basic validation
        // Simulate successful authentication
        tailscale_ip = "100.64.0." + String(random(1, 254));
        status = TailscaleStatus::CONNECTED;
        
        Serial.printf("[Tailscale] Connected with IP: %s\n", tailscale_ip.c_str());
        return true;
    } else {
        last_error = "Invalid authentication key";
        status = TailscaleStatus::ERROR;
        return false;
    }
}

void TailscaleManager::update_peers() {
    // Simulate peer discovery
    // In real implementation, this would query the Tailscale daemon
    
    if (peers.empty()) {
        // Add some example peers
        TailscalePeer peer1 = {
            "peer1",
            "laptop",
            "100.64.0.1",
            "linux",
            true,
            millis(),
            false,
            "tag:device"
        };
        
        TailscalePeer peer2 = {
            "peer2",
            "server",
            "100.64.0.2",
            "linux",
            true,
            millis(),
            true,
            "tag:server"
        };
        
        peers.push_back(peer1);
        peers.push_back(peer2);
        
        stats.peer_count = peers.size();
    }
}

void TailscaleManager::update_statistics() {
    // Update network statistics
    // In real implementation, this would read from network interfaces
    
    stats.connection_time = millis() - connection_start_time;
    stats.bytes_sent += random(100, 1000);
    stats.bytes_received += random(100, 1000);
    stats.packets_sent += random(1, 10);
    stats.packets_received += random(1, 10);
}

bool TailscaleManager::check_daemon_status() {
    // Check if daemon is still running
    // In real implementation, this would check the actual daemon process
    
    if (status == TailscaleStatus::CONNECTED) {
        // Simulate occasional disconnections
        if (random(0, 1000) < 5) { // 0.5% chance
            status = TailscaleStatus::DISCONNECTED;
            last_error = "Connection lost";
            return false;
        }
    }
    
    return true;
}

String TailscaleManager::execute_command(const String& command, const String& args) {
    // Simulate command execution
    // In real implementation, this would execute actual Tailscale commands
    
    Serial.printf("[Tailscale] Executing: %s %s\n", command.c_str(), args.c_str());
    
    if (command == "status") {
        return "Connected to Tailscale";
    } else if (command == "set") {
        return "Settings updated";
    }
    
    return "Command executed";
}

bool TailscaleManager::parse_status(const String& status_output) {
    // Parse status output and update internal state
    // In real implementation, this would parse actual Tailscale status output
    
    if (status_output.indexOf("Connected") >= 0) {
        status = TailscaleStatus::CONNECTED;
        return true;
    }
    
    return false;
}

bool TailscaleManager::parse_peers(const String& peers_output) {
    // Parse peers output and update peer list
    // In real implementation, this would parse actual Tailscale peer output
    
    return true;
}

void TailscaleManager::save_config() {
    if (!SPIFFS.begin()) {
        return;
    }
    
    File file = SPIFFS.open(CONFIG_FILE, "w");
    if (file) {
        file.print(get_config_json());
        file.close();
        Serial.println("[Tailscale] Configuration saved");
    }
}

void TailscaleManager::load_config() {
    if (!SPIFFS.begin()) {
        return;
    }
    
    File file = SPIFFS.open(CONFIG_FILE, "r");
    if (file) {
        String config_data = file.readString();
        file.close();
        
        if (!config_data.isEmpty()) {
            set_config_json(config_data);
            Serial.println("[Tailscale] Configuration loaded");
        }
    }
}

void TailscaleManager::handle_status_change(TailscaleStatus new_status) {
    if (status != new_status) {
        TailscaleStatus old_status = status;
        status = new_status;
        
        Serial.printf("[Tailscale] Status changed: %d -> %d\n", (int)old_status, (int)new_status);
        
        // Handle specific status changes
        if (new_status == TailscaleStatus::CONNECTED) {
            Serial.printf("[Tailscale] Connected with IP: %s\n", tailscale_ip.c_str());
        } else if (new_status == TailscaleStatus::ERROR) {
            Serial.printf("[Tailscale] Error: %s\n", last_error.c_str());
        }
    }
}

bool TailscaleManager::setup_network_interface() {
    // Setup network interface for Tailscale
    // In real implementation, this would configure TUN/TAP interface
    
    Serial.println("[Tailscale] Setting up network interface");
    return true;
}

void TailscaleManager::cleanup_network_interface() {
    // Cleanup network interface
    Serial.println("[Tailscale] Cleaning up network interface");
}