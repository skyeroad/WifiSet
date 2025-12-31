import Foundation

/// Encodes protocol messages to binary format
public class ProtocolEncoder {
    private var sequenceCounter: UInt8 = 0

    public init() {}

    /// Reset sequence counter
    public func resetSequence() {
        sequenceCounter = 0
    }

    /// Get current sequence number
    public func getSequence() -> UInt8 {
        return sequenceCounter
    }

    // MARK: - Message Encoding

    /// Encode credential write message
    public func encodeCredentialWrite(ssid: String, password: String) throws -> Data {
        var payload = Data()

        // Validate lengths
        guard let ssidData = ssid.data(using: .utf8), ssidData.count <= 32 else {
            throw ProtocolError.encodingFailed("SSID too long or invalid")
        }

        guard let passwordData = password.data(using: .utf8), passwordData.count <= 63 else {
            throw ProtocolError.encodingFailed("Password too long or invalid")
        }

        // Build payload: SSID_len + SSID + Password_len + Password
        try payload.appendLengthPrefixedString(ssid)
        try payload.appendLengthPrefixedString(password)

        // Build message
        let header = MessageHeader(
            type: .credentialWrite,
            sequenceNumber: sequenceCounter,
            payloadLength: UInt16(payload.count)
        )

        var message = header.encode()
        message.append(payload)

        incrementSequence()
        return message
    }

    /// Encode status request message
    public func encodeStatusRequest() -> Data {
        let header = MessageHeader(
            type: .statusRequest,
            sequenceNumber: sequenceCounter,
            payloadLength: 0
        )

        incrementSequence()
        return header.encode()
    }

    // MARK: - Private Helpers

    private func incrementSequence() {
        sequenceCounter = sequenceCounter &+ 1  // Wraps at 255
    }
}
