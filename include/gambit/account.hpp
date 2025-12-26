#pragma once
#include <cstdint>

namespace gambit {

struct Account {
    std::uint64_t balance{0};
    std::uint64_t nonce{0};
    // Future: codeHash, storageRoot
};

} // namespace gambit
