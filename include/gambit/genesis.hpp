#pragma once
#include <vector>
#include <cstdint>
#include "gambit/address.hpp"

namespace gambit {

struct GenesisAccount {
    Address address;
    std::uint64_t balance{0};
};

struct GenesisConfig {
    std::vector<GenesisAccount> premine;
    std::uint64_t chainId{1};
};

} // namespace gambit
