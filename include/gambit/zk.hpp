#pragma once
#include <string>

namespace gambit {

struct ZkProof {
    std::string proof;        // opaque proof blob
    std::string stateBefore;  // state root before block
    std::string stateAfter;   // state root after block
    std::string txRoot;       // merkle root of tx list
    std::string commitment;   // hash(proof || stateBefore || stateAfter || txRoot)
};

class ZkProver {
public:
    // Generate a mock proof for a block
    static ZkProof generate(const std::string& stateBefore,
                            const std::string& stateAfter,
                            const std::string& txRoot);
};

class ZkVerifier {
public:
    // Verify a mock proof
    static bool verify(const ZkProof& proof);
};

} // namespace gambit
