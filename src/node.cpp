#include "gambit/node.hpp"
#include <iostream>
#include <thread>

namespace gambit {

Node::Node(const NodeConfig& config)
    : config_(config) {
    initGenesis();
}

Node::~Node() {
    stop();
}

void Node::initGenesis() {
    // Dev keypair - deterministic seed for testing
    // Private key: 0xac0974bec39a17e36ba4a6b4d238ff944bacb478cbed5efcae784d7bf4f2ff80
    // (This is a well-known Hardhat/Foundry dev key - DO NOT use in production!)
    Bytes32 devPrivKey = {
        0xac, 0x09, 0x74, 0xbe, 0xc3, 0x9a, 0x17, 0xe3,
        0x6b, 0xa4, 0xa6, 0xb4, 0xd2, 0x38, 0xff, 0x94,
        0x4b, 0xac, 0xb4, 0x78, 0xcb, 0xed, 0x5e, 0xfc,
        0xae, 0x78, 0x4d, 0x7b, 0xf4, 0xf2, 0xff, 0x80
    };

    KeyPair devKey = KeyPair::fromPrivateKey(devPrivKey);
    Address coinbase = devKey.address();

    GenesisConfig genesis;
    genesis.chainId = config_.chainId;
    genesis.premine.push_back({ coinbase, config_.premineAmount });

    chain_ = std::make_unique<Blockchain>(genesis);

    emitStatus("Genesis block created. Chain ID: " + std::to_string(config_.chainId));
}

void Node::start() {
    if (running_) return;
    running_ = true;

    const Block& genesisBlock = chain_->chain().front();
    emitStatus("Genesis Hash: " + genesisBlock.hash);
    emitStatus("State Root: " + genesisBlock.stateAfter);

    // Start P2P node if enabled
    if (config_.enableP2P) {
        p2pNode_ = std::make_unique<P2PNode>(*chain_, config_.p2pPort);
        p2pNode_->start();
        emitStatus("P2P node listening on port " + std::to_string(config_.p2pPort));
    } else {
        emitStatus("P2P networking disabled");
    }

    // Start RPC server if enabled
    if (config_.enableRPC) {
        rpcServer_ = std::make_unique<RpcServer>(*chain_, config_.rpcPort);
        rpcServer_->start();
        emitStatus("RPC ready on http://127.0.0.1:" + std::to_string(config_.rpcPort));
    } else {
        emitStatus("RPC server disabled");
    }

    // Start mining if enabled
    if (config_.enableMining) {
        startMining();
    }

    // Mine specific number of blocks if requested
    if (config_.mineBlocks > 0) {
        emitStatus("Mining " + std::to_string(config_.mineBlocks) + " blocks...");
        for (uint32_t i = 0; i < config_.mineBlocks; ++i) {
            Block block = mineOneBlock();
            emitStatus("Mined block #" + std::to_string(block.index) + 
                      " hash=" + block.hash);
            
            // Pause between blocks (skip on last block)
            if (i + 1 < config_.mineBlocks) {
                std::this_thread::sleep_for(std::chrono::seconds(5));
            }
        }
        emitStatus("Finished mining " + std::to_string(config_.mineBlocks) + " blocks.");
    }
}

void Node::stop() {
    if (!running_) return;
    running_ = false;

    stopMining();

    if (rpcServer_) {
        rpcServer_->stop();
        rpcServer_.reset();
    }

    if (p2pNode_) {
        p2pNode_->stop();
        p2pNode_.reset();
    }

    emitStatus("Node stopped");
}

void Node::startMining() {
    if (miningActive_) return;

    if (!p2pNode_) {
        emitStatus("Warning: Mining enabled but P2P disabled - blocks won't be broadcast");
    }

    miningEngine_ = std::make_unique<ZkMiningEngine>();
    
    if (p2pNode_) {
        miner_ = std::make_unique<Miner>(*chain_, *p2pNode_, *miningEngine_);
        miner_->setInterval(std::chrono::seconds(5));
        miner_->start();
        miningActive_ = true;
        emitStatus("Miner started (5 second interval)");
    } else {
        emitStatus("Mining skipped - requires P2P node");
    }
}

void Node::stopMining() {
    if (!miningActive_) return;
    miningActive_ = false;

    if (miner_) {
        miner_->stop();
        miner_.reset();
    }
    miningEngine_.reset();
    
    emitStatus("Miner stopped");
}

Block Node::mineOneBlock() {
    Block block = chain_->mineBlock();
    
    if (p2pNode_) {
        p2pNode_->broadcastNewBlock(block);
    }

    if (blockCallback_) {
        blockCallback_(block);
    }

    return block;
}

uint64_t Node::blockHeight() const {
    return chain_->chain().size() - 1;
}

void Node::emitStatus(const std::string& msg) {
    std::cout << msg << std::endl;
    if (statusCallback_) {
        statusCallback_(msg);
    }
}

} // namespace gambit

