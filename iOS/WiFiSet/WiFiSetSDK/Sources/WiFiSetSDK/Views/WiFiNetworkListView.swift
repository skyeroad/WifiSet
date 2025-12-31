import SwiftUI
import CoreBluetooth

/// View for displaying and selecting WiFi networks from ESP32
public struct WiFiNetworkListView: View {
    @ObservedObject var bleManager: BLEManager
    let device: BLEPeripheral

    @State private var networks: [WiFiNetwork] = []
    @State private var selectedNetwork: WiFiNetwork?
    @State private var isLoading = true
    @State private var showError = false
    @State private var errorMessage = ""
    @Environment(\.dismiss) private var dismiss

    public init(bleManager: BLEManager, device: BLEPeripheral) {
        self.bleManager = bleManager
        self.device = device
    }

    public var body: some View {
        ZStack {
            if isLoading {
                loadingView
            } else if networks.isEmpty {
                emptyStateView
            } else {
                networkListView
            }
        }
        .navigationTitle("Select Network")
        .navigationBarTitleDisplayMode(.inline)
        .navigationBarBackButtonHidden(bleManager.connectedDevice != nil)
        .toolbar {
            if bleManager.connectedDevice != nil {
                ToolbarItem(placement: .navigationBarLeading) {
                    Button("Disconnect") {
                        bleManager.disconnect()
                        dismiss()
                    }
                }
            }
        }
        .sheet(item: $selectedNetwork) { network in
            WiFiCredentialInputView(
                network: network,
                bleManager: bleManager,
                onSubmit: { ssid, password in
                    selectedNetwork = nil
                }
            )
        }
        .alert("Error", isPresented: $showError) {
            Button("OK") { }
        } message: {
            Text(errorMessage)
        }
        .onAppear {
            setupCallbacks()
            bleManager.connect(to: device)
        }
    }

    // MARK: - Subviews

    private var loadingView: some View {
        VStack(spacing: 20) {
            ProgressView()
                .scaleEffect(1.5)

            Text("Connecting to \(device.name)...")
                .font(.headline)
                .foregroundColor(.secondary)

            Text("Scanning for WiFi networks")
                .font(.subheadline)
                .foregroundColor(.secondary)
        }
    }

    private var emptyStateView: some View {
        VStack(spacing: 20) {
            Image(systemName: "wifi.exclamationmark")
                .font(.system(size: 60))
                .foregroundColor(.secondary)

            Text("No Networks Found")
                .font(.title2)
                .fontWeight(.semibold)

            Text("The ESP32 couldn't find any WiFi networks")
                .font(.body)
                .foregroundColor(.secondary)
                .multilineTextAlignment(.center)

            Button("Try Again") {
                bleManager.disconnect()
                DispatchQueue.main.asyncAfter(deadline: .now() + 0.5) {
                    bleManager.connect(to: device)
                }
            }
            .buttonStyle(.borderedProminent)
        }
        .padding()
    }

    private var networkListView: some View {
        List {
            // Show current WiFi status if ESP32 has credentials configured
            if let status = bleManager.deviceStatus,
               status.connectionState != .notConfigured {
                Section {
                    CurrentStatusRow(status: status)
                } header: {
                    Text("Current Connection")
                } footer: {
                    if status.connectionState == .connected {
                        Text("Select a different network below to change WiFi settings")
                    } else if status.connectionState == .configuredNotConnected {
                        Text("Credentials saved but not connected. Select a network to reconfigure.")
                    }
                }
            }

            Section {
                ForEach(sortedNetworks) { network in
                    NetworkRow(network: network)
                        .contentShape(Rectangle())
                        .onTapGesture {
                            selectedNetwork = network
                        }
                }
            } header: {
                Text("Available Networks")
            } footer: {
                Text("Found \(networks.count) network\(networks.count == 1 ? "" : "s")")
            }
        }
        .listStyle(.insetGrouped)
    }

    private var sortedNetworks: [WiFiNetwork] {
        networks.sorted { $0.rssi > $1.rssi }
    }

    // MARK: - Setup

    private func setupCallbacks() {
        bleManager.onWiFiNetworksReceived = { receivedNetworks in
            DispatchQueue.main.async {
                self.networks = receivedNetworks
                self.isLoading = false
            }
        }

        bleManager.onError = { error in
            DispatchQueue.main.async {
                self.errorMessage = error.localizedDescription
                self.showError = true
                self.isLoading = false
            }
        }

        bleManager.onConnectionStateChanged = { isConnected in
            if !isConnected {
                DispatchQueue.main.async {
                    self.isLoading = true
                }
            }
        }
    }
}

// MARK: - Current Status Row

private struct CurrentStatusRow: View {
    let status: DeviceStatus

    var body: some View {
        VStack(alignment: .leading, spacing: 8) {
            HStack {
                Image(systemName: statusIcon)
                    .font(.title2)
                    .foregroundColor(statusColor)

                VStack(alignment: .leading, spacing: 2) {
                    Text(status.ssid.isEmpty ? "Unknown Network" : status.ssid)
                        .font(.headline)

                    Text(statusText)
                        .font(.subheadline)
                        .foregroundColor(statusColor)
                }

                Spacer()

                if status.connectionState == .connected {
                    VStack(alignment: .trailing, spacing: 2) {
                        Text(status.ipAddress)
                            .font(.caption)
                            .foregroundColor(.secondary)

                        Text("\(status.rssi) dBm")
                            .font(.caption2)
                            .foregroundColor(.secondary)
                    }
                }
            }
        }
        .padding(.vertical, 4)
    }

    private var statusIcon: String {
        switch status.connectionState {
        case .connected:
            return "wifi"
        case .connecting:
            return "wifi.exclamationmark"
        case .configuredNotConnected:
            return "wifi.slash"
        case .connectionFailed:
            return "xmark.circle"
        case .notConfigured:
            return "questionmark.circle"
        }
    }

    private var statusColor: Color {
        switch status.connectionState {
        case .connected:
            return .green
        case .connecting:
            return .orange
        case .configuredNotConnected:
            return .yellow
        case .connectionFailed:
            return .red
        case .notConfigured:
            return .secondary
        }
    }

    private var statusText: String {
        switch status.connectionState {
        case .connected:
            return "Connected"
        case .connecting:
            return "Connecting..."
        case .configuredNotConnected:
            return "Not Connected"
        case .connectionFailed:
            return "Connection Failed"
        case .notConfigured:
            return "Not Configured"
        }
    }
}

// MARK: - Network Row

private struct NetworkRow: View {
    let network: WiFiNetwork

    var body: some View {
        HStack {
            VStack(alignment: .leading, spacing: 4) {
                Text(network.ssid)
                    .font(.headline)

                HStack(spacing: 8) {
                    Label(network.securityType.displayName, systemImage: network.securityType.iconName)
                        .font(.caption)
                        .foregroundColor(.secondary)

                    Text("Ch \(network.channel)")
                        .font(.caption)
                        .foregroundColor(.secondary)
                }
            }

            Spacer()

            VStack(alignment: .trailing, spacing: 4) {
                signalIcon

                Text(network.signalQuality)
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
                    .fill(index < network.signalStrength ? signalColor : Color.gray.opacity(0.3))
                    .frame(width: 4, height: CGFloat(8 + index * 3))
            }
        }
    }

    private var signalColor: Color {
        switch network.signalStrength {
        case 4: return .green
        case 3: return .green
        case 2: return .yellow
        case 1: return .orange
        default: return .red
        }
    }
}

// MARK: - Preview
// Note: Preview disabled because CBPeripheral cannot be initialized directly
// Test this view with a real ESP32 device on physical hardware
