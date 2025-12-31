import SwiftUI

/// View for monitoring ESP32 WiFi connection status
public struct ConnectionStatusView: View {
    @ObservedObject var bleManager: BLEManager
    @State private var status: DeviceStatus?
    @State private var showError = false
    @State private var errorMessage = ""
    @Environment(\.dismiss) private var dismiss

    public init(bleManager: BLEManager) {
        self.bleManager = bleManager
    }

    public var body: some View {
        NavigationStack {
            List {
                if let status = status {
                    connectionStateSection(status)
                    networkInfoSection(status)
                } else {
                    loadingSection
                }
            }
            .listStyle(.insetGrouped)
            .navigationTitle("Connection Status")
            .navigationBarTitleDisplayMode(.inline)
            .toolbar {
                ToolbarItem(placement: .navigationBarTrailing) {
                    Button("Done") {
                        dismiss()
                    }
                }

                ToolbarItem(placement: .navigationBarLeading) {
                    Button(action: refreshStatus) {
                        Image(systemName: "arrow.clockwise")
                    }
                }
            }
            .alert("Error", isPresented: $showError) {
                Button("OK") { }
            } message: {
                Text(errorMessage)
            }
            .onAppear {
                setupCallbacks()
                refreshStatus()
            }
        }
    }

    // MARK: - Sections

    private var loadingSection: some View {
        Section {
            HStack {
                ProgressView()
                Text("Loading status...")
                    .foregroundColor(.secondary)
            }
        }
    }

    private func connectionStateSection(_ status: DeviceStatus) -> some View {
        Section {
            HStack {
                Image(systemName: status.connectionState.iconName)
                    .foregroundColor(statusColor(for: status.connectionState))
                    .font(.title2)

                VStack(alignment: .leading, spacing: 4) {
                    Text(status.connectionState.description)
                        .font(.headline)

                    if status.connectionState == .connecting {
                        Text("Please wait...")
                            .font(.caption)
                            .foregroundColor(.secondary)
                    } else if status.connectionState == .connectionFailed {
                        Text("Check password and try again")
                            .font(.caption)
                            .foregroundColor(.red)
                    }
                }

                Spacer()

                if status.connectionState == .connected {
                    Image(systemName: "checkmark.circle.fill")
                        .foregroundColor(.green)
                        .font(.title2)
                }
            }
            .padding(.vertical, 8)
        } header: {
            Text("Status")
        }
    }

    private func networkInfoSection(_ status: DeviceStatus) -> some View {
        Section("Network Details") {
            if !status.ssid.isEmpty {
                LabeledContent("SSID", value: status.ssid)
            }

            if status.isConnected {
                if !status.ipAddress.isEmpty && status.ipAddress != "0.0.0.0" {
                    LabeledContent("IP Address", value: status.ipAddress)
                        .contextMenu {
                            Button(action: {
                                UIPasteboard.general.string = status.ipAddress
                            }) {
                                Label("Copy IP Address", systemImage: "doc.on.doc")
                            }
                        }
                }

                if status.rssi != 0 {
                    LabeledContent("Signal Strength") {
                        HStack(spacing: 4) {
                            Text("\(status.rssi) dBm")
                            signalQualityIndicator(for: status.rssi)
                        }
                    }
                }
            }
        }
    }

    // MARK: - Helpers

    private func statusColor(for state: ConnectionState) -> Color {
        switch state {
        case .notConfigured:
            return .orange
        case .configuredNotConnected:
            return .orange
        case .connecting:
            return .blue
        case .connected:
            return .green
        case .connectionFailed:
            return .red
        }
    }

    private func signalQualityIndicator(for rssi: Int8) -> some View {
        let signalStrength: Int
        if rssi >= -50 { signalStrength = 4 }
        else if rssi >= -60 { signalStrength = 3 }
        else if rssi >= -70 { signalStrength = 2 }
        else if rssi >= -80 { signalStrength = 1 }
        else { signalStrength = 0 }

        return HStack(spacing: 2) {
            ForEach(0..<4) { index in
                Rectangle()
                    .fill(index < signalStrength ? signalColor(for: signalStrength) : Color.gray.opacity(0.3))
                    .frame(width: 3, height: CGFloat(6 + index * 2))
            }
        }
    }

    private func signalColor(for strength: Int) -> Color {
        switch strength {
        case 4, 3: return .green
        case 2: return .yellow
        case 1: return .orange
        default: return .red
        }
    }

    private func refreshStatus() {
        bleManager.requestStatus()
    }

    private func setupCallbacks() {
        bleManager.onStatusReceived = { newStatus in
            DispatchQueue.main.async {
                self.status = newStatus
            }
        }

        bleManager.onError = { error in
            DispatchQueue.main.async {
                self.errorMessage = error.localizedDescription
                self.showError = true
            }
        }
    }
}

// MARK: - Preview

#Preview {
    ConnectionStatusView(bleManager: BLEManager())
}
