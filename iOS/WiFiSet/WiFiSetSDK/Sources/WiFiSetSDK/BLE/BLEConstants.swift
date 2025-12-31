import CoreBluetooth
import Foundation

/// BLE UUIDs and constants for WiFiSet protocol
public enum BLEConstants {
    /// WiFiSet service UUID
    public static let serviceUUID = CBUUID(string: "4FAFC201-1FB5-459E-8FCC-C5C9C331914B")

    /// WiFi List characteristic UUID (READ, NOTIFY)
    public static let wifiListCharacteristicUUID = CBUUID(string: "4FAFC202-1FB5-459E-8FCC-C5C9C331914B")

    /// Credential Write characteristic UUID (WRITE)
    public static let credentialWriteCharacteristicUUID = CBUUID(string: "4FAFC203-1FB5-459E-8FCC-C5C9C331914B")

    /// Status characteristic UUID (READ, NOTIFY)
    public static let statusCharacteristicUUID = CBUUID(string: "4FAFC204-1FB5-459E-8FCC-C5C9C331914B")

    /// Preferred MTU size for BLE communication
    public static let preferredMTU = 512

    /// Scan timeout (seconds)
    public static let scanTimeout: TimeInterval = 30

    /// Connection timeout (seconds)
    public static let connectionTimeout: TimeInterval = 10

    /// WiFi list receive timeout (seconds)
    public static let wifiListTimeout: TimeInterval = 30
}
