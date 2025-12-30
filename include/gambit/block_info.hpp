#pragma once
#include <string>
#include <cstdint>
#include <stdexcept>
#include "gambit/zk.hpp"

namespace gambit {
    struct BlockInfo {
        std::string hash;
        std::string parentHash;
        uint64_t    number;
        size_t      txCount;
        ZkProof     proof;
    };
} // namespace gambit