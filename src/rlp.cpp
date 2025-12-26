#include "gambit/rlp.hpp"
#include <stdexcept>

namespace gambit {
namespace rlp {

static Bytes encodeLength(std::size_t len, std::uint8_t offset) {
    Bytes out;

    if (len < 56) {
        out.push_back(offset + static_cast<std::uint8_t>(len));
    } else {
        // Long form
        Bytes lenBytes;
        std::size_t tmp = len;
        while (tmp > 0) {
            lenBytes.insert(lenBytes.begin(), static_cast<std::uint8_t>(tmp & 0xFF));
            tmp >>= 8;
        }
        out.push_back(offset + 55 + static_cast<std::uint8_t>(lenBytes.size()));
        out.insert(out.end(), lenBytes.begin(), lenBytes.end());
    }

    return out;
}

Bytes encodeBytes(const Bytes& input) {
    if (input.size() == 1 && input[0] < 0x80) {
        // Single byte < 0x80 is its own encoding
        return input;
    }

    Bytes out = encodeLength(input.size(), 0x80);
    out.insert(out.end(), input.begin(), input.end());
    return out;
}

Bytes encodeString(const std::string& input) {
    Bytes bytes(input.begin(), input.end());
    return encodeBytes(bytes);
}

Bytes encodeUint(std::uint64_t value) {
    if (value == 0) {
        return Bytes{0x80}; // empty string
    }

    Bytes tmp;
    while (value > 0) {
        tmp.insert(tmp.begin(), static_cast<std::uint8_t>(value & 0xFF));
        value >>= 8;
    }

    return encodeBytes(tmp);
}

Bytes encodeList(const std::vector<Bytes>& items) {
    Bytes payload;
    for (const auto& item : items) {
        payload.insert(payload.end(), item.begin(), item.end());
    }

    Bytes out = encodeLength(payload.size(), 0xC0);
    out.insert(out.end(), payload.begin(), payload.end());
    return out;
}

Bytes concat(const std::vector<Bytes>& parts) {
    Bytes out;
    for (const auto& p : parts) {
        out.insert(out.end(), p.begin(), p.end());
    }
    return out;
}

} // namespace rlp
} // namespace gambit

namespace gambit {
namespace rlp {

static size_t readLen(const Bytes& in, size_t& offset, size_t prefix, size_t base) {
    size_t len = prefix - base;
    offset++;
    if (len <= 55) return len;

    size_t numBytes = len - 55;
    if (offset + numBytes > in.size()) throw std::runtime_error("RLP long length overflow");

    size_t out = 0;
    for (size_t i = 0; i < numBytes; i++) {
        out = (out << 8) | in[offset + i];
    }
    offset += numBytes;
    return out;
}

Decoded decode(const Bytes& in, size_t& offset) {
    if (offset >= in.size()) throw std::runtime_error("RLP decode overflow");

    uint8_t prefix = in[offset];

    // Single byte < 0x80
    if (prefix < 0x80) {
        offset++;
        return Decoded{false, Bytes{prefix}, {}};
    }

    // String
    if (prefix < 0xC0) {
        size_t len = readLen(in, offset, prefix, 0x80);
        if (offset + len > in.size()) throw std::runtime_error("RLP string overflow");
        Bytes b(in.begin() + offset, in.begin() + offset + len);
        offset += len;
        return Decoded{false, b, {}};
    }

    // List
    size_t len = readLen(in, offset, prefix, 0xC0);
    size_t end = offset + len;
    if (end > in.size()) throw std::runtime_error("RLP list overflow");

    std::vector<Decoded> items;
    while (offset < end) {
        items.push_back(decode(in, offset));
    }

    return Decoded{true, {}, items};
}

Decoded decode(const Bytes& in) {
    size_t off = 0;
    return decode(in, off);
}

} // namespace rlp
} // namespace gambit
