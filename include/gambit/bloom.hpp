#pragma once
#include <array>
#include <string>
#include <vector>
#include <cstdint>

#include "gambit/hash.hpp"

namespace gambit {

class Bloom {
public:
    static constexpr size_t kBytes = 256; // 2048 bits
    std::array<uint8_t, kBytes> bits{};

    void add(const Bytes& data);
    void add(const std::string& hex);

    std::string toHex() const;
};

} // namespace gambit
