/**
 * @file      service_container.h
 * @author    T-Deck-Pro OS Team
 * @license   MIT
 * @copyright Copyright (c) 2025
 * @date      2025-01-11
 * @brief     Lightweight dependency injection container for T-Deck-Pro OS
 */

#ifndef SERVICE_CONTAINER_H
#define SERVICE_CONTAINER_H

// Only compile if integration layer is enabled
#ifdef INTEGRATION_LAYER_ENABLED

#include <Arduino.h>
#include <map>
#include <memory>
#include <functional>
#include <typeinfo>
#include <vector>
#include "simple_logger.h"

// Forward declarations
class ServiceContainer;

/**
 * @brief Service Interface
 * 
 * Base interface that all services must implement
 */
class IService {
public:
    virtual ~IService() = default;
    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
    virtual const char* getServiceName() const = 0;
    virtual bool isInitialized() const = 0;
};

/**
 * @brief Service Factory Function Type
 * 
 * Function signature for service factory functions
 */
typedef std::function<std::shared_ptr<IService>(ServiceContainer*)> ServiceFactory;

/**
 * @brief Service Registration Information
 * 
 * Contains metadata about a registered service
 */
struct ServiceRegistration {
    String name;
    ServiceFactory factory;
    bool singleton;
    std::shared_ptr<IService> instance;
    bool initialized;
    
    ServiceRegistration() : singleton(true), initialized(false) {}
    ServiceRegistration(const String& n, ServiceFactory f, bool s = true) 
        : name(n), factory(f), singleton(s), initialized(false) {}
};

/**
 * @brief Lightweight Dependency Injection Container
 * 
 * Manages service registration, creation, and lifecycle for T-Deck-Pro OS
 * Supports singleton and transient service lifetimes
 */
class ServiceContainer {
private:
    std::map<String, ServiceRegistration> services;
    std::map<String, std::shared_ptr<IService>> instances;
    bool container_initialized;
    
    // Prevent copying
    ServiceContainer(const ServiceContainer&) = delete;
    ServiceContainer& operator=(const ServiceContainer&) = delete;

public:
    ServiceContainer();
    ~ServiceContainer();
    
    // Service registration
    template<typename T>
    void registerService(const String& name, bool singleton = true);
    
    void registerService(const String& name, ServiceFactory factory, bool singleton = true);
    
    // Service resolution
    template<typename T>
    std::shared_ptr<T> getService(const String& name);
    
    std::shared_ptr<IService> getService(const String& name);
    
    // Service management
    bool initializeService(const String& name);
    bool initializeAllServices();
    void shutdownService(const String& name);
    void shutdownAllServices();
    
    // Container management
    bool initialize();
    void shutdown();
    bool isInitialized() const { return container_initialized; }
    
    // Service information
    bool hasService(const String& name) const;
    bool isServiceInitialized(const String& name) const;
    std::vector<String> getRegisteredServices() const;
    std::vector<String> getInitializedServices() const;
    
    // Utility methods
    void printServiceStatus() const;
    size_t getServiceCount() const { return services.size(); }
    
private:
    std::shared_ptr<IService> createServiceInstance(const String& name);
};

/**
 * @brief Global Service Container Instance
 * 
 * Provides global access to the service container
 */
extern ServiceContainer* GlobalServiceContainer;

// Convenience macros for service registration and resolution
#define REGISTER_SERVICE(container, type, name) \
    container->registerService<type>(name)

#define REGISTER_SERVICE_FACTORY(container, name, factory) \
    container->registerService(name, factory)

#define GET_SERVICE(container, type, name) \
    container->getService<type>(name)

#define RESOLVE_SERVICE(type, name) \
    GlobalServiceContainer->getService<type>(name)

// Template implementations
template<typename T>
void ServiceContainer::registerService(const String& name, bool singleton) {
    LOG_INFOF("ServiceContainer", "Registering service: %s (singleton: %s)", 
              name.c_str(), singleton ? "true" : "false");
    
    ServiceFactory factory = [](ServiceContainer* container) -> std::shared_ptr<IService> {
        return std::make_shared<T>();
    };
    
    services[name] = ServiceRegistration(name, factory, singleton);
}

template<typename T>
std::shared_ptr<T> ServiceContainer::getService(const String& name) {
    auto service = getService(name);
    if (!service) {
        return nullptr;
    }
    
    return std::dynamic_pointer_cast<T>(service);
}

#endif // INTEGRATION_LAYER_ENABLED

#endif // SERVICE_CONTAINER_H
