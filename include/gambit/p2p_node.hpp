#pragma once
#include <vector>
#include <memory>
#include <mutex>
#include <thread>
#include <atomic>

#include "gambit/p2p_peer.hpp"
#include "gambit/blockchain.hpp"
#include "gambit/keys.hpp"
#include "gambit/zk_seeder_client.hpp"

namespace gambit {

class P2PNode {
public:
    P2PNode(Blockchain& chain, std::uint16_t listenPort);

    // In p2p_node.hpp, add to public methods:
    void bootstrapWithSeeder();

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
    // In p2p_node.hpp, add to private members:
    KeyPair keyPair_;
    std::string publicIp_;  // Add if you want to configure public IP
    std::mutex peersMutex_;
    std::vector<std::shared_ptr<Peer>> peers_;
    
    // In p2p_node.hpp, add to private methods:
    Address myNodeId() const;
    Bytes buildSeederProof(PeerInfo& peer);
    
    void acceptLoop();
    void onMessage(const Message& msg, std::shared_ptr<Peer> peer);
    // Handlers
    void handleNewTx(const Message& msg);
    void handleNewBlock(const Message& msg);
};

} // namespace gambit
