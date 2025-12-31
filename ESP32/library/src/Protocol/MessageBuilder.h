#ifndef MESSAGE_BUILDER_H
#define MESSAGE_BUILDER_H

#include <Arduino.h>
#include <vector>

namespace WiFiSet {

// Message Types (as defined in PROTOCOL.md)
enum class MessageType : uint8_t {
    WIFI_LIST_START = 0x01,
    WIFI_NETWORK_ENTRY = 0x02,
    WIFI_LIST_END = 0x03,
    CREDENTIAL_WRITE = 0x10,
    CREDENTIAL_WRITE_ACK = 0x11,
    STATUS_REQUEST = 0x20,
    STATUS_RESPONSE = 0x21,
    ERROR = 0xFF
};

// WiFi Security Types
enum class SecurityType : uint8_t {
    OPEN = 0x00,
    WEP = 0x01,
    WPA_PSK = 0x02,
    WPA2_ENTERPRISE = 0x03,
    WPA3 = 0x04
};

// Connection States
enum class ConnectionState : uint8_t {
    NOT_CONFIGURED = 0x00,
    CONFIGURED_NOT_CONNECTED = 0x01,
    CONNECTING = 0x02,
    CONNECTED = 0x03,
    CONNECTION_FAILED = 0x04
};

// Error Codes
enum class ErrorCode : uint8_t {
    INVALID_MESSAGE_FORMAT = 0x01,
    SCAN_FAILED = 0x02,
    CREDENTIAL_WRITE_FAILED = 0x03,
    STORAGE_ERROR = 0x04,
    CONNECTION_TIMEOUT = 0x05,
    UNKNOWN_MESSAGE_TYPE = 0x06
};

// WiFi Network Information
struct WiFiNetworkInfo {
    String ssid;
    int8_t rssi;
    SecurityType securityType;
    uint8_t channel;
};

/**
 * MessageBuilder - Builds binary protocol messages
 *
 * Handles encoding of messages according to the WiFiSet protocol specification.
 * All messages include a 4-byte header: [Type, Sequence, Length_Low, Length_High]
 */
class MessageBuilder {
public:
    MessageBuilder();

    /**
     * Build WiFi List Start message
     * Indicates beginning of WiFi network list
     */
    std::vector<uint8_t> buildWiFiListStart();

    /**
     * Build WiFi Network Entry message
     * Contains information about a single WiFi network
     */
    std::vector<uint8_t> buildWiFiNetworkEntry(const WiFiNetworkInfo& network);

    /**
     * Build WiFi List End message
     * Indicates end of WiFi network list
     * @param networkCount Total number of networks in the list
     */
    std::vector<uint8_t> buildWiFiListEnd(uint8_t networkCount);

    /**
     * Build Credential Write Acknowledgment message
     * Confirms receipt and storage of WiFi credentials
     * @param statusCode 0x00=Success, 0x01=Invalid SSID, 0x02=Invalid Password, 0x03=Storage failure
     */
    std::vector<uint8_t> buildCredentialWriteAck(uint8_t statusCode);

    /**
     * Build Status Response message
     * Contains current WiFi connection status
     */
    std::vector<uint8_t> buildStatusResponse(
        ConnectionState state,
        int8_t rssi,
        IPAddress ipAddress,
        const String& ssid
    );

    /**
     * Build Error message
     * Reports an error condition to the client
     */
    std::vector<uint8_t> buildError(ErrorCode errorCode, const String& errorMessage);

    /**
     * Reset sequence counter
     */
    void resetSequence();

    /**
     * Get current sequence number
     */
    uint8_t getSequence() const { return sequenceCounter; }

private:
    uint8_t sequenceCounter;

    /**
     * Build message header
     * Returns 4-byte header: [Type, Sequence, Length_Low, Length_High]
     */
    std::vector<uint8_t> buildHeader(MessageType type, uint16_t payloadLength);

    /**
     * Increment sequence counter (wraps at 255)
     */
    void incrementSequence();
};

} // namespace WiFiSet

#endif // MESSAGE_BUILDER_H
