import CoreBluetooth
import Foundation

/// Represents a discovered ESP32 device
public class BLEPeripheral: Identifiable, ObservableObject {
    public let id: UUID
    public let name: String
    public let peripheral: CBPeripheral
    @Published public var rssi: Int

    public init(peripheral: CBPeripheral, advertisementData: [String: Any], rssi: NSNumber) {
        self.id = peripheral.identifier
        self.peripheral = peripheral
        self.name = peripheral.name ?? "Unknown Device"
        self.rssi = rssi.intValue
    }

    /// Signal strength as 0-4 bars
    public var signalStrength: Int {
        let rssiValue = rssi
        if rssiValue >= -50 { return 4 }
        if rssiValue >= -60 { return 3 }
        if rssiValue >= -70 { return 2 }
        if rssiValue >= -80 { return 1 }
        return 0
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

    /// Update RSSI value
    public func updateRSSI(_ newRSSI: NSNumber) {
        self.rssi = newRSSI.intValue
    }
}

// MARK: - Equatable

extension BLEPeripheral: Equatable {
    public static func == (lhs: BLEPeripheral, rhs: BLEPeripheral) -> Bool {
        lhs.id == rhs.id
    }
}

// MARK: - Hashable

extension BLEPeripheral: Hashable {
    public func hash(into hasher: inout Hasher) {
        hasher.combine(id)
    }
}
