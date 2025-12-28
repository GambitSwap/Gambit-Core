#pragma once

#include "gambit/keys.hpp"
#include "gambit/address.hpp"
#include <map>
#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <cstdint>

namespace gambit {

/**
 * Wallet - BIP39/BIP32 hierarchical deterministic wallet with AES-256-GCM encryption
 * 
 * Features:
 * - BIP39: Mnemonic to seed conversion (12-24 word mnemonics)
 * - BIP32: Hierarchical key derivation (m/44'/60'/0'/0/0 paths)
 * - Encryption: AES-256-GCM with scrypt key derivation
 * - Persistence: JSON keystore format (Ethereum-compatible)
 * - Multi-account: Derive multiple accounts from single mnemonic
 */
class Wallet {
public:
    struct Account {
        std::string name;
        Address address;
        KeyPair keypair;
        std::string derivationPath;  // e.g., "m/44'/60'/0'/0/0"
    };

    // Create new wallet with random 12-word BIP39 mnemonic
    static std::shared_ptr<Wallet> create(const std::string& walletPath, const std::string& password);

    // Load wallet from encrypted keystore file
    static std::shared_ptr<Wallet> load(const std::string& walletPath, const std::string& password);

    // Import wallet from existing BIP39 mnemonic
    static std::shared_ptr<Wallet> importMnemonic(const std::string& mnemonic, 
                                                   const std::string& walletPath,
                                                   const std::string& password);

    // Add new account derived from mnemonic
    void addAccount(const std::string& name, const std::string& derivationPath = "m/44'/60'/0'/0/0");

    // Get account by address
    std::optional<Account> getAccount(const Address& addr) const;

    // Get account by name
    std::optional<Account> getAccount(const std::string& name) const;

    // List all accounts in wallet
    std::vector<Account> listAccounts() const;

    // Export private key as hex (password-protected)
    std::string exportPrivateKey(const Address& addr, const std::string& password) const;

    // Export mnemonic phrase (password-protected)
    std::string exportMnemonic(const std::string& password) const;

    // Save wallet to encrypted keystore file
    void save(const std::string& password);

    // Get wallet file path
    const std::string& getPath() const { return walletPath_; }

    // Validate BIP39 mnemonic phrase
    static bool validateMnemonic(const std::string& mnemonic);

    // Constructor (allow for std::make_shared)
    Wallet(const std::string& walletPath, const std::string& mnemonic);

private:

    std::string walletPath_;
    std::string mnemonic_;              // BIP39 mnemonic phrase
    std::string salt_;                  // Random salt for scrypt
    std::vector<Account> accounts_;     // Changed from map to vector (avoids Address comparison)
    std::vector<std::uint8_t> masterKey_;   // BIP32 master private key (32 bytes)
    std::vector<std::uint8_t> masterChainCode_;  // BIP32 chain code (32 bytes)

    // ===== BIP39 Helpers =====
    
    // Generate random 128-bit entropy and encode as 12-word mnemonic
    static std::string generateMnemonic();

    // Convert BIP39 mnemonic to 512-bit seed via PBKDF2
    static std::vector<std::uint8_t> mnemonicToSeed(const std::string& mnemonic, 
                                                     const std::string& passphrase = "");

    // Get BIP39 English wordlist (2048 words)
    static const std::vector<std::string>& getBIP39Wordlist();

    // ===== BIP32 Helpers =====

    // Initialize master key from seed
    void initializeFromSeed(const std::vector<std::uint8_t>& seed);

    // Derive child key at given index
    std::pair<std::vector<std::uint8_t>, std::vector<std::uint8_t>> 
    deriveChildKey(const std::vector<std::uint8_t>& parentKey,
                   const std::vector<std::uint8_t>& chainCode,
                   std::uint32_t childIndex);

    // Derive keypair from BIP32 path (e.g., "m/44'/60'/0'/0/0")
    KeyPair deriveKeyFromPath(const std::string& derivationPath);

    // Parse BIP32 derivation path into indices
    static std::vector<std::uint32_t> parseBIP32Path(const std::string& path);

    // ===== Encryption Helpers =====

    // Derive encryption key and IV from password using scrypt
    std::pair<std::vector<std::uint8_t>, std::vector<std::uint8_t>> 
    deriveKeysFromPassword(const std::string& password) const;

    // Encrypt plaintext with AES-256-GCM
    std::string encryptAES256GCM(const std::string& plaintext, const std::string& password);

    // Decrypt ciphertext with AES-256-GCM
    std::string decryptAES256GCM(const std::string& ciphertext, const std::string& password);

    // ===== Persistence Helpers =====

    // Serialize wallet to JSON format
    std::string toJson() const;

    // Deserialize wallet from JSON format
    void fromJson(const std::string& jsonStr);
};

} // namespace gambit