# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

WiFiSet is a monorepo containing an ESP32 BLE-based WiFi configuration system with an iOS companion app. ESP32 devices advertise over BLE, iOS app discovers them, user configures WiFi credentials, ESP32 stores credentials in NVS and auto-connects on subsequent boots.

**Key Architecture Pattern**: Custom binary protocol over BLE for efficient communication between platforms. Protocol must remain synchronized between ESP32 and iOS implementations.

## Critical Build Commands

### ESP32 (PlatformIO)

**Important**: This project uses ESP32-S3 hardware with 4MB flash and 2MB PSRAM. The `platformio.ini` is specifically configured for this hardware.

```bash
# Build and flash ESP32-S3
cd ESP32/example
pio run --target upload

# Monitor serial output
pio device monitor -b 115200

# Clean build
pio run --target clean

# Erase flash (if needed for partition changes)
pio run --target erase
```

**Hardware-specific notes**:
- Board: `esp32-s3-devkitc-1`
- Flash: 4MB (configured with `board_build.flash_size = 4MB`)
- Partition: `no_ota.csv` (gives ~2MB app space)
- USB: Native USB-Serial/JTAG (`ARDUINO_USB_CDC_ON_BOOT=1`)

### iOS (Xcode/Swift)

```bash
# Build from command line (for testing)
cd iOS/WiFiSet
xcodebuild -scheme WiFiSet -destination 'platform=iOS Simulator,name=iPhone 15' build

# Or open in Xcode
open WiFiSet.xcodeproj
```

**Important**:
- iOS SDK is a **local Swift Package** at `iOS/WiFiSet/WiFiSetSDK/`
- BLE requires **physical iOS device** (not simulator)
- iOS 17.0+ minimum requirement (uses `.navigationDestination(item:)`)

## Protocol Synchronization

**CRITICAL**: The binary protocol between ESP32 and iOS must stay in sync. When modifying protocol:

1. Update `PROTOCOL.md` first
2. Update `ESP32/library/src/Protocol/MessageBuilder.h` (ESP32 side)
3. Update `iOS/WiFiSet/WiFiSetSDK/Sources/WiFiSetSDK/Protocol/MessageTypes.swift` (iOS side)
4. Test end-to-end before committing

**Key protocol files**:
- ESP32: `MessageBuilder.h/cpp`, `ProtocolHandler.h/cpp`
- iOS: `MessageTypes.swift`, `ProtocolEncoder.swift`, `ProtocolDecoder.swift`

**Common sync issues**:
- Enum value mismatches (e.g., `SCAN_FAILED` vs `wifiScanFailed`)
- Little-endian byte order for multi-byte integers
- UTF-8 string encoding with length prefixes
- Message sequence numbers

## Architecture Layers

### ESP32 Library Structure

```
WiFiSetESP32 (public API)
├── BLEService (BLE GATT server)
│   └── Manages 3 characteristics: WiFi List, Credentials, Status
├── WiFiManager (WiFi operations)
│   └── Scanning, connection, status monitoring
├── NVSManager (persistent storage)
│   └── Saves credentials to ESP32 NVS
└── Protocol (message encoding/decoding)
    ├── MessageBuilder: Creates binary messages
    └── ProtocolHandler: Parses incoming messages
```

**Key pattern**: `WiFiSetESP32` is a facade providing simple callbacks. All complexity hidden behind `begin()` and `loop()`.

**Callback flow**: BLE events → BLEService → WiFiSetESP32 → User callbacks

### iOS SDK Structure

```
WiFiSetSDK (Swift Package)
├── BLE/
│   ├── BLEManager: CoreBluetooth orchestration
│   ├── BLEPeripheral: Device model wrapper
│   └── BLEConstants: UUIDs
├── Protocol/
│   ├── ProtocolEncoder: Binary message creation
│   ├── ProtocolDecoder: Binary message parsing
│   └── WiFiNetwork: Data models
├── Storage/
│   └── KeychainManager: Secure password storage
└── Views/ (SwiftUI)
    ├── DeviceScannerView
    ├── WiFiNetworkListView
    ├── WiFiCredentialInputView
    └── ConnectionStatusView
```

**Key pattern**: `BLEManager` is `ObservableObject` used by SwiftUI views. Protocol encoding/decoding isolated from UI.

**Data flow**: BLE notifications → ProtocolDecoder → BLEManager → @Published properties → SwiftUI views

## Common Issues & Solutions

### BLE Callback Pattern (CRITICAL)

**Symptom**: Serial output corruption, crashes, or ESP32 reboots when BLE client connects or credentials received.

**Root cause**: Heavy work (WiFi scan, NVS write, BLE notifications, user callbacks) done inside BLE callback context. The BLE task has limited stack and runs at high priority.

**Solution**: Use flag-based deferred execution. BLE callbacks only set flags, heavy work happens in `loop()`:
```cpp
// In header:
volatile bool pendingClientConnect;
volatile bool pendingCredentials;
String pendingSSID;
String pendingPassword;

// In BLE callback:
void WiFiSetESP32::onClientConnected() {
    pendingClientConnect = true;  // Just set flag
}

void WiFiSetESP32::onCredentialsReceived(const String& ssid, const String& password) {
    pendingSSID = ssid;
    pendingPassword = password;
    pendingCredentials = true;  // Just set flag
}

// In loop():
void WiFiSetESP32::loop() {
    if (pendingClientConnect) {
        pendingClientConnect = false;
        // Now safe to do heavy work: user callbacks, WiFi scan, BLE sends
        if (bleClientConnectedCallback) bleClientConnectedCallback();
        performWiFiScanAndSend();
    }
    if (pendingCredentials) {
        pendingCredentials = false;
        handleWiFiConnection(pendingSSID, pendingPassword);
    }
}
```

### ESP32-S3 USB CDC Serial Issues

**Symptom**: `Serial.flush()` doesn't prevent output truncation, or makes it worse.

**Root cause**: ESP32-S3 USB CDC implementation handles `Serial.flush()` differently than traditional UART.

**Solution**: Don't rely on `Serial.flush()` for timing. Use delays and reduce output volume during BLE operations. The deferred callback pattern (above) is the real fix.

### WiFi Connection State Tracking

**Symptom**: State shows `NOT_CONFIGURED` even after credentials saved, because `WiFi.SSID()` returns empty after `WiFi.disconnect()`.

**Solution**: WiFiManager now has `credentialsConfigured` flag, set when credentials are saved:
```cpp
wifiManager.setCredentialsConfigured(true);  // After NVS save
```

This flag persists in memory and properly returns `CONFIGURED_NOT_CONNECTED` state.

### Example Code Ordering

**Important**: In `main.cpp`, call `getSavedCredentials()` AFTER `wifiSet.begin()`:
```cpp
// WRONG - NVS not initialized yet:
WiFiSet::WiFiSetCredentials savedCreds = wifiSet.getSavedCredentials();
wifiSet.begin();

// CORRECT - NVS initialized in begin():
wifiSet.begin();
WiFiSet::WiFiSetCredentials savedCreds = wifiSet.getSavedCredentials();
```

### iOS Protocol Decoder Empty Data

**Symptom**: "Invalid message format: Header too short" error when iOS connects.

**Root cause**: iOS immediately reads status characteristic, but ESP32 hasn't set a value yet.

**Solution**: Skip decoding if data is too short:
```swift
guard let data = characteristic.value, data.count >= MessageHeader.size else {
    return  // Skip empty or too-short data
}
```

### ESP32 BLE Advertising Not Visible

**Symptom**: iOS app cannot find ESP32 device.

**Root cause**: Custom BLE advertising data can override service UUID advertising.

**Solution**: Keep advertising simple - only add service UUID, don't manually set advertisement data:
```cpp
BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
pAdvertising->addServiceUUID(WIFISET_SERVICE_UUID);
pAdvertising->setScanResponse(true);
BLEDevice::startAdvertising();
// DO NOT manually call setAdvertisementData() - breaks iOS discovery
```

### ESP32-S3 Build Errors

**Naming conflicts with Arduino libraries**:
- ESP32 WiFi lib defines `WIFI_SCAN_FAILED` macro → Use `SCAN_FAILED` in our enum
- ESP32 BLE lib has `BLEService` class → Use `WiFiSetBLEService` for our class

**Flash size mismatch**:
- Symptom: Boot loop with "Detected size(4096k) smaller than header(8192k)"
- Solution: Erase flash (`pio run --target erase`), then rebuild with correct partition

### iOS Build Errors

**Section syntax** (iOS 17 requirement):
```swift
// Old (fails):
Section("Header") { }

// New (works):
Section { } header: { Text("Header") }
```

**CBPeripheral preview**: Cannot instantiate `CBPeripheral()` directly in SwiftUI previews - disable previews for views using real BLE objects.

### WiFi Scan Crashes ESP32

**Symptom**: ESP32 crashes/reboots when iOS connects and WiFi scan starts.

**Root cause**: WiFi scan while BLE is active can cause resource conflicts.

**Solution**: Add delay before scan, check client still connected:
```cpp
delay(500);  // Let BLE connection stabilize
if (bleService.isClientConnected()) {
    std::vector<WiFiNetworkInfo> networks = wifiManager.scanNetworks();
    bleService.sendWiFiNetworkList(networks);
}
```

### BLE Notification Sending Rate

**Symptom**: ESP32 crashes when sending multiple BLE notifications rapidly (e.g., WiFi network list).

**Root cause**: BLE stack has limited queue for outgoing notifications. Rapid sends overflow the queue.

**Solution**: Add delays and yield() between notifications:
```cpp
void WiFiSetBLEService::sendWiFiNetworkList(const std::vector<WiFiNetworkInfo>& networks) {
    sendNotification(pWiFiListCharacteristic, startMsg);
    delay(100);  // 100ms between notifications
    yield();     // Let BLE stack process

    for (const auto& network : networks) {
        sendNotification(pWiFiListCharacteristic, entryMsg);
        delay(100);
        yield();
    }
}
```

### WiFi Connection Timeout

Default timeout changed from 30 seconds to 10 seconds (`WiFiManager.h`). Faster failure feedback for wrong password or out-of-range networks.

## Testing Workflow

### ESP32 Testing
1. Flash firmware: `pio run --target upload`
2. Open serial monitor: `pio device monitor -b 115200`
3. Verify BLE advertising message appears
4. Use nRF Connect app (iOS/Android) to verify:
   - Service UUID visible: `4FAFC201-1FB5-459E-8FCC-C5C9C331914B`
   - Three characteristics present
5. Test with iOS app for end-to-end flow

### iOS Testing
1. Build and run on **physical iPhone** (BLE requires hardware)
2. Verify Bluetooth permission granted
3. Test device discovery
4. Monitor console for protocol decoding errors
5. Verify Keychain storage (check Settings → Passwords)

### End-to-End Integration
1. ESP32 powered on → Serial shows "BLE Advertising Active"
2. iOS app scanning → ESP32 appears in list
3. Tap device → ESP32 serial shows "[BLE] Client Connected"
4. ESP32 scans WiFi → iOS receives network list
5. Select network, enter password → ESP32 serial shows credentials received
6. ESP32 connects → iOS shows "Connected" with IP address
7. Reboot ESP32 → Auto-connects using saved credentials

## BLE Service UUIDs

**Service**: `4FAFC201-1FB5-459E-8FCC-C5C9C331914B`

**Characteristics**:
- WiFi List (NOTIFY): `4FAFC202-1FB5-459E-8FCC-C5C9C331914B`
- Credentials (WRITE): `4FAFC203-1FB5-459E-8FCC-C5C9C331914B`
- Status (NOTIFY): `4FAFC204-1FB5-459E-8FCC-C5C9C331914B`

**Do not change these** - hardcoded in both platforms. Changing requires coordinated update.

## Credential Storage

**ESP32**: Uses NVS (Non-Volatile Storage)
- Namespace: `"wifiset"`
- Keys: `"ssid"`, `"password"`
- Persists across reboots and power cycles
- Access via `NVSManager` class

**iOS**: Uses Keychain
- Service identifier: `"com.skyeroad.WiFiSet"`
- Key format: `"wifi.password.<ssid>"`
- Encrypted at rest by iOS
- Access via `KeychainManager` class

## Code Naming Conventions

**ESP32** (C++):
- Class names: `WiFiSetBLEService`, `WiFiManager` (avoid conflicts with Arduino libs)
- Enums: `PascalCase` with `enum class` (e.g., `ConnectionState::CONNECTED`)
- Callbacks: `onEventName` (e.g., `onWiFiConnected`)
- Private methods: lowercase with underscores (e.g., `send_data`)

**iOS** (Swift):
- Class names: `BLEManager`, `KeychainManager`
- Enums: `camelCase` values (e.g., `ConnectionState.connected`)
- Published properties: `@Published` for SwiftUI binding
- Callbacks: closures stored as properties (e.g., `onWiFiNetworksReceived`)

## Message Sequence Numbers

Both platforms maintain independent sequence counters (0-255, wraps). Used for debugging packet loss but not enforced. Future versions may add reliability using sequence numbers.

Current implementation: Increment on send, log on receive, but don't validate.

## Binary Protocol Format

All messages: **4-byte header + variable payload**

Header: `[Type(1), Sequence(1), Length_Low(1), Length_High(1)]`

**Little-endian** for multi-byte integers (uint16 length, IPv4 address).

String encoding: Length-prefixed UTF-8 (`[length(1), bytes(N)]`)

See `PROTOCOL.md` for complete specification with hex examples.

## Platform Requirements

**ESP32**:
- ESP32-S3 (or original ESP32, ESP32-C3 with BLE)
- NOT ESP32-S2 (no BLE support)
- PlatformIO with Arduino framework
- ESP32 BLE Arduino library 2.0.0+

**iOS**:
- iOS 17.0+ (changed from 16.0 due to `.navigationDestination` API)
- Xcode 15.0+
- Swift 5.9+
- Physical device required (Simulator has no BLE)

## Important File Locations

**Protocol specification**: `PROTOCOL.md` (source of truth)

**ESP32 public API**: `ESP32/library/src/WiFiSetESP32.h`

**iOS public API**: Views in `iOS/WiFiSet/WiFiSetSDK/Sources/WiFiSetSDK/Views/`

**Example usage**:
- ESP32: `ESP32/example/src/main.cpp`
- iOS: `iOS/WiFiSet/WiFiSet/ContentView.swift`

**PlatformIO config**: `ESP32/example/platformio.ini` (hardware-specific settings)

**Swift Package manifest**: `iOS/WiFiSet/WiFiSetSDK/Package.swift`

## Current Status / Known Issues

### WiFi Connection Debugging (In Progress)

WiFi connection from iOS app to ESP32 is currently failing even with correct credentials. Debug logging has been added to `WiFiManager::connect()` to diagnose the issue. Serial output will show:
- SSID and password length being used
- WiFi status codes during connection attempt (0=IDLE, 1=NO_SSID, 4=CONNECT_FAILED, 6=DISCONNECTED)
- Timeout or failure reason

Check serial monitor output to see what status code is being returned.

### Items Working

- ✅ BLE advertising and iOS discovery
- ✅ BLE connection stability (using deferred callback pattern)
- ✅ WiFi network scanning and list transmission to iOS
- ✅ Credential transmission from iOS to ESP32
- ✅ NVS credential storage (persists across reboots)
- ✅ Serial output no longer corrupted
- ✅ iOS protocol decoder handles empty data gracefully

### Items To Test/Fix

- ⏳ WiFi connection with received credentials (debug logging added)
- ⏳ Full end-to-end flow: scan → select network → enter password → ESP32 connects
