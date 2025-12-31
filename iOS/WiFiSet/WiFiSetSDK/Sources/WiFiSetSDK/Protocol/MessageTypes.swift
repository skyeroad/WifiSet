import Foundation

/// Protocol message types (as defined in PROTOCOL.md)
public enum MessageType: UInt8 {
    case wifiListStart = 0x01
    case wifiNetworkEntry = 0x02
    case wifiListEnd = 0x03
    case credentialWrite = 0x10
    case credentialWriteAck = 0x11
    case statusRequest = 0x20
    case statusResponse = 0x21
    case error = 0xFF
}

/// Protocol error codes
public enum ProtocolErrorCode: UInt8 {
    case invalidMessageFormat = 0x01
    case scanFailed = 0x02
    case credentialWriteFailed = 0x03
    case storageError = 0x04
    case connectionTimeout = 0x05
    case unknownMessageType = 0x06
}

/// Message header (4 bytes)
public struct MessageHeader {
    public let type: MessageType
    public let sequenceNumber: UInt8
    public let payloadLength: UInt16

    public init(type: MessageType, sequenceNumber: UInt8, payloadLength: UInt16) {
        self.type = type
        self.sequenceNumber = sequenceNumber
        self.payloadLength = payloadLength
    }

    /// Header size in bytes
    public static let size = 4

    /// Encode header to Data
    public func encode() -> Data {
        var data = Data()
        data.append(type.rawValue)
        data.append(sequenceNumber)
        // Little-endian encoding
        data.append(UInt8(payloadLength & 0xFF))
        data.append(UInt8((payloadLength >> 8) & 0xFF))
        return data
    }

    /// Decode header from Data
    public static func decode(from data: Data) throws -> MessageHeader {
        guard data.count >= size else {
            throw ProtocolError.invalidMessageFormat("Header too short")
        }

        guard let type = MessageType(rawValue: data[0]) else {
            throw ProtocolError.unknownMessageType(data[0])
        }

        let sequenceNumber = data[1]
        let payloadLength = UInt16(data[2]) | (UInt16(data[3]) << 8) // Little-endian

        return MessageHeader(
            type: type,
            sequenceNumber: sequenceNumber,
            payloadLength: payloadLength
        )
    }
}

/// Protocol errors
public enum ProtocolError: Error, LocalizedError {
    case invalidMessageFormat(String)
    case unknownMessageType(UInt8)
    case payloadTooLarge
    case decodingFailed(String)
    case encodingFailed(String)
    case insufficientData

    public var errorDescription: String? {
        switch self {
        case .invalidMessageFormat(let detail):
            return "Invalid message format: \(detail)"
        case .unknownMessageType(let type):
            return "Unknown message type: 0x\(String(format: "%02X", type))"
        case .payloadTooLarge:
            return "Payload too large"
        case .decodingFailed(let detail):
            return "Decoding failed: \(detail)"
        case .encodingFailed(let detail):
            return "Encoding failed: \(detail)"
        case .insufficientData:
            return "Insufficient data"
        }
    }
}

/// Protocol message structures
public enum ProtocolMessage {
    case wifiListStart
    case wifiNetworkEntry(WiFiNetwork)
    case wifiListEnd(networkCount: UInt8)
    case credentialWrite(ssid: String, password: String)
    case credentialWriteAck(statusCode: UInt8)
    case statusRequest
    case statusResponse(DeviceStatus)
    case error(code: ProtocolErrorCode, message: String)

    /// Message type
    public var type: MessageType {
        switch self {
        case .wifiListStart: return .wifiListStart
        case .wifiNetworkEntry: return .wifiNetworkEntry
        case .wifiListEnd: return .wifiListEnd
        case .credentialWrite: return .credentialWrite
        case .credentialWriteAck: return .credentialWriteAck
        case .statusRequest: return .statusRequest
        case .statusResponse: return .statusResponse
        case .error: return .error
        }
    }
}

// MARK: - Utility Extensions

extension Data {
    /// Read a UTF-8 string with length prefix
    /// Returns (string, bytesRead) or throws
    func readLengthPrefixedString(at offset: Int, maxLength: Int = 255) throws -> (String, Int) {
        guard offset < count else {
            throw ProtocolError.insufficientData
        }

        let length = Int(self[offset])
        guard length <= maxLength else {
            throw ProtocolError.decodingFailed("String length \(length) exceeds maximum \(maxLength)")
        }

        let endIndex = offset + 1 + length
        guard endIndex <= count else {
            throw ProtocolError.insufficientData
        }

        let stringData = self[offset + 1..<endIndex]
        guard let string = String(data: stringData, encoding: .utf8) else {
            throw ProtocolError.decodingFailed("Invalid UTF-8 string")
        }

        return (string, 1 + length)
    }

    /// Write a UTF-8 string with length prefix
    mutating func appendLengthPrefixedString(_ string: String) throws {
        guard let stringData = string.data(using: .utf8) else {
            throw ProtocolError.encodingFailed("Failed to encode string as UTF-8")
        }

        guard stringData.count <= 255 else {
            throw ProtocolError.encodingFailed("String too long: \(stringData.count) bytes")
        }

        self.append(UInt8(stringData.count))
        self.append(stringData)
    }
}

extension UInt16 {
    /// Little-endian bytes
    var littleEndianBytes: [UInt8] {
        [UInt8(self & 0xFF), UInt8((self >> 8) & 0xFF)]
    }
}
