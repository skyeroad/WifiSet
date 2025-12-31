import Foundation

/// WiFi security type
public enum WiFiSecurityType: UInt8, Codable, CaseIterable {
    case open = 0x00
    case wep = 0x01
    case wpaPsk = 0x02
    case wpa2Enterprise = 0x03
    case wpa3 = 0x04

    /// Human-readable security type name
    public var displayName: String {
        switch self {
        case .open:
            return "Open"
        case .wep:
            return "WEP"
        case .wpaPsk:
            return "WPA/WPA2"
        case .wpa2Enterprise:
            return "WPA2 Enterprise"
        case .wpa3:
            return "WPA3"
        }
    }

    /// Whether this security type requires a password
    public var requiresPassword: Bool {
        self != .open
    }

    /// Icon name for security type
    public var iconName: String {
        requiresPassword ? "lock.fill" : "lock.open.fill"
    }
}

/// WiFi network information
public struct WiFiNetwork: Identifiable, Codable, Equatable {
    public let id = UUID()
    public let ssid: String
    public let rssi: Int8
    public let securityType: WiFiSecurityType
    public let channel: UInt8

    public init(ssid: String, rssi: Int8, securityType: WiFiSecurityType, channel: UInt8) {
        self.ssid = ssid
        self.rssi = rssi
        self.securityType = securityType
        self.channel = channel
    }

    /// Signal strength as 0-4 bars
    public var signalStrength: Int {
        if rssi >= -50 { return 4 }
        if rssi >= -60 { return 3 }
        if rssi >= -70 { return 2 }
        if rssi >= -80 { return 1 }
        return 0
    }

    /// Signal strength as percentage (0-100)
    public var signalPercentage: Int {
        let percentage = 2 * (Int(rssi) + 100)
        return max(0, min(100, percentage))
    }

    /// Signal quality description
    public var signalQuality: String {
        switch signalStrength {
        case 4: return "Excellent"
        case 3: return "Good"
        case 2: return "Fair"
        case 1: return "Weak"
        default: return "Very Weak"
        }
    }

    /// Icon name for signal strength
    public var signalIcon: String {
        switch signalStrength {
        case 4: return "wifi"
        case 3: return "wifi"
        case 2: return "wifi"
        case 1: return "wifi"
        default: return "wifi.slash"
        }
    }
}

/// ESP32 WiFi connection state
public enum ConnectionState: UInt8, Codable {
    case notConfigured = 0x00
    case configuredNotConnected = 0x01
    case connecting = 0x02
    case connected = 0x03
    case connectionFailed = 0x04

    /// Human-readable state description
    public var description: String {
        switch self {
        case .notConfigured:
            return "Not Configured"
        case .configuredNotConnected:
            return "Configured (Not Connected)"
        case .connecting:
            return "Connecting..."
        case .connected:
            return "Connected"
        case .connectionFailed:
            return "Connection Failed"
        }
    }

    /// Icon name for connection state
    public var iconName: String {
        switch self {
        case .notConfigured:
            return "wifi.exclamationmark"
        case .configuredNotConnected:
            return "wifi.slash"
        case .connecting:
            return "wifi"
        case .connected:
            return "wifi"
        case .connectionFailed:
            return "wifi.exclamationmark"
        }
    }

    /// Whether this is a successful state
    public var isSuccessful: Bool {
        self == .connected
    }

    /// Whether this is a failed state
    public var isFailed: Bool {
        self == .connectionFailed
    }
}

/// ESP32 device status
public struct DeviceStatus: Codable {
    public let connectionState: ConnectionState
    public let rssi: Int8
    public let ipAddress: String
    public let ssid: String

    public init(connectionState: ConnectionState, rssi: Int8, ipAddress: String, ssid: String) {
        self.connectionState = connectionState
        self.rssi = rssi
        self.ipAddress = ipAddress
        self.ssid = ssid
    }

    /// Whether device is connected to WiFi
    public var isConnected: Bool {
        connectionState == .connected
    }
}

// MARK: - Coding Keys

extension WiFiNetwork {
    enum CodingKeys: String, CodingKey {
        case ssid, rssi, securityType, channel
    }
}
