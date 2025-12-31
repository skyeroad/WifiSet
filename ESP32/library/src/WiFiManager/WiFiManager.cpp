#include "WiFiManager.h"

namespace WiFiSet {

WiFiManager::WiFiManager() : lastError(""), connectionState(ConnectionState::NOT_CONFIGURED), credentialsConfigured(false) {}

void WiFiManager::setError(const String& error) {
    lastError = error;
}

void WiFiManager::begin() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    updateConnectionState();
}

SecurityType WiFiManager::convertEncryptionType(wifi_auth_mode_t authMode) {
    switch (authMode) {
        case WIFI_AUTH_OPEN:
            return SecurityType::OPEN;
        case WIFI_AUTH_WEP:
            return SecurityType::WEP;
        case WIFI_AUTH_WPA_PSK:
        case WIFI_AUTH_WPA2_PSK:
        case WIFI_AUTH_WPA_WPA2_PSK:
            return SecurityType::WPA_PSK;
        case WIFI_AUTH_WPA2_ENTERPRISE:
            return SecurityType::WPA2_ENTERPRISE;
        case WIFI_AUTH_WPA3_PSK:
            return SecurityType::WPA3;
        default:
            return SecurityType::WPA_PSK;
    }
}

std::vector<WiFiNetworkInfo> WiFiManager::scanNetworks() {
    std::vector<WiFiNetworkInfo> networks;

    // Start WiFi scan
    int numNetworks = WiFi.scanNetworks();

    if (numNetworks == -1) {
        setError("WiFi scan failed");
        return networks;
    }

    if (numNetworks == 0) {
        // No networks found, but not an error
        return networks;
    }

    // Convert scan results to WiFiNetworkInfo
    for (int i = 0; i < numNetworks && i < 50; i++) { // Limit to 50 networks
        WiFiNetworkInfo info;
        info.ssid = WiFi.SSID(i);
        info.rssi = static_cast<int8_t>(WiFi.RSSI(i));
        info.securityType = convertEncryptionType(WiFi.encryptionType(i));
        info.channel = static_cast<uint8_t>(WiFi.channel(i));

        networks.push_back(info);
    }

    // Clean up scan results
    WiFi.scanDelete();

    return networks;
}

WiFiConnectResult WiFiManager::connect(const String& ssid, const String& password, unsigned long timeoutMs) {
    if (ssid.length() == 0) {
        setError("SSID cannot be empty");
        return WiFiConnectResult::FAILED_UNKNOWN;
    }

    Serial.printf("[WiFi] Connecting to: '%s'\n", ssid.c_str());
    Serial.printf("[WiFi] Password length: %d\n", password.length());

    // Disconnect if already connected
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("[WiFi] Disconnecting from current network...");
        WiFi.disconnect();
        delay(100);
    }

    // Update state to connecting
    connectionState = ConnectionState::CONNECTING;

    // Start connection
    Serial.println("[WiFi] Calling WiFi.begin()...");
    WiFi.begin(ssid.c_str(), password.c_str());

    // Wait for connection with timeout
    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - startTime > timeoutMs) {
            Serial.printf("[WiFi] Timeout after %lu ms\n", timeoutMs);
            setError("Connection timeout");
            connectionState = ConnectionState::CONNECTION_FAILED;
            WiFi.disconnect();
            return WiFiConnectResult::FAILED_TIMEOUT;
        }

        // Check for specific failure reasons
        wl_status_t status = WiFi.status();
        if (status == WL_CONNECT_FAILED) {
            Serial.println("[WiFi] WL_CONNECT_FAILED - wrong password?");
            setError("Connection failed - wrong password or network issue");
            connectionState = ConnectionState::CONNECTION_FAILED;
            WiFi.disconnect();
            return WiFiConnectResult::FAILED_WRONG_PASSWORD;
        } else if (status == WL_NO_SSID_AVAIL) {
            Serial.println("[WiFi] WL_NO_SSID_AVAIL - network not found");
            setError("Network not found");
            connectionState = ConnectionState::CONNECTION_FAILED;
            WiFi.disconnect();
            return WiFiConnectResult::FAILED_NOT_FOUND;
        }

        // Print status every second
        static unsigned long lastPrint = 0;
        if (millis() - lastPrint > 1000) {
            Serial.printf("[WiFi] Status: %d, waiting...\n", status);
            lastPrint = millis();
        }

        delay(100);
    }

    // Connection successful
    Serial.printf("[WiFi] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
    connectionState = ConnectionState::CONNECTED;
    return WiFiConnectResult::SUCCESS;
}

void WiFiManager::disconnect() {
    WiFi.disconnect();
    updateConnectionState();
}

bool WiFiManager::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

void WiFiManager::updateConnectionState() {
    if (WiFi.status() == WL_CONNECTED) {
        connectionState = ConnectionState::CONNECTED;
    } else if (credentialsConfigured) {
        // Credentials are saved in NVS but not currently connected
        connectionState = ConnectionState::CONFIGURED_NOT_CONNECTED;
    } else {
        connectionState = ConnectionState::NOT_CONFIGURED;
    }
}

void WiFiManager::setCredentialsConfigured(bool configured, const String& ssid) {
    credentialsConfigured = configured;
    if (!ssid.isEmpty()) {
        configuredSSID = ssid;
    }
    updateConnectionState();
}

ConnectionState WiFiManager::getConnectionState() {
    updateConnectionState();
    return connectionState;
}

int8_t WiFiManager::getRSSI() {
    if (isConnected()) {
        return static_cast<int8_t>(WiFi.RSSI());
    }
    return 0;
}

IPAddress WiFiManager::getIPAddress() {
    if (isConnected()) {
        return WiFi.localIP();
    }
    return IPAddress(0, 0, 0, 0);
}

String WiFiManager::getSSID() {
    // If connected, return the actual connected SSID
    if (isConnected()) {
        return WiFi.SSID();
    }
    // Otherwise return the configured SSID (from NVS)
    return configuredSSID;
}

} // namespace WiFiSet
