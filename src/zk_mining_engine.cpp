#include "gambit/zk_mining_engine.hpp"

namespace gambit {

Block ZkMiningEngine::buildBlockTemplate(Blockchain& chain) {
    const auto& mempool = chain.mempool();

    std::string before = chain.state().root();

    // Apply txs to a temporary state (if any)
    State temp = chain.state();
    for (const auto& tx : mempool) {
        temp.applyTransaction(tx.from, tx);
    }

    std::string after = temp.root();
    std::string txRoot = chain.computeTxRoot(mempool);

    ZkProof proof = ZkProver::generate(before, after, txRoot);

    Block b(
        chain.chain().size(),
        chain.chain().back().hash,
        before,
        after,
        txRoot,
        proof
    );

    b.transactions = mempool;  // may be empty
    return b;
}

bool ZkMiningEngine::validateMinedBlock(const Block& block, Blockchain& chain) {
    return ZkVerifier::verify(block.proof);
}

} // namespace gambit
