import Foundation

/// Decodes binary protocol messages from ESP32
public class ProtocolDecoder {
    public init() {}

    /// Decode any message from Data
    public func decode(_ data: Data) throws -> ProtocolMessage {
        // Parse header
        let header = try MessageHeader.decode(from: data)

        // Validate total length
        guard data.count >= MessageHeader.size + Int(header.payloadLength) else {
            throw ProtocolError.insufficientData
        }

        let payload = data.subdata(in: MessageHeader.size..<data.count)

        // Decode based on message type
        switch header.type {
        case .wifiListStart:
            return try decodeWiFiListStart(payload: payload)
        case .wifiNetworkEntry:
            return try decodeWiFiNetworkEntry(payload: payload)
        case .wifiListEnd:
            return try decodeWiFiListEnd(payload: payload)
        case .credentialWriteAck:
            return try decodeCredentialWriteAck(payload: payload)
        case .statusResponse:
            return try decodeStatusResponse(payload: payload)
        case .error:
            return try decodeError(payload: payload)
        default:
            throw ProtocolError.unknownMessageType(header.type.rawValue)
        }
    }

    // MARK: - Message Decoders

    private func decodeWiFiListStart(payload: Data) throws -> ProtocolMessage {
        // WiFi List Start has no payload
        return .wifiListStart
    }

    private func decodeWiFiNetworkEntry(payload: Data) throws -> ProtocolMessage {
        var offset = 0

        // Read SSID (length-prefixed string)
        let (ssid, ssidBytesRead) = try payload.readLengthPrefixedString(at: offset, maxLength: 32)
        offset += ssidBytesRead

        // Read RSSI (1 byte, signed)
        guard offset < payload.count else {
            throw ProtocolError.insufficientData
        }
        let rssi = Int8(bitPattern: payload[offset])
        offset += 1

        // Read Security Type (1 byte)
        guard offset < payload.count else {
            throw ProtocolError.insufficientData
        }
        guard let securityType = WiFiSecurityType(rawValue: payload[offset]) else {
            throw ProtocolError.decodingFailed("Invalid security type: \(payload[offset])")
        }
        offset += 1

        // Read Channel (1 byte)
        guard offset < payload.count else {
            throw ProtocolError.insufficientData
        }
        let channel = payload[offset]

        let network = WiFiNetwork(
            ssid: ssid,
            rssi: rssi,
            securityType: securityType,
            channel: channel
        )

        return .wifiNetworkEntry(network)
    }

    private func decodeWiFiListEnd(payload: Data) throws -> ProtocolMessage {
        guard payload.count >= 1 else {
            throw ProtocolError.insufficientData
        }

        let networkCount = payload[0]
        return .wifiListEnd(networkCount: networkCount)
    }

    private func decodeCredentialWriteAck(payload: Data) throws -> ProtocolMessage {
        guard payload.count >= 1 else {
            throw ProtocolError.insufficientData
        }

        let statusCode = payload[0]
        return .credentialWriteAck(statusCode: statusCode)
    }

    private func decodeStatusResponse(payload: Data) throws -> ProtocolMessage {
        var offset = 0

        // Read Connection State (1 byte)
        guard offset < payload.count else {
            throw ProtocolError.insufficientData
        }
        guard let connectionState = ConnectionState(rawValue: payload[offset]) else {
            throw ProtocolError.decodingFailed("Invalid connection state: \(payload[offset])")
        }
        offset += 1

        // Read RSSI (1 byte, signed)
        guard offset < payload.count else {
            throw ProtocolError.insufficientData
        }
        let rssi = Int8(bitPattern: payload[offset])
        offset += 1

        // Read IP Address (4 bytes, network byte order)
        guard offset + 4 <= payload.count else {
            throw ProtocolError.insufficientData
        }
        let ipBytes = Array(payload[offset..<offset+4])
        let ipAddress = "\(ipBytes[0]).\(ipBytes[1]).\(ipBytes[2]).\(ipBytes[3])"
        offset += 4

        // Read SSID (length-prefixed string)
        let (ssid, _) = try payload.readLengthPrefixedString(at: offset, maxLength: 32)

        let status = DeviceStatus(
            connectionState: connectionState,
            rssi: rssi,
            ipAddress: ipAddress,
            ssid: ssid
        )

        return .statusResponse(status)
    }

    private func decodeError(payload: Data) throws -> ProtocolMessage {
        var offset = 0

        // Read Error Code (1 byte)
        guard offset < payload.count else {
            throw ProtocolError.insufficientData
        }
        guard let errorCode = ProtocolErrorCode(rawValue: payload[offset]) else {
            throw ProtocolError.decodingFailed("Invalid error code: \(payload[offset])")
        }
        offset += 1

        // Read Error Message (length-prefixed string)
        let (errorMessage, _) = try payload.readLengthPrefixedString(at: offset, maxLength: 255)

        return .error(code: errorCode, message: errorMessage)
    }
}
