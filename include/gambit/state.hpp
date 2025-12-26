#pragma once
#include <unordered_map>
#include <string>
#include <vector>

#include "gambit/address.hpp"
#include "gambit/account.hpp"
#include "gambit/transaction.hpp"
#include "gambit/hash.hpp"
#include "gambit/genesis.hpp"
#include "gambit/mpt.hpp"

namespace gambit {

class State {
public:
    State() = default;
    explicit State(const GenesisConfig& genesis);

    Account& getOrCreate(const Address& addr);
    const Account* get(const Address& addr) const;

    // Apply tx from a known sender address
    void applyTransaction(const Address& from, const Transaction& tx);

    // Compute Merkle-Patricia state root
    std::string root() const;

private:
    // Keyed by lowercase hex address
    std::unordered_map<std::string, Account> accounts_;
};

} // namespace gambit
