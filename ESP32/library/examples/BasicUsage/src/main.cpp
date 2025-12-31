/**
 * WiFiSetESP32 - Basic Usage Example
 *
 * This example demonstrates the basic usage of the WiFiSetESP32 library.
 *
 * Features demonstrated:
 * - Automatic WiFi connection using saved credentials
 * - BLE advertising for WiFi configuration
 * - Callbacks for connection status
 * - Serial output for debugging
 *
 * How to use:
 * 1. Flash this example to your ESP32
 * 2. Open Serial Monitor (115200 baud)
 * 3. Use the WiFiSet iOS app to configure WiFi credentials
 * 4. ESP32 will connect to WiFi and save credentials
 * 5. On subsequent reboots, ESP32 will automatically connect
 */

#include <Arduino.h>
#include <WiFiSetESP32.h>

// Create WiFiSetESP32 instance with custom device name
WiFiSetESP32 wifiSet("MyESP32Device");

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n\n========================================");
    Serial.println("WiFiSetESP32 - Basic Usage Example");
    Serial.println("========================================\n");

    // Set up callbacks
    wifiSet.onCredentialsReceived([](const String& ssid, const String& password) {
        Serial.println("\n[CALLBACK] New credentials received:");
        Serial.printf("  SSID: %s\n", ssid.c_str());
        Serial.printf("  Password: %s\n", password.length() > 0 ? "********" : "(none - open network)");
    });

    wifiSet.onConnectionStatusChanged([](WiFiSet::WiFiSetConnectionStatus status) {
        Serial.print("\n[CALLBACK] Connection status changed: ");
        switch (status) {
            case WiFiSet::WiFiSetConnectionStatus::NOT_CONFIGURED:
                Serial.println("NOT_CONFIGURED");
                break;
            case WiFiSet::WiFiSetConnectionStatus::CONFIGURED_NOT_CONNECTED:
                Serial.println("CONFIGURED_NOT_CONNECTED");
                break;
            case WiFiSet::WiFiSetConnectionStatus::CONNECTING:
                Serial.println("CONNECTING...");
                break;
            case WiFiSet::WiFiSetConnectionStatus::CONNECTED:
                Serial.println("CONNECTED");
                break;
            case WiFiSet::WiFiSetConnectionStatus::CONNECTION_FAILED:
                Serial.println("CONNECTION_FAILED");
                break;
        }
    });

    wifiSet.onWiFiConnected([](IPAddress ip) {
        Serial.println("\n[CALLBACK] WiFi connected!");
        Serial.printf("  IP Address: %s\n", ip.toString().c_str());
        Serial.printf("  RSSI: %d dBm\n", wifiSet.getRSSI());
        Serial.printf("  SSID: %s\n", wifiSet.getSSID().c_str());
    });

    wifiSet.onWiFiConnectionFailed([]() {
        Serial.println("\n[CALLBACK] WiFi connection failed!");
        Serial.println("  Please reconfigure using the iOS app");
        Serial.println("  BLE advertising is active");
    });

    wifiSet.onBLEClientConnected([]() {
        Serial.println("\n[CALLBACK] BLE client connected");
        Serial.println("  iOS app is connected via BLE");
        Serial.println("  Sending WiFi network list...");
    });

    wifiSet.onBLEClientDisconnected([]() {
        Serial.println("\n[CALLBACK] BLE client disconnected");
    });

    // Check for saved credentials
    WiFiSet::WiFiSetCredentials savedCreds = wifiSet.getSavedCredentials();
    if (savedCreds.isValid) {
        Serial.println("Found saved credentials:");
        Serial.printf("  SSID: %s\n", savedCreds.ssid.c_str());
        Serial.println("  Will attempt to connect automatically...\n");
    } else {
        Serial.println("No saved credentials found");
        Serial.println("BLE advertising will start\n");
    }

    // Initialize WiFiSet library
    Serial.println("Initializing WiFiSet...");
    wifiSet.begin();
    Serial.println("WiFiSet initialized!\n");

    if (wifiSet.isBLERunning()) {
        Serial.println("========================================");
        Serial.println("BLE Advertising Active");
        Serial.println("========================================");
        Serial.println("Device Name: MyESP32Device");
        Serial.println("Use the WiFiSet iOS app to configure WiFi");
        Serial.println("========================================\n");
    }
}

void loop() {
    // Must call loop() regularly
    wifiSet.loop();

    // Your application code here
    static unsigned long lastPrint = 0;
    if (millis() - lastPrint > 30000) { // Print status every 30 seconds
        lastPrint = millis();

        Serial.println("\n--- Status Update ---");
        Serial.printf("WiFi Connected: %s\n", wifiSet.isConnected() ? "YES" : "NO");
        if (wifiSet.isConnected()) {
            Serial.printf("  SSID: %s\n", wifiSet.getSSID().c_str());
            Serial.printf("  IP: %s\n", wifiSet.getIPAddress().toString().c_str());
            Serial.printf("  RSSI: %d dBm\n", wifiSet.getRSSI());
        }
        Serial.printf("BLE Running: %s\n", wifiSet.isBLERunning() ? "YES" : "NO");
        Serial.println("--------------------\n");
    }

    delay(100);
}
