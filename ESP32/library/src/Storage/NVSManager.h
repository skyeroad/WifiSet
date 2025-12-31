#ifndef NVS_MANAGER_H
#define NVS_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>

namespace WiFiSet {

/**
 * Stored WiFi credentials
 */
struct StoredCredentials {
    String ssid;
    String password;
    bool isValid;

    StoredCredentials() : isValid(false) {}
    StoredCredentials(const String& s, const String& p) : ssid(s), password(p), isValid(true) {}
};

/**
 * NVSManager - Manages persistent storage of WiFi credentials
 *
 * Uses ESP32's NVS (Non-Volatile Storage) via the Preferences library
 * to store WiFi credentials that persist across reboots.
 *
 * Storage namespace: "wifiset"
 * Keys:
 *   - "ssid": WiFi network name
 *   - "password": WiFi password
 */
class NVSManager {
public:
    NVSManager();
    ~NVSManager();

    /**
     * Initialize NVS
     * Must be called before other operations
     * @return true if initialization successful
     */
    bool begin();

    /**
     * Save WiFi credentials to NVS
     * @param ssid WiFi network name (max 32 bytes)
     * @param password WiFi password (max 63 bytes)
     * @return true if save successful
     */
    bool saveCredentials(const String& ssid, const String& password);

    /**
     * Load saved WiFi credentials from NVS
     * @return Stored credentials (isValid will be false if none saved)
     */
    StoredCredentials loadCredentials();

    /**
     * Check if credentials are stored in NVS
     * @return true if credentials exist
     */
    bool hasCredentials();

    /**
     * Clear stored credentials from NVS
     * @return true if clear successful
     */
    bool clearCredentials();

    /**
     * Get last error message
     */
    const String& getLastError() const { return lastError; }

private:
    Preferences preferences;
    String lastError;
    bool initialized;

    static const char* NAMESPACE;
    static const char* KEY_SSID;
    static const char* KEY_PASSWORD;

    /**
     * Set error message
     */
    void setError(const String& error);
};

} // namespace WiFiSet

#endif // NVS_MANAGER_H
