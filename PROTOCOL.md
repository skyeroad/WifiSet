# WiFiSet BLE Protocol Specification

Version 1.0

## Overview

This document specifies the binary communication protocol used between the ESP32 WiFi configuration library and iOS SDK over Bluetooth Low Energy (BLE). The protocol uses a custom binary format for efficiency over BLE's limited bandwidth.

## BLE Service and Characteristics

### Service

**WiFiSet Service UUID**: `4FAFC201-1FB5-459E-8FCC-C5C9C331914B`

### Characteristics

| Characteristic | UUID | Properties | Description |
|---------------|------|------------|-------------|
| WiFi List | `4FAFC202-1FB5-459E-8FCC-C5C9C331914B` | READ, NOTIFY | ESP32 sends WiFi network list to iOS |
| Credential Write | `4FAFC203-1FB5-459E-8FCC-C5C9C331914B` | WRITE | iOS sends WiFi credentials to ESP32 |
| Status | `4FAFC204-1FB5-459E-8FCC-C5C9C331914B` | READ, NOTIFY | ESP32 sends connection status to iOS |

## Binary Protocol Format

All multi-byte integers use **little-endian** byte order.

### Message Header

Every message begins with a 4-byte header:

```
Byte 0: Message Type (uint8)
Byte 1: Sequence Number (uint8)
Bytes 2-3: Payload Length (uint16, little-endian)
```

**Message Type**: Identifies the type of message (see Message Types below)
**Sequence Number**: Incremental counter for tracking packets (0-255, wraps around)
**Payload Length**: Number of bytes following the header

### Message Types

| Type | Value | Direction | Description |
|------|-------|-----------|-------------|
| WiFi List Start | `0x01` | ESP32 → iOS | Indicates start of WiFi network list |
| WiFi Network Entry | `0x02` | ESP32 → iOS | Single WiFi network information |
| WiFi List End | `0x03` | ESP32 → iOS | Indicates end of WiFi network list |
| Credential Write | `0x10` | iOS → ESP32 | WiFi credentials (SSID + password) |
| Credential Write ACK | `0x11` | ESP32 → iOS | Acknowledgment of credential receipt |
| Status Request | `0x20` | iOS → ESP32 | Request current connection status |
| Status Response | `0x21` | ESP32 → iOS | Current connection status |
| Error | `0xFF` | ESP32 → iOS | Error message |

## Message Formats

### WiFi List Start (0x01)

Sent by ESP32 when a client connects and before WiFi scan begins.

```
Header (4 bytes):
  Message Type: 0x01
  Sequence Number: <counter>
  Payload Length: 0
```

No payload.

### WiFi Network Entry (0x02)

Sent by ESP32 for each discovered WiFi network.

```
Header (4 bytes):
  Message Type: 0x02
  Sequence Number: <counter>
  Payload Length: <variable>

Payload:
  SSID Length (1 byte): N (0-32)
  SSID (N bytes): UTF-8 encoded network name
  RSSI (1 byte): Signed int8, signal strength (-128 to 127 dBm)
  Security Type (1 byte): Encryption type (see Security Types)
  Channel (1 byte): WiFi channel number (1-14)
```

**Security Types:**
- `0x00`: Open (no encryption)
- `0x01`: WEP
- `0x02`: WPA/WPA2-PSK
- `0x03`: WPA2-Enterprise
- `0x04`: WPA3

**SSID Length**: Must be 0-32 bytes (WiFi standard maximum)
**RSSI**: Typical values range from -30 (excellent) to -90 (poor)

### WiFi List End (0x03)

Sent by ESP32 after all networks have been transmitted.

```
Header (4 bytes):
  Message Type: 0x03
  Sequence Number: <counter>
  Payload Length: 1

Payload:
  Network Count (1 byte): Total number of networks sent
```

### Credential Write (0x10)

Sent by iOS to configure WiFi credentials on ESP32.

```
Header (4 bytes):
  Message Type: 0x10
  Sequence Number: <counter>
  Payload Length: <variable>

Payload:
  SSID Length (1 byte): N (1-32)
  SSID (N bytes): UTF-8 encoded network name
  Password Length (1 byte): M (0-63)
  Password (M bytes): UTF-8 encoded password
```

**SSID Length**: Must be 1-32 bytes
**Password Length**: 0-63 bytes (0 for open networks)
**Password**: Empty (length 0) for open networks

### Credential Write Acknowledgment (0x11)

Sent by ESP32 to confirm credential receipt and storage.

```
Header (4 bytes):
  Message Type: 0x11
  Sequence Number: <counter>
  Payload Length: 1

Payload:
  Status Code (1 byte):
    0x00: Success - credentials stored
    0x01: Invalid SSID length
    0x02: Invalid password length
    0x03: Storage failure
```

### Status Request (0x20)

Sent by iOS to request current connection status (optional - status also sent via NOTIFY).

```
Header (4 bytes):
  Message Type: 0x20
  Sequence Number: <counter>
  Payload Length: 0
```

No payload.

### Status Response (0x21)

Sent by ESP32 in response to status request or automatically when connection state changes.

```
Header (4 bytes):
  Message Type: 0x21
  Sequence Number: <counter>
  Payload Length: <variable>

Payload:
  Connection State (1 byte): Current WiFi connection state (see Connection States)
  RSSI (1 byte): Signed int8, current WiFi RSSI (0 if not connected)
  IP Address (4 bytes): IPv4 address in network byte order (0.0.0.0 if not connected)
  SSID Length (1 byte): N (0-32)
  SSID (N bytes): UTF-8 encoded currently configured network name
```

**Connection States:**
- `0x00`: Not Configured - No credentials stored
- `0x01`: Configured Not Connected - Credentials stored but not connected
- `0x02`: Connecting - Attempting to connect to WiFi
- `0x03`: Connected - Successfully connected to WiFi
- `0x04`: Connection Failed - Failed to connect (wrong password, network not found, etc.)

**IP Address**: Stored as 4 bytes in network byte order (big-endian). Example: 192.168.1.100 = `0xC0 0xA8 0x01 0x64`

### Error (0xFF)

Sent by ESP32 when an error occurs.

```
Header (4 bytes):
  Message Type: 0xFF
  Sequence Number: <counter>
  Payload Length: <variable>

Payload:
  Error Code (1 byte): Error type (see Error Codes)
  Error Message Length (1 byte): N (0-255)
  Error Message (N bytes): UTF-8 encoded human-readable error description
```

**Error Codes:**
- `0x01`: Invalid Message Format
- `0x02`: WiFi Scan Failed
- `0x03`: Credential Write Failed
- `0x04`: Storage Error
- `0x05`: Connection Timeout
- `0x06`: Unknown Message Type

## Protocol Flow

### Initial Connection and WiFi List Transfer

```
1. iOS app connects to ESP32 via BLE
2. iOS subscribes to WiFi List characteristic (NOTIFY)
3. ESP32 sends: WiFi List Start (0x01)
4. ESP32 performs WiFi scan
5. ESP32 sends: WiFi Network Entry (0x02) for each network found
6. ESP32 sends: WiFi List End (0x03) with total count
```

### WiFi Credential Configuration

```
1. User selects network in iOS app
2. User enters password (if required)
3. iOS sends: Credential Write (0x10) with SSID and password
4. ESP32 validates and stores credentials in NVS
5. ESP32 sends: Credential Write ACK (0x11) with status
6. ESP32 attempts to connect to WiFi
7. ESP32 sends: Status Response (0x21) updates as connection progresses:
   - Connecting (0x02)
   - Connected (0x03) OR Connection Failed (0x04)
```

### Status Monitoring

```
1. iOS subscribes to Status characteristic (NOTIFY)
2. ESP32 automatically sends Status Response (0x21) when connection state changes
3. iOS can also explicitly request status via Status Request (0x20)
```

## MTU and Packet Size Considerations

### MTU Negotiation
- iOS should request maximum MTU (512 bytes) during connection
- ESP32 should accept MTU negotiation up to 512 bytes
- Minimum supported MTU: 23 bytes (20 bytes effective after ATT header)

### Maximum Message Sizes
With minimum MTU (20 bytes effective):
- WiFi List Start: 4 bytes (fits)
- WiFi Network Entry: ~40 bytes (may require MTU > 23)
- Credential Write: ~70 bytes (may require MTU > 23)
- Status Response: ~45 bytes (may require MTU > 23)

**Implementation Note**: Ensure MTU is negotiated to at least 128 bytes for reliable operation. Most modern devices support 512 bytes.

### Chunking
If messages exceed MTU:
- Not typically required with proper MTU negotiation
- If needed, use sequence numbers to detect packet loss
- Receiver should handle receiving notifications incrementally

## Sequence Number Handling

- Each endpoint maintains its own sequence counter
- Sequence number increments for each message sent (0-255, wraps to 0)
- Receiver can use sequence numbers to detect:
  - Packet loss (gap in sequence)
  - Duplicate packets (repeated sequence)
  - Out-of-order delivery (decreasing sequence after wrap-around)

**Implementation Note**: Basic implementations may ignore sequence numbers. They are provided for future reliability enhancements.

## Error Handling

### iOS Client
- If WiFi List End not received within 30 seconds, timeout and retry
- If Credential Write ACK not received within 5 seconds, retry
- If Status indicates Connection Failed, allow user to re-enter credentials
- Handle BLE disconnections gracefully (show UI feedback)

### ESP32 Server
- If invalid message received, send Error (0xFF) with appropriate error code
- If WiFi scan fails, send Error (0xFF) with code 0x02
- If credential storage fails, send Credential Write ACK with error status
- WiFi connection timeout: 30 seconds, then send Status Response with Connection Failed

## Security Considerations

### Unencrypted Communication
- This protocol version uses unencrypted BLE communication
- WiFi passwords are transmitted in plain text over BLE
- Acceptable for local device configuration scenarios
- Users should ensure they are connecting to the correct device

### Recommendations
- Verify device identity before sending credentials (check device name, MAC address)
- Use in controlled environments (not public spaces)
- Consider adding device pairing in future versions
- Credentials are encrypted at rest (Keychain on iOS, NVS on ESP32)

## Versioning

**Current Version**: 1.0

### Version History
- 1.0 (2025-12-30): Initial protocol specification

### Future Considerations
- Add protocol version field to header
- Add message authentication codes (MAC)
- Add BLE pairing requirement
- Support for multiple credential storage
- Support for WiFi network priority
- Support for advanced WiFi settings (static IP, DNS, etc.)

## Example Message Sequences

### Example 1: WiFi Network Entry

```
Hex: 02 05 28 00 0C 4D 79 4E 65 74 77 6F 72 6B 32 2E 34 C8 02 06
```

Breakdown:
- `02`: Message Type = WiFi Network Entry
- `05`: Sequence Number = 5
- `28 00`: Payload Length = 40 bytes (little-endian)
- `0C`: SSID Length = 12
- `4D 79 ... 34`: SSID = "MyNetwork2.4" (UTF-8)
- `C8`: RSSI = -56 dBm (signed)
- `02`: Security Type = WPA/WPA2-PSK
- `06`: Channel = 6

### Example 2: Credential Write

```
Hex: 10 01 20 00 0C 4D 79 4E 65 74 77 6F 72 6B 32 2E 34 0C 6D 79 70 61 73 73 77 6F 72 64 31 32 33
```

Breakdown:
- `10`: Message Type = Credential Write
- `01`: Sequence Number = 1
- `20 00`: Payload Length = 32 bytes (little-endian)
- `0C`: SSID Length = 12
- `4D 79 ... 34`: SSID = "MyNetwork2.4"
- `0C`: Password Length = 12
- `6D 79 ... 33`: Password = "mypassword123"

### Example 3: Status Response (Connected)

```
Hex: 21 0A 16 00 03 C8 C0 A8 01 64 0C 4D 79 4E 65 74 77 6F 72 6B 32 2E 34
```

Breakdown:
- `21`: Message Type = Status Response
- `0A`: Sequence Number = 10
- `16 00`: Payload Length = 22 bytes
- `03`: Connection State = Connected
- `C8`: RSSI = -56 dBm
- `C0 A8 01 64`: IP Address = 192.168.1.100
- `0C`: SSID Length = 12
- `4D 79 ... 34`: SSID = "MyNetwork2.4"

## Implementation Checklist

### ESP32 Implementation
- [ ] Implement message header encoding/decoding
- [ ] Implement all message builders (0x01-0x03, 0x11, 0x21, 0xFF)
- [ ] Implement Credential Write parser (0x10)
- [ ] Handle WiFi scan and network list generation
- [ ] Implement NVS credential storage
- [ ] Implement WiFi connection management
- [ ] Send Status Response on connection state changes
- [ ] Handle MTU negotiation
- [ ] Implement error handling and Error messages

### iOS Implementation
- [ ] Implement message header encoding/decoding
- [ ] Implement WiFi Network Entry parser (0x02)
- [ ] Implement WiFi List End parser (0x03)
- [ ] Implement Credential Write builder (0x10)
- [ ] Implement Status Response parser (0x21)
- [ ] Implement Error parser (0xFF)
- [ ] Handle BLE connection and characteristic subscription
- [ ] Request maximum MTU during connection
- [ ] Implement timeout handling for network list and ACK
- [ ] Display connection status to user
- [ ] Store credentials in Keychain

## References

- [Bluetooth Core Specification](https://www.bluetooth.com/specifications/bluetooth-core-specification/)
- [IEEE 802.11 WiFi Standards](https://standards.ieee.org/standard/802_11-2020.html)
- [ESP32 NVS Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/storage/nvs_flash.html)
- [Apple CoreBluetooth Framework](https://developer.apple.com/documentation/corebluetooth)
