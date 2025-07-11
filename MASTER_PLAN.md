# T-Deck-Pro OS Development Documentation

## 1. Project Overview

The T-Deck-Pro OS is a comprehensive firmware solution for the T-Deck-Pro device, which is based on the ESP32-S3 microcontroller. This documentation serves as a framework for both completed components and pending development tasks, with a focus on hardware integration, software architecture, and system optimization.

### 1.1 Hardware Specifications

#### Core Components
- **MCU**: ESP32-S3 (240MHz, 16MB Flash, 8MB PSRAM)
- **Display**: 320x240 E-paper (GDEQ031T10) with UC8253 driver
- **Communication**: 
  - LoRa SX1262
  - GPS MIA-M10Q
  - A7682E 4G modem
  - WiFi 802.11 b/g/n
- **Input**: 
  - TCA8418 keyboard matrix
  - CST328 touch controller
- **Sensors**: 
  - BHI260AP gyroscope
  - BQ27220 battery gauge
  - BQ25896 power management
- **Storage**: MicroSD card slot
- **Power**: 18650 battery with USB-C charging

#### Pin Configuration

| Component | Pin | Function | Notes |
|-----------|-----|----------|-------|
| **Display** |
| | GPIO6 | E-paper BUSY | Input |
| | GPIO7 | E-paper DC | Output |
| | GPIO10 | E-paper CS | Output |
| | GPIO11 | E-paper RESET | Output |
| | GPIO12 | SPI MOSI | Output |
| | GPIO13 | SPI MISO | Input |
| | GPIO14 | SPI CLK | Output |
| **Touch** |
| | GPIO16 | Touch SDA | I2C Data |
| | GPIO15 | Touch SCL | I2C Clock |
| | GPIO17 | Touch INT | Interrupt |
| | GPIO18 | Touch RESET | Output |
| **LoRa** |
| | GPIO37 | LoRa DIO1 | Interrupt |
| | GPIO38 | LoRa BUSY | Input |
| | GPIO39 | LoRa RESET | Output |
| | GPIO40 | LoRa CS | Output |
| **Keyboard** |
| | GPIO1 | Keyboard SDA | I2C Data |
| | GPIO2 | Keyboard SCL | I2C Clock |
| | GPIO3 | Keyboard INT | Interrupt |
| **4G Modem** |
| | GPIO42 | 4G TX | UART TX |
| | GPIO41 | 4G RX | UART RX |
| | GPIO21 | 4G PWRKEY | Output |
| | GPIO47 | 4G RESET | Output |
| **SD Card** |
| | GPIO36 | SD CMD | Command |
| | GPIO35 | SD CLK | Clock |
| | GPIO37 | SD D0 | Data 0 |
| | GPIO38 | SD D1 | Data 1 |
| | GPIO33 | SD D2 | Data 2 |
| | GPIO34 | SD D3 | Data 3 |
| **Power Management** |
| | GPIO4 | PMU SDA | I2C Data |
| | GPIO5 | PMU SCL | I2C Clock |
| | GPIO46 | PMU IRQ | Interrupt |

### 1.2 Memory Constraints

- **Internal RAM**: ~512KB available for critical operations
- **PSRAM**: 8MB for display buffers, app data, and dynamic loading
- **Flash**: 16MB total with ~4MB available for applications
- **SD Card**: External storage for apps and user data

### 1.3 Display Limitations

- **Refresh Rate**: 3-second full refresh, 0.5-second partial refresh
- **Color Depth**: 1-bit (black/white only)
- **Partial Update Requirements**: 8-pixel boundary alignment
- **Buffer Size**: 9,600 bytes for full screen (320×240÷8)

## 2. System Architecture

### 2.1 Overall Architecture

The T-Deck-Pro OS is structured in two main phases:

1. **Phase 1: Hardware Abstraction Layer (HAL)**
   - Direct hardware interaction
   - Low-level drivers
   - Basic system services

2. **Phase 2: Core Services Architecture**
   - MQTT-centric communication
   - Service management
   - Application framework
   - Dynamic loading

3. **Integration Layer**
   - Dependency injection
   - Event bridging
   - Configuration management

### 2.2 MQTT-Centric Architecture

All data flows through MQTT, creating a single, manageable data bus that decouples applications from the underlying network hardware.

#### Network Priority
1. WiFi (primary)
2. 4G (secondary)
3. LoRa/Meshtastic (fallback)

#### Security
- WireGuard VPN for secure communication
- End-to-end encryption for sensitive data

### 2.3 Application Framework

- SD card-based app management
- Dynamic loading and unloading
- Event-driven UI updates
- Standardized API for hardware access

## 3. Current Implementation Status

### 3.1 Completed Components

- [ ] Basic hardware initialization
- [ ] Display driver integration
- [ ] Touch controller setup
- [ ] Keyboard matrix configuration
- [ ] Power management initialization
- [ ] SD card interface
- [ ] WiFi connectivity
- [ ] Basic logging system

### 3.2 Pending Components

- [ ] LoRa integration
- [ ] 4G modem support
- [ ] MQTT client implementation
- [ ] WireGuard VPN integration
- [ ] Dynamic app loading
- [ ] Service manager
- [ ] Event system
- [ ] UI framework
- [ ] Battery management
- [ ] OTA updates

## 4. Development Plan

### 4.1 Phase 1: Component Fixes & Preparation

#### 4.1.1 Fix Compilation Issues

1. **SimpleLogger Implementation**
   - Add missing methods to `simple_logger.h/cpp`
   - Implement proper error handling
   - Add log level filtering

2. **Hardware Library Compatibility**
   - Create wrapper classes for touch driver
   - Fix RadioLib integration for LoRa
   - Resolve GxEPD2 initialization issues

#### 4.1.2 Hardware Abstraction Layer

1. **Display Abstraction**
   - Implement partial update optimization
   - Create buffer management system
   - Add LVGL integration with custom flush callback

2. **Input Abstraction**
   - Unify keyboard and touch input
   - Create event-based input handling
   - Implement gesture recognition

3. **Communication Abstraction**
   - Create unified interface for WiFi, 4G, and LoRa
   - Implement automatic failover
   - Add status monitoring

4. **Power Management**
   - Implement battery monitoring
   - Create power saving modes
   - Add charging control

### 4.2 Phase 2: Integration Layer Development

1. **Dependency Injection System**
   - Create service locator pattern
   - Implement singleton management
   - Add lifecycle hooks

2. **Event Bridge**
   - Create event dispatcher
   - Implement subscription mechanism
   - Add event prioritization

3. **Configuration Management**
   - Create centralized configuration system
   - Implement persistent storage
   - Add validation and defaults

### 4.3 Build System Configuration

1. **PlatformIO Setup**
   - Configure build environments
   - Set up library dependencies
   - Add conditional compilation flags

2. **Memory Optimization**
   - Configure partition scheme
   - Optimize PSRAM usage
   - Set up filesystem allocation

### 4.4 Testing and Validation

1. **Unit Testing**
   - Set up testing framework
   - Create component tests
   - Implement mocking system

2. **Integration Testing**
   - Create system-level tests
   - Implement hardware-in-the-loop testing
   - Add performance benchmarks

### 4.5 Debugging Procedures

1. **Systematic Debugging**
   - Create debugging tools
   - Implement logging enhancements
   - Add memory monitoring

2. **Rollback Procedures**
   - Set up checkpoint system
   - Create component isolation
   - Implement fallback mechanisms

### 4.6 Documentation and Deployment

1. **Developer Documentation**
   - Create API reference
   - Document integration points
   - Add troubleshooting guide

2. **Deployment Procedures**
   - Set up release build configuration
   - Create deployment checklist
   - Implement OTA update system

## 5. Implementation Plan

### 5.1 Phase 1 Fixes (Days 1-3)

#### Day 1: SimpleLogger Implementation

1. **Task**: Implement missing methods in `simple_logger.h/cpp`
   - Add printf-style methods
   - Implement log level filtering
   - Add component tagging

2. **Task**: Create unit tests for SimpleLogger
   - Test all log levels
   - Verify format handling
   - Check performance impact

#### Day 2: Hardware Library Compatibility

1. **Task**: Create touch driver wrapper
   - Implement CST328 driver compatibility
   - Add gesture recognition
   - Create event-based interface

2. **Task**: Fix RadioLib integration
   - Resolve pin configuration issues
   - Implement proper initialization sequence
   - Add error handling

#### Day 3: Display Driver Integration

1. **Task**: Implement GxEPD2 integration
   - Configure for GDEQ031T10 display
   - Implement partial update optimization
   - Create buffer management system

2. **Task**: Add LVGL integration
   - Create custom flush callback
   - Configure for 1-bit color depth
   - Implement monochrome theme

### 5.2 Integration Layer (Days 4-6)

#### Day 4: Dependency Injection System

1. **Task**: Create service locator pattern
   - Implement singleton management
   - Add lifecycle hooks
   - Create service registry

2. **Task**: Implement hardware service integration
   - Register hardware components
   - Create service interfaces
   - Add dependency resolution

#### Day 5: Event Bridge

1. **Task**: Create event dispatcher
   - Implement subscription mechanism
   - Add event prioritization
   - Create event types

2. **Task**: Connect hardware events to services
   - Map input events to handlers
   - Connect network status events
   - Add power management events

#### Day 6: Configuration Management

1. **Task**: Create centralized configuration system
   - Implement JSON-based configuration
   - Add persistent storage
   - Create default configurations

2. **Task**: Connect configuration to services
   - Implement configuration injection
   - Add change notification
   - Create configuration validation

### 5.3 Build System (Day 7)

1. **Task**: Configure PlatformIO environments
   - Set up development environment
   - Create testing environment
   - Add release configuration

2. **Task**: Configure memory partitions
   - Optimize flash layout
   - Configure PSRAM usage
   - Set up filesystem allocation

### 5.4 Testing (Days 8-10)

#### Day 8: Unit Testing

1. **Task**: Set up testing framework
   - Configure Unity test framework
   - Create test runners
   - Implement mocking system

2. **Task**: Create component tests
   - Test logger functionality
   - Verify hardware abstraction
   - Check service management

#### Day 9: Integration Testing

1. **Task**: Create system-level tests
   - Test boot sequence
   - Verify service initialization
   - Check event propagation

2. **Task**: Implement hardware-in-the-loop testing
   - Test display functionality
   - Verify input handling
   - Check communication systems

#### Day 10: Performance Testing

1. **Task**: Create performance benchmarks
   - Measure boot time
   - Check memory usage
   - Test service responsiveness

2. **Task**: Optimize critical paths
   - Improve display update speed
   - Optimize event handling
   - Enhance service initialization

### 5.5 Debugging (Days 11-12)

#### Day 11: Debugging Tools

1. **Task**: Create debugging infrastructure
   - Implement system state reporting
   - Add memory usage monitoring
   - Create performance tracking

2. **Task**: Document common issues
   - Create troubleshooting guide
   - Add error code reference
   - Document recovery procedures

#### Day 12: Rollback Procedures

1. **Task**: Set up checkpoint system
   - Create git branch strategy
   - Implement component isolation
   - Add fallback mechanisms

2. **Task**: Test recovery procedures
   - Verify rollback functionality
   - Test component isolation
   - Check error recovery

### 5.6 Documentation (Day 13)

1. **Task**: Create developer documentation
   - Document API reference
   - Create integration guide
   - Add troubleshooting information

2. **Task**: Prepare deployment procedures
   - Create release checklist
   - Document OTA update process
   - Add version management

## 6. Testing Build and Identifying Issues

To execute a test build and identify potential issues, I'll follow these steps:

1. **Environment Setup**
   - Verify PlatformIO installation
   - Check ESP32-S3 toolchain
   - Confirm library dependencies

2. **Initial Build**
   - Run `pio run -e T-Deck-Pro-Integrated`
   - Capture build output
   - Identify compilation errors

3. **Error Analysis**
   - Categorize errors (linking, compilation, etc.)
   - Identify missing implementations
   - Check library compatibility

4. **Fix Implementation**
   - Address missing methods
   - Resolve library conflicts
   - Fix configuration issues

5. **Verification Build**
   - Run build after fixes
   - Verify successful compilation
   - Check binary size and memory usage

## 7. Common Issues and Solutions

### 7.1 Compilation Issues

| Issue | Symptoms | Solution |
|-------|----------|----------|
| Undefined references | Linker errors for missing methods | Implement missing methods or add stubs |
| Library conflicts | Multiple definition errors | Update library versions or use conditional compilation |
| Memory allocation | Stack overflow or heap corruption | Optimize memory usage and add bounds checking |
| Pin conflicts | Hardware initialization failures | Verify pin assignments and resolve conflicts |

### 7.2 Runtime Issues

| Issue | Symptoms | Solution |
|-------|----------|----------|
| Display not updating | Blank or corrupted screen | Check initialization sequence and buffer management |
| Network connection failures | Unable to connect to WiFi/4G | Verify credentials and implement proper retry logic |
| Power management issues | Unexpected shutdowns or battery drain | Check power configuration and implement proper sleep modes |
| SD card access failures | Unable to read/write files | Verify card format and implement proper error handling |

## 8. Next Steps

1. **Execute Phase 1 Fixes**
   - Implement missing methods
   - Resolve library conflicts
   - Fix hardware initialization

2. **Develop Integration Layer**
   - Create dependency injection system
   - Implement event bridge
   - Set up configuration management

3. **Configure Build System**
   - Set up PlatformIO environments
   - Optimize memory usage
   - Configure conditional compilation

4. **Implement Testing Framework**
   - Create unit tests
   - Develop integration tests
   - Set up performance benchmarks

5. **Prepare Documentation**
   - Document API reference
   - Create troubleshooting guide
   - Prepare deployment procedures

This documentation framework will be continuously updated as development progresses, ensuring that all components are properly documented and all issues are tracked and resolved.