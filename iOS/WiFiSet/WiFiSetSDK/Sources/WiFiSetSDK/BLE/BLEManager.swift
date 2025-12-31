import CoreBluetooth
import Combine
import Foundation

/// Manages BLE communication with ESP32 devices
public class BLEManager: NSObject, ObservableObject {
    // MARK: - Published Properties

    @Published public private(set) var discoveredDevices: [BLEPeripheral] = []
    @Published public private(set) var isScanning = false
    @Published public private(set) var connectedDevice: BLEPeripheral?
    @Published public private(set) var bluetoothState: CBManagerState = .unknown

    // MARK: - Callbacks

    public var onWiFiNetworksReceived: (([WiFiNetwork]) -> Void)?
    public var onStatusReceived: ((DeviceStatus) -> Void)?
    public var onError: ((Error) -> Void)?
    public var onConnectionStateChanged: ((Bool) -> Void)?

    // MARK: - Private Properties

    private var centralManager: CBCentralManager!
    private var peripherals: [UUID: CBPeripheral] = [:]

    private var wifiListCharacteristic: CBCharacteristic?
    private var credentialCharacteristic: CBCharacteristic?
    private var statusCharacteristic: CBCharacteristic?

    private let encoder = ProtocolEncoder()
    private let decoder = ProtocolDecoder()

    private var receivedNetworks: [WiFiNetwork] = []
    private var isReceivingNetworkList = false

    // MARK: - Initialization

    public override init() {
        super.init()
        centralManager = CBCentralManager(delegate: self, queue: nil)
    }

    // MARK: - Public Methods

    /// Start scanning for ESP32 devices
    public func startScanning() {
        guard centralManager.state == .poweredOn else {
            onError?(BLEError.bluetoothUnavailable)
            return
        }

        discoveredDevices.removeAll()
        peripherals.removeAll()

        centralManager.scanForPeripherals(
            withServices: [BLEConstants.serviceUUID],
            options: [CBCentralManagerScanOptionAllowDuplicatesKey: false]
        )

        isScanning = true
    }

    /// Stop scanning for devices
    public func stopScanning() {
        centralManager.stopScan()
        isScanning = false
    }

    /// Connect to a discovered device
    public func connect(to device: BLEPeripheral) {
        guard let peripheral = peripherals[device.id] else {
            onError?(BLEError.deviceNotFound)
            return
        }

        stopScanning()
        centralManager.connect(peripheral, options: nil)
    }

    /// Disconnect from current device
    public func disconnect() {
        guard let device = connectedDevice,
              let peripheral = peripherals[device.id] else { return }

        centralManager.cancelPeripheralConnection(peripheral)
        connectedDevice = nil
        wifiListCharacteristic = nil
        credentialCharacteristic = nil
        statusCharacteristic = nil
    }

    /// Send WiFi credentials to ESP32
    public func sendCredentials(ssid: String, password: String) {
        guard let characteristic = credentialCharacteristic,
              let peripheral = connectedDevice?.peripheral else {
            onError?(BLEError.notConnected)
            return
        }

        do {
            let data = try encoder.encodeCredentialWrite(ssid: ssid, password: password)
            peripheral.writeValue(data, for: characteristic, type: .withResponse)
        } catch {
            onError?(error)
        }
    }

    /// Request current status from ESP32
    public func requestStatus() {
        guard let characteristic = statusCharacteristic,
              let peripheral = connectedDevice?.peripheral else {
            onError?(BLEError.notConnected)
            return
        }

        // Simply read the status characteristic
        peripheral.readValue(for: characteristic)
    }
}

// MARK: - CBCentralManagerDelegate

extension BLEManager: CBCentralManagerDelegate {
    public func centralManagerDidUpdateState(_ central: CBCentralManager) {
        bluetoothState = central.state

        switch central.state {
        case .poweredOn:
            break
        case .poweredOff:
            onError?(BLEError.bluetoothPoweredOff)
            isScanning = false
        case .unsupported:
            onError?(BLEError.bluetoothUnsupported)
        case .unauthorized:
            onError?(BLEError.bluetoothUnauthorized)
        default:
            break
        }
    }

    public func centralManager(_ central: CBCentralManager,
                              didDiscover peripheral: CBPeripheral,
                              advertisementData: [String: Any],
                              rssi RSSI: NSNumber) {
        // Avoid duplicate entries
        guard peripherals[peripheral.identifier] == nil else {
            return
        }

        peripherals[peripheral.identifier] = peripheral

        let device = BLEPeripheral(
            peripheral: peripheral,
            advertisementData: advertisementData,
            rssi: RSSI
        )

        DispatchQueue.main.async {
            self.discoveredDevices.append(device)
        }
    }

    public func centralManager(_ central: CBCentralManager,
                              didConnect peripheral: CBPeripheral) {
        peripheral.delegate = self
        peripheral.discoverServices([BLEConstants.serviceUUID])

        if let device = discoveredDevices.first(where: { $0.peripheral === peripheral }) {
            DispatchQueue.main.async {
                self.connectedDevice = device
                self.onConnectionStateChanged?(true)
            }
        }
    }

    public func centralManager(_ central: CBCentralManager,
                              didFailToConnect peripheral: CBPeripheral,
                              error: Error?) {
        onError?(error ?? BLEError.connectionFailed)
        DispatchQueue.main.async {
            self.connectedDevice = nil
            self.onConnectionStateChanged?(false)
        }
    }

    public func centralManager(_ central: CBCentralManager,
                              didDisconnectPeripheral peripheral: CBPeripheral,
                              error: Error?) {
        if let error = error {
            onError?(error)
        }

        DispatchQueue.main.async {
            self.connectedDevice = nil
            self.onConnectionStateChanged?(false)
        }
    }
}

// MARK: - CBPeripheralDelegate

extension BLEManager: CBPeripheralDelegate {
    public func peripheral(_ peripheral: CBPeripheral, didDiscoverServices error: Error?) {
        if let error = error {
            onError?(error)
            return
        }

        guard let service = peripheral.services?.first(where: { $0.uuid == BLEConstants.serviceUUID }) else {
            onError?(BLEError.serviceNotFound)
            return
        }

        peripheral.discoverCharacteristics([
            BLEConstants.wifiListCharacteristicUUID,
            BLEConstants.credentialWriteCharacteristicUUID,
            BLEConstants.statusCharacteristicUUID
        ], for: service)
    }

    public func peripheral(_ peripheral: CBPeripheral,
                          didDiscoverCharacteristicsFor service: CBService,
                          error: Error?) {
        if let error = error {
            onError?(error)
            return
        }

        guard let characteristics = service.characteristics else { return }

        for characteristic in characteristics {
            switch characteristic.uuid {
            case BLEConstants.wifiListCharacteristicUUID:
                wifiListCharacteristic = characteristic
                peripheral.setNotifyValue(true, for: characteristic)

            case BLEConstants.credentialWriteCharacteristicUUID:
                credentialCharacteristic = characteristic

            case BLEConstants.statusCharacteristicUUID:
                statusCharacteristic = characteristic
                peripheral.setNotifyValue(true, for: characteristic)
                peripheral.readValue(for: characteristic)

            default:
                break
            }
        }
    }

    public func peripheral(_ peripheral: CBPeripheral,
                          didUpdateValueFor characteristic: CBCharacteristic,
                          error: Error?) {
        if let error = error {
            onError?(error)
            return
        }

        guard let data = characteristic.value else { return }

        do {
            let message = try decoder.decode(data)
            handleMessage(message)
        } catch {
            onError?(error)
        }
    }

    public func peripheral(_ peripheral: CBPeripheral,
                          didWriteValueFor characteristic: CBCharacteristic,
                          error: Error?) {
        if let error = error {
            onError?(error)
        }
    }

    // MARK: - Message Handling

    private func handleMessage(_ message: ProtocolMessage) {
        switch message {
        case .wifiListStart:
            receivedNetworks.removeAll()
            isReceivingNetworkList = true

        case .wifiNetworkEntry(let network):
            if isReceivingNetworkList {
                receivedNetworks.append(network)
            }

        case .wifiListEnd:
            isReceivingNetworkList = false
            onWiFiNetworksReceived?(receivedNetworks)

        case .credentialWriteAck(let statusCode):
            if statusCode != 0x00 {
                onError?(BLEError.credentialWriteFailed(statusCode))
            }

        case .statusResponse(let status):
            onStatusReceived?(status)

        case .error(let code, let errorMessage):
            onError?(BLEError.esp32Error(code: code, message: errorMessage))

        default:
            break
        }
    }
}

// MARK: - BLE Errors

public enum BLEError: LocalizedError {
    case bluetoothUnavailable
    case bluetoothPoweredOff
    case bluetoothUnsupported
    case bluetoothUnauthorized
    case deviceNotFound
    case notConnected
    case connectionFailed
    case serviceNotFound
    case characteristicNotFound
    case credentialWriteFailed(UInt8)
    case esp32Error(code: ProtocolErrorCode, message: String)

    public var errorDescription: String? {
        switch self {
        case .bluetoothUnavailable:
            return "Bluetooth is not available"
        case .bluetoothPoweredOff:
            return "Bluetooth is powered off. Please enable Bluetooth in Settings."
        case .bluetoothUnsupported:
            return "Bluetooth is not supported on this device"
        case .bluetoothUnauthorized:
            return "Bluetooth access is not authorized. Please enable in Settings."
        case .deviceNotFound:
            return "Device not found"
        case .notConnected:
            return "Not connected to a device"
        case .connectionFailed:
            return "Failed to connect to device"
        case .serviceNotFound:
            return "WiFiSet service not found on device"
        case .characteristicNotFound:
            return "Required characteristic not found"
        case .credentialWriteFailed(let code):
            return "Failed to write credentials (code: \(code))"
        case .esp32Error(_, let message):
            return "ESP32 error: \(message)"
        }
    }
}
