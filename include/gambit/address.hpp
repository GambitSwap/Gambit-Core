#pragma once
#include <array>
#include <string>
#include <cstdint>
#include <vector>

namespace gambit {

class Address {
public:
    static constexpr std::size_t kSize = 20;

    Address();
    explicit Address(const std::array<std::uint8_t, kSize>& bytes);

    static Address fromBytes(const std::vector<std::uint8_t>& bytes);
    static Address fromHex(const std::string& hex);
    static Address fromPublicKey(const std::vector<std::uint8_t>& uncompressedPubKey);

    std::string toHex(bool checksum = true) const;
    const std::array<std::uint8_t, kSize>& bytes() const { return bytes_; }

    bool isZero() const;
    bool operator==(const Address& other) const;
    bool operator!=(const Address& other) const;

private:
    std::array<std::uint8_t, kSize> bytes_;

    static std::string toChecksumHex(const std::array<std::uint8_t, kSize>& raw);
};

} // namespace gambit
