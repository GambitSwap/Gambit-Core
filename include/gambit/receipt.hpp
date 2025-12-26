#pragma once
#include <vector>
#include <cstdint>
#include <string>

#include "gambit/log.hpp"
#include "gambit/rlp.hpp"

namespace gambit {

struct Receipt {
    bool status{true};
    std::uint64_t cumulativeGasUsed{0};
    std::vector<Log> logs;

    // Minimal RLP encoding
    Bytes rlpEncode() const;
};

} // namespace gambit
