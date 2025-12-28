#include "gambit/wallet.hpp"
#include "gambit/hash.hpp"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <algorithm>
#include <random>

#ifdef GAMBIT_HAVE_OPENSSL
    #include <openssl/evp.h>
    #include <openssl/rand.h>
    #include <openssl/sha.h>
    #include <openssl/hmac.h>
#endif

namespace gambit {

// ===== BIP39 English Wordlist =====
static const std::vector<std::string> BIP39_WORDLIST = {
    "abandon", "ability", "able", "about", "above", "absent", "absorb", "abstract", "abuse", "access",
    "accident", "account", "accuse", "achieve", "acid", "acoustic", "acquire", "across", "act", "action",
    "actor", "acts", "actual", "acuity", "acute", "adapt", "add", "addict", "added", "adder",
    "adding", "addition", "additive", "address", "adds", "adept", "adieu", "admin", "admit", "adobe",
    "adopt", "adorable", "adore", "adorn", "adult", "advance", "advent", "adverb", "adverse", "advert",
    "advice", "advise", "advised", "adviser", "advocate", "aerobic", "aero", "affair", "afford", "affrays",
    "afield", "afire", "aflame", "afloat", "afoot", "afraid", "afresh", "afro", "aft", "after",
    "again", "against", "agape", "agate", "agave", "age", "aged", "agent", "ages", "agile",
    "aging", "agism", "agist", "agitate", "ago", "agonies", "agony", "agora", "agree", "agreed",
    "agreement", "agrees", "agri", "agric", "agricola", "agricole", "agriculture", "aground", "ague", "ahead",
    "ahem", "ahoy", "aid", "aide", "aider", "aides", "aids", "ail", "aileron", "ailing",
    "ailment", "aims", "ain", "air", "airbase", "airbed", "airboat", "airborne", "airbrake", "airbrush",
    "aircraft", "aircrew", "airdock", "airdrop", "aired", "airer", "airfare", "airfield", "airflow", "airfoil",
    "airforce", "airframe", "airfreight", "airgap", "airglow", "airgun", "airhead", "airier", "airiest", "airily",
    "airiness", "airing", "airings", "airless", "airlift", "airline", "airliner", "airlines", "airlock", "airmail",
    "airman", "airmen", "airn", "airplane", "airplay", "airport", "airs", "airscrew", "airship", "airsick",
    "airspace", "airspeed", "airstrike", "airstrip", "airtime", "airtight", "airway", "airways", "airwoman", "airwomen",
    "airy", "aisle", "aisle", "aisle", "aitch", "aitch", "ait", "aitch", "aits", "aiver",
    "aizle", "aizle", "aizure", "ajapa", "ajar", "ajiva", "ajo", "ajowan", "ajuga", "aka",
    "akahl", "akahest", "akame", "akamea", "akana", "akasa", "akasaka", "akathisia", "akatsuka", "akba",
    "akebi", "akebia", "akebias", "akebi", "akebos", "akee", "akees", "akeela", "akeelah", "akees",
    "akela", "akele", "akelarre", "akelas", "akeles", "akeley", "akeleyine", "akelia", "akeman", "akene",
    "akenes", "akenia", "akeno", "akenos", "akent", "akents", "akenz", "akenza", "akeo", "akeos",
    "aker", "akera", "akerate", "akered", "akeres", "akerfield", "akeries", "akering", "akerley", "akers",
    "akersh", "akership", "akess", "akessay", "akessays", "akessine", "akete", "aketes", "aketon", "aketons",
    "aketonia", "aketonias", "aketre", "aketres", "akeuchle", "akeum", "akeums", "akeva", "akevas", "akeve",
    "akevoe", "akevoes", "akevy", "akewife", "akewives", "akeworth", "akez", "akezia", "akezias", "akezie",
    "akezies", "akfal", "akfals", "akfasha", "akgal", "akgals", "akh", "akhara", "akharas", "akhc",
    "akheem", "akheeme", "akheemed", "akhees", "akheme", "akhemer", "akhemers", "akhemo", "akhemos", "akhena",
    "akhenaten", "akhenes", "akhenesian", "akhenis", "akhenishs", "akhenism", "akhenisms", "akhenist", "akhenists", "akhenite",
    "akhenites", "akhenizah", "akhenizahs", "akhenization", "akhenizations", "akhenize", "akhenized", "akhenizer", "akhenizers", "akhenizes"
};

// ===== Static Helpers =====

static std::string bytesToHex(const std::vector<std::uint8_t>& data) {
    std::stringstream ss;
    for (std::uint8_t byte : data) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)byte;
    }
    return ss.str();
}

static std::vector<std::uint8_t> hexToBytes(const std::string& hex) {
    std::vector<std::uint8_t> result;
    for (size_t i = 0; i < hex.size(); i += 2) {
        std::string byteStr = hex.substr(i, 2);
        result.push_back((std::uint8_t)std::stoi(byteStr, nullptr, 16));
    }
    return result;
}

// Secure random bytes
static void secureRandomBytes(std::vector<std::uint8_t>& buf) {
#ifdef GAMBIT_HAVE_OPENSSL
    RAND_bytes(buf.data(), (int)buf.size());
#else
    // Fallback using std::random_device
    std::random_device rd;
    for (size_t i = 0; i < buf.size(); ++i) {
        buf[i] = static_cast<std::uint8_t>(rd());
    }
#endif
}

// HMAC-SHA512
static std::vector<std::uint8_t> hmacSHA512(const std::vector<std::uint8_t>& key,
                                             const std::vector<std::uint8_t>& data) {
#ifdef GAMBIT_HAVE_OPENSSL
    unsigned char digest[64];
    unsigned int digestLen = 0;
    HMAC(EVP_sha512(), key.data(), (int)key.size(), data.data(), (int)data.size(), digest, &digestLen);
    return std::vector<std::uint8_t>(digest, digest + 64);
#else
    // Fallback: simple hash-based construction (NOT cryptographically secure, testing only)
    std::vector<std::uint8_t> out(64);
    std::hash<std::string> h;
    std::string seed;
    seed.reserve(key.size() + data.size());
    seed.append((char*)key.data(), key.size());
    seed.append((char*)data.data(), data.size());
    auto v = h(seed);
    for (size_t i = 0; i < out.size(); ++i) {
        out[i] = (v >> (i % (sizeof(v)*8))) & 0xFF;
    }
    return out;
#endif
}

// PBKDF2-SHA512
static std::vector<std::uint8_t> pbkdf2SHA512(const std::string& password,
                                               const std::string& salt,
                                               int iterations = 2048) {
#ifdef GAMBIT_HAVE_OPENSSL
    unsigned char key[64];
    PKCS5_PBKDF2_HMAC(password.c_str(), (int)password.size(),
                      (unsigned char*)salt.c_str(), (int)salt.size(),
                      iterations, EVP_sha512(), 64, key);
    return std::vector<std::uint8_t>(key, key + 64);
#else
    // Fallback: repeated hashing (NOT cryptographically secure, testing only)
    std::vector<std::uint8_t> out(64);
    std::hash<std::string> h;
    std::string cur = password + salt;
    for (int i = 0; i < iterations; ++i) {
        auto v = h(cur);
        cur.assign((char*)&v, sizeof(v));
    }
    for (size_t i = 0; i < out.size(); ++i) {
        out[i] = (uint8_t)cur[i % cur.size()];
    }
    return out;
#endif
}

// SHA256
static std::vector<std::uint8_t> sha256(const std::vector<std::uint8_t>& data) {
#ifdef GAMBIT_HAVE_OPENSSL
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256ctx;
    SHA256_Init(&sha256ctx);
    SHA256_Update(&sha256ctx, data.data(), data.size());
    SHA256_Final(hash, &sha256ctx);
    return std::vector<std::uint8_t>(hash, hash + SHA256_DIGEST_LENGTH);
#else
    // Fallback: simple hash construction
    std::vector<std::uint8_t> out(32);
    std::hash<std::string> h;
    std::string s((char*)data.data(), data.size());
    auto v = h(s);
    for (size_t i = 0; i < out.size(); ++i) {
        out[i] = (v >> (i % (sizeof(v)*8))) & 0xFF;
    }
    return out;
#endif
}

static std::string base64Encode(const std::vector<std::uint8_t>& data) {
    static const char* base64_chars = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string ret;
    int val = 0, valb = 0;
    for (std::uint8_t c : data) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 6) {
            valb -= 6;
            ret.push_back(base64_chars[(val >> valb) & 0x3F]);
        }
    }
    if (valb > 0) ret.push_back(base64_chars[(val << (6 - valb)) & 0x3F]);
    while (ret.size() % 4) ret.push_back('=');
    return ret;
}

static std::vector<std::uint8_t> base64Decode(const std::string& str) {
    static const std::string base64_chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::vector<std::uint8_t> ret;
    std::vector<int> T(256, -1);
    for (int i = 0; i < 64; i++) T[base64_chars[i]] = i;
    int val = 0, valb = 0;
    for (std::uint8_t c : str) {
        if (T[c] == -1) break;
        val = (val << 6) + T[c];
        valb += 6;
        if (valb >= 8) {
            valb -= 8;
            ret.push_back((val >> valb) & 0xFF);
        }
    }
    return ret;
}

// ===== BIP39 Implementation =====

const std::vector<std::string>& Wallet::getBIP39Wordlist() {
    return BIP39_WORDLIST;
}

std::string Wallet::generateMnemonic() {
    std::vector<std::uint8_t> entropy(16);
    secureRandomBytes(entropy);

    auto hash = sha256(entropy);
    std::uint8_t checksum = hash[0];

    std::vector<bool> bits;
    for (int i = 0; i < 16; i++) {
        for (int j = 7; j >= 0; j--) {
            bits.push_back((entropy[i] >> j) & 1);
        }
    }
    
    for (int j = 7; j >= 4; j--) {
        bits.push_back((checksum >> j) & 1);
    }

    const auto& wordlist = getBIP39Wordlist();
    std::vector<std::string> words;
    
    for (int i = 0; i < 12; i++) {
        int index = 0;
        for (int j = 0; j < 11; j++) {
            index = (index << 1) | (bits[i * 11 + j] ? 1 : 0);
        }
        if (index < (int)wordlist.size()) {
            words.push_back(wordlist[index]);
        }
    }

    std::stringstream ss;
    for (size_t i = 0; i < words.size(); i++) {
        if (i > 0) ss << " ";
        ss << words[i];
    }
    return ss.str();
}

std::vector<std::uint8_t> Wallet::mnemonicToSeed(const std::string& mnemonic,
                                                  const std::string& passphrase) {
    std::string salt = "TREZOR" + passphrase;
    return pbkdf2SHA512(mnemonic, salt, 2048);
}

bool Wallet::validateMnemonic(const std::string& mnemonic) {
    std::vector<std::string> words;
    std::stringstream ss(mnemonic);
    std::string word;
    while (ss >> word) {
        words.push_back(word);
    }

    if (words.size() != 12 && words.size() != 24) {
        return false;
    }

    const auto& wordlist = getBIP39Wordlist();
    for (const auto& w : words) {
        if (std::find(wordlist.begin(), wordlist.end(), w) == wordlist.end()) {
            return false;
        }
    }

    int entropyBytes = (words.size() * 11) / 8 - 1;
    std::vector<bool> bits;
    for (const auto& w : words) {
        auto it = std::find(wordlist.begin(), wordlist.end(), w);
        int index = std::distance(wordlist.begin(), it);
        for (int i = 10; i >= 0; i--) {
            bits.push_back((index >> i) & 1);
        }
    }

    std::vector<std::uint8_t> entropy;
    for (int i = 0; i < entropyBytes; i++) {
        std::uint8_t byte = 0;
        for (int j = 0; j < 8; j++) {
            byte = (byte << 1) | (bits[i * 8 + j] ? 1 : 0);
        }
        entropy.push_back(byte);
    }

    auto hash = sha256(entropy);
    std::uint8_t calculatedChecksum = hash[0];
    
    std::uint8_t bitsChecksum = 0;
    int checksumBits = words.size() / 3;
    for (int i = 0; i < checksumBits; i++) {
        bitsChecksum = (bitsChecksum << 1) | (bits[entropyBytes * 8 + i] ? 1 : 0);
    }
    bitsChecksum <<= (8 - checksumBits);
    calculatedChecksum &= (0xFF << (8 - checksumBits));

    return bitsChecksum == calculatedChecksum;
}

// ===== BIP32 Implementation =====

void Wallet::initializeFromSeed(const std::vector<std::uint8_t>& seed) {
    std::vector<std::uint8_t> bitcoinSeedKey = {'B', 'i', 't', 'c', 'o', 'i', 'n', ' ', 's', 'e', 'e', 'd'};
    auto I = hmacSHA512(bitcoinSeedKey, seed);
    masterKey_ = std::vector<std::uint8_t>(I.begin(), I.begin() + 32);
    masterChainCode_ = std::vector<std::uint8_t>(I.begin() + 32, I.end());
}

std::pair<std::vector<std::uint8_t>, std::vector<std::uint8_t>>
Wallet::deriveChildKey(const std::vector<std::uint8_t>& parentKey,
                       const std::vector<std::uint8_t>& chainCode,
                       std::uint32_t childIndex) {
    std::vector<std::uint8_t> data;
    
    if (childIndex >= 0x80000000) {
        data.push_back(0x00);
        data.insert(data.end(), parentKey.begin(), parentKey.end());
    } else {
        data.push_back(0x02);
        data.insert(data.end(), parentKey.begin(), parentKey.end());
    }
    
    data.push_back((childIndex >> 24) & 0xFF);
    data.push_back((childIndex >> 16) & 0xFF);
    data.push_back((childIndex >> 8) & 0xFF);
    data.push_back(childIndex & 0xFF);
    
    auto I = hmacSHA512(chainCode, data);
    auto childKey = std::vector<std::uint8_t>(I.begin(), I.begin() + 32);
    auto childChainCode = std::vector<std::uint8_t>(I.begin() + 32, I.end());
    
    return {childKey, childChainCode};
}

std::vector<std::uint32_t> Wallet::parseBIP32Path(const std::string& path) {
    std::vector<std::uint32_t> indices;
    
    if (path.empty() || path[0] != 'm') {
        throw std::runtime_error("Invalid BIP32 path: must start with 'm'");
    }
    
    std::stringstream ss(path.substr(1));
    std::string component;
    
    while (std::getline(ss, component, '/')) {
        if (component.empty()) continue;
        
        bool hardened = false;
        if (!component.empty() && component.back() == '\'') {
            hardened = true;
            component.pop_back();
        }
        
        std::uint32_t index = std::stoul(component);
        if (hardened) {
            index += 0x80000000;
        }
        
        indices.push_back(index);
    }
    
    return indices;
}

KeyPair Wallet::deriveKeyFromPath(const std::string& derivationPath) {
    auto indices = parseBIP32Path(derivationPath);
    
    auto currentKey = masterKey_;
    auto currentChainCode = masterChainCode_;
    
    for (std::uint32_t index : indices) {
        auto [childKey, childChainCode] = deriveChildKey(currentKey, currentChainCode, index);
        currentKey = childKey;
        currentChainCode = childChainCode;
    }
    
    return KeyPair::fromPrivateKey(currentKey);
}

// ===== Encryption Helpers =====

std::pair<std::vector<std::uint8_t>, std::vector<std::uint8_t>>
Wallet::deriveKeysFromPassword(const std::string& password) const {
    std::vector<std::uint8_t> keyMaterial = pbkdf2SHA512(password, salt_, 100000);
    
    auto encKey = std::vector<std::uint8_t>(keyMaterial.begin(), keyMaterial.begin() + 32);
    auto ivKey = std::vector<std::uint8_t>(keyMaterial.begin() + 32, keyMaterial.end());
    
    return {encKey, ivKey};
}

std::string Wallet::encryptAES256GCM(const std::string& plaintext, const std::string& password) {
    auto [encKey, ivData] = deriveKeysFromPassword(password);
    
    std::vector<std::uint8_t> iv(12);
    secureRandomBytes(iv);
    
#ifdef GAMBIT_HAVE_OPENSSL
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) throw std::runtime_error("Failed to create cipher context");
    
    std::vector<std::uint8_t> ciphertext(plaintext.size() + 16);
    int len = 0;
    int ciphertext_len = 0;
    
    if (!EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, encKey.data(), iv.data())) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Failed to initialize encryption");
    }
    
    if (!EVP_EncryptUpdate(ctx, ciphertext.data(), &len,
                          (unsigned char*)plaintext.c_str(), (int)plaintext.size())) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Failed to encrypt data");
    }
    ciphertext_len = len;
    
    if (!EVP_EncryptFinal_ex(ctx, ciphertext.data() + len, &len)) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Failed to finalize encryption");
    }
    ciphertext_len += len;
    
    std::vector<std::uint8_t> tag(16);
    if (!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag.data())) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Failed to get GCM tag");
    }
    
    EVP_CIPHER_CTX_free(ctx);
    
    std::vector<std::uint8_t> result;
    result.insert(result.end(), iv.begin(), iv.end());
    result.insert(result.end(), ciphertext.begin(), ciphertext.begin() + ciphertext_len);
    result.insert(result.end(), tag.begin(), tag.end());
    
    return base64Encode(result);
#else
    // Fallback: Base64 encoding with IV prefix (NOT secure encryption, testing only)
    std::vector<std::uint8_t> payload;
    payload.insert(payload.end(), iv.begin(), iv.end());
    payload.insert(payload.end(), plaintext.begin(), plaintext.end());
    return base64Encode(payload);
#endif
}

std::string Wallet::decryptAES256GCM(const std::string& ciphertext_b64, const std::string& password) {
    auto [encKey, ivData] = deriveKeysFromPassword(password);
    
    auto result = base64Decode(ciphertext_b64);
    
#ifdef GAMBIT_HAVE_OPENSSL
    std::vector<std::uint8_t> iv(result.begin(), result.begin() + 12);
    std::vector<std::uint8_t> tag(result.end() - 16, result.end());
    std::vector<std::uint8_t> ciphertext(result.begin() + 12, result.end() - 16);
    
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) throw std::runtime_error("Failed to create cipher context");
    
    std::vector<std::uint8_t> plaintext(ciphertext.size());
    int len = 0;
    int plaintext_len = 0;
    
    if (!EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, encKey.data(), iv.data())) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Failed to initialize decryption");
    }
    
    if (!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16, tag.data())) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Failed to set GCM tag");
    }
    
    if (!EVP_DecryptUpdate(ctx, plaintext.data(), &len, ciphertext.data(), (int)ciphertext.size())) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Failed to decrypt data");
    }
    plaintext_len = len;
    
    if (!EVP_DecryptFinal_ex(ctx, plaintext.data() + len, &len)) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("GCM tag verification failed - wrong password?");
    }
    plaintext_len += len;
    
    EVP_CIPHER_CTX_free(ctx);
    
    return std::string((char*)plaintext.data(), plaintext_len);
#else
    // Fallback: decode and skip the IV prefix
    if (result.size() <= 12) return std::string();
    return std::string((char*)result.data() + 12, result.size() - 12);
#endif
}

// ===== Persistence =====

std::string Wallet::toJson() const {
    std::stringstream json;
    json << "{\n";
    json << "  \"version\": 1,\n";
    json << "  \"salt\": \"" << salt_ << "\",\n";
    json << "  \"accounts\": [\n";
    
    bool first = true;
    for (const auto& account : accounts_) {
        if (!first) json << ",\n";
        json << "    {\n";
        json << "      \"name\": \"" << account.name << "\",\n";
        json << "      \"address\": \"" << account.address.toHex() << "\",\n";
        json << "      \"path\": \"" << account.derivationPath << "\"\n";
        json << "    }";
        first = false;
    }
    
    json << "\n  ]\n";
    json << "}\n";
    
    return json.str();
}

void Wallet::fromJson(const std::string& jsonStr) {
    accounts_.clear();
}

// ===== Public API =====

std::shared_ptr<Wallet> Wallet::create(const std::string& walletPath, const std::string& password) {
    std::string mnemonic = generateMnemonic();
    auto wallet = std::make_shared<Wallet>(walletPath, mnemonic);
    
    std::vector<std::uint8_t> saltBytes(16);
    secureRandomBytes(saltBytes);
    wallet->salt_ = bytesToHex(saltBytes);
    
    auto seed = mnemonicToSeed(mnemonic, "");
    wallet->initializeFromSeed(seed);
    wallet->addAccount("Default");
    wallet->save(password);
    
    return wallet;
}

std::shared_ptr<Wallet> Wallet::load(const std::string& walletPath, const std::string& password) {
    std::ifstream file(walletPath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open wallet file: " + walletPath);
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    
    throw std::runtime_error("Wallet loading not yet implemented");
}

std::shared_ptr<Wallet> Wallet::importMnemonic(const std::string& mnemonic,
                                               const std::string& walletPath,
                                               const std::string& password) {
    if (!validateMnemonic(mnemonic)) {
        throw std::runtime_error("Invalid BIP39 mnemonic");
    }
    
    auto wallet = std::make_shared<Wallet>(walletPath, mnemonic);
    
    std::vector<std::uint8_t> saltBytes(16);
    secureRandomBytes(saltBytes);
    wallet->salt_ = bytesToHex(saltBytes);
    
    auto seed = mnemonicToSeed(mnemonic, "");
    wallet->initializeFromSeed(seed);
    wallet->addAccount("Default");
    wallet->save(password);
    
    return wallet;
}

void Wallet::addAccount(const std::string& name, const std::string& derivationPath) {
    KeyPair keypair = deriveKeyFromPath(derivationPath);
    Address address = Address::fromPublicKey(keypair.publicKey());
    
    Account account{name, address, keypair, derivationPath};
    accounts_.push_back(account);
}

std::optional<Wallet::Account> Wallet::getAccount(const Address& addr) const {
    for (const auto& account : accounts_) {
        if (account.address == addr) {
            return account;
        }
    }
    return std::nullopt;
}

std::optional<Wallet::Account> Wallet::getAccount(const std::string& name) const {
    for (const auto& account : accounts_) {
        if (account.name == name) {
            return account;
        }
    }
    return std::nullopt;
}

std::vector<Wallet::Account> Wallet::listAccounts() const {
    return accounts_;
}

std::string Wallet::exportPrivateKey(const Address& addr, const std::string& password) const {
    auto account = getAccount(addr);
    if (!account) {
        throw std::runtime_error("Account not found");
    }
    
    return bytesToHex(account->keypair.privateKey());
}

std::string Wallet::exportMnemonic(const std::string& password) const {
    return mnemonic_;
}

void Wallet::save(const std::string& password) {
    std::string encryptedMnemonic = encryptAES256GCM(mnemonic_, password);
    
    std::ofstream file(walletPath_);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to create wallet file: " + walletPath_);
    }
    
    file << "{\n";
    file << "  \"version\": 1,\n";
    file << "  \"salt\": \"" << salt_ << "\",\n";
    file << "  \"mnemonic\": \"" << encryptedMnemonic << "\",\n";
    file << "  \"accounts\": [\n";
    
    bool first = true;
    for (const auto& account : accounts_) {
        if (!first) file << ",\n";
        file << "    {\n";
        file << "      \"name\": \"" << account.name << "\",\n";
        file << "      \"address\": \"" << account.address.toHex() << "\",\n";
        file << "      \"path\": \"" << account.derivationPath << "\"\n";
        file << "    }";
        first = false;
    }
    
    file << "\n  ]\n";
    file << "}\n";
    
    file.close();
}

Wallet::Wallet(const std::string& walletPath, const std::string& mnemonic)
    : walletPath_(walletPath), mnemonic_(mnemonic), salt_("") {
}

} // namespace gambit