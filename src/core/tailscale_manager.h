/**
 * @file tailscale_manager.h
 * @brief T-Deck-Pro Tailscale Manager - VPN and secure networking
 * @author T-Deck-Pro OS Team
 * @date 2025
 */

#ifndef TAILSCALE_MANAGER_H
#define TAILSCALE_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "../config/os_config.h"

/**
 * @brief Tailscale connection status
 */
enum class TailscaleStatus {
    TAILSCALE_DISABLED,
    INITIALIZING,
    AUTHENTICATING,
    CONNECTING,
    CONNECTED,
    DISCONNECTED,
    ERROR
};

/**
 * @brief Tailscale peer information
 */
struct TailscalePeer {
    String node_id;
    String hostname;
    String ip_address;
    String os;
    bool online;
    uint32_t last_seen;
    bool is_exit_node;
    String tags;
};

/**
 * @brief Tailscale network statistics
 */
struct TailscaleStats {
    uint32_t bytes_sent;
    uint32_t bytes_received;
    uint32_t packets_sent;
    uint32_t packets_received;
    uint32_t connection_time;
    String exit_node;
    int peer_count;
};

/**
 * @brief Tailscale Manager handles VPN connectivity
 */
class TailscaleManager {
public:
    TailscaleManager();
    ~TailscaleManager();
    
    /**
     * @brief Initialize Tailscale manager
     * @return true if successful
     */
    bool initialize();
    
    /**
     * @brief Update Tailscale manager (call periodically)
     */
    void update();
    
    /**
     * @brief Start Tailscale connection
     * @param auth_key Tailscale auth key
     * @param hostname Device hostname
     * @return true if connection initiated
     */
    bool connect(const String& auth_key, const String& hostname = "");
    
    /**
     * @brief Disconnect from Tailscale
     */
    void disconnect();
    
    /**
     * @brief Check if connected to Tailscale
     * @return true if connected
     */
    bool is_connected() const;
    
    /**
     * @brief Get current Tailscale status
     * @return Tailscale status
     */
    TailscaleStatus get_status() const;
    
    /**
     * @brief Get Tailscale IP address
     * @return Tailscale IP address or empty string
     */
    String get_tailscale_ip() const;
    
    /**
     * @brief Get device hostname in Tailscale network
     * @return Hostname
     */
    String get_hostname() const;
    
    /**
     * @brief Get list of Tailscale peers
     * @return Vector of peer information
     */
    std::vector<TailscalePeer> get_peers() const;
    
    /**
     * @brief Get network statistics
     * @return Network statistics
     */
    TailscaleStats get_statistics() const;
    
    /**
     * @brief Set exit node
     * @param node_id Node ID to use as exit node (empty to disable)
     * @return true if successful
     */
    bool set_exit_node(const String& node_id = "");
    
    /**
     * @brief Enable/disable subnet routing
     * @param enabled Enable subnet routing
     * @param subnets Comma-separated list of subnets to advertise
     * @return true if successful
     */
    bool set_subnet_routing(bool enabled, const String& subnets = "");
    
    /**
     * @brief Enable/disable SSH access
     * @param enabled Enable SSH access
     * @return true if successful
     */
    bool set_ssh_enabled(bool enabled);
    
    /**
     * @brief Get authentication URL for device registration
     * @return Authentication URL or empty string
     */
    String get_auth_url() const;
    
    /**
     * @brief Check if device needs authentication
     * @return true if authentication required
     */
    bool needs_authentication() const;
    
    /**
     * @brief Get Tailscale configuration as JSON
     * @return JSON configuration string
     */
    String get_config_json() const;
    
    /**
     * @brief Set Tailscale configuration from JSON
     * @param config_json JSON configuration
     * @return true if successful
     */
    bool set_config_json(const String& config_json);
    
    /**
     * @brief Enable/disable Tailscale
     * @param enabled Enable Tailscale
     */
    void set_enabled(bool enabled);
    
    /**
     * @brief Check if Tailscale is enabled
     * @return true if enabled
     */
    bool is_enabled() const;
    
    /**
     * @brief Get last error message
     * @return Error message
     */
    String get_last_error() const;
    
    /**
     * @brief Ping a Tailscale peer
     * @param peer_ip Peer IP address
     * @param timeout_ms Timeout in milliseconds
     * @return Ping time in ms, or -1 if failed
     */
    int ping_peer(const String& peer_ip, int timeout_ms = 5000);
    
private:
    /**
     * @brief Initialize Tailscale daemon
     * @return true if successful
     */
    bool init_daemon();
    
    /**
     * @brief Start Tailscale daemon
     * @return true if successful
     */
    bool start_daemon();
    
    /**
     * @brief Stop Tailscale daemon
     * @return true if successful
     */
    bool stop_daemon();
    
    /**
     * @brief Authenticate with Tailscale
     * @param auth_key Authentication key
     * @return true if successful
     */
    bool authenticate(const String& auth_key);
    
    /**
     * @brief Update peer list
     */
    void update_peers();
    
    /**
     * @brief Update network statistics
     */
    void update_statistics();
    
    /**
     * @brief Check daemon status
     * @return true if daemon is running
     */
    bool check_daemon_status();
    
    /**
     * @brief Execute Tailscale command
     * @param command Command to execute
     * @param args Command arguments
     * @return Command output
     */
    String execute_command(const String& command, const String& args = "");
    
    /**
     * @brief Parse status output
     * @param status_output Raw status output
     * @return true if parsing successful
     */
    bool parse_status(const String& status_output);
    
    /**
     * @brief Parse peer list output
     * @param peers_output Raw peers output
     * @return true if parsing successful
     */
    bool parse_peers(const String& peers_output);
    
    /**
     * @brief Save configuration to storage
     */
    void save_config();
    
    /**
     * @brief Load configuration from storage
     */
    void load_config();
    
    /**
     * @brief Handle connection state change
     * @param new_status New connection status
     */
    void handle_status_change(TailscaleStatus new_status);
    
    /**
     * @brief Setup network interface
     * @return true if successful
     */
    bool setup_network_interface();
    
    /**
     * @brief Cleanup network interface
     */
    void cleanup_network_interface();
    
    // Status and configuration
    TailscaleStatus status;
    bool enabled;
    String auth_key;
    String hostname;
    String tailscale_ip;
    String auth_url;
    String last_error;
    
    // Network information
    std::vector<TailscalePeer> peers;
    TailscaleStats stats;
    String current_exit_node;
    bool ssh_enabled;
    String advertised_subnets;
    
    // Timing
    uint32_t last_status_check;
    uint32_t last_peer_update;
    uint32_t last_stats_update;
    uint32_t connection_start_time;
    
    // Configuration
    struct TailscaleConfig {
        bool auto_connect;
        bool accept_routes;
        bool accept_dns;
        String login_server;
        String control_url;
        int keepalive_interval;
        bool shields_up;
    } config;
    
    // Constants
    static const uint32_t STATUS_CHECK_INTERVAL = 10000;  // 10 seconds
    static const uint32_t PEER_UPDATE_INTERVAL = 30000;   // 30 seconds
    static const uint32_t STATS_UPDATE_INTERVAL = 60000;  // 1 minute
    static const uint32_t AUTH_TIMEOUT = 300000;          // 5 minutes
    static const int MAX_PEERS = 50;
    static const int MAX_COMMAND_OUTPUT = 4096;
    
    // File paths
    static const String CONFIG_FILE;
    static const String STATE_FILE;
    static const String LOG_FILE;
};

#endif // TAILSCALE_MANAGER_H