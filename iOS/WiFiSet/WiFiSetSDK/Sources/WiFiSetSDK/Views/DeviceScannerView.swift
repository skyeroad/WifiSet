import SwiftUI

/// View for scanning and selecting ESP32 devices
public struct DeviceScannerView: View {
    @StateObject private var bleManager = BLEManager()
    @State private var selectedDevice: BLEPeripheral?
    @State private var showError = false
    @State private var errorMessage = ""

    public init() {}

    public var body: some View {
        NavigationStack {
            ZStack {
                if discoveredDevices.isEmpty && !bleManager.isScanning {
                    emptyStateView
                } else {
                    deviceListView
                }
            }
            .navigationTitle("WiFi Setup")
            .navigationBarTitleDisplayMode(.large)
            .toolbar {
                ToolbarItem(placement: .navigationBarTrailing) {
                    scanButton
                }
            }
            .navigationDestination(item: $selectedDevice) { device in
                WiFiNetworkListView(bleManager: bleManager, device: device)
            }
            .alert("Error", isPresented: $showError) {
                Button("OK") { }
            } message: {
                Text(errorMessage)
            }
            .onAppear {
                setupBLECallbacks()
                if bleManager.bluetoothState == .poweredOn {
                    bleManager.startScanning()
                }
            }
            .onDisappear {
                bleManager.stopScanning()
            }
        }
    }

    // MARK: - Subviews

    private var discoveredDevices: [BLEPeripheral] {
        bleManager.discoveredDevices.sorted { $0.rssi > $1.rssi }
    }

    private var emptyStateView: some View {
        VStack(spacing: 20) {
            Image(systemName: "antenna.radiowaves.left.and.right")
                .font(.system(size: 60))
                .foregroundColor(.secondary)

            Text("No Devices Found")
                .font(.title2)
                .fontWeight(.semibold)

            Text("Make sure your ESP32 is powered on and in range")
                .font(.body)
                .foregroundColor(.secondary)
                .multilineTextAlignment(.center)
                .padding(.horizontal)

            if bleManager.bluetoothState != .poweredOn {
                bluetoothStateMessage
            }

            Button(action: {
                bleManager.startScanning()
            }) {
                Label("Start Scanning", systemImage: "magnifyingglass")
                    .font(.headline)
            }
            .buttonStyle(.borderedProminent)
            .disabled(bleManager.bluetoothState != .poweredOn)
        }
        .padding()
    }

    @ViewBuilder
    private var bluetoothStateMessage: some View {
        switch bleManager.bluetoothState {
        case .poweredOff:
            Label("Bluetooth is off. Please enable Bluetooth.", systemImage: "exclamationmark.triangle")
                .foregroundColor(.orange)
        case .unauthorized:
            Label("Bluetooth access not authorized. Check Settings.", systemImage: "exclamationmark.triangle")
                .foregroundColor(.orange)
        case .unsupported:
            Label("Bluetooth is not supported on this device.", systemImage: "exclamationmark.triangle")
                .foregroundColor(.red)
        default:
            EmptyView()
        }
    }

    private var deviceListView: some View {
        List {
            if bleManager.isScanning {
                Section {
                    HStack {
                        ProgressView()
                        Text("Scanning for devices...")
                            .foregroundColor(.secondary)
                    }
                }
            }

            if !discoveredDevices.isEmpty {
                Section("Available Devices") {
                    ForEach(discoveredDevices) { device in
                        DeviceRow(device: device)
                            .contentShape(Rectangle())
                            .onTapGesture {
                                selectedDevice = device
                            }
                    }
                }
            }
        }
        .listStyle(.insetGrouped)
    }

    private var scanButton: some View {
        Button(action: {
            if bleManager.isScanning {
                bleManager.stopScanning()
            } else {
                bleManager.startScanning()
            }
        }) {
            Text(bleManager.isScanning ? "Stop" : "Scan")
                .fontWeight(.semibold)
        }
        .disabled(bleManager.bluetoothState != .poweredOn)
    }

    // MARK: - Setup

    private func setupBLECallbacks() {
        bleManager.onError = { error in
            errorMessage = error.localizedDescription
            showError = true
        }
    }
}

// MARK: - Device Row

private struct DeviceRow: View {
    @ObservedObject var device: BLEPeripheral

    var body: some View {
        HStack {
            VStack(alignment: .leading, spacing: 4) {
                Text(device.name)
                    .font(.headline)

                Text("\(device.signalQuality) Signal")
                    .font(.caption)
                    .foregroundColor(.secondary)
            }

            Spacer()

            VStack(alignment: .trailing, spacing: 4) {
                signalIcon

                Text("\(device.rssi) dBm")
                    .font(.caption2)
                    .foregroundColor(.secondary)
            }
        }
        .padding(.vertical, 4)
    }

    private var signalIcon: some View {
        HStack(spacing: 2) {
            ForEach(0..<4) { index in
                Rectangle()
                    .fill(index < device.signalStrength ? Color.blue : Color.gray.opacity(0.3))
                    .frame(width: 4, height: CGFloat(8 + index * 3))
            }
        }
    }
}

// MARK: - Preview

#Preview {
    DeviceScannerView()
}
