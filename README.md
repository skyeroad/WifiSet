# WiFiSet

A comprehensive WiFi configuration system for ESP32 devices using Bluetooth Low Energy (BLE) and iOS.

## Overview

WiFiSet provides a seamless way to configure WiFi credentials on ESP32 devices using an iOS app over Bluetooth Low Energy. No need for SoftAP mode, web servers, or complicated provisioning flows - just simple BLE communication between your iPhone and ESP32.

The system consists of two main components:

- **ESP32 Library** (PlatformIO): A reusable library that handles BLE advertising, WiFi scanning, credential storage, and WiFi connection management
- **iOS SDK** (Swift Package): A SwiftUI-based library with BLE communication and pre-built UI components for device discovery and WiFi configuration

## Features

### ESP32 Library
- BLE-based WiFi configuration (no SoftAP required)
- Automatic WiFi network scanning and list broadcasting
- Persistent credential storage using ESP32 NVS
- Simple callback-based API
- Auto-reconnect on boot with saved credentials
- Real-time connection status updates
- Complete abstraction of BLE complexity

### iOS SDK
- Device discovery and BLE connection management
- WiFi network list display with signal strength indicators
- Secure credential entry and validation
- Keychain storage for WiFi passwords
- SwiftUI views ready to drop into any project
- Real-time ESP32 connection status monitoring
- Support for credential updates

### Communication Protocol
- Custom binary protocol for efficient BLE communication
- Low bandwidth usage
- Support for all WiFi security types (Open, WEP, WPA/WPA2, WPA3)
- MTU negotiation for optimal performance
- See [PROTOCOL.md](PROTOCOL.md) for complete specification

## Repository Structure

```
WiFiSet/
├── README.md                    # This file
├── PROTOCOL.md                  # BLE protocol specification
├── .gitignore                   # Root gitignore
├── iOS/
│   └── WiFiSet/                 # Xcode project
│       ├── WiFiSet.xcodeproj/   # Xcode project
│       ├── WiFiSet/             # Demo iOS app
│       └── WiFiSetSDK/          # Swift Package library
└── ESP32/
    ├── library/                 # PlatformIO library
    │   ├── examples/            # Example usage
    │   └── src/                 # Library source code
    └── example/                 # Standalone test project
```

## Quick Start

### ESP32 (PlatformIO)

1. **Install the Library**

   Add to your `platformio.ini`:
   ```ini
   lib_deps =
       WiFiSetESP32
   ```

   Or install locally:
   ```bash
   cd your-project
   cp -r WiFiSet/ESP32/library .pio/libdeps/your-env/WiFiSetESP32
   ```

2. **Basic Usage**

   ```cpp
   #include <WiFiSetESP32.h>

   WiFiSetESP32 wifiSet("MyESP32Device");

   void setup() {
       Serial.begin(115200);

       // Set up callbacks
       wifiSet.onWiFiConnected([](IPAddress ip) {
           Serial.printf("WiFi connected! IP: %s\n", ip.toString().c_str());
       });

       wifiSet.onCredentialsReceived([](const String& ssid, const String& password) {
           Serial.printf("New credentials received: %s\n", ssid.c_str());
       });

       // Start the library (begins BLE advertising)
       wifiSet.begin();
   }

   void loop() {
       wifiSet.loop();  // Must be called regularly

       // Your application code here
       if (wifiSet.isConnected()) {
           // Do something when connected to WiFi
       }
   }
   ```

3. **Build and Flash**

   ```bash
   cd ESP32/example
   pio run --target upload
   pio device monitor
   ```

For detailed ESP32 documentation, see [ESP32/library/README.md](ESP32/library/README.md)

### iOS (Swift Package)

1. **Add WiFiSetSDK to Your Project**

   The WiFiSetSDK is a local Swift Package within the Xcode project:

   - Open your Xcode project
   - File → Add Package Dependencies → Add Local
   - Select `iOS/WiFiSet/WiFiSetSDK` folder
   - Add WiFiSetSDK to your app target

2. **Configure Info.plist**

   Add Bluetooth permission:
   ```xml
   <key>NSBluetoothAlwaysUsageDescription</key>
   <string>This app uses Bluetooth to configure WiFi on your ESP32 device</string>
   ```

3. **Basic Usage**

   ```swift
   import SwiftUI
   import WiFiSetSDK

   struct ContentView: View {
       var body: some View {
           DeviceScannerView()
       }
   }
   ```

   That's it! The SDK provides complete UI flows for:
   - Device scanning and discovery
   - WiFi network selection
   - Credential entry
   - Connection status monitoring

4. **Run on Physical Device**

   BLE requires a physical iOS device (not simulator):
   - Connect your iPhone
   - Select it as the build target
   - Build and run (Cmd+R)

For detailed iOS documentation, see [iOS/WiFiSet/WiFiSetSDK/README.md](iOS/WiFiSet/WiFiSetSDK/README.md)

## How It Works

1. **ESP32** starts up and checks for saved WiFi credentials in NVS
2. If credentials exist, **ESP32** automatically connects to WiFi
3. If no credentials or connection fails, **ESP32** starts BLE advertising
4. **iOS app** scans for nearby ESP32 devices
5. User selects the ESP32 device to configure
6. **ESP32** scans for WiFi networks and sends the list over BLE
7. **iOS app** displays available networks with signal strength
8. User selects a network and enters the password
9. **iOS app** sends credentials to ESP32 over BLE
10. **ESP32** saves credentials to NVS and attempts to connect
11. **ESP32** sends connection status updates back to iOS app
12. On subsequent boots, **ESP32** automatically connects using saved credentials

## Protocol

WiFiSet uses a custom binary protocol over BLE for efficient communication. The protocol supports:

- WiFi network list transmission (SSID, signal strength, security type, channel)
- Credential configuration (SSID and password)
- Connection status monitoring (state, IP address, signal strength)
- Error reporting

For complete protocol specification, see [PROTOCOL.md](PROTOCOL.md)

## Testing

### ESP32
- Unit tests for protocol encoding/decoding
- Example project with serial monitor output
- Test with nRF Connect app (iOS/Android) to verify BLE service
- Hardware testing with actual WiFi networks

### iOS
- Unit tests for protocol, keychain, and BLE components
- SwiftUI previews for all views
- Manual testing with ESP32 hardware

### End-to-End
1. Flash ESP32 with example project
2. Run iOS demo app on iPhone
3. Complete full workflow: scan → connect → view networks → configure → verify connection

## Examples

### ESP32 Examples
- [BasicUsage](ESP32/library/examples/BasicUsage/) - Minimal example with callbacks
- [example](ESP32/example/) - Full featured example with detailed logging

### iOS Examples
- [WiFiSet Demo App](iOS/WiFiSet/) - Complete demo application using WiFiSetSDK

## Requirements

### ESP32
- ESP32 (any variant with BLE support)
- PlatformIO or Arduino IDE
- ESP32 Arduino framework
- ESP32 BLE Arduino library (2.0.0+)

### iOS
- iOS 17.0 or later (uses `.navigationDestination(item:)` API)
- Xcode 15.0 or later
- Physical iOS device (BLE not available in simulator)
- Swift 5.9+

## Roadmap

Future enhancements being considered:

- [ ] BLE encryption and pairing
- [ ] Multiple credential storage (store multiple networks)
- [ ] Network priority management
- [ ] Connection diagnostics and logs
- [ ] OTA firmware updates over BLE
- [ ] Android SDK
- [ ] Web-based configuration fallback
- [ ] IPv6 support
- [ ] Enterprise WiFi (802.1X) support

## Contributing

Contributions are welcome! Please feel free to submit pull requests or open issues for bugs and feature requests.

## License

MIT License

Copyright (c) 2025 Matt Long

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

## Acknowledgments

- ESP32 Arduino framework by Espressif Systems
- Apple CoreBluetooth framework
- SwiftUI framework

## Support

For questions, issues, or feature requests:
- Open an issue on GitHub
- Check the [PROTOCOL.md](PROTOCOL.md) for protocol details
- See platform-specific READMEs for detailed documentation

## Security

This version uses unencrypted BLE communication, which is acceptable for local device configuration scenarios. Users should:
- Verify device identity before sending credentials
- Use in controlled environments
- Be aware that WiFi passwords are transmitted in plain text over BLE

Credentials are encrypted at rest (Keychain on iOS, NVS on ESP32).

Future versions may include BLE encryption and pairing requirements for enhanced security.
