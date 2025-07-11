/**
 * @file      service_manager.h
 * @author    T-Deck-Pro OS Team
 * @license   MIT
 * @copyright Copyright (c) 2025
 * @date      2025-01-11
 * @brief     Service manager for T-Deck-Pro OS Phase 2 services
 */

#ifndef SERVICE_MANAGER_H
#define SERVICE_MANAGER_H

#include <Arduino.h>
#include <vector>
#include <memory>
#include "simple_logger.h"
#include "service_container.h"
#include "event_bridge.h"
#include "config_manager.h"

// Forward declarations
class ServiceManager;
class SimpleHardware;

/**
 * @brief Service Startup Order
 * 
 * Defines the order in which services should be started
 */
enum class ServiceStartupOrder {
    CRITICAL = 0,    // Must start first (logging, config, events)
    CORE = 1,        // Core services (hardware abstraction)
    COMMUNICATION = 2, // Communication services (WiFi, MQTT, LoRa)
    APPLICATION = 3,   // Application services (UI, apps)
    OPTIONAL = 4       // Optional services (can fail without system failure)
};

/**
 * @brief Service Registration Info
 * 
 * Contains information about how a service should be managed
 */
struct ServiceInfo {
    String name;
    String description;
    ServiceStartupOrder startup_order;
    bool required;
    bool auto_start;
    uint32_t startup_timeout_ms;
    uint32_t shutdown_timeout_ms;
    std::vector<String> dependencies;

    // Default constructor
    ServiceInfo() : startup_order(ServiceStartupOrder::APPLICATION), required(false), auto_start(true),
                   startup_timeout_ms(10000), shutdown_timeout_ms(5000) {}

    ServiceInfo(const String& n, const String& desc, ServiceStartupOrder order = ServiceStartupOrder::APPLICATION)
        : name(n), description(desc), startup_order(order), required(false), auto_start(true),
          startup_timeout_ms(10000), shutdown_timeout_ms(5000) {}
};

/**
 * @brief Service Manager Class
 * 
 * Manages the lifecycle of all Phase 2 services in T-Deck-Pro OS
 * Handles service registration, dependency resolution, startup/shutdown ordering
 */
class ServiceManager : public IService {
private:
    std::shared_ptr<ServiceContainer> service_container;
    std::shared_ptr<EventBridge> event_bridge;
    std::shared_ptr<ConfigManager> config_manager;
    SimpleHardware* hardware;
    
    std::map<String, ServiceInfo> service_registry;
    std::vector<String> startup_order;
    std::vector<String> running_services;
    
    bool initialized;
    bool services_started;
    bool shutdown_in_progress;
    
    // Statistics
    uint32_t total_startup_time;
    uint32_t total_shutdown_time;
    uint32_t services_started_count;
    uint32_t services_failed_count;
    
public:
    ServiceManager();
    ~ServiceManager() override;
    
    // IService implementation
    bool initialize() override;
    void shutdown() override;
    const char* getServiceName() const override { return "ServiceManager"; }
    bool isInitialized() const override { return initialized; }
    
    // Core system integration
    bool initializeWithHardware(SimpleHardware* hw);
    void setServiceContainer(std::shared_ptr<ServiceContainer> container);
    void setEventBridge(std::shared_ptr<EventBridge> bridge);
    void setConfigManager(std::shared_ptr<ConfigManager> config);
    
    // Service registration
    void registerService(const ServiceInfo& info);
    void registerCoreServices();
    void registerCommunicationServices();
    void registerApplicationServices();
    
    // Service lifecycle management
    bool startAllServices();
    bool startService(const String& name);
    bool stopService(const String& name);
    bool stopAllServices();
    bool restartService(const String& name);
    
    // Dependency management
    bool resolveDependencies();
    std::vector<String> getDependencies(const String& service_name) const;
    bool checkDependencies(const String& service_name) const;
    
    // Service information
    bool isServiceRegistered(const String& name) const;
    bool isServiceRunning(const String& name) const;
    ServiceInfo getServiceInfo(const String& name) const;
    std::vector<String> getRegisteredServices() const;
    std::vector<String> getRunningServices() const;
    std::vector<String> getFailedServices() const;

    // System update method
    void update();
    void performHealthChecks();

    // System management
    bool performHealthCheck();
    void handleServiceFailure(const String& service_name);
    
    // Configuration
    void loadServiceConfiguration();
    void saveServiceConfiguration();
    
    // Status and diagnostics
    void printServiceStatus() const;
    void printServiceRegistry() const;
    void printStartupOrder() const;
    
    // Statistics
    uint32_t getTotalStartupTime() const { return total_startup_time; }
    uint32_t getTotalShutdownTime() const { return total_shutdown_time; }
    uint32_t getServicesStartedCount() const { return services_started_count; }
    uint32_t getServicesFailedCount() const { return services_failed_count; }
    
private:
    // Internal service management
    bool startServiceInternal(const String& name, bool check_deps = true);
    bool stopServiceInternal(const String& name);
    void calculateStartupOrder();
    bool waitForServiceStartup(const String& name, uint32_t timeout_ms);
    bool waitForServiceShutdown(const String& name, uint32_t timeout_ms);
    
    // Event handling
    void onServiceStarted(const String& name);
    void onServiceStopped(const String& name);
    void onServiceFailed(const String& name, const String& error);
    
    // Utility methods
    void publishServiceEvent(EventType type, const String& service_name, const String& message = "");
    std::vector<String> sortServicesByStartupOrder() const;
    bool isSystemService(const String& name) const;
};

/**
 * @brief Global Service Manager Instance
 */
extern ServiceManager* GlobalServiceManager;

// Convenience macros for service management
#define REGISTER_SERVICE_INFO(manager, info) \
    manager->registerService(info)

#define START_SERVICE(name) \
    (GlobalServiceManager ? GlobalServiceManager->startService(name) : false)

#define STOP_SERVICE(name) \
    (GlobalServiceManager ? GlobalServiceManager->stopService(name) : false)

#define IS_SERVICE_RUNNING(name) \
    (GlobalServiceManager ? GlobalServiceManager->isServiceRunning(name) : false)

// Helper macros for creating service info
#define CREATE_SERVICE_INFO(name, desc, order) \
    ServiceInfo(name, desc, ServiceStartupOrder::order)

#define CREATE_REQUIRED_SERVICE_INFO(name, desc, order) \
    []() { \
        ServiceInfo info(name, desc, ServiceStartupOrder::order); \
        info.required = true; \
        return info; \
    }()

#define CREATE_OPTIONAL_SERVICE_INFO(name, desc, order) \
    []() { \
        ServiceInfo info(name, desc, ServiceStartupOrder::order); \
        info.required = false; \
        info.auto_start = false; \
        return info; \
    }()

#endif // SERVICE_MANAGER_H
