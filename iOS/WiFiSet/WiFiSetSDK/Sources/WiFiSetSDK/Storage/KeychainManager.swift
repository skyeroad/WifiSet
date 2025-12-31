import Foundation
import Security

/// Manages secure storage of WiFi credentials in the iOS Keychain
public class KeychainManager {
    private let service = "com.skyeroad.WiFiSet"

    public init() {}

    // MARK: - Public Methods

    /// Save WiFi credentials to Keychain
    /// - Parameters:
    ///   - ssid: WiFi network name
    ///   - password: WiFi password
    /// - Throws: KeychainError if save fails
    public func saveCredentials(ssid: String, password: String) throws {
        let account = "wifi_\(ssid)"

        guard let passwordData = password.data(using: .utf8) else {
            throw KeychainError.encodingFailed
        }

        // Delete existing item first (to avoid duplicate errors)
        try? deleteCredentials(for: ssid)

        // Prepare query
        let query: [String: Any] = [
            kSecClass as String: kSecClassGenericPassword,
            kSecAttrService as String: service,
            kSecAttrAccount as String: account,
            kSecValueData as String: passwordData,
            kSecAttrAccessible as String: kSecAttrAccessibleWhenUnlocked
        ]

        // Add to keychain
        let status = SecItemAdd(query as CFDictionary, nil)

        guard status == errSecSuccess else {
            throw KeychainError.saveFailed(status)
        }
    }

    /// Load password for a specific WiFi network
    /// - Parameter ssid: WiFi network name
    /// - Returns: Password if found, nil otherwise
    /// - Throws: KeychainError if load fails (except for "not found")
    public func loadPassword(for ssid: String) throws -> String? {
        let account = "wifi_\(ssid)"

        let query: [String: Any] = [
            kSecClass as String: kSecClassGenericPassword,
            kSecAttrService as String: service,
            kSecAttrAccount as String: account,
            kSecReturnData as String: true,
            kSecMatchLimit as String: kSecMatchLimitOne
        ]

        var result: AnyObject?
        let status = SecItemCopyMatching(query as CFDictionary, &result)

        // Item not found is not an error, just return nil
        if status == errSecItemNotFound {
            return nil
        }

        guard status == errSecSuccess else {
            throw KeychainError.loadFailed(status)
        }

        guard let data = result as? Data,
              let password = String(data: data, encoding: .utf8) else {
            throw KeychainError.decodingFailed
        }

        return password
    }

    /// Delete credentials for a specific WiFi network
    /// - Parameter ssid: WiFi network name
    /// - Throws: KeychainError if delete fails (except for "not found")
    public func deleteCredentials(for ssid: String) throws {
        let account = "wifi_\(ssid)"

        let query: [String: Any] = [
            kSecClass as String: kSecClassGenericPassword,
            kSecAttrService as String: service,
            kSecAttrAccount as String: account
        ]

        let status = SecItemDelete(query as CFDictionary)

        // Not found is acceptable
        guard status == errSecSuccess || status == errSecItemNotFound else {
            throw KeychainError.deleteFailed(status)
        }
    }

    /// Check if credentials exist for a specific WiFi network
    /// - Parameter ssid: WiFi network name
    /// - Returns: true if credentials exist
    public func hasCredentials(for ssid: String) -> Bool {
        return (try? loadPassword(for: ssid)) != nil
    }

    /// Delete all WiFi credentials
    /// - Throws: KeychainError if delete fails
    public func deleteAllCredentials() throws {
        let query: [String: Any] = [
            kSecClass as String: kSecClassGenericPassword,
            kSecAttrService as String: service
        ]

        let status = SecItemDelete(query as CFDictionary)

        // Not found is acceptable
        guard status == errSecSuccess || status == errSecItemNotFound else {
            throw KeychainError.deleteFailed(status)
        }
    }
}

// MARK: - Keychain Errors

public enum KeychainError: LocalizedError {
    case saveFailed(OSStatus)
    case loadFailed(OSStatus)
    case deleteFailed(OSStatus)
    case encodingFailed
    case decodingFailed

    public var errorDescription: String? {
        switch self {
        case .saveFailed(let status):
            return "Failed to save to Keychain (status: \(status))"
        case .loadFailed(let status):
            return "Failed to load from Keychain (status: \(status))"
        case .deleteFailed(let status):
            return "Failed to delete from Keychain (status: \(status))"
        case .encodingFailed:
            return "Failed to encode password"
        case .decodingFailed:
            return "Failed to decode password"
        }
    }

    public var recoverySuggestion: String? {
        switch self {
        case .saveFailed, .loadFailed, .deleteFailed:
            return "Please check that the app has Keychain access permissions."
        case .encodingFailed, .decodingFailed:
            return "The password contains invalid characters."
        }
    }
}
