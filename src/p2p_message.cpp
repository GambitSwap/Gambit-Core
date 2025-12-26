#include "gambit/p2p_message.hpp"
#include <stdexcept>

namespace gambit {

std::vector<std::uint8_t> Message::encode() const {
    std::vector<std::uint8_t> out;

    out.push_back(static_cast<std::uint8_t>(type));

    std::uint32_t len = static_cast<std::uint32_t>(payload.size());
    out.push_back((len >> 24) & 0xFF);
    out.push_back((len >> 16) & 0xFF);
    out.push_back((len >> 8) & 0xFF);
    out.push_back(len & 0xFF);

    out.insert(out.end(), payload.begin(), payload.end());
    return out;
}

Message Message::decode(const std::vector<std::uint8_t>& data) {
    if (data.size() < 5) {
        throw std::runtime_error("Message::decode: too short");
    }

    Message msg;
    msg.type = static_cast<MessageType>(data[0]);

    std::uint32_t len =
        (data[1] << 24) |
        (data[2] << 16) |
        (data[3] << 8) |
        (data[4]);

    if (data.size() < 5 + len) {
        throw std::runtime_error("Message::decode: incomplete payload");
    }

    msg.payload.assign(data.begin() + 5, data.begin() + 5 + len);
    return msg;
}

} // namespace gambit
