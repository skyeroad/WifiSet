# WiFiSetESP32

BLE-based WiFi configuration library for ESP32 with persistent credential storage.

## Overview

WiFiSetESP32 provides a simple API to enable WiFi configuration via Bluetooth Low Energy (BLE) on ESP32 devices. No need for SoftAP mode, web servers, or complicated provisioning flows.

### Features

- **Simple API**: Just a few lines of code to add WiFi configuration to your project
- **BLE-based**: Uses Bluetooth Low Energy for configuration (no SoftAP required)
- **Persistent Storage**: WiFi credentials saved in ESP32 NVS (survives reboots)
- **Auto-Reconnect**: Automatically connects on boot using saved credentials
- **WiFi Scanning**: Automatically scans and sends available networks to iOS app
- **Status Monitoring**: Real-time connection status updates
- **Callback-based**: React to events with user-defined callbacks
- **iOS App**: Works with the WiFiSet iOS SDK and demo app

## Requirements

- ESP32 (any variant with BLE support)
- PlatformIO or Arduino IDE
- ESP32 Arduino framework 2.0.0+
- ESP32 BLE Arduino library 2.0.0+

## Installation

### PlatformIO

Add to your `platformio.ini`:

```ini
lib_deps =
    WiFiSetESP32
```

Or install from local directory:

```bash
cd your-project
cp -r path/to/WiFiSet/ESP32/library .pio/libdeps/your-env/WiFiSetESP32
```

### Arduino IDE

1. Download or clone this repository
2. Copy `ESP32/library` folder to your Arduino `libraries` directory
3. Rename it to `WiFiSetESP32`
4. Restart Arduino IDE

## Quick Start

```cpp
#include <WiFiSetESP32.h>

WiFiSetESP32 wifiSet("MyESP32Device");

void setup() {
    Serial.begin(115200);

    // Set up callbacks (optional)
    wifiSet.onWiFiConnected([](IPAddress ip) {
        Serial.printf("Connected! IP: %s\n", ip.toString().c_str());
    });

    // Initialize and start
    wifiSet.begin();
}

void loop() {
    wifiSet.loop();  // Must be called regularly

    // Your code here
    if (wifiSet.isConnected()) {
        // Do WiFi-related tasks
    }
}
```

That's it! Your ESP32 will now:
1. Check for saved WiFi credentials
2. Auto-connect if credentials exist
3. Start BLE advertising if no credentials or connection fails
4. Accept WiFi configuration from iOS app
5. Save credentials and connect

## API Reference

### Initialization

#### `WiFiSetESP32(const char* deviceName = "ESP32-WiFiSet")`

Constructor. Creates a WiFiSetESP32 instance with specified BLE device name.

```cpp
WiFiSetESP32 wifiSet("MyDevice");
```

#### `void begin()`

Initialize the library. Must be called in `setup()`.

- Initializes BLE, WiFi, and storage
- Loads saved credentials
- Attempts auto-connect if credentials exist
- Starts BLE advertising if needed

```cpp
void setup() {
    wifiSet.begin();
}
```

#### `void loop()`

Process library tasks. Must be called regularly in `loop()`.

```cpp
void loop() {
    wifiSet.loop();
}
```

### Callbacks

All callbacks are optional but recommended for monitoring status.

#### `void onCredentialsReceived(callback)`

Called when new credentials are received from iOS app.

```cpp
wifiSet.onCredentialsReceived([](const String& ssid, const String& password) {
    Serial.printf("New WiFi: %s\n", ssid.c_str());
});
```

#### `void onWiFiConnected(callback)`

Called when successfully connected to WiFi.

```cpp
wifiSet.onWiFiConnected([](IPAddress ip) {
    Serial.printf("Connected! IP: %s\n", ip.toString().c_str());
});
```

#### `void onWiFiConnectionFailed(callback)`

Called when WiFi connection fails.

```cpp
wifiSet.onWiFiConnectionFailed([]() {
    Serial.println("Connection failed!");
});
```

#### `void onConnectionStatusChanged(callback)`

Called when connection status changes.

```cpp
wifiSet.onConnectionStatusChanged([](WiFiSetConnectionStatus status) {
    switch (status) {
        case WiFiSetConnectionStatus::NOT_CONFIGURED:
            Serial.println("No credentials");
            break;
        case WiFiSetConnectionStatus::CONNECTING:
            Serial.println("Connecting...");
            break;
        case WiFiSetConnectionStatus::CONNECTED:
            Serial.println("Connected!");
            break;
        // ... other states
    }
});
```

#### `void onBLEClientConnected(callback)`

Called when iOS app connects via BLE.

```cpp
wifiSet.onBLEClientConnected([]() {
    Serial.println("iOS app connected");
});
```

#### `void onBLEClientDisconnected(callback)`

Called when iOS app disconnects from BLE.

```cpp
wifiSet.onBLEClientDisconnected([]() {
    Serial.println("iOS app disconnected");
});
```

### Status Methods

#### `bool isConnected()`

Check if connected to WiFi.

```cpp
if (wifiSet.isConnected()) {
    // Do something
}
```

#### `WiFiSetConnectionStatus getConnectionStatus()`

Get current connection status.

Returns: `NOT_CONFIGURED`, `CONFIGURED_NOT_CONNECTED`, `CONNECTING`, `CONNECTED`, or `CONNECTION_FAILED`

#### `IPAddress getIPAddress()`

Get current IP address (0.0.0.0 if not connected).

#### `int getRSSI()`

Get WiFi signal strength in dBm (0 if not connected).

#### `String getSSID()`

Get currently connected/configured SSID (empty if not configured).

### Credential Management

#### `WiFiSetCredentials getSavedCredentials()`

Load saved credentials from NVS.

```cpp
WiFiSetCredentials creds = wifiSet.getSavedCredentials();
if (creds.isValid) {
    Serial.printf("Saved SSID: %s\n", creds.ssid.c_str());
}
```

#### `bool clearCredentials()`

Clear saved credentials from NVS. Does not disconnect from current WiFi.

```cpp
wifiSet.clearCredentials();
```

### Manual WiFi Control

#### `bool connectWiFi(ssid, password, save = true)`

Manually connect to WiFi network.

```cpp
if (wifiSet.connectWiFi("MyNetwork", "password123")) {
    Serial.println("Connected!");
}
```

Parameters:
- `ssid`: Network name
- `password`: Network password (empty for open networks)
- `save`: Save credentials to NVS (default: true)

#### `void disconnectWiFi()`

Disconnect from WiFi.

### BLE Control

#### `void startBLE()`

Start BLE advertising. Useful if you stopped BLE and want to restart it.

#### `void stopBLE()`

Stop BLE advertising. Useful to save power when WiFi is connected.

#### `bool isBLERunning()`

Check if BLE is running.

## Connection Status States

| State | Description |
|-------|-------------|
| `NOT_CONFIGURED` | No credentials stored in NVS |
| `CONFIGURED_NOT_CONNECTED` | Credentials saved but not connected |
| `CONNECTING` | Attempting to connect to WiFi |
| `CONNECTED` | Successfully connected to WiFi |
| `CONNECTION_FAILED` | Failed to connect (wrong password, network not found, etc.) |

## Examples

### Basic Usage

See `examples/BasicUsage/` for a simple example with callbacks.

### Full Featured Example

See the main example in `../example/` for detailed logging and all features.

### Advanced: Conditional BLE

Stop BLE when WiFi is connected to save power:

```cpp
wifiSet.onWiFiConnected([](IPAddress ip) {
    Serial.println("WiFi connected, stopping BLE to save power");
    wifiSet.stopBLE();
});
```

Restart BLE when WiFi fails:

```cpp
wifiSet.onWiFiConnectionFailed([]() {
    Serial.println("WiFi failed, restarting BLE for reconfiguration");
    wifiSet.startBLE();
});
```

## How It Works

1. **First Boot** (no credentials):
   - ESP32 starts BLE advertising
   - iOS app connects and sends WiFi network list request
   - ESP32 scans for WiFi networks and sends list
   - User selects network and enters password in iOS app
   - iOS app sends credentials to ESP32
   - ESP32 saves credentials to NVS and attempts connection
   - If successful, ESP32 is now connected to WiFi

2. **Subsequent Boots** (credentials saved):
   - ESP32 loads credentials from NVS
   - Automatically attempts to connect
   - If connection successful, BLE may be stopped to save power
   - If connection fails, BLE advertising starts for reconfiguration

## Protocol

WiFiSetESP32 uses a custom binary protocol over BLE for efficient communication. See `PROTOCOL.md` in the repository root for complete specification.

### BLE Service UUID
`4FAFC201-1FB5-459E-8FCC-C5C9C331914B`

### Characteristics
- WiFi List (NOTIFY): `4FAFC202-1FB5-459E-8FCC-C5C9C331914B`
- Credentials (WRITE): `4FAFC203-1FB5-459E-8FCC-C5C9C331914B`
- Status (NOTIFY): `4FAFC204-1FB5-459E-8FCC-C5C9C331914B`

## Troubleshooting

### BLE Not Advertising

- Check Serial output for initialization errors
- Verify ESP32 has BLE support (not all ESP32 variants do)
- Try resetting ESP32
- Check if BLE was manually stopped

### WiFi Not Connecting

- Verify credentials are correct (case-sensitive!)
- Check WiFi network is in range
- Verify router is working
- Check Serial output for error messages
- Try clearing credentials: `wifiSet.clearCredentials()`

### Credentials Not Persisting

- Verify NVS partition exists in partition table
- Check available NVS space
- Try erasing flash and reflashing

### High Memory Usage

- BLE and WiFi both use significant memory
- Consider stopping BLE when WiFi connected
- Monitor free heap: `ESP.getFreeHeap()`

## Platform Support

| Platform | Support |
|----------|---------|
| ESP32 | ✅ Yes |
| ESP32-S2 | ❌ No (no BLE) |
| ESP32-S3 | ✅ Yes |
| ESP32-C3 | ✅ Yes |
| ESP8266 | ❌ No (no BLE) |

## License

MIT License - see LICENSE file for details.

## Related

- [WiFiSet iOS SDK](../iOS/WiFiSetSDK/) - Swift Package for iOS apps
- [WiFiSet iOS Demo App](../iOS/WiFiSet/) - Example iOS application
- [Protocol Specification](../../PROTOCOL.md) - BLE protocol details

## Support

For issues, questions, or contributions, please visit the GitHub repository.
