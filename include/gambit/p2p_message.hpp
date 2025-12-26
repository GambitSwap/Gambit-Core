#pragma once
#include <vector>
#include <string>
#include <cstdint>

namespace gambit {

enum class MessageType : std::uint8_t {
    HELLO = 0,
    NEW_TX = 1,
    NEW_BLOCK = 2,
    GET_BLOCKS = 3,
    BLOCKS_RESPONSE = 4,
    PING = 5,
    PONG = 6
};

struct Message {
    MessageType type;
    std::vector<std::uint8_t> payload;

    // Encode: [type (1 byte)] [len (4 bytes big-endian)] [payload]
    std::vector<std::uint8_t> encode() const;

    static Message decode(const std::vector<std::uint8_t>& data);
};

} // namespace gambit
