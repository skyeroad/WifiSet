#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <vector>
#include "../Protocol/MessageBuilder.h"

namespace WiFiSet {

/**
 * WiFi connection result
 */
enum class WiFiConnectResult {
    SUCCESS,
    FAILED_WRONG_PASSWORD,
    FAILED_NOT_FOUND,
    FAILED_TIMEOUT,
    FAILED_UNKNOWN
};

/**
 * WiFiManager - Manages WiFi scanning and connections
 *
 * Handles WiFi network scanning, connection management, and status monitoring.
 */
class WiFiManager {
public:
    WiFiManager();

    /**
     * Initialize WiFi
     * Sets WiFi mode to STA (station mode)
     */
    void begin();

    /**
     * Scan for available WiFi networks
     * @return Vector of discovered networks
     */
    std::vector<WiFiNetworkInfo> scanNetworks();

    /**
     * Connect to WiFi network
     * @param ssid Network name
     * @param password Network password (empty for open networks)
     * @param timeoutMs Connection timeout in milliseconds (default 10000)
     * @return Connection result
     */
    WiFiConnectResult connect(const String& ssid, const String& password, unsigned long timeoutMs = 10000);

    /**
     * Disconnect from WiFi
     */
    void disconnect();

    /**
     * Check if connected to WiFi
     * @return true if connected
     */
    bool isConnected();

    /**
     * Get connection status
     * @return Current connection state
     */
    ConnectionState getConnectionState();

    /**
     * Get current RSSI (signal strength)
     * @return RSSI in dBm (0 if not connected)
     */
    int8_t getRSSI();

    /**
     * Get current IP address
     * @return IP address (0.0.0.0 if not connected)
     */
    IPAddress getIPAddress();

    /**
     * Get currently connected SSID
     * @return SSID (empty if not connected)
     */
    String getSSID();

    /**
     * Get last error message
     */
    const String& getLastError() const { return lastError; }

    /**
     * Convert ESP32 WiFi encryption type to protocol security type
     */
    static SecurityType convertEncryptionType(wifi_auth_mode_t authMode);

    /**
     * Mark that credentials have been configured
     * Called after credentials are saved to NVS
     */
    void setCredentialsConfigured(bool configured);

private:
    String lastError;
    ConnectionState connectionState;
    bool credentialsConfigured;

    /**
     * Set error message
     */
    void setError(const String& error);

    /**
     * Update connection state
     */
    void updateConnectionState();
};

} // namespace WiFiSet

#endif // WIFI_MANAGER_H
