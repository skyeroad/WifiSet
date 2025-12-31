/**
 * WiFiSetESP32 - Full Featured Example
 *
 * This example demonstrates all features of the WiFiSetESP32 library.
 */

#include <Arduino.h>
#include <WiFiSetESP32.h>

// Create WiFiSetESP32 instance
const char* DEVICE_NAME = "ESP32-WiFiSet-Test";
WiFiSetESP32 wifiSet(DEVICE_NAME);

void printSeparator() {
    Serial.println("========================================");
}

void setup() {
    Serial.begin(115200);
    delay(2000);

    Serial.println("\n\n");
    printSeparator();
    Serial.println("WiFiSetESP32 - Full Featured Example");
    printSeparator();
    Serial.printf("Chip Model: %s\n", ESP.getChipModel());
    Serial.printf("Chip Revision: %d\n", ESP.getChipRevision());
    Serial.printf("CPU Frequency: %d MHz\n", ESP.getCpuFreqMHz());
    Serial.printf("Free Heap: %d bytes\n", ESP.getFreeHeap());
    printSeparator();
    Serial.println();

    // Set up all callbacks with detailed logging
    wifiSet.onCredentialsReceived([](const String& ssid, const String& password) {
        printSeparator();
        Serial.println("[EVENT] Credentials Received");
        printSeparator();
        Serial.printf("SSID: %s\n", ssid.c_str());
        Serial.printf("Password Length: %d\n", password.length());
        Serial.printf("Security: %s\n", password.length() > 0 ? "Secured" : "Open");
        printSeparator();
        Serial.println();
    });

    wifiSet.onConnectionStatusChanged([](WiFiSet::WiFiSetConnectionStatus status) {
        Serial.print("[STATUS] Connection State: ");
        switch (status) {
            case WiFiSet::WiFiSetConnectionStatus::NOT_CONFIGURED:
                Serial.println("NOT_CONFIGURED");
                Serial.println("         No credentials stored. BLE advertising should be active.");
                break;
            case WiFiSet::WiFiSetConnectionStatus::CONFIGURED_NOT_CONNECTED:
                Serial.println("CONFIGURED_NOT_CONNECTED");
                Serial.println("         Credentials saved but not connected to WiFi.");
                break;
            case WiFiSet::WiFiSetConnectionStatus::CONNECTING:
                Serial.println("CONNECTING");
                Serial.println("         Attempting to connect to WiFi...");
                break;
            case WiFiSet::WiFiSetConnectionStatus::CONNECTED:
                Serial.println("CONNECTED");
                Serial.println("         Successfully connected to WiFi!");
                break;
            case WiFiSet::WiFiSetConnectionStatus::CONNECTION_FAILED:
                Serial.println("CONNECTION_FAILED");
                Serial.println("         Failed to connect. Check credentials or network.");
                break;
        }
        Serial.println();
    });

    wifiSet.onWiFiConnected([](IPAddress ip) {
        printSeparator();
        Serial.println("[SUCCESS] WiFi Connected!");
        printSeparator();
        Serial.printf("SSID: %s\n", wifiSet.getSSID().c_str());
        Serial.printf("IP Address: %s\n", ip.toString().c_str());
        Serial.printf("Signal Strength: %d dBm\n", wifiSet.getRSSI());
        Serial.printf("Gateway: %s\n", WiFi.gatewayIP().toString().c_str());
        Serial.printf("DNS: %s\n", WiFi.dnsIP().toString().c_str());
        printSeparator();
        Serial.println();

        // Optional: Stop BLE to save power once WiFi is connected
        // wifiSet.stopBLE();
        // Serial.println("[INFO] BLE stopped to save power");
    });

    wifiSet.onWiFiConnectionFailed([]() {
        printSeparator();
        Serial.println("[ERROR] WiFi Connection Failed!");
        printSeparator();
        Serial.println("Possible reasons:");
        Serial.println("  - Wrong password");
        Serial.println("  - Network not in range");
        Serial.println("  - Router issue");
        Serial.println();
        Serial.println("Solution:");
        Serial.println("  - Use iOS app to reconfigure WiFi");
        Serial.println("  - BLE advertising is active");
        printSeparator();
        Serial.println();
    });

    wifiSet.onBLEClientConnected([]() {
        printSeparator();
        Serial.println("[BLE] Client Connected");
        printSeparator();
        Serial.println("iOS app connected via BLE");
        Serial.println("Performing WiFi scan...");
        Serial.println("Network list will be sent to iOS app");
        printSeparator();
        Serial.println();
    });

    wifiSet.onBLEClientDisconnected([]() {
        Serial.println("[BLE] Client Disconnected");
        Serial.println("      BLE advertising will resume automatically\n");
    });

    // Initialize WiFiSet first (this initializes NVS)
    Serial.println("[INIT] Starting WiFiSet library...");
    wifiSet.begin();
    Serial.println("[INIT] WiFiSet library started!\n");

    // Check for saved credentials AFTER initialization
    Serial.println("[INIT] Checking saved credentials...");
    WiFiSet::WiFiSetCredentials savedCreds = wifiSet.getSavedCredentials();
    if (savedCreds.isValid) {
        Serial.println("[INIT] Found saved credentials:");
        Serial.printf("       SSID: %s\n", savedCreds.ssid.c_str());
    } else {
        Serial.println("[INIT] No saved credentials - BLE advertising active\n");
    }

    if (wifiSet.isBLERunning()) {
        printSeparator();
        Serial.println("BLE Advertising Active");
        printSeparator();
        Serial.printf("Device Name: %s\n", DEVICE_NAME);
        Serial.println();
        Serial.println("To configure WiFi:");
        Serial.println("1. Open WiFiSet app on iPhone");
        Serial.println("2. Scan for devices");
        Serial.printf("3. Select '%s'\n", DEVICE_NAME);
        Serial.println("4. Choose WiFi network");
        Serial.println("5. Enter password");
        printSeparator();
        Serial.println();
    }

    if (wifiSet.isConnected()) {
        Serial.println("[INFO] Already connected to WiFi");
        Serial.printf("       You can access this device at: %s\n\n", wifiSet.getIPAddress().toString().c_str());
    }
}

unsigned long lastStatusPrint = 0;
unsigned long lastHeapPrint = 0;

void loop() {
    // Must call loop() regularly
    wifiSet.loop();

    // Print status every 30 seconds
    if (millis() - lastStatusPrint >= 30000) {
        lastStatusPrint = millis();

        Serial.println("\n--- Periodic Status ---");
        Serial.printf("Uptime: %lu seconds\n", millis() / 1000);
        Serial.printf("WiFi Status: %s\n", wifiSet.isConnected() ? "Connected" : "Disconnected");

        if (wifiSet.isConnected()) {
            Serial.printf("  Network: %s\n", wifiSet.getSSID().c_str());
            Serial.printf("  IP: %s\n", wifiSet.getIPAddress().toString().c_str());
            Serial.printf("  RSSI: %d dBm\n", wifiSet.getRSSI());
        }

        Serial.printf("BLE: %s\n", wifiSet.isBLERunning() ? "Advertising" : "Stopped");
        Serial.println("----------------------\n");
    }

    // Print heap status every 60 seconds
    if (millis() - lastHeapPrint >= 60000) {
        lastHeapPrint = millis();

        Serial.printf("[HEAP] Free: %d bytes, Min Free: %d bytes\n\n",
                     ESP.getFreeHeap(), ESP.getMinFreeHeap());
    }

    // Your application code here
    // Example: If connected to WiFi, you could make HTTP requests, MQTT, etc.

    delay(100);
}
