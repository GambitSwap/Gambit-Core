#pragma once
#include <memory>
#include <atomic>
#include <cstdint>
#include <string>
#include <functional>

#include "gambit/blockchain.hpp"
#include "gambit/p2p_node.hpp"
#include "gambit/rpc_server.hpp"
#include "gambit/miner.hpp"
#include "gambit/zk_mining_engine.hpp"
#include "gambit/keys.hpp"
#include "gambit/genesis.hpp"

namespace gambit {

// Node configuration - shared between CLI and GUI
struct NodeConfig {
    bool enableP2P = true;
    bool enableRPC = false;
    bool enableMining = false;
    bool enableGUI = false;

    uint32_t mineBlocks = 0;  // 0 = don't mine specific blocks
    uint16_t p2pPort = 30303;
    uint16_t rpcPort = 8545;
    uint64_t chainId = 1337;
    uint64_t premineAmount = 1000000;

    // For GUI RPC client mode
    std::string rpcUrl;  // If set, GUI connects to remote node instead
};

// Callback types for node events
using BlockCallback = std::function<void(const Block&)>;
using StatusCallback = std::function<void(const std::string&)>;

class Node {
public:
    explicit Node(const NodeConfig& config);
    ~Node();

    // Lifecycle
    void start();
    void stop();
    bool isRunning() const { return running_; }

    // Mining controls
    void startMining();
    void stopMining();
    bool isMining() const { return miningActive_; }
    Block mineOneBlock();

    // Event callbacks (for GUI updates)
    void setBlockCallback(BlockCallback cb) { blockCallback_ = std::move(cb); }
    void setStatusCallback(StatusCallback cb) { statusCallback_ = std::move(cb); }

    // Accessors for GUI
    Blockchain& blockchain() { return *chain_; }
    const Blockchain& blockchain() const { return *chain_; }
    
    P2PNode* p2pNode() { return p2pNode_.get(); }
    const P2PNode* p2pNode() const { return p2pNode_.get(); }
    
    RpcServer* rpcServer() { return rpcServer_.get(); }
    const RpcServer* rpcServer() const { return rpcServer_.get(); }

    const NodeConfig& config() const { return config_; }

    // Status info
    uint64_t blockHeight() const;
    uint64_t chainId() const { return config_.chainId; }
    bool isP2PConnected() const { return p2pNode_ != nullptr; }
    bool isRPCRunning() const { return rpcServer_ != nullptr; }

private:
    NodeConfig config_;
    std::atomic<bool> running_{false};
    std::atomic<bool> miningActive_{false};

    // Core components
    std::unique_ptr<Blockchain> chain_;
    std::unique_ptr<P2PNode> p2pNode_;
    std::unique_ptr<RpcServer> rpcServer_;
    std::unique_ptr<ZkMiningEngine> miningEngine_;
    std::unique_ptr<Miner> miner_;

    // Callbacks
    BlockCallback blockCallback_;
    StatusCallback statusCallback_;

    void initGenesis();
    void emitStatus(const std::string& msg);
};

} // namespace gambit

