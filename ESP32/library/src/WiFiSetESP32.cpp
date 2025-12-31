#include "WiFiSetESP32.h"

using namespace WiFiSet;

WiFiSetESP32::WiFiSetESP32(const char* deviceName)
    : deviceName(deviceName),
      lastConnectionState(ConnectionState::NOT_CONFIGURED),
      lastStatusUpdate(0),
      pendingClientConnect(false),
      pendingClientDisconnect(false) {}

WiFiSetESP32::~WiFiSetESP32() {}

void WiFiSetESP32::begin() {
    // Initialize NVS
    nvsManager.begin();

    // Initialize WiFi Manager
    wifiManager.begin();

    // Initialize BLE Service
    bleService.begin(deviceName.c_str());
    bleService.setCallbacks(this);

    // Load saved credentials
    StoredCredentials credentials = nvsManager.loadCredentials();

    if (credentials.isValid) {
        // Attempt to connect with saved credentials
        WiFiConnectResult result = wifiManager.connect(credentials.ssid, credentials.password);

        if (result == WiFiConnectResult::SUCCESS) {
            lastConnectionState = ConnectionState::CONNECTED;
            if (wifiConnectedCallback) {
                wifiConnectedCallback(wifiManager.getIPAddress());
            }
        } else {
            lastConnectionState = ConnectionState::CONNECTION_FAILED;
            if (wifiConnectionFailedCallback) {
                wifiConnectionFailedCallback();
            }
            // Start BLE advertising since WiFi failed
            bleService.startAdvertising();
        }
    } else {
        // No saved credentials, start BLE advertising
        lastConnectionState = ConnectionState::NOT_CONFIGURED;
        bleService.startAdvertising();
    }
}

void WiFiSetESP32::loop() {
    bleService.loop();

    // Handle deferred BLE client connect (do heavy work outside callback)
    if (pendingClientConnect) {
        pendingClientConnect = false;

        // Notify user callback (now safe - we're in main loop)
        if (bleClientConnectedCallback) {
            bleClientConnectedCallback();
        }

        // Small delay to let serial output complete
        delay(100);

        // Perform WiFi scan and send results
        performWiFiScanAndSend();

        // Send current status
        delay(100);
        sendCurrentStatus();
    }

    // Handle deferred BLE client disconnect
    if (pendingClientDisconnect) {
        pendingClientDisconnect = false;

        if (bleClientDisconnectedCallback) {
            bleClientDisconnectedCallback();
        }
    }

    monitorConnection();
}

WiFiSetConnectionStatus WiFiSetESP32::convertConnectionState(ConnectionState state) {
    switch (state) {
        case ConnectionState::NOT_CONFIGURED:
            return WiFiSetConnectionStatus::NOT_CONFIGURED;
        case ConnectionState::CONFIGURED_NOT_CONNECTED:
            return WiFiSetConnectionStatus::CONFIGURED_NOT_CONNECTED;
        case ConnectionState::CONNECTING:
            return WiFiSetConnectionStatus::CONNECTING;
        case ConnectionState::CONNECTED:
            return WiFiSetConnectionStatus::CONNECTED;
        case ConnectionState::CONNECTION_FAILED:
            return WiFiSetConnectionStatus::CONNECTION_FAILED;
        default:
            return WiFiSetConnectionStatus::NOT_CONFIGURED;
    }
}

void WiFiSetESP32::monitorConnection() {
    // Check connection state periodically
    ConnectionState currentState = wifiManager.getConnectionState();

    // Send status update to BLE client if state changed or every 10 seconds
    if (currentState != lastConnectionState || (millis() - lastStatusUpdate > 10000)) {
        if (bleService.isClientConnected()) {
            sendCurrentStatus();
        }

        // Trigger user callback if state changed
        if (currentState != lastConnectionState) {
            lastConnectionState = currentState;
            if (connectionStatusCallback) {
                connectionStatusCallback(convertConnectionState(currentState));
            }

            // Trigger specific callbacks
            if (currentState == ConnectionState::CONNECTED && wifiConnectedCallback) {
                wifiConnectedCallback(wifiManager.getIPAddress());
            } else if (currentState == ConnectionState::CONNECTION_FAILED && wifiConnectionFailedCallback) {
                wifiConnectionFailedCallback();
            }
        }

        lastStatusUpdate = millis();
    }
}

void WiFiSetESP32::sendCurrentStatus() {
    ConnectionState state = wifiManager.getConnectionState();
    int8_t rssi = wifiManager.getRSSI();
    IPAddress ip = wifiManager.getIPAddress();
    String ssid = wifiManager.getSSID();

    bleService.sendStatusResponse(state, rssi, ip, ssid);
}

void WiFiSetESP32::performWiFiScanAndSend() {
    // Give time for BLE connection to stabilize
    delay(1000);
    yield();

    // Perform WiFi scan (minimal debug output)
    std::vector<WiFiNetworkInfo> networks = wifiManager.scanNetworks();

    delay(100);
    yield();

    // Send network list to BLE client
    if (bleService.isClientConnected()) {
        bleService.sendWiFiNetworkList(networks);
        delay(100);
        Serial.printf("[SCAN] Sent %d networks\n", networks.size());
    }
}

void WiFiSetESP32::handleWiFiConnection(const String& ssid, const String& password) {
    // Save credentials to NVS
    if (!nvsManager.saveCredentials(ssid, password)) {
        bleService.sendError(ErrorCode::STORAGE_ERROR, nvsManager.getLastError());
        return;
    }

    // Attempt to connect
    lastConnectionState = ConnectionState::CONNECTING;
    sendCurrentStatus();

    WiFiConnectResult result = wifiManager.connect(ssid, password);

    if (result == WiFiConnectResult::SUCCESS) {
        lastConnectionState = ConnectionState::CONNECTED;
        sendCurrentStatus();

        if (wifiConnectedCallback) {
            wifiConnectedCallback(wifiManager.getIPAddress());
        }
    } else {
        lastConnectionState = ConnectionState::CONNECTION_FAILED;
        sendCurrentStatus();

        if (wifiConnectionFailedCallback) {
            wifiConnectionFailedCallback();
        }

        // Send error to BLE client
        String errorMsg = "Connection failed: " + wifiManager.getLastError();
        bleService.sendError(ErrorCode::CONNECTION_TIMEOUT, errorMsg);
    }
}

//
// BLEServiceCallbacks implementation
//

void WiFiSetESP32::onCredentialsReceived(const String& ssid, const String& password) {
    // Notify user callback
    if (credentialsReceivedCallback) {
        credentialsReceivedCallback(ssid, password);
    }

    // Handle WiFi connection
    handleWiFiConnection(ssid, password);
}

void WiFiSetESP32::onClientConnected() {
    // Just set flag - heavy work done in loop() to avoid BLE callback issues
    pendingClientConnect = true;
}

void WiFiSetESP32::onClientDisconnected() {
    // Just set flag - callback done in loop() to avoid BLE callback issues
    pendingClientDisconnect = true;
}

void WiFiSetESP32::onStatusRequest() {
    // Send current status
    sendCurrentStatus();
}

//
// Public API - Callbacks
//

void WiFiSetESP32::onCredentialsReceived(std::function<void(const String&, const String&)> callback) {
    credentialsReceivedCallback = callback;
}

void WiFiSetESP32::onConnectionStatusChanged(std::function<void(WiFiSetConnectionStatus)> callback) {
    connectionStatusCallback = callback;
}

void WiFiSetESP32::onWiFiConnected(std::function<void(IPAddress)> callback) {
    wifiConnectedCallback = callback;
}

void WiFiSetESP32::onWiFiConnectionFailed(std::function<void()> callback) {
    wifiConnectionFailedCallback = callback;
}

void WiFiSetESP32::onBLEClientConnected(std::function<void()> callback) {
    bleClientConnectedCallback = callback;
}

void WiFiSetESP32::onBLEClientDisconnected(std::function<void()> callback) {
    bleClientDisconnectedCallback = callback;
}

//
// Public API - Credential Management
//

WiFiSetCredentials WiFiSetESP32::getSavedCredentials() {
    WiFiSetCredentials result;
    StoredCredentials stored = nvsManager.loadCredentials();

    result.ssid = stored.ssid;
    result.password = stored.password;
    result.isValid = stored.isValid;

    return result;
}

bool WiFiSetESP32::clearCredentials() {
    return nvsManager.clearCredentials();
}

//
// Public API - WiFi Control
//

bool WiFiSetESP32::connectWiFi(const String& ssid, const String& password, bool save) {
    if (save) {
        if (!nvsManager.saveCredentials(ssid, password)) {
            return false;
        }
    }

    WiFiConnectResult result = wifiManager.connect(ssid, password);
    return (result == WiFiConnectResult::SUCCESS);
}

void WiFiSetESP32::disconnectWiFi() {
    wifiManager.disconnect();
}

//
// Public API - Status
//

WiFiSetConnectionStatus WiFiSetESP32::getConnectionStatus() {
    return convertConnectionState(wifiManager.getConnectionState());
}

bool WiFiSetESP32::isConnected() {
    return wifiManager.isConnected();
}

IPAddress WiFiSetESP32::getIPAddress() {
    return wifiManager.getIPAddress();
}

int WiFiSetESP32::getRSSI() {
    return static_cast<int>(wifiManager.getRSSI());
}

String WiFiSetESP32::getSSID() {
    return wifiManager.getSSID();
}

//
// Public API - BLE Control
//

void WiFiSetESP32::startBLE() {
    bleService.startAdvertising();
}

void WiFiSetESP32::stopBLE() {
    bleService.stopAdvertising();
}

bool WiFiSetESP32::isBLERunning() {
    return bleService.isRunning();
}
