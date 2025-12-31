#include "BLEService.h"

namespace WiFiSet {

//
// ServerCallbacks Implementation
//

void ServerCallbacks::onConnect(BLEServer* pServer) {
    bleService->clientConnected = true;
    if (bleService->callbacks) {
        bleService->callbacks->onClientConnected();
    }
}

void ServerCallbacks::onDisconnect(BLEServer* pServer) {
    bleService->clientConnected = false;
    if (bleService->callbacks) {
        bleService->callbacks->onClientDisconnected();
    }

    // Restart advertising when client disconnects
    delay(500);
    bleService->startAdvertising();
}

//
// CredentialCharacteristicCallbacks Implementation
//

void CredentialCharacteristicCallbacks::onWrite(BLECharacteristic* pCharacteristic) {
    std::string value = pCharacteristic->getValue();

    if (value.length() > 0) {
        // Parse credential write message
        const uint8_t* data = reinterpret_cast<const uint8_t*>(value.data());
        CredentialData credentials = bleService->protocolHandler.parseCredentialWrite(data, value.length());

        if (credentials.isValid) {
            // Notify callback
            if (bleService->callbacks) {
                bleService->callbacks->onCredentialsReceived(credentials.ssid, credentials.password);
            }

            // Send acknowledgment (success)
            bleService->sendCredentialAck(0x00);
        } else {
            // Send acknowledgment (failure)
            // Determine failure reason
            uint8_t statusCode = 0x01; // Default to invalid SSID
            String error = bleService->protocolHandler.getLastError();
            if (error.indexOf("password") >= 0) {
                statusCode = 0x02; // Invalid password
            } else if (error.indexOf("storage") >= 0 || error.indexOf("Storage") >= 0) {
                statusCode = 0x03; // Storage failure
            }

            bleService->sendCredentialAck(statusCode);
            bleService->sendError(ErrorCode::CREDENTIAL_WRITE_FAILED, error);
        }
    }
}

//
// WiFiSetBLEService Implementation
//

WiFiSetBLEService::WiFiSetBLEService()
    : pServer(nullptr),
      pService(nullptr),
      pWiFiListCharacteristic(nullptr),
      pCredentialCharacteristic(nullptr),
      pStatusCharacteristic(nullptr),
      callbacks(nullptr),
      bleInitialized(false),
      clientConnected(false) {}

WiFiSetBLEService::~WiFiSetBLEService() {
    if (bleInitialized) {
        stopAdvertising();
    }
}

bool WiFiSetBLEService::begin(const char* deviceName) {
    if (bleInitialized) {
        return true;
    }

    this->deviceName = String(deviceName);

    // Initialize BLE Device
    BLEDevice::init(deviceName);

    // Create BLE Server
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks(this));

    // Create BLE Service
    pService = pServer->createService(WIFISET_SERVICE_UUID);

    // Create WiFi List Characteristic (READ, NOTIFY)
    pWiFiListCharacteristic = pService->createCharacteristic(
        WIFI_LIST_CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
    );
    pWiFiListCharacteristic->addDescriptor(new BLE2902());

    // Create Credential Write Characteristic (WRITE)
    pCredentialCharacteristic = pService->createCharacteristic(
        CREDENTIAL_WRITE_CHAR_UUID,
        BLECharacteristic::PROPERTY_WRITE
    );
    pCredentialCharacteristic->setCallbacks(new CredentialCharacteristicCallbacks(this));

    // Create Status Characteristic (READ, NOTIFY)
    pStatusCharacteristic = pService->createCharacteristic(
        STATUS_CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
    );
    pStatusCharacteristic->addDescriptor(new BLE2902());

    // Start the service
    pService->start();

    bleInitialized = true;
    return true;
}

void WiFiSetBLEService::startAdvertising() {
    if (!bleInitialized) {
        return;
    }

    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(WIFISET_SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);  // Help with iPhone connections
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();
}

void WiFiSetBLEService::stopAdvertising() {
    if (!bleInitialized) {
        return;
    }

    BLEDevice::getAdvertising()->stop();
}

void WiFiSetBLEService::sendData(BLECharacteristic* characteristic, const std::vector<uint8_t>& data) {
    if (!bleInitialized || !clientConnected || !characteristic) {
        return;
    }

    characteristic->setValue(const_cast<uint8_t*>(data.data()), data.size());
    characteristic->notify();
}

void WiFiSetBLEService::sendNotification(BLECharacteristic* characteristic, const std::vector<uint8_t>& data) {
    sendData(characteristic, data);
}

void WiFiSetBLEService::sendWiFiNetworkList(const std::vector<WiFiNetworkInfo>& networks) {
    if (!clientConnected || !pWiFiListCharacteristic) {
        return;
    }

    // Send List Start
    std::vector<uint8_t> startMsg = messageBuilder.buildWiFiListStart();
    sendNotification(pWiFiListCharacteristic, startMsg);

    delay(100); // Longer delay to prevent BLE stack overflow
    yield();    // Let watchdog timer reset

    // Send each network entry
    for (size_t i = 0; i < networks.size(); i++) {
        if (!clientConnected) {
            return; // Client disconnected during transmission
        }

        std::vector<uint8_t> entryMsg = messageBuilder.buildWiFiNetworkEntry(networks[i]);
        sendNotification(pWiFiListCharacteristic, entryMsg);
        delay(100); // Longer delay between notifications
        yield();    // Let watchdog timer reset
    }

    // Send List End
    uint8_t networkCount = networks.size() > 255 ? 255 : static_cast<uint8_t>(networks.size());
    std::vector<uint8_t> endMsg = messageBuilder.buildWiFiListEnd(networkCount);
    sendNotification(pWiFiListCharacteristic, endMsg);

    delay(100); // Final delay after list end
    yield();
}

void WiFiSetBLEService::sendCredentialAck(uint8_t statusCode) {
    if (!clientConnected) {
        return;
    }

    std::vector<uint8_t> ackMsg = messageBuilder.buildCredentialWriteAck(statusCode);
    sendNotification(pCredentialCharacteristic, ackMsg);
}

void WiFiSetBLEService::sendStatusResponse(ConnectionState state, int8_t rssi, IPAddress ipAddress, const String& ssid) {
    if (!clientConnected) {
        return;
    }

    std::vector<uint8_t> statusMsg = messageBuilder.buildStatusResponse(state, rssi, ipAddress, ssid);
    sendNotification(pStatusCharacteristic, statusMsg);
}

void WiFiSetBLEService::sendError(ErrorCode errorCode, const String& errorMessage) {
    if (!clientConnected) {
        return;
    }

    std::vector<uint8_t> errorMsg = messageBuilder.buildError(errorCode, errorMessage);
    sendNotification(pStatusCharacteristic, errorMsg); // Send errors via status characteristic
}

void WiFiSetBLEService::setCallbacks(BLEServiceCallbacks* callbacks) {
    this->callbacks = callbacks;
}

void WiFiSetBLEService::loop() {
    // Nothing to do in loop for now
    // BLE events are handled via callbacks
}

} // namespace WiFiSet
