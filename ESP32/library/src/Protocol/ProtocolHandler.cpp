#include "ProtocolHandler.h"

namespace WiFiSet {

ProtocolHandler::ProtocolHandler() : lastError("") {}

void ProtocolHandler::setError(const String& error) {
    lastError = error;
}

MessageHeader ProtocolHandler::parseHeader(const uint8_t* data, size_t length) {
    MessageHeader header;

    if (length < 4) {
        setError("Message too short for header");
        return header;
    }

    header.type = static_cast<MessageType>(data[0]);
    header.sequence = data[1];
    header.payloadLength = data[2] | (static_cast<uint16_t>(data[3]) << 8); // Little-endian
    header.isValid = true;

    return header;
}

bool ProtocolHandler::validateMessage(const uint8_t* data, size_t length) {
    if (length < 4) {
        setError("Message too short");
        return false;
    }

    MessageHeader header = parseHeader(data, length);
    if (!header.isValid) {
        return false;
    }

    // Check if total length matches header + payload
    size_t expectedLength = 4 + header.payloadLength;
    if (length != expectedLength) {
        setError("Message length mismatch");
        return false;
    }

    return true;
}

bool ProtocolHandler::extractString(const uint8_t* data, size_t available,
                                    uint8_t maxLength, String& outString,
                                    size_t& outBytesRead) {
    if (available < 1) {
        setError("Not enough data for string length");
        return false;
    }

    uint8_t stringLength = data[0];

    if (stringLength > maxLength) {
        setError("String length exceeds maximum");
        return false;
    }

    if (available < 1 + stringLength) {
        setError("Not enough data for string content");
        return false;
    }

    // Extract string
    outString = "";
    outString.reserve(stringLength);
    for (uint8_t i = 0; i < stringLength; i++) {
        outString += (char)data[1 + i];
    }

    outBytesRead = 1 + stringLength;
    return true;
}

CredentialData ProtocolHandler::parseCredentialWrite(const uint8_t* data, size_t length) {
    CredentialData credentials;

    if (!validateMessage(data, length)) {
        return credentials;
    }

    MessageHeader header = parseHeader(data, length);
    if (header.type != MessageType::CREDENTIAL_WRITE) {
        setError("Not a Credential Write message");
        return credentials;
    }

    const uint8_t* payload = data + 4;
    size_t payloadRemaining = header.payloadLength;

    // Extract SSID
    String ssid;
    size_t bytesRead = 0;
    if (!extractString(payload, payloadRemaining, 32, ssid, bytesRead)) {
        return credentials;
    }

    payload += bytesRead;
    payloadRemaining -= bytesRead;

    // Validate SSID
    if (ssid.length() == 0) {
        setError("SSID cannot be empty");
        return credentials;
    }

    // Extract Password
    String password;
    if (!extractString(payload, payloadRemaining, 63, password, bytesRead)) {
        return credentials;
    }

    // Password can be empty for open networks

    credentials.ssid = ssid;
    credentials.password = password;
    credentials.isValid = true;

    return credentials;
}

bool ProtocolHandler::parseStatusRequest(const uint8_t* data, size_t length) {
    if (!validateMessage(data, length)) {
        return false;
    }

    MessageHeader header = parseHeader(data, length);
    if (header.type != MessageType::STATUS_REQUEST) {
        setError("Not a Status Request message");
        return false;
    }

    if (header.payloadLength != 0) {
        setError("Status Request should have no payload");
        return false;
    }

    return true;
}

} // namespace WiFiSet
