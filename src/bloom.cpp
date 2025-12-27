#include "gambit/bloom.hpp"

namespace gambit {

void Bloom::add(const Bytes& data) {
    Bytes h = keccak256(data);
    // Use three 11-bit positions (Ethereum style)
    for (int i = 0; i < 3; ++i) {
        uint16_t v = (static_cast<uint16_t>(h[2*i]) << 8) | h[2*i + 1];
        v = v & 2047; // 0..2047
        size_t byteIndex = v >> 3;
        uint8_t bit = 1 << (v & 7);
        bits[byteIndex] |= bit;
    }
}

void Bloom::add(const std::string& hex) {
    Bytes d = fromHex(hex);
    add(d);
}

std::string Bloom::toHex() const {
    return "0x" + gambit::toHex(Bytes(bits.begin(), bits.end()));
}

} // namespace gambit
