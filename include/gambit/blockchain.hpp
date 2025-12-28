#pragma once
#include <vector>
#include <mutex>

#include "gambit/block.hpp"
#include "gambit/state.hpp"
#include "gambit/transaction.hpp"
#include "gambit/genesis.hpp"

namespace gambit {

class Blockchain {
public:
    explicit Blockchain(const GenesisConfig& genesis);

    // Add a transaction to the mempool
    void addTransaction(const Transaction& tx);

    // Mine a block from current mempool
    Block mineBlock();

    // Validate and append a received block
    bool addBlock(const Block& block);

    const std::vector<Block>& chain() const { return chain_; }
    const State& state() const { return state_; }

    std::uint64_t chainId() const { return chainId_; }

    // Simple accessors for RPC
    const std::vector<Transaction>& mempool() const { return mempool_; }
    
    bool validateTransaction(const Transaction& tx, std::string& err) const;

    std::string computeTxRoot(const std::vector<Transaction>& txs) const; // make public for engine

private:
    std::vector<Block> chain_;
    State state_;
    std::vector<Transaction> mempool_;
    std::mutex mutex_;
    std::uint64_t chainId_{0};

    void initGenesis(const GenesisConfig& genesis);
};

} // namespace gambit
