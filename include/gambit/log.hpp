#pragma once
#include <vector>
#include <string>
#include <cstdint>

#include "gambit/address.hpp"
#include "gambit/hash.hpp"

namespace gambit {

struct Log {
    Address address;
    std::vector<std::string> topics; // 32-byte hex strings
    Bytes data;
};

} // namespace gambit
