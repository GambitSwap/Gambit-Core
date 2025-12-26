#include "gambit/zk.hpp"
#include "gambit/hash.hpp"

namespace gambit {

ZkProof ZkProver::generate(const std::string& stateBefore,
                           const std::string& stateAfter,
                           const std::string& txRoot)
{
    ZkProof p;
    p.stateBefore = stateBefore;
    p.stateAfter  = stateAfter;
    p.txRoot      = txRoot;

    // Mock proof: hash of inputs
    std::string concat = stateBefore + "|" + stateAfter + "|" + txRoot;
    p.proof = toHex(keccak256(concat));

    // Commitment = keccak256(proof || stateBefore || stateAfter || txRoot)
    std::string c = p.proof + "|" + stateBefore + "|" + stateAfter + "|" + txRoot;
    p.commitment = toHex(keccak256(c));

    return p;
}

bool ZkVerifier::verify(const ZkProof& proof) {
    // Recompute commitment
    std::string c = proof.proof + "|" + proof.stateBefore + "|" + proof.stateAfter + "|" + proof.txRoot;
    std::string expected = toHex(keccak256(c));

    return expected == proof.commitment;
}

} // namespace gambit
