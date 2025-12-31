# WiFiSetSDK

Swift Package for configuring ESP32 WiFi over Bluetooth Low Energy (BLE).

## Overview

WiFiSetSDK provides SwiftUI components and BLE communication to configure WiFi credentials on ESP32 devices. It includes ready-to-use views for device discovery, network selection, and credential entry.

### Features

- **SwiftUI Views**: Drop-in UI components for complete configuration flow
- **BLE Communication**: CoreBluetooth integration with ESP32 devices
- **Secure Storage**: Keychain integration for WiFi passwords
- **Binary Protocol**: Efficient custom protocol matching ESP32 implementation
- **Real-time Status**: Monitor ESP32 WiFi connection status
- **iOS 16+**: Modern Swift and SwiftUI

## Requirements

- iOS 17.0+
- Xcode 15.0+
- Swift 5.9+
- Physical iOS device (BLE not available in Simulator)

## Installation

### Local Package (Recommended for Development)

This SDK is designed as a local Swift Package within the WiFiSet Xcode project:

1. Open your Xcode project
2. File → Add Package Dependencies → Add Local
3. Select the `WiFiSetSDK` folder
4. Add WiFiSetSDK to your app target

### Standalone Swift Package

If you want to use this in a different project:

1. Copy the `WiFiSetSDK` folder to your project
2. File → Add Package Dependencies → Add Local
3. Select the copied `WiFiSetSDK` folder

## Quick Start

### 1. Configure Info.plist

Add Bluetooth permission to your `Info.plist`:

```xml
<key>NSBluetoothAlwaysUsageDescription</key>
<string>This app uses Bluetooth to configure WiFi on your ESP32 device</string>
```

### 2. Import and Use

```swift
import SwiftUI
import WiFiSetSDK

struct ContentView: View {
    var body: some View {
        DeviceScannerView()
    }
}
```

That's it! The SDK provides a complete UI flow:
1. Scan for ESP32 devices
2. Connect to selected device
3. Display WiFi networks from ESP32
4. Enter WiFi password
5. Send credentials to ESP32
6. Monitor connection status

## Components

### Views

#### `DeviceScannerView`

Scans for and displays available ESP32 devices.

```swift
DeviceScannerView()
```

Features:
- Automatic BLE scanning
- List of discovered devices with signal strength
- Scan/Stop toggle
- Navigation to network selection

#### `WiFiNetworkListView`

Displays WiFi networks discovered by ESP32.

```swift
WiFiNetworkListView(bleManager: bleManager, device: device)
```

Features:
- WiFi network list with signal strength icons
- Security type indicators
- Network selection
- Connection status

#### `WiFiCredentialInputView`

Password entry for selected network.

```swift
WiFiCredentialInputView(
    network: network,
    bleManager: bleManager,
    onSubmit: { ssid, password in
        // Credentials submitted
    }
)
```

Features:
- Secure password entry
- Optional Keychain storage
- Network information display
- Validation

#### `ConnectionStatusView`

Monitors ESP32 WiFi connection status.

```swift
ConnectionStatusView(bleManager: bleManager)
```

Features:
- Real-time status updates
- IP address display
- Signal strength
- Error reporting

### Core Classes

#### `BLEManager`

Manages BLE communication with ESP32.

```swift
let bleManager = BLEManager()

bleManager.startScanning()
bleManager.connect(to: device)
bleManager.sendCredentials(ssid: "Network", password: "password")
```

#### `KeychainManager`

Securely stores WiFi passwords.

```swift
let keychain = KeychainManager()

try keychain.saveCredentials(ssid: "Network", password: "password")
let password = try keychain.loadPassword(for: "Network")
```

### Data Models

#### `WiFiNetwork`

Represents a WiFi network.

```swift
struct WiFiNetwork {
    let ssid: String
    let rssi: Int8
    let securityType: WiFiSecurityType
    let channel: UInt8
    var signalStrength: Int  // 0-4 bars
}
```

#### `WiFiSecurityType`

WiFi security types.

```swift
enum WiFiSecurityType {
    case open
    case wep
    case wpaPsk
    case wpa2Enterprise
    case wpa3
}
```

#### `ConnectionState`

ESP32 WiFi connection states.

```swift
enum ConnectionState {
    case notConfigured
    case configuredNotConnected
    case connecting
    case connected
    case connectionFailed
}
```

## Advanced Usage

### Custom UI

You can use the core components without the provided views:

```swift
import WiFiSetSDK

class MyViewModel: ObservableObject {
    let bleManager = BLEManager()

    func scanForDevices() {
        bleManager.startScanning()
    }

    func configureWiFi(ssid: String, password: String) {
        bleManager.sendCredentials(ssid: ssid, password: password)
    }
}
```

### Manual BLE Control

```swift
let bleManager = BLEManager()

// Set up callbacks
bleManager.onWiFiNetworksReceived = { networks in
    print("Found \(networks.count) networks")
}

bleManager.onStatusReceived = { status in
    print("ESP32 status: \(status.connectionState)")
}

bleManager.onError = { error in
    print("Error: \(error)")
}

// Start scanning
bleManager.startScanning()
```

### Keychain Integration

```swift
let keychain = KeychainManager()

// Save password
try? keychain.saveCredentials(ssid: "MyNetwork", password: "password123")

// Load password
if let password = try? keychain.loadPassword(for: "MyNetwork") {
    print("Found saved password")
}

// Delete
try? keychain.deleteCredentials(for: "MyNetwork")
```

## Protocol

WiFiSetSDK implements a custom binary protocol for efficient BLE communication. See the root `PROTOCOL.md` for complete specification.

### BLE Service UUID
`4FAFC201-1FB5-459E-8FCC-C5C9C331914B`

### Characteristics
- WiFi List (NOTIFY): `4FAFC202-1FB5-459E-8FCC-C5C9C331914B`
- Credentials (WRITE): `4FAFC203-1FB5-459E-8FCC-C5C9C331914B`
- Status (NOTIFY): `4FAFC204-1FB5-459E-8FCC-C5C9C331914B`

## Architecture

```
WiFiSetSDK/
├── BLE/
│   ├── BLEConstants.swift      # UUIDs and constants
│   ├── BLEPeripheral.swift     # Device model
│   └── BLEManager.swift        # CoreBluetooth integration
├── Protocol/
│   ├── MessageTypes.swift      # Protocol enums and types
│   ├── ProtocolEncoder.swift   # Encode messages to binary
│   ├── ProtocolDecoder.swift   # Decode binary messages
│   └── WiFiNetwork.swift       # Network data model
├── Storage/
│   └── KeychainManager.swift   # Secure credential storage
└── Views/
    ├── DeviceScannerView.swift
    ├── WiFiNetworkListView.swift
    ├── WiFiCredentialInputView.swift
    └── ConnectionStatusView.swift
```

## Testing

### Unit Tests

Run tests in Xcode:
- Product → Test (Cmd+U)
- Tests cover protocol encoding/decoding, keychain operations

### Manual Testing

1. Flash ESP32 with WiFiSetESP32 library example
2. Run iOS app on physical device (BLE requires hardware)
3. Test complete flow:
   - Device discovery
   - Connection
   - Network list
   - Credential submission
   - Status monitoring

## Troubleshooting

### BLE Not Working

- Verify you're running on a physical device (not Simulator)
- Check Bluetooth permissions in Settings → Privacy
- Ensure Bluetooth is enabled
- Check Info.plist has `NSBluetoothAlwaysUsageDescription`

### Can't Find ESP32

- Verify ESP32 is powered on and running WiFiSet library
- Check ESP32 Serial output for BLE advertising status
- Ensure ESP32 is in range (BLE has limited range)
- Try restarting both devices

### Credentials Not Sending

- Verify BLE connection is established
- Check ESP32 Serial output for errors
- Ensure password meets network requirements

### Keychain Errors

- Check app has proper entitlements
- Verify not running in Simulator (Keychain behaves differently)
- Check for error messages in console

## Example

See the `WiFiSet` demo app in the parent directory for a complete working example.

## License

MIT License - see LICENSE file for details.

## Related

- [WiFiSetESP32 Library](../../../ESP32/library/) - ESP32 library
- [Protocol Specification](../../../PROTOCOL.md) - BLE protocol details
- [Demo App](../) - Example iOS application

## Support

For issues, questions, or contributions, please visit the GitHub repository.
