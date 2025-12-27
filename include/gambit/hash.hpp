#pragma once
#include <string>
#include <vector>
#include <array>
#include <cstdint>

namespace gambit {

using Bytes  = std::vector<std::uint8_t>;
using Bytes32 = std::array<std::uint8_t, 32>;

// Hex helpers
std::string toHex(const Bytes& data);
std::string toHex(const Bytes32& data);
Bytes       fromHex(const std::string& hex);

// Keccak-256 hashing
Bytes   keccak256(const Bytes& input);
Bytes   keccak256(const std::string& input);
Bytes32 keccak256_32(const Bytes& input);
Bytes32 keccak256_32(const std::string& input);

} // namespace gambit
