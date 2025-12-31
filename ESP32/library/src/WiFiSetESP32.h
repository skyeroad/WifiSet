#ifndef WIFISET_ESP32_H
#define WIFISET_ESP32_H

#include <Arduino.h>
#include <functional>
#include <WiFi.h>
#include "BLEService/BLEService.h"
#include "WiFiManager/WiFiManager.h"
#include "Storage/NVSManager.h"

namespace WiFiSet {

/**
 * WiFi connection status for user callbacks
 */
enum class WiFiSetConnectionStatus {
    NOT_CONFIGURED,           // No credentials stored
    CONFIGURED_NOT_CONNECTED, // Credentials stored but not connected
    CONNECTING,               // Attempting to connect
    CONNECTED,                // Successfully connected
    CONNECTION_FAILED         // Failed to connect
};

/**
 * Stored WiFi credentials
 */
struct WiFiSetCredentials {
    String ssid;
    String password;
    bool isValid;

    WiFiSetCredentials() : isValid(false) {}
};

} // namespace WiFiSet

/**
 * WiFiSetESP32 - Main public API for WiFi configuration over BLE
 *
 * This class provides a simple interface to enable WiFi configuration
 * via BLE on ESP32 devices. It handles all the complexity of BLE communication,
 * WiFi scanning, credential storage, and connection management.
 *
 * Basic usage:
 *   WiFiSetESP32 wifiSet("MyDevice");
 *   wifiSet.begin();
 *
 *   void loop() {
 *     wifiSet.loop();
 *   }
 */
class WiFiSetESP32 : private WiFiSet::BLEServiceCallbacks {
public:
    /**
     * Constructor
     * @param deviceName BLE device name for advertising (default: "ESP32-WiFiSet")
     */
    WiFiSetESP32(const char* deviceName = "ESP32-WiFiSet");

    /**
     * Destructor
     */
    ~WiFiSetESP32();

    /**
     * Initialize the library
     * - Initializes BLE service
     * - Initializes WiFi manager
     * - Loads saved credentials
     * - Attempts to connect if credentials exist
     * - Starts BLE advertising if not connected
     *
     * Must be called from setup()
     */
    void begin();

    /**
     * Main loop processing
     * - Monitors WiFi connection status
     * - Sends status updates to connected BLE clients
     *
     * Must be called from loop()
     */
    void loop();

    // ==================== Callbacks ====================

    /**
     * Set callback for when credentials are received via BLE
     * @param callback Function called with (ssid, password)
     */
    void onCredentialsReceived(std::function<void(const String& ssid, const String& password)> callback);

    /**
     * Set callback for when WiFi connection status changes
     * @param callback Function called with new status
     */
    void onConnectionStatusChanged(std::function<void(WiFiSet::WiFiSetConnectionStatus status)> callback);

    /**
     * Set callback for when WiFi successfully connects
     * @param callback Function called with IP address
     */
    void onWiFiConnected(std::function<void(IPAddress ip)> callback);

    /**
     * Set callback for when WiFi connection fails
     * @param callback Function called when connection fails
     */
    void onWiFiConnectionFailed(std::function<void()> callback);

    /**
     * Set callback for when a BLE client connects
     * @param callback Function called when client connects
     */
    void onBLEClientConnected(std::function<void()> callback);

    /**
     * Set callback for when a BLE client disconnects
     * @param callback Function called when client disconnects
     */
    void onBLEClientDisconnected(std::function<void()> callback);

    // ==================== Credential Management ====================

    /**
     * Get saved WiFi credentials from NVS
     * @return Credentials (isValid will be false if none saved)
     */
    WiFiSet::WiFiSetCredentials getSavedCredentials();

    /**
     * Clear saved credentials from NVS
     * Does not disconnect from current WiFi
     * @return true if successful
     */
    bool clearCredentials();

    // ==================== WiFi Control ====================

    /**
     * Manually connect to WiFi network
     * @param ssid Network name
     * @param password Network password
     * @param save Save credentials to NVS (default: true)
     * @return true if connection successful
     */
    bool connectWiFi(const String& ssid, const String& password, bool save = true);

    /**
     * Disconnect from WiFi
     */
    void disconnectWiFi();

    // ==================== Status ====================

    /**
     * Get current WiFi connection status
     * @return Current status
     */
    WiFiSet::WiFiSetConnectionStatus getConnectionStatus();

    /**
     * Check if connected to WiFi
     * @return true if connected
     */
    bool isConnected();

    /**
     * Get current IP address
     * @return IP address (0.0.0.0 if not connected)
     */
    IPAddress getIPAddress();

    /**
     * Get current WiFi RSSI (signal strength)
     * @return RSSI in dBm (0 if not connected)
     */
    int getRSSI();

    /**
     * Get currently connected/configured SSID
     * @return SSID (empty if not configured)
     */
    String getSSID();

    // ==================== BLE Control ====================

    /**
     * Start BLE advertising
     * Useful if you stopped BLE and want to restart it
     */
    void startBLE();

    /**
     * Stop BLE advertising
     * Useful to save power when WiFi is connected and you don't need BLE
     */
    void stopBLE();

    /**
     * Check if BLE is running
     * @return true if BLE initialized and advertising
     */
    bool isBLERunning();

private:
    WiFiSet::WiFiSetBLEService bleService;
    WiFiSet::WiFiManager wifiManager;
    WiFiSet::NVSManager nvsManager;

    String deviceName;
    WiFiSet::ConnectionState lastConnectionState;
    unsigned long lastStatusUpdate;

    // User callbacks
    std::function<void(const String&, const String&)> credentialsReceivedCallback;
    std::function<void(WiFiSet::WiFiSetConnectionStatus)> connectionStatusCallback;
    std::function<void(IPAddress)> wifiConnectedCallback;
    std::function<void()> wifiConnectionFailedCallback;
    std::function<void()> bleClientConnectedCallback;
    std::function<void()> bleClientDisconnectedCallback;

    /**
     * BLEServiceCallbacks implementation
     */
    void onCredentialsReceived(const String& ssid, const String& password) override;
    void onClientConnected() override;
    void onClientDisconnected() override;
    void onStatusRequest() override;

    /**
     * Convert internal ConnectionState to user-facing WiFiSetConnectionStatus
     */
    static WiFiSet::WiFiSetConnectionStatus convertConnectionState(WiFiSet::ConnectionState state);

    /**
     * Handle WiFi connection with received credentials
     */
    void handleWiFiConnection(const String& ssid, const String& password);

    /**
     * Send current status to BLE client
     */
    void sendCurrentStatus();

    /**
     * Monitor WiFi connection and send updates
     */
    void monitorConnection();

    /**
     * Trigger WiFi scan and send results to BLE client
     */
    void performWiFiScanAndSend();
};

#endif // WIFISET_ESP32_H
