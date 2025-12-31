#ifndef BLE_SERVICE_H
#define BLE_SERVICE_H

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <functional>
#include "../Protocol/MessageBuilder.h"
#include "../Protocol/ProtocolHandler.h"
#include "../WiFiManager/WiFiManager.h"

namespace WiFiSet {

// BLE UUIDs (as defined in PROTOCOL.md)
#define WIFISET_SERVICE_UUID           "4FAFC201-1FB5-459E-8FCC-C5C9C331914B"
#define WIFI_LIST_CHARACTERISTIC_UUID  "4FAFC202-1FB5-459E-8FCC-C5C9C331914B"
#define CREDENTIAL_WRITE_CHAR_UUID     "4FAFC203-1FB5-459E-8FCC-C5C9C331914B"
#define STATUS_CHARACTERISTIC_UUID     "4FAFC204-1FB5-459E-8FCC-C5C9C331914B"

/**
 * Callbacks for BLE events
 */
class BLEServiceCallbacks {
public:
    virtual ~BLEServiceCallbacks() {}

    /**
     * Called when credentials are received from iOS client
     * @param ssid WiFi network name
     * @param password WiFi password
     */
    virtual void onCredentialsReceived(const String& ssid, const String& password) {}

    /**
     * Called when a client connects
     */
    virtual void onClientConnected() {}

    /**
     * Called when a client disconnects
     */
    virtual void onClientDisconnected() {}

    /**
     * Called when a status request is received
     */
    virtual void onStatusRequest() {}
};

/**
 * WiFiSetBLEService - Manages BLE GATT service and characteristics
 *
 * Handles all BLE communication including:
 * - BLE server initialization and advertising
 * - WiFi network list transmission
 * - Credential reception
 * - Status updates
 */
class WiFiSetBLEService {
public:
    WiFiSetBLEService();
    ~WiFiSetBLEService();

    /**
     * Initialize BLE service
     * @param deviceName Device name for BLE advertising
     * @return true if initialization successful
     */
    bool begin(const char* deviceName);

    /**
     * Start BLE advertising
     */
    void startAdvertising();

    /**
     * Stop BLE advertising
     */
    void stopAdvertising();

    /**
     * Check if BLE is running
     */
    bool isRunning() const { return bleInitialized; }

    /**
     * Check if a client is connected
     */
    bool isClientConnected() const { return clientConnected; }

    /**
     * Send WiFi network list to connected client
     * Automatically sends List Start, Network Entries, and List End
     * @param networks Vector of WiFi networks to send
     */
    void sendWiFiNetworkList(const std::vector<WiFiNetworkInfo>& networks);

    /**
     * Send credential write acknowledgment
     * @param statusCode 0x00=Success, 0x01=Invalid SSID, 0x02=Invalid Password, 0x03=Storage failure
     */
    void sendCredentialAck(uint8_t statusCode);

    /**
     * Send status response
     * @param state Current connection state
     * @param rssi WiFi RSSI (0 if not connected)
     * @param ipAddress IP address (0.0.0.0 if not connected)
     * @param ssid Currently configured SSID
     */
    void sendStatusResponse(ConnectionState state, int8_t rssi, IPAddress ipAddress, const String& ssid);

    /**
     * Send error message
     * @param errorCode Error code
     * @param errorMessage Human-readable error description
     */
    void sendError(ErrorCode errorCode, const String& errorMessage);

    /**
     * Set callbacks for BLE events
     */
    void setCallbacks(BLEServiceCallbacks* callbacks);

    /**
     * Main loop processing
     * Must be called regularly from main loop
     */
    void loop();

private:
    BLEServer* pServer;
    BLEService* pService;
    BLECharacteristic* pWiFiListCharacteristic;
    BLECharacteristic* pCredentialCharacteristic;
    BLECharacteristic* pStatusCharacteristic;

    MessageBuilder messageBuilder;
    ProtocolHandler protocolHandler;
    BLEServiceCallbacks* callbacks;

    bool bleInitialized;
    bool clientConnected;
    String deviceName;

    /**
     * Send data via a characteristic using NOTIFY
     */
    void sendNotification(BLECharacteristic* characteristic, const std::vector<uint8_t>& data);

    /**
     * Send data via a characteristic (split into MTU-sized chunks if needed)
     */
    void sendData(BLECharacteristic* characteristic, const std::vector<uint8_t>& data);

    // Friend classes for callback access
    friend class ServerCallbacks;
    friend class CredentialCharacteristicCallbacks;
};

/**
 * BLE Server callbacks
 */
class ServerCallbacks : public BLEServerCallbacks {
public:
    ServerCallbacks(WiFiSetBLEService* service) : bleService(service) {}

    void onConnect(BLEServer* pServer) override;
    void onDisconnect(BLEServer* pServer) override;

private:
    WiFiSetBLEService* bleService;
};

/**
 * Credential Write characteristic callbacks
 */
class CredentialCharacteristicCallbacks : public BLECharacteristicCallbacks {
public:
    CredentialCharacteristicCallbacks(WiFiSetBLEService* service) : bleService(service) {}

    void onWrite(BLECharacteristic* pCharacteristic) override;

private:
    WiFiSetBLEService* bleService;
};

} // namespace WiFiSet

#endif // BLE_SERVICE_H
