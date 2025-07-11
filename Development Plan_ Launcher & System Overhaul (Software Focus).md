## **Development Plan: Launcher & System Overhaul (Software Focus)**

This plan outlines a comprehensive strategy to refactor your device's launcher and underlying system architecture. The primary goals are to create a **lightweight, resilient, and simple** user experience, free from lag, while establishing a robust and scalable application framework.

**A quick reminder for our coding AI:** This is a high-level plan. Remember to **write complete code, avoid stubs, and always double-check library examples and software configurations** before and during implementation.

---

### **1\. Core Architecture: MQTT-Centric & Secure**

The central nervous system of our device will be MQTT. All data, without exception, will flow through it. This creates a single, manageable data bus and decouples applications from the underlying network hardware.

#### **Key Implementation Steps:**

1. **Establish the MQTT Broker:** Set up a central MQTT broker on a home server. This broker is the single point of truth for all device data.  
2. **Unified Connectivity Manager:**  
   * Create a core service that is always running. This service will manage the network connections (WiFi, 4G, LoRa/Meshtastic) and the Wireguard overlay network.  
   * It will be responsible for connecting to the MQTT broker through the most efficient available network.  
3. **Failover Logic:**  
   * The priority of connections should be: **WiFi \> 4G \> LoRa (Meshtastic)**.  
   * The Connectivity Manager must constantly monitor the connection status. If the primary connection (WiFi) is lost, it must automatically attempt to connect via 4G, and so on.  
   * The connection to the MQTT broker will be made over **Wireguard**, ensuring a secure, private network regardless of the underlying physical connection.  
4. **Application Data Framework:**  
   * All applications that need to send or receive data must do so by publishing and subscribing to MQTT topics.  
   * Create a simple API or wrapper for apps to use the MQTT service.

Python  
\# Example of an App's interaction with the MQTT service  
\# Reminder: This is a conceptual example. Flesh out the error handling and topic structure.

class MqttClient:  
    def \_\_init\_\_(self, app\_name):  
        self.app\_name \= app\_name  
        \# Connect to the system's MQTT service

    def publish(self, topic, payload):  
        \# Publishes a message to a specific topic  
        full\_topic \= f"apps/{self.app\_name}/{topic}"  
        \# Send the message to the MQTT broker

    def subscribe(self, topic, callback):  
        \# Subscribes to a topic and registers a callback function  
        full\_topic \= f"apps/{self.app\_name}/{topic}"  
        \# Register the callback to be executed on new messages

\# In an app:  
mqtt \= MqttClient("WeatherApp")

def update\_weather\_display(message):  
    \# Code to update the UI  
    pass

mqtt.subscribe("weather/update", update\_weather\_display)  
mqtt.publish("weather/get\_forecast", {"location": "current"})

5. **Notification-to-App Framework:**  
   * When an MQTT message is received for an app that isn't in the foreground, a notification should be generated.  
   * The notification system needs to be able to launch the corresponding app when the notification is selected.  
   * This will likely involve a central notification manager that listens to specific MQTT topics.

---

### **2\. SD Card-Based App Management & Dynamic Loading**

To keep the internal memory free for essential processes, all non-essential applications will be loaded from the SD card.

#### **Boot Process:**

1. **Check for SD Card and "Apps" folder.**  
2. **If /sdcard/Apps exists:**  
   * Recursively scan the folder structure.  
   * For each folder, create a corresponding folder icon on the home screen.  
   * For each application (which could be a folder with a specific manifest file, or a self-contained executable), create a launcher icon within its parent folder on the screen.  
   * **Crucially, only the top-level folder structure is loaded into memory initially.**  
3. **If /sdcard/Apps does not exist:**  
   * Attempt to create the folder.  
   * If successful, display a message "Apps folder created on SD card."  
   * If creating the folder fails (e.g., read-only filesystem), display a persistent dialog box:"SD Card Error: Could not write to the SD card. Please format it to FAT32 and reboot the device."  
     \[OK\]

#### **Dynamic Memory Management:**

* When a user navigates into a folder, the contents of that folder (sub-folders and app metadata) are loaded into memory.  
* When the user navigates back out or into a different folder, the previous folder's contents are offloaded from memory. This keeps the RAM footprint minimal.

---

### **3\. UI/UX Enhancements**

A responsive UI is critical. The user must always know if the system is working or frozen.

1. **"Working" Indicator:**  
   * Create a simple, spinning animation (an 80's Mac-style watch or a modern spinning circle).  
   * This animation must be displayed in the center of the screen whenever a blocking operation is in progress (e.g., loading an app, connecting to a network).  
   * This should be a system-level overlay that any application can trigger.  
2. **Home Widget:**  
   * The top home widget should be **event-driven, not polled**.  
   * It should subscribe to relevant MQTT topics (e.g., system/time/update, system/battery/update, network/status/update).  
   * The widget will only redraw itself when it receives a new message on one of these topics. This prevents constant, unnecessary processing.

---

### **4\. Storage Optimization and App Organization**

1. **First Boot Process:**  
   * On the very first boot (or after a factory reset), a script must run that:  
     * Identifies all testing and non-essential settings apps in the internal storage.  
     * Moves them to the /sdcard/Apps/ directory.  
     * Deletes them from the internal storage.  
2. **"Connectivity" Folder:**  
   * The core, background-capable connectivity apps **must remain in internal memory** for stability.  
   * These include: LoRa, WiFi, 4G, MQTT, Bluetooth, Wireguard, and GPS settings.  
   * On the home screen, create a folder named "Connectivity" and place the launchers for these applications inside it.

---

### **5\. Debugging Plan (Software Focus)**

#### **WiFi Module: Scans but Fails to Connect**

This points to a software configuration issue in the connection process.

**Debugging Steps:**

1. **Enable Verbose Logging:**  
   * Modify the WiFi connection manager to output the most detailed logs possible. The logs will often contain a specific reason for the failure (e.g., "authentication timeout," "handshake failed," "DHCP error"), which will point you to the exact software problem.  
2. **Check Region/Country Settings:**  
   * Ensure the WiFi country code is set correctly in your device's software configuration. An incorrect setting can prevent a successful connection handshake.  
3. **Isolate with a Test Script:**  
   * Write a minimal, standalone script that does nothing but attempt to connect to the WiFi network. This will rule out any interference from other parts of your main application.

---

### **6\. Application Development Framework**

To streamline app development, we will create a standardized framework and template.

1. **Hardware Abstraction Layer (HAL):**  
   * Create a central library that provides a simplified, high-level API for interacting with the hardware (display, buttons, sensors, etc.).  
   * This prevents each app from needing to know the low-level details of the hardware.  
   * Example: Instead of an app writing directly to the framebuffer, it would call hal.display.draw\_text(x, y, "Hello").  
2. **App Template:**  
   * Create a template project that new apps can be built from. This template will include:  
     * The basic file structure.  
     * The HAL library.  
     * The MQTT client wrapper.  
     * A manifest.json file to describe the app (name, icon, etc.).  
     * A main.py (or equivalent) with a basic app loop structure.  
3. **Configuration File (settings.json):**  
   * A single JSON file stored in a known location (e.g., /sdcard/config/settings.json) will contain all device-wide settings.  
   * On startup, every app will read this file to get its necessary configuration (e.g., MQTT broker address, WireGuard keys, user preferences).
   * This centralizes configuration and makes it easy to update.

JSON  
// Example settings.json  
{  
    "device\_name": "MyLauncher",  
    "mqtt": {  
        "broker\_address": "100.101.102.103",  
        "port": 1883  
    },  
    "wifi": {  
        "ssid": "MyHomeNetwork",  
        "password": "supersecretpassword"  
    },  
    "user\_preferences": {  
        "theme": "dark",  
        "timezone": "UTC-5"  
    }  
}  
