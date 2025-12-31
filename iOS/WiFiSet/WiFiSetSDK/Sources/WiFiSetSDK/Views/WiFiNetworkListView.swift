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
