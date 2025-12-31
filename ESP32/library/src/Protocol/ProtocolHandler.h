#ifndef PROTOCOL_HANDLER_H
#define PROTOCOL_HANDLER_H

#include <Arduino.h>
#include "MessageBuilder.h"

namespace WiFiSet {

/**
 * Parsed credential data from Credential Write message
 */
struct CredentialData {
    String ssid;
    String password;
    bool isValid;

    CredentialData() : isValid(false) {}
    CredentialData(const String& s, const String& p) : ssid(s), password(p), isValid(true) {}
};

/**
 * Message header information
 */
struct MessageHeader {
    MessageType type;
    uint8_t sequence;
    uint16_t payloadLength;
    bool isValid;

    MessageHeader() : type(MessageType::ERROR), sequence(0), payloadLength(0), isValid(false) {}
};

/**
 * ProtocolHandler - Parses binary protocol messages
 *
 * Handles decoding of messages received from iOS client according to
 * the WiFiSet protocol specification.
 */
class ProtocolHandler {
public:
    ProtocolHandler();

    /**
     * Parse message header
     * @param data Raw message data
     * @param length Length of data
     * @return Parsed header information
     */
    MessageHeader parseHeader(const uint8_t* data, size_t length);

    /**
     * Parse Credential Write message
     * Extracts SSID and password from the message
     * @param data Raw message data (including header)
     * @param length Length of data
     * @return Parsed credential data
     */
    CredentialData parseCredentialWrite(const uint8_t* data, size_t length);

    /**
     * Parse Status Request message
     * @param data Raw message data (including header)
     * @param length Length of data
     * @return true if valid status request, false otherwise
     */
    bool parseStatusRequest(const uint8_t* data, size_t length);

    /**
     * Validate message format
     * Checks if the message has valid header and payload length
     * @param data Raw message data
     * @param length Length of data
     * @return true if message format is valid
     */
    bool validateMessage(const uint8_t* data, size_t length);

    /**
     * Get last error message
     */
    const String& getLastError() const { return lastError; }

private:
    String lastError;

    /**
     * Set error message
     */
    void setError(const String& error);

    /**
     * Extract string from message data
     * @param data Pointer to start of string length byte
     * @param maxLength Maximum allowed string length
     * @param outString Output string
     * @param outBytesRead Number of bytes read (including length byte)
     * @return true if successful
     */
    bool extractString(const uint8_t* data, size_t available, uint8_t maxLength,
                      String& outString, size_t& outBytesRead);
};

} // namespace WiFiSet

#endif // PROTOCOL_HANDLER_H
