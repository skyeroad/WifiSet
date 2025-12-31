import SwiftUI

/// View for entering WiFi credentials
public struct WiFiCredentialInputView: View {
    let network: WiFiNetwork
    let bleManager: BLEManager
    let onSubmit: (String, String) -> Void

    @Environment(\.dismiss) private var dismiss
    @State private var password = ""
    @State private var saveToKeychain = true
    @State private var isSubmitting = false
    @State private var showError = false
    @State private var errorMessage = ""
    @State private var showPassword = false

    private let keychainManager = KeychainManager()

    public init(network: WiFiNetwork, bleManager: BLEManager, onSubmit: @escaping (String, String) -> Void) {
        self.network = network
        self.bleManager = bleManager
        self.onSubmit = onSubmit
    }

    public var body: some View {
        NavigationStack {
            Form {
                networkInfoSection

                if network.securityType.requiresPassword {
                    credentialsSection
                }

                statusSection

                submitSection
            }
            .navigationTitle("Configure WiFi")
            .navigationBarTitleDisplayMode(.inline)
            .toolbar {
                ToolbarItem(placement: .cancellationAction) {
                    Button("Cancel") {
                        dismiss()
                    }
                    .disabled(isSubmitting)
                }
            }
            .alert("Error", isPresented: $showError) {
                Button("OK") { }
            } message: {
                Text(errorMessage)
            }
            .onAppear {
                loadSavedPassword()
            }
            .interactiveDismissDisabled(isSubmitting)
        }
    }

    // MARK: - Sections

    private var networkInfoSection: some View {
        Section {
            LabeledContent("SSID", value: network.ssid)
            LabeledContent("Security", value: network.securityType.displayName)
            LabeledContent("Signal") {
                HStack(spacing: 4) {
                    signalBars
                    Text(network.signalQuality)
                        .font(.caption)
                        .foregroundColor(.secondary)
                }
            }
            LabeledContent("Channel", value: String(network.channel))
        } header: {
            Text("Network Information")
        }
    }

    private var credentialsSection: some View {
        Section {
            HStack {
                if showPassword {
                    TextField("Password", text: $password)
                        .textContentType(.password)
                        .autocapitalization(.none)
                        .autocorrectionDisabled()
                } else {
                    SecureField("Password", text: $password)
                        .textContentType(.password)
                        .autocapitalization(.none)
                        .autocorrectionDisabled()
                }

                Button(action: { showPassword.toggle() }) {
                    Image(systemName: showPassword ? "eye.slash.fill" : "eye.fill")
                        .foregroundColor(.secondary)
                }
                .buttonStyle(.plain)
            }

            Toggle("Save to Keychain", isOn: $saveToKeychain)
        } header: {
            Text("Credentials")
        } footer: {
            if saveToKeychain {
                Text("The password will be securely stored in your device's Keychain for future use.")
            }
        }
    }

    private var statusSection: some View {
        Section {
            if isSubmitting {
                HStack {
                    ProgressView()
                    Text("Sending credentials to ESP32...")
                        .foregroundColor(.secondary)
                }
            }
        }
    }

    private var submitSection: some View {
        Section {
            Button(action: submit) {
                HStack {
                    Spacer()
                    Text("Connect")
                        .fontWeight(.semibold)
                    Spacer()
                }
            }
            .disabled(isSubmitDisabled)
        }
    }

    private var signalBars: some View {
        HStack(spacing: 2) {
            ForEach(0..<4) { index in
                Rectangle()
                    .fill(index < network.signalStrength ? signalColor : Color.gray.opacity(0.3))
                    .frame(width: 3, height: CGFloat(6 + index * 2))
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

    private var isSubmitDisabled: Bool {
        isSubmitting || (network.securityType.requiresPassword && password.isEmpty)
    }

    // MARK: - Actions

    private func loadSavedPassword() {
        if let savedPassword = try? keychainManager.loadPassword(for: network.ssid) {
            password = savedPassword
        }
    }

    private func submit() {
        isSubmitting = true

        // Save to keychain if requested
        if saveToKeychain && network.securityType.requiresPassword && !password.isEmpty {
            do {
                try keychainManager.saveCredentials(ssid: network.ssid, password: password)
            } catch {
                // Don't fail the submission if keychain save fails, just log
                print("Failed to save to keychain: \(error)")
            }
        }

        // Send credentials to ESP32
        bleManager.sendCredentials(ssid: network.ssid, password: password)

        // Wait a moment for the write to complete, then show status
        DispatchQueue.main.asyncAfter(deadline: .now() + 1.0) {
            isSubmitting = false
            onSubmit(network.ssid, password)

            // Navigate to status view
            DispatchQueue.main.asyncAfter(deadline: .now() + 0.5) {
                dismiss()
            }
        }
    }
}

// MARK: - Preview

#Preview {
    WiFiCredentialInputView(
        network: WiFiNetwork(
            ssid: "MyHomeNetwork",
            rssi: -45,
            securityType: .wpaPsk,
            channel: 6
        ),
        bleManager: BLEManager(),
        onSubmit: { _, _ in }
    )
}
