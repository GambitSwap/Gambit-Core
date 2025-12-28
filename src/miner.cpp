#include "gambit/miner.hpp"
#include <iostream>

namespace gambit {

Miner::Miner(Blockchain& chain, P2PNode& p2p, MiningEngine& engine)
    : chain_(chain), p2p_(p2p), engine_(engine) {}

void Miner::start() {
    running_ = true;
    thread_ = std::thread(&Miner::loop, this);
    std::cout << "[Miner] Started\n";
}

void Miner::stop() {
    running_ = false;
    if (thread_.joinable()) thread_.join();
    std::cout << "[Miner] Stopped\n";
}

void Miner::setInterval(std::chrono::milliseconds ms) {
    interval_ = ms;
}

Block Miner::getWork() {
    return engine_.buildBlockTemplate(chain_);
}

bool Miner::submitWork(const Block& b) {
    if (!engine_.validateMinedBlock(b, chain_)) {
        return false;
    }

    chain_.addBlock(b);
    p2p_.broadcastNewBlock(b);
    return true;
}

void Miner::loop() {
    while (running_) {
        try {
            Block block = engine_.buildBlockTemplate(chain_);
            chain_.addBlock(block);
            p2p_.broadcastNewBlock(block);
            std::cout << "[Miner] Mined block #" << block.index << "\n";
            std::cout << "Mined block #" << block.index 
                      << " hash=0x" << block.hash
                      << " proof=" << block.proof.proof
                      << " (nonce=" << block.nonce << ")\n";
        } catch (...) {
            // ignore
        }

        std::this_thread::sleep_for(interval_);
    }
}

} // namespace gambit
