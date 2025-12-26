#include "gambit/hash.hpp"
#include "keccak.hpp" // from external/tiny-keccak/keccak.hpp
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <algorithm>

namespace gambit {

std::string toHex(const Bytes& data) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (auto b : data) {
        oss << std::setw(2) << static_cast<int>(b);
    }
    return oss.str();
}

std::string toHex(const Bytes32& data) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (auto b : data) {
        oss << std::setw(2) << static_cast<int>(b);
    }
    return oss.str();
}

static std::uint8_t hexCharToNibble(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
    throw std::runtime_error("Invalid hex char");
}

Bytes fromHex(const std::string& hex) {
    std::string s = hex;
    if (s.rfind("0x", 0) == 0 || s.rfind("0X", 0) == 0) {
        s = s.substr(2);
    }
    if (s.size() % 2 != 0) {
        throw std::runtime_error("Hex string length must be even");
    }
    Bytes out;
    out.reserve(s.size() / 2);
    for (std::size_t i = 0; i < s.size(); i += 2) {
        std::uint8_t hi = hexCharToNibble(s[i]);
        std::uint8_t lo = hexCharToNibble(s[i + 1]);
        out.push_back((hi << 4) | lo);
    }
    return out;
}

Bytes keccak256(const Bytes& input) {
    Bytes out(32);
    tinykeccak::keccak_256(input.data(), input.size(), out.data());
    return out;
}

Bytes keccak256(const std::string& input) {
    Bytes in(input.begin(), input.end());
    return keccak256(in);
}

Bytes32 keccak256_32(const Bytes& input) {
    Bytes32 out{};
    tinykeccak::keccak_256(input.data(), input.size(), out.data());
    return out;
}

Bytes32 keccak256_32(const std::string& input) {
    Bytes in(input.begin(), input.end());
    return keccak256_32(in);
}

} // namespace gambit
