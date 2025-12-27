#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace gambit {

using Bytes = std::vector<std::uint8_t>;

namespace rlp {

// Encode a byte string
Bytes encodeBytes(const Bytes& input);

// Encode a string (UTF-8)
Bytes encodeString(const std::string& input);

// Encode an unsigned integer (big-endian, minimal)
Bytes encodeUint(std::uint64_t value);

// Encode a list of RLP-encoded items
Bytes encodeList(const std::vector<Bytes>& items);

// Utility: concatenate many byte arrays
Bytes concat(const std::vector<Bytes>& parts);

struct Decoded {
    bool isList{false};
    Bytes bytes;
    std::vector<Decoded> list;
};

// Decode a single RLP item
Decoded decode(const Bytes& in, size_t& offset);

// Convenience: decode full buffer
Decoded decode(const Bytes& in);

} // namespace rlp
} // namespace gambit
