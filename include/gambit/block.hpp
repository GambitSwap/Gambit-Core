#pragma once
#include <string>
#include <vector>
#include <cstdint>

#include "gambit/transaction.hpp"
#include "gambit/zk.hpp"
#include "gambit/hash.hpp"

namespace gambit {

class Block {
public:
    std::uint64_t index{0};
    std::string   prevHash;
    std::string   stateBefore;
    std::string   stateAfter;
    std::string   txRoot;
    std::string   receiptsRoot;
    ZkProof       proof;
    std::uint64_t timestamp{0};
    std::string   hash;

    std::vector<Transaction> transactions;
    std::vector<Receipt> receipts;
    Bloom logsBloom;

    Block() = default;

    Block(std::uint64_t idx,
          const std::string& prev,
          const std::string& before,
          const std::string& after,
          const std::string& txRoot_,
          const ZkProof& proof_);

    // Compute block hash
    std::string computeHash() const;

    // RLP
    Bytes rlpEncode() const;
    static Block rlpDecode(const Bytes& raw);


    // Serialize block to bytes (for P2P)
    std::string toHex() const;

    // Deserialize (optional)
    static Block fromHex(const std::string& hex);
};

} // namespace gambit
