#include "MessageBuilder.h"

namespace WiFiSet {

MessageBuilder::MessageBuilder() : sequenceCounter(0) {}

std::vector<uint8_t> MessageBuilder::buildHeader(MessageType type, uint16_t payloadLength) {
    std::vector<uint8_t> header(4);
    header[0] = static_cast<uint8_t>(type);
    header[1] = sequenceCounter;
    header[2] = payloadLength & 0xFF;        // Low byte (little-endian)
    header[3] = (payloadLength >> 8) & 0xFF; // High byte
    return header;
}

void MessageBuilder::incrementSequence() {
    sequenceCounter++;
    // Wraps automatically at 255 (uint8_t overflow)
}

std::vector<uint8_t> MessageBuilder::buildWiFiListStart() {
    std::vector<uint8_t> message = buildHeader(MessageType::WIFI_LIST_START, 0);
    incrementSequence();
    return message;
}

std::vector<uint8_t> MessageBuilder::buildWiFiNetworkEntry(const WiFiNetworkInfo& network) {
    // Calculate payload size
    uint8_t ssidLength = network.ssid.length();
    if (ssidLength > 32) {
        ssidLength = 32; // WiFi SSID maximum length
    }

    uint16_t payloadLength = 1 + ssidLength + 1 + 1 + 1; // SSID_len + SSID + RSSI + Security + Channel

    // Build message
    std::vector<uint8_t> message = buildHeader(MessageType::WIFI_NETWORK_ENTRY, payloadLength);

    // Add payload
    message.push_back(ssidLength);

    // Add SSID bytes
    for (uint8_t i = 0; i < ssidLength; i++) {
        message.push_back(network.ssid[i]);
    }

    message.push_back(static_cast<uint8_t>(network.rssi));
    message.push_back(static_cast<uint8_t>(network.securityType));
    message.push_back(network.channel);

    incrementSequence();
    return message;
}

std::vector<uint8_t> MessageBuilder::buildWiFiListEnd(uint8_t networkCount) {
    std::vector<uint8_t> message = buildHeader(MessageType::WIFI_LIST_END, 1);
    message.push_back(networkCount);
    incrementSequence();
    return message;
}

std::vector<uint8_t> MessageBuilder::buildCredentialWriteAck(uint8_t statusCode) {
    std::vector<uint8_t> message = buildHeader(MessageType::CREDENTIAL_WRITE_ACK, 1);
    message.push_back(statusCode);
    incrementSequence();
    return message;
}

std::vector<uint8_t> MessageBuilder::buildStatusResponse(
    ConnectionState state,
    int8_t rssi,
    IPAddress ipAddress,
    const String& ssid
) {
    uint8_t ssidLength = ssid.length();
    if (ssidLength > 32) {
        ssidLength = 32;
    }

    // Payload: State(1) + RSSI(1) + IP(4) + SSID_len(1) + SSID(N)
    uint16_t payloadLength = 1 + 1 + 4 + 1 + ssidLength;

    std::vector<uint8_t> message = buildHeader(MessageType::STATUS_RESPONSE, payloadLength);

    // Add payload
    message.push_back(static_cast<uint8_t>(state));
    message.push_back(static_cast<uint8_t>(rssi));

    // Add IP address (4 bytes, network byte order - big-endian for IP)
    message.push_back(ipAddress[0]);
    message.push_back(ipAddress[1]);
    message.push_back(ipAddress[2]);
    message.push_back(ipAddress[3]);

    message.push_back(ssidLength);

    // Add SSID bytes
    for (uint8_t i = 0; i < ssidLength; i++) {
        message.push_back(ssid[i]);
    }

    incrementSequence();
    return message;
}

std::vector<uint8_t> MessageBuilder::buildError(ErrorCode errorCode, const String& errorMessage) {
    uint8_t messageLength = errorMessage.length();
    if (messageLength > 255) {
        messageLength = 255;
    }

    // Payload: ErrorCode(1) + MsgLength(1) + Message(N)
    uint16_t payloadLength = 1 + 1 + messageLength;

    std::vector<uint8_t> message = buildHeader(MessageType::ERROR, payloadLength);

    // Add payload
    message.push_back(static_cast<uint8_t>(errorCode));
    message.push_back(messageLength);

    // Add error message bytes
    for (uint8_t i = 0; i < messageLength; i++) {
        message.push_back(errorMessage[i]);
    }

    incrementSequence();
    return message;
}

void MessageBuilder::resetSequence() {
    sequenceCounter = 0;
}

} // namespace WiFiSet
