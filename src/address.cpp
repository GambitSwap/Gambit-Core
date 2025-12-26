#include "gambit/address.hpp"
#include "gambit/hash.hpp"
#include <algorithm>
#include <stdexcept>
#include <sstream>
#include <iomanip>

namespace gambit {

Address::Address() : bytes_{} {}

Address::Address(const std::array<std::uint8_t, kSize>& b) : bytes_(b) {}

Address Address::fromBytes(const std::vector<std::uint8_t>& bytes) {
    if (bytes.size() != kSize) {
        throw std::runtime_error("Address::fromBytes: must be 20 bytes");
    }
    std::array<std::uint8_t, kSize> arr{};
    std::copy(bytes.begin(), bytes.end(), arr.begin());
    return Address(arr);
}

Address Address::fromHex(const std::string& hex) {
    auto raw = fromHex(hex);
    return fromBytes(raw);
}

Address Address::fromPublicKey(const std::vector<std::uint8_t>& pubKey) {
    // Ethereum: keccak256(uncompressed_pubkey[1:]) last 20 bytes
    // pubKey must be 64 bytes (x||y) or 65 bytes with prefix 0x04
    std::vector<std::uint8_t> key = pubKey;

    if (key.size() == 65 && key[0] == 0x04) {
        key.erase(key.begin());
    }

    if (key.size() != 64) {
        throw std::runtime_error("Address::fromPublicKey: expected 64-byte uncompressed key");
    }

    auto hash = keccak256(key);
    if (hash.size() < kSize) {
        throw std::runtime_error("Address::fromPublicKey: hash too short");
    }

    std::array<std::uint8_t, kSize> addr{};
    std::copy(hash.end() - kSize, hash.end(), addr.begin());
    return Address(addr);
}

std::string Address::toChecksumHex(const std::array<std::uint8_t, kSize>& raw) {
    // Convert to lowercase hex
    std::string hex;
    hex.reserve(40);
    for (auto b : raw) {
        std::ostringstream oss;
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)b;
        hex += oss.str();
    }

    // Compute keccak256 of lowercase hex string
    auto hash = keccak256(hex);
    std::string hashHex = toHex(hash);

    // Apply EIP-55 checksum
    std::string out = "0x";
    for (std::size_t i = 0; i < hex.size(); ++i) {
        char c = hex[i];
        char h = hashHex[i];

        if (c >= '0' && c <= '9') {
            out += c;
        } else {
            // If hash nibble >= 8, uppercase
            int nibble = (h >= '0' && h <= '9') ? (h - '0') : (10 + (h - 'a'));
            if (nibble >= 8) {
                out += std::toupper(c);
            } else {
                out += c;
            }
        }
    }

    return out;
}

std::string Address::toHex(bool checksum) const {
    if (!checksum) {
        return "0x" + gambit::toHex(bytes_);
    }
    return toChecksumHex(bytes_);
}

bool Address::operator==(const Address& other) const {
    return bytes_ == other.bytes_;
}

bool Address::operator!=(const Address& other) const {
    return !(*this == other);
}

} // namespace gambit
