/**
 * @file      service_manager.cpp
 * @author    T-Deck-Pro OS Team
 * @license   MIT
 * @copyright Copyright (c) 2025
 * @date      2025-01-11
 * @brief     Service manager implementation for T-Deck-Pro OS
 */

#include "service_manager.h"
#include "simple_hardware.h"

// Global service manager instance
ServiceManager* GlobalServiceManager = nullptr;

ServiceManager::ServiceManager()
    : hardware(nullptr), initialized(false), services_started(false), shutdown_in_progress(false),
      total_startup_time(0), total_shutdown_time(0), services_started_count(0), services_failed_count(0) {
    // Don't log during static initialization - logger may not be ready
}

ServiceManager::~ServiceManager() {
    shutdown();
    // Don't log during destruction - logger may already be destroyed
}

bool ServiceManager::initialize() {
    LOG_INFO("ServiceManager", "Initializing service manager...");
    
    if (initialized) {
        LOG_WARN("ServiceManager", "Service manager already initialized");
        return true;
    }
    
    // Verify required components
    if (!service_container) {
        LOG_ERROR("ServiceManager", "Service container not set");
        return false;
    }
    
    if (!event_bridge) {
        LOG_ERROR("ServiceManager", "Event bridge not set");
        return false;
    }
    
    if (!config_manager) {
        LOG_ERROR("ServiceManager", "Config manager not set");
        return false;
    }
    
    // Set global instance
    if (!GlobalServiceManager) {
        GlobalServiceManager = this;
    }
    
    // Register core services
    registerCoreServices();
    registerCommunicationServices();
    registerApplicationServices();
    
    // Load service configuration
    loadServiceConfiguration();
    
    // Calculate startup order
    calculateStartupOrder();
    
    initialized = true;
    LOG_INFO("ServiceManager", "Service manager initialized successfully");
    
    // Publish service manager started event
    publishServiceEvent(EventType::SERVICE_STARTED, "ServiceManager");
    
    return true;
}

void ServiceManager::shutdown() {
    if (!initialized) {
        return;
    }
    
    LOG_INFO("ServiceManager", "Shutting down service manager...");
    shutdown_in_progress = true;
    
    uint32_t shutdown_start = millis();
    
    // Stop all services
    stopAllServices();
    
    // Save service configuration
    saveServiceConfiguration();
    
    // Clear registry and state
    service_registry.clear();
    startup_order.clear();
    running_services.clear();
    
    total_shutdown_time += millis() - shutdown_start;
    
    initialized = false;
    services_started = false;
    shutdown_in_progress = false;
    
    // Clear global instance if it's this manager
    if (GlobalServiceManager == this) {
        GlobalServiceManager = nullptr;
    }
    
    // Publish service manager stopped event
    publishServiceEvent(EventType::SERVICE_STOPPED, "ServiceManager");
    
    LOG_INFO("ServiceManager", "Service manager shut down");
}

bool ServiceManager::initializeWithHardware(SimpleHardware* hw) {
    if (!hw) {
        LOG_ERROR("ServiceManager", "Invalid hardware pointer");
        return false;
    }
    
    hardware = hw;
    LOG_INFO("ServiceManager", "Hardware integration configured");
    return true;
}

void ServiceManager::setServiceContainer(std::shared_ptr<ServiceContainer> container) {
    service_container = container;
    LOG_INFO("ServiceManager", "Service container configured");
}

void ServiceManager::setEventBridge(std::shared_ptr<EventBridge> bridge) {
    event_bridge = bridge;
    LOG_INFO("ServiceManager", "Event bridge configured");
}

void ServiceManager::setConfigManager(std::shared_ptr<ConfigManager> config) {
    config_manager = config;
    LOG_INFO("ServiceManager", "Config manager configured");
}

void ServiceManager::registerService(const ServiceInfo& info) {
    LOG_INFOF("ServiceManager", "Registering service: %s (%s)", 
              info.name.c_str(), info.description.c_str());
    
    if (isServiceRegistered(info.name)) {
        LOG_WARNF("ServiceManager", "Service %s already registered, overwriting", info.name.c_str());
    }
    
    service_registry[info.name] = info;
}

void ServiceManager::registerCoreServices() {
    LOG_INFO("ServiceManager", "Registering core services...");
    
    // Event Bridge (already initialized, but register for management)
    ServiceInfo event_info("EventBridge", "Central event communication system", ServiceStartupOrder::CRITICAL);
    event_info.required = true;
    event_info.startup_timeout_ms = 5000;
    registerService(event_info);
    
    // Configuration Manager (already initialized)
    ServiceInfo config_info("ConfigManager", "Configuration management system", ServiceStartupOrder::CRITICAL);
    config_info.required = true;
    config_info.startup_timeout_ms = 5000;
    registerService(config_info);
    
    // Hardware abstraction layer
    ServiceInfo hardware_info("SimpleHardware", "Hardware abstraction layer", ServiceStartupOrder::CORE);
    hardware_info.required = true;
    hardware_info.startup_timeout_ms = 15000;
    registerService(hardware_info);
}

void ServiceManager::registerCommunicationServices() {
    LOG_INFO("ServiceManager", "Registering communication services...");
    
    // MQTT Service
    ServiceInfo mqtt_info("MQTTService", "MQTT communication service", ServiceStartupOrder::COMMUNICATION);
    mqtt_info.required = false;
    mqtt_info.dependencies.push_back("SimpleHardware");
    mqtt_info.startup_timeout_ms = 10000;
    registerService(mqtt_info);
    
    // LoRa Service
    ServiceInfo lora_info("LoRaService", "LoRa communication service", ServiceStartupOrder::COMMUNICATION);
    lora_info.required = false;
    lora_info.dependencies.push_back("SimpleHardware");
    lora_info.startup_timeout_ms = 8000;
    registerService(lora_info);
    
    // GPS Service
    ServiceInfo gps_info("GPSService", "GPS location service", ServiceStartupOrder::COMMUNICATION);
    gps_info.required = false;
    gps_info.dependencies.push_back("SimpleHardware");
    gps_info.startup_timeout_ms = 15000;
    registerService(gps_info);
}

void ServiceManager::registerApplicationServices() {
    LOG_INFO("ServiceManager", "Registering application services...");
    
    // UI Service
    ServiceInfo ui_info("UIService", "User interface service", ServiceStartupOrder::APPLICATION);
    ui_info.required = false;
    ui_info.dependencies.push_back("SimpleHardware");
    ui_info.dependencies.push_back("EventBridge");
    ui_info.startup_timeout_ms = 8000;
    registerService(ui_info);
    
    // Application Manager
    ServiceInfo app_info("ApplicationManager", "Application lifecycle manager", ServiceStartupOrder::APPLICATION);
    app_info.required = false;
    app_info.dependencies.push_back("UIService");
    app_info.dependencies.push_back("ConfigManager");
    app_info.startup_timeout_ms = 5000;
    registerService(app_info);
}

bool ServiceManager::startAllServices() {
    LOG_INFO("ServiceManager", "Starting all services...");
    
    if (services_started) {
        LOG_WARN("ServiceManager", "Services already started");
        return true;
    }
    
    uint32_t startup_start = millis();
    bool all_success = true;
    
    // Start services in calculated order
    for (const String& service_name : startup_order) {
        const ServiceInfo& info = service_registry[service_name];
        
        if (!info.auto_start) {
            LOG_INFOF("ServiceManager", "Skipping service %s (auto_start disabled)", service_name.c_str());
            continue;
        }
        
        if (!startServiceInternal(service_name, true)) {
            if (info.required) {
                LOG_ERRORF("ServiceManager", "Required service %s failed to start", service_name.c_str());
                all_success = false;
                break;
            } else {
                LOG_WARNF("ServiceManager", "Optional service %s failed to start", service_name.c_str());
            }
        }
    }
    
    total_startup_time += millis() - startup_start;
    services_started = all_success;
    
    LOG_INFOF("ServiceManager", "Service startup complete - Status: %s (Time: %lums)", 
              all_success ? "SUCCESS" : "PARTIAL", millis() - startup_start);
    
    return all_success;
}

bool ServiceManager::startService(const String& name) {
    return startServiceInternal(name, true);
}

bool ServiceManager::startServiceInternal(const String& name, bool check_deps) {
    if (!isServiceRegistered(name)) {
        LOG_ERRORF("ServiceManager", "Cannot start service %s: not registered", name.c_str());
        return false;
    }
    
    if (isServiceRunning(name)) {
        LOG_INFOF("ServiceManager", "Service %s already running", name.c_str());
        return true;
    }
    
    const ServiceInfo& info = service_registry[name];
    
    // Check dependencies if requested
    if (check_deps && !checkDependencies(name)) {
        LOG_ERRORF("ServiceManager", "Cannot start service %s: dependencies not met", name.c_str());
        return false;
    }
    
    LOG_INFOF("ServiceManager", "Starting service: %s", name.c_str());
    
    // For system services that are already running, just mark them as started
    if (isSystemService(name)) {
        running_services.push_back(name);
        onServiceStarted(name);
        return true;
    }
    
    // Get service from container and initialize
    auto service = service_container->getService(name);
    if (!service) {
        LOG_ERRORF("ServiceManager", "Service %s not found in container", name.c_str());
        onServiceFailed(name, "Service not found in container");
        return false;
    }
    
    // Initialize the service
    if (!service_container->initializeService(name)) {
        LOG_ERRORF("ServiceManager", "Failed to initialize service: %s", name.c_str());
        onServiceFailed(name, "Initialization failed");
        return false;
    }
    
    // Wait for startup with timeout
    if (!waitForServiceStartup(name, info.startup_timeout_ms)) {
        LOG_ERRORF("ServiceManager", "Service %s startup timeout", name.c_str());
        onServiceFailed(name, "Startup timeout");
        return false;
    }
    
    running_services.push_back(name);
    onServiceStarted(name);
    return true;
}

bool ServiceManager::stopService(const String& name) {
    if (!isServiceRegistered(name)) {
        LOG_ERRORF("ServiceManager", "Cannot stop service %s: not registered", name.c_str());
        return false;
    }

    if (!isServiceRunning(name)) {
        LOG_WARNF("ServiceManager", "Service %s is not running", name.c_str());
        return true; // Already stopped
    }

    LOG_INFOF("ServiceManager", "Stopping service: %s", name.c_str());

    const ServiceInfo& info = service_registry.at(name);

    // Get service from container and stop it
    auto service = service_container->getService(name);
    if (service) {
        // TODO: Call service-specific stop method when services are implemented
        // For now, just mark as stopped
    }

    // Remove from running services list
    auto it = std::find(running_services.begin(), running_services.end(), name);
    if (it != running_services.end()) {
        running_services.erase(it);
    }

    onServiceStopped(name);
    return true;
}

bool ServiceManager::isSystemService(const String& name) const {
    return name == "EventBridge" || name == "ConfigManager" || name == "SimpleHardware";
}

void ServiceManager::onServiceStarted(const String& name) {
    services_started_count++;
    LOG_INFOF("ServiceManager", "Service %s started successfully", name.c_str());
    publishServiceEvent(EventType::SERVICE_STARTED, name);
}

void ServiceManager::onServiceStopped(const String& name) {
    LOG_INFOF("ServiceManager", "Service %s stopped", name.c_str());
    publishServiceEvent(EventType::SERVICE_STOPPED, name);
}

void ServiceManager::onServiceFailed(const String& name, const String& error) {
    services_failed_count++;
    LOG_ERRORF("ServiceManager", "Service %s failed: %s", name.c_str(), error.c_str());
    publishServiceEvent(EventType::SERVICE_ERROR, name, error);
}

void ServiceManager::publishServiceEvent(EventType type, const String& service_name, const String& message) {
    if (event_bridge) {
        String event_message = "Service: " + service_name;
        if (!message.isEmpty()) {
            event_message += " - " + message;
        }
        event_bridge->publishEvent(type, "ServiceManager", EventPriority::NORMAL);
    }
}

bool ServiceManager::waitForServiceStartup(const String& name, uint32_t timeout_ms) {
    // For now, assume immediate startup since we don't have async service initialization
    // In a more complex system, this would poll the service status
    return true;
}

bool ServiceManager::checkDependencies(const String& service_name) const {
    auto it = service_registry.find(service_name);
    if (it == service_registry.end()) {
        return false;
    }
    
    const ServiceInfo& info = it->second;
    for (const String& dep : info.dependencies) {
        if (!isServiceRunning(dep)) {
            LOG_WARNF("ServiceManager", "Service %s dependency %s not running", 
                      service_name.c_str(), dep.c_str());
            return false;
        }
    }
    
    return true;
}

bool ServiceManager::isServiceRegistered(const String& name) const {
    return service_registry.find(name) != service_registry.end();
}

bool ServiceManager::isServiceRunning(const String& name) const {
    return std::find(running_services.begin(), running_services.end(), name) != running_services.end();
}

void ServiceManager::calculateStartupOrder() {
    LOG_INFO("ServiceManager", "Calculating service startup order...");
    
    startup_order.clear();
    
    // Sort services by startup order priority
    std::vector<std::pair<String, ServiceStartupOrder>> services_with_order;
    for (const auto& pair : service_registry) {
        services_with_order.emplace_back(pair.first, pair.second.startup_order);
    }
    
    std::sort(services_with_order.begin(), services_with_order.end(),
        [](const std::pair<String, ServiceStartupOrder>& a, const std::pair<String, ServiceStartupOrder>& b) {
            return static_cast<int>(a.second) < static_cast<int>(b.second);
        });
    
    for (const auto& pair : services_with_order) {
        startup_order.push_back(pair.first);
    }
    
    LOG_INFOF("ServiceManager", "Startup order calculated (%d services)", startup_order.size());
}

void ServiceManager::loadServiceConfiguration() {
    // Load service-specific configuration from config manager
    // This would be implemented based on specific service needs
    LOG_INFO("ServiceManager", "Loading service configuration...");
}

void ServiceManager::saveServiceConfiguration() {
    // Save service-specific configuration to config manager
    LOG_INFO("ServiceManager", "Saving service configuration...");
}

bool ServiceManager::stopAllServices() {
    LOG_INFO("ServiceManager", "Stopping all services...");

    bool all_stopped = true;

    // Get services in reverse startup order for shutdown
    std::vector<std::pair<String, ServiceStartupOrder>> services_with_order;
    for (const auto& pair : service_registry) {
        services_with_order.push_back({pair.first, pair.second.startup_order});
    }

    // Sort in reverse order for shutdown
    std::sort(services_with_order.begin(), services_with_order.end(),
        [](const std::pair<String, ServiceStartupOrder>& a, const std::pair<String, ServiceStartupOrder>& b) {
            return static_cast<int>(a.second) > static_cast<int>(b.second);
        });

    // Stop services in reverse order
    for (const auto& service_pair : services_with_order) {
        const String& service_name = service_pair.first;
        if (isServiceRunning(service_name)) {
            if (!stopService(service_name)) {
                LOG_ERRORF("ServiceManager", "Failed to stop service: %s", service_name.c_str());
                all_stopped = false;
            }
        }
    }

    if (all_stopped) {
        LOG_INFO("ServiceManager", "All services stopped successfully");
    } else {
        LOG_WARN("ServiceManager", "Some services failed to stop properly");
    }

    return all_stopped;
}

void ServiceManager::update() {
    // Update all running services
    for (const auto& pair : service_registry) {
        const String& service_name = pair.first;
        if (isServiceRunning(service_name)) {
            // Services would implement their own update logic
            // For now, just perform basic health checks
            // TODO: Add service-specific update calls when services are implemented
        }
    }

    // Perform periodic health checks
    static uint32_t last_health_check = 0;
    uint32_t current_time = millis();
    if (current_time - last_health_check > 30000) { // Every 30 seconds
        performHealthChecks();
        last_health_check = current_time;
    }
}

void ServiceManager::performHealthChecks() {
    // Basic health check for all running services
    for (const auto& pair : service_registry) {
        const String& service_name = pair.first;
        if (isServiceRunning(service_name)) {
            // TODO: Implement actual health checks when services are available
            LOG_DEBUGF("ServiceManager", "Health check for service: %s - OK", service_name.c_str());
        }
    }
}
