#pragma once
#include <vector>
#include <memory>
#include <mutex>
#include <thread>
#include <atomic>

#include "gambit/p2p_peer.hpp"
#include "gambit/blockchain.hpp"

namespace gambit {

class P2PNode {
public:
    P2PNode(Blockchain& chain, std::uint16_t listenPort);

    void start();
    void stop();

    void connectTo(const std::string& host, std::uint16_t port);

    void broadcastNewTx(const Transaction& tx);
    void broadcastNewBlock(const Block& block);

private:
    Blockchain& chain_;
    std::uint16_t listenPort_;
    int listenSock_{-1};
    std::thread acceptThread_;
    std::atomic<bool> running_{false};

    std::mutex peersMutex_;
    std::vector<std::shared_ptr<Peer>> peers_;

    void acceptLoop();
    void onMessage(const Message& msg, std::shared_ptr<Peer> peer);

    // Handlers
    void handleNewTx(const Message& msg);
    void handleNewBlock(const Message& msg);
};

} // namespace gambit
