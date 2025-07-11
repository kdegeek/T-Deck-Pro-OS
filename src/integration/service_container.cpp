/**
 * @file      service_container.cpp
 * @author    T-Deck-Pro OS Team
 * @license   MIT
 * @copyright Copyright (c) 2025
 * @date      2025-01-11
 * @brief     Lightweight dependency injection container implementation
 */

#include "service_container.h"

// Only compile if integration layer is enabled
#ifdef INTEGRATION_LAYER_ENABLED

// Global service container instance
ServiceContainer* GlobalServiceContainer = nullptr;

ServiceContainer::ServiceContainer() : container_initialized(false) {
    // Don't log during static initialization - logger may not be ready
}

ServiceContainer::~ServiceContainer() {
    shutdown();
    // Don't log during destruction - logger may already be destroyed
}

void ServiceContainer::registerService(const String& name, ServiceFactory factory, bool singleton) {
    LOG_INFOF("ServiceContainer", "Registering service: %s (singleton: %s)", 
              name.c_str(), singleton ? "true" : "false");
    
    if (hasService(name)) {
        LOG_WARNF("ServiceContainer", "Service %s already registered, overwriting", name.c_str());
    }
    
    services[name] = ServiceRegistration(name, factory, singleton);
}

std::shared_ptr<IService> ServiceContainer::getService(const String& name) {
    auto it = services.find(name);
    if (it == services.end()) {
        LOG_ERRORF("ServiceContainer", "Service not found: %s", name.c_str());
        return nullptr;
    }
    
    ServiceRegistration& registration = it->second;
    
    // For singleton services, return existing instance if available
    if (registration.singleton && registration.instance) {
        return registration.instance;
    }
    
    // Create new instance
    std::shared_ptr<IService> instance = createServiceInstance(name);
    if (!instance) {
        LOG_ERRORF("ServiceContainer", "Failed to create service instance: %s", name.c_str());
        return nullptr;
    }
    
    // Store instance for singleton services
    if (registration.singleton) {
        registration.instance = instance;
        instances[name] = instance;
    }
    
    return instance;
}

std::shared_ptr<IService> ServiceContainer::createServiceInstance(const String& name) {
    auto it = services.find(name);
    if (it == services.end()) {
        return nullptr;
    }
    
    try {
        LOG_DEBUGF("ServiceContainer", "Creating instance of service: %s", name.c_str());
        return it->second.factory(this);
    } catch (const std::exception& e) {
        LOG_ERRORF("ServiceContainer", "Exception creating service %s: %s", name.c_str(), e.what());
        return nullptr;
    } catch (...) {
        LOG_ERRORF("ServiceContainer", "Unknown exception creating service: %s", name.c_str());
        return nullptr;
    }
}

bool ServiceContainer::initializeService(const String& name) {
    LOG_INFOF("ServiceContainer", "Initializing service: %s", name.c_str());
    
    auto service = getService(name);
    if (!service) {
        LOG_ERRORF("ServiceContainer", "Cannot initialize service %s: service not found", name.c_str());
        return false;
    }
    
    try {
        if (service->initialize()) {
            services[name].initialized = true;
            LOG_INFOF("ServiceContainer", "Service %s initialized successfully", name.c_str());
            return true;
        } else {
            LOG_ERRORF("ServiceContainer", "Service %s initialization failed", name.c_str());
            return false;
        }
    } catch (const std::exception& e) {
        LOG_ERRORF("ServiceContainer", "Exception initializing service %s: %s", name.c_str(), e.what());
        return false;
    } catch (...) {
        LOG_ERRORF("ServiceContainer", "Unknown exception initializing service: %s", name.c_str());
        return false;
    }
}

bool ServiceContainer::initializeAllServices() {
    LOG_INFO("ServiceContainer", "Initializing all services...");
    
    bool all_success = true;
    for (auto& pair : services) {
        if (!initializeService(pair.first)) {
            all_success = false;
        }
    }
    
    LOG_INFOF("ServiceContainer", "Service initialization complete - Status: %s", 
              all_success ? "SUCCESS" : "PARTIAL");
    return all_success;
}

void ServiceContainer::shutdownService(const String& name) {
    LOG_INFOF("ServiceContainer", "Shutting down service: %s", name.c_str());
    
    auto it = services.find(name);
    if (it == services.end()) {
        LOG_WARNF("ServiceContainer", "Cannot shutdown service %s: not found", name.c_str());
        return;
    }
    
    ServiceRegistration& registration = it->second;
    
    if (registration.instance && registration.initialized) {
        try {
            registration.instance->shutdown();
            registration.initialized = false;
            LOG_INFOF("ServiceContainer", "Service %s shut down successfully", name.c_str());
        } catch (const std::exception& e) {
            LOG_ERRORF("ServiceContainer", "Exception shutting down service %s: %s", name.c_str(), e.what());
        } catch (...) {
            LOG_ERRORF("ServiceContainer", "Unknown exception shutting down service: %s", name.c_str());
        }
    }
    
    // Remove instance for singleton services
    if (registration.singleton) {
        registration.instance.reset();
        instances.erase(name);
    }
}

void ServiceContainer::shutdownAllServices() {
    LOG_INFO("ServiceContainer", "Shutting down all services...");
    
    // Shutdown in reverse order of registration
    std::vector<String> service_names;
    for (const auto& pair : services) {
        service_names.push_back(pair.first);
    }
    
    for (auto it = service_names.rbegin(); it != service_names.rend(); ++it) {
        shutdownService(*it);
    }
    
    LOG_INFO("ServiceContainer", "All services shut down");
}

bool ServiceContainer::initialize() {
    LOG_INFO("ServiceContainer", "Initializing service container...");
    
    if (container_initialized) {
        LOG_WARN("ServiceContainer", "Service container already initialized");
        return true;
    }
    
    // Set global instance
    if (!GlobalServiceContainer) {
        GlobalServiceContainer = this;
    }
    
    container_initialized = true;
    LOG_INFO("ServiceContainer", "Service container initialized successfully");
    return true;
}

void ServiceContainer::shutdown() {
    if (!container_initialized) {
        return;
    }
    
    LOG_INFO("ServiceContainer", "Shutting down service container...");
    
    shutdownAllServices();
    services.clear();
    instances.clear();
    
    container_initialized = false;
    
    // Clear global instance if it's this container
    if (GlobalServiceContainer == this) {
        GlobalServiceContainer = nullptr;
    }
    
    LOG_INFO("ServiceContainer", "Service container shut down");
}

bool ServiceContainer::hasService(const String& name) const {
    return services.find(name) != services.end();
}

bool ServiceContainer::isServiceInitialized(const String& name) const {
    auto it = services.find(name);
    return it != services.end() && it->second.initialized;
}

std::vector<String> ServiceContainer::getRegisteredServices() const {
    std::vector<String> names;
    for (const auto& pair : services) {
        names.push_back(pair.first);
    }
    return names;
}

std::vector<String> ServiceContainer::getInitializedServices() const {
    std::vector<String> names;
    for (const auto& pair : services) {
        if (pair.second.initialized) {
            names.push_back(pair.first);
        }
    }
    return names;
}

void ServiceContainer::printServiceStatus() const {
    LOG_INFO("ServiceContainer", "=== Service Container Status ===");
    LOG_INFOF("ServiceContainer", "Container initialized: %s", container_initialized ? "true" : "false");
    LOG_INFOF("ServiceContainer", "Total services: %d", services.size());

    for (const auto& pair : services) {
        const ServiceRegistration& reg = pair.second;
        LOG_INFOF("ServiceContainer", "Service: %s | Type: %s | Initialized: %s",
                  reg.name.c_str(),
                  reg.singleton ? "Singleton" : "Transient",
                  reg.initialized ? "true" : "false");
    }
}

#endif // INTEGRATION_LAYER_ENABLED
