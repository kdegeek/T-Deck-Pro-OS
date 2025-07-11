# WireGuard Integration for T-Deck-Pro OS

## Overview

WireGuard VPN functionality has been integrated into the T-Deck-Pro OS by adding the `esp_wireguard` library to the project's library collection.

## Integration Details

### Library Location
- **Path**: `lib/wireguard/`
- **Source**: https://github.com/trombik/esp_wireguard
- **Integration Date**: January 10, 2025

### Library Structure
```
lib/wireguard/
├── CMakeLists.txt          # Build configuration
├── component.mk            # ESP-IDF component makefile
├── idf_component.yml       # Component manifest
├── Kconfig                 # Configuration options
├── LICENSE                 # BSD 3-Clause license
├── README.md               # Library documentation
├── examples/               # Example implementations
│   └── demo/
├── include/                # Header files
└── src/                    # Source code
```

## Library Capabilities

### Supported Features
- **WireGuard VPN tunneling** for ESP32-based devices
- **Single tunnel support** to WireGuard peers
- **IPv4 and IPv6 support** (IPv6 is alpha)
- **ESP-NETIF and TCP/IP Adapter** compatibility

### Supported ESP-IDF Versions
- ESP-IDF master
- ESP-IDF v4.2.x, v4.3.x, v4.4.x
- ESP8266 RTOS SDK v3.4

### Supported Targets
- ESP32 (✅ Compatible with T-Deck-Pro)
- ESP32-S2
- ESP32-C3
- ESP8266

## Integration with T-Deck-Pro OS

### VPN Architecture
The T-Deck-Pro OS uses WireGuard as its primary VPN solution, providing modern, secure, and high-performance networking capabilities for the device.

### Potential Integration Points
1. **Core Services**: Create `wireguard_manager.h/cpp` similar to existing managers
2. **Settings App**: Add WireGuard configuration options
3. **Network Management**: Integrate with existing connectivity management
4. **MQTT Integration**: Report WireGuard status via MQTT telemetry

## Usage Example

```cpp
#include <esp_wireguard.h>

// Basic WireGuard configuration
wireguard_config_t wg_config = ESP_WIREGUARD_CONFIG_DEFAULT();
wg_config.private_key = CONFIG_WG_PRIVATE_KEY;
wg_config.listen_port = CONFIG_WG_LOCAL_PORT;
wg_config.public_key = CONFIG_WG_PEER_PUBLIC_KEY;
wg_config.allowed_ip = CONFIG_WG_LOCAL_IP_ADDRESS;
wg_config.allowed_ip_mask = CONFIG_WG_LOCAL_IP_NETMASK;
wg_config.endpoint = CONFIG_WG_PEER_ADDRESS;
wg_config.port = CONFIG_WG_PEER_PORT;

// Initialize and connect
wireguard_ctx_t ctx = {0};
esp_wireguard_init(&wg_config, &ctx);
esp_wireguard_connect(&ctx);

// Check connection status
if (esp_wireguardif_peer_is_up(&ctx) == ESP_OK) {
    // WireGuard tunnel is active
}

// Disconnect when done
esp_wireguard_disconnect(&ctx);
```

## Configuration Requirements

### Prerequisites
- **Time synchronization**: Both peers must have synced time
- **Network interface**: Working WiFi connection required
- **Menuconfig**: Configure TCP/IP adapter (ESP-NETIF recommended)

### Configuration Options
Available in menuconfig under `[Component config]` -> `[WireGuard]`:
- **TCP/IP Adapter**: Choose ESP-NETIF or TCP/IP Adapter
- **x25519 Implementation**: Default or NaCL variants
- **IPv6 Support**: Enable `CONFIG_LWIP_IPV6` if needed

## Known Limitations

- **Alpha status**: Code is in alpha development stage
- **Single tunnel**: Only one WireGuard tunnel supported per device
- **WiFi only**: Ethernet interfaces not supported
- **IPv6**: IPv6 support is experimental and may be unstable
- **Dual stack**: IPv4/IPv6 dual stack not supported

## Next Steps for Integration

1. **Create WireGuard Manager**: Implement `src/core/wireguard_manager.h/cpp`
2. **Settings Integration**: Add WireGuard config to Settings app
3. **UI Integration**: Add WireGuard status to launcher interface
4. **MQTT Telemetry**: Include WireGuard status in system telemetry
5. **Configuration Storage**: Implement persistent WireGuard configuration
6. **Network Management**: Integrate with existing network connectivity services

## References

- **Library Repository**: https://github.com/trombik/esp_wireguard
- **WireGuard Official**: https://www.wireguard.com/
- **Base Implementation**: https://github.com/smartalock/wireguard-lwip
- **Examples**: `lib/wireguard/examples/demo/`

---

*This integration provides the T-Deck-Pro OS with professional-grade VPN capabilities suitable for secure mesh networking and remote device management.*