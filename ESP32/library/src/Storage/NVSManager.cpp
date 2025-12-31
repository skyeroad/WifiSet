#include "NVSManager.h"

namespace WiFiSet {

const char* NVSManager::NAMESPACE = "wifiset";
const char* NVSManager::KEY_SSID = "ssid";
const char* NVSManager::KEY_PASSWORD = "password";

NVSManager::NVSManager() : initialized(false), lastError("") {}

NVSManager::~NVSManager() {
    if (initialized) {
        preferences.end();
    }
}

void NVSManager::setError(const String& error) {
    lastError = error;
}

bool NVSManager::begin() {
    if (initialized) {
        return true;
    }

    initialized = true;
    return true;
}

bool NVSManager::saveCredentials(const String& ssid, const String& password) {
    if (!initialized) {
        setError("NVS not initialized");
        return false;
    }

    // Validate input
    if (ssid.length() == 0 || ssid.length() > 32) {
        setError("Invalid SSID length");
        return false;
    }

    if (password.length() > 63) {
        setError("Invalid password length");
        return false;
    }

    // Open preferences for writing
    if (!preferences.begin(NAMESPACE, false)) {
        setError("Failed to open NVS for writing");
        return false;
    }

    // Save credentials
    size_t ssidWritten = preferences.putString(KEY_SSID, ssid);
    size_t passwordWritten = preferences.putString(KEY_PASSWORD, password);

    preferences.end();

    if (ssidWritten == 0) {
        setError("Failed to write SSID to NVS");
        return false;
    }

    if (password.length() > 0 && passwordWritten == 0) {
        setError("Failed to write password to NVS");
        return false;
    }

    return true;
}

StoredCredentials NVSManager::loadCredentials() {
    StoredCredentials credentials;

    if (!initialized) {
        setError("NVS not initialized");
        return credentials;
    }

    // Open preferences for reading
    if (!preferences.begin(NAMESPACE, true)) {
        setError("Failed to open NVS for reading");
        return credentials;
    }

    // Load credentials
    String ssid = preferences.getString(KEY_SSID, "");
    String password = preferences.getString(KEY_PASSWORD, "");

    preferences.end();

    // Validate loaded data
    if (ssid.length() == 0) {
        setError("No credentials stored");
        return credentials;
    }

    credentials.ssid = ssid;
    credentials.password = password;
    credentials.isValid = true;

    return credentials;
}

bool NVSManager::hasCredentials() {
    if (!initialized) {
        return false;
    }

    // Open preferences for reading
    if (!preferences.begin(NAMESPACE, true)) {
        return false;
    }

    bool exists = preferences.isKey(KEY_SSID);

    preferences.end();

    return exists;
}

bool NVSManager::clearCredentials() {
    if (!initialized) {
        setError("NVS not initialized");
        return false;
    }

    // Open preferences for writing
    if (!preferences.begin(NAMESPACE, false)) {
        setError("Failed to open NVS for writing");
        return false;
    }

    // Clear all keys in the namespace
    bool success = preferences.clear();

    preferences.end();

    if (!success) {
        setError("Failed to clear credentials from NVS");
        return false;
    }

    return true;
}

} // namespace WiFiSet
