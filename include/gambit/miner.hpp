#pragma once
#include <thread>
#include <atomic>
#include <chrono>

#include "gambit/mining_engine.hpp"
#include "gambit/p2p_node.hpp"

namespace gambit {

class Miner {
public:
    Miner(Blockchain& chain, P2PNode& p2p, MiningEngine& engine);

    void start();
    void stop();
    void setInterval(std::chrono::milliseconds ms);

    // For external miners
    Block getWork();
    bool submitWork(const Block& b);

private:
    Blockchain& chain_;
    P2PNode& p2p_;
    MiningEngine& engine_;

    std::chrono::milliseconds interval_{1000};
    std::thread thread_;
    std::atomic<bool> running_{false};

    void loop();
};

} // namespace gambit
