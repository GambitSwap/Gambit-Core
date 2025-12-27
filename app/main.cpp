#include <iostream>
#include <thread>
#include <memory>
#include <string>
#include <cstring>

#include "gambit/blockchain.hpp"
#include "gambit/p2p_node.hpp"
#include "gambit/keys.hpp"
#include "gambit/transaction.hpp"
#include "gambit/rpc_server.hpp"
#include "gambit/miner.hpp"
#include "gambit/zk_mining_engine.hpp"

using namespace gambit;

// Node configuration toggles
struct NodeConfig {
    bool enableP2P = true;
    bool enableRPC = false;
    bool enableMining = false;
    uint32_t mineBlocks = 0;  // 0 = don't mine specific blocks
    uint16_t p2pPort = 30303;
    uint16_t rpcPort = 8545;
};

void printHelp(const char* programName) {
    std::cout << "Usage: " << programName << " [options]\n\n";
    std::cout << "Options:\n";
    std::cout << "  --help              Show this help message\n";
    std::cout << "  --no-p2p            Disable P2P networking\n";
    std::cout << "  --enable-rpc        Enable RPC server\n";
    std::cout << "  --auto-mining       Enable continuous mining (5 sec interval)\n";
    std::cout << "  --mine-blocks=<n>   Mine N blocks then continue running\n";
    std::cout << "  --p2p-port=<port>   Set P2P port (default: 30303)\n";
    std::cout << "  --rpc-port=<port>   Set RPC port (default: 8545)\n";
    std::cout << "\nExamples:\n";
    std::cout << "  " << programName << " --mine-blocks=10 --enable-rpc\n";
    std::cout << "  " << programName << " --auto-mining --rpc-port=8080\n";
    std::cout << "  " << programName << " --no-p2p --mine-blocks=5\n";
}

bool parseArgs(int argc, char* argv[], NodeConfig& config) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "--help" || arg == "-h") {
            printHelp(argv[0]);
            return false;
        }
        else if (arg == "--no-p2p") {
            config.enableP2P = false;
        }
        else if (arg == "--enable-rpc") {
            config.enableRPC = true;
        }
        else if (arg == "--auto-mining") {
            config.enableMining = true;
        }
        else if (arg.rfind("--mine-blocks=", 0) == 0) {
            std::string numStr = arg.substr(14);
            try {
                int num = std::stoi(numStr);
                if (num < 1) {
                    std::cerr << "Error: --mine-blocks must be at least 1\n";
                    return false;
                }
                config.mineBlocks = static_cast<uint32_t>(num);
            } catch (...) {
                std::cerr << "Error: Invalid block count: " << numStr << "\n";
                return false;
            }
        }
        else if (arg.rfind("--p2p-port=", 0) == 0) {
            std::string portStr = arg.substr(11);
            try {
                int port = std::stoi(portStr);
                if (port < 1 || port > 65535) {
                    std::cerr << "Error: P2P port must be between 1 and 65535\n";
                    return false;
                }
                config.p2pPort = static_cast<uint16_t>(port);
            } catch (...) {
                std::cerr << "Error: Invalid P2P port number: " << portStr << "\n";
                return false;
            }
        }
        else if (arg.rfind("--rpc-port=", 0) == 0) {
            std::string portStr = arg.substr(11);
            try {
                int port = std::stoi(portStr);
                if (port < 1 || port > 65535) {
                    std::cerr << "Error: RPC port must be between 1 and 65535\n";
                    return false;
                }
                config.rpcPort = static_cast<uint16_t>(port);
            } catch (...) {
                std::cerr << "Error: Invalid RPC port number: " << portStr << "\n";
                return false;
            }
        }
        else {
            std::cerr << "Error: Unknown option: " << arg << "\n";
            std::cerr << "Use --help for usage information.\n";
            return false;
        }
    }
    return true;
}

int main(int argc, char* argv[]) {
    // Parse command line arguments
    NodeConfig config;
    if (!parseArgs(argc, argv, config)) {
        return 1;
    }
    std::cout << "=== Gambit Node Starting ===\n";

    // Start Genesis Creation

    std::cout << "=== Creating Gambit Genesis ===\n";
    // 1. Create keypairs
    KeyPair kpA = KeyPair::random();
    KeyPair kpB = KeyPair::random();

    Address addrA = kpA.address();
    Address addrB = kpB.address();

    std::cout << "Address A: " << addrA.toHex() << "\n";
    std::cout << "Address B: " << addrB.toHex() << "\n";

    // 2. Genesis config
    GenesisConfig genesis;
    genesis.chainId = 1337;
    genesis.premine.push_back({ addrA, 1000 });

    // 3. Blockchain
    Blockchain chain(genesis);
    
    std::cout << "=== Gambit Genesis Created ===\n";
    
    // add in genesis verification;
    
    std::cout << "Genesis block hash: " << chain.chain().back().hash << "\n";

    // 4. P2P node (optional)
    std::unique_ptr<P2PNode> node;
    if (config.enableP2P) {
        node = std::make_unique<P2PNode>(chain, config.p2pPort);
        node->start();
        std::cout << "P2P node listening on port " << config.p2pPort << "\n";
    } else {
        std::cout << "P2P networking disabled\n";
    }

    // 5. RPC server (optional)
    std::unique_ptr<RpcServer> rpc;
    if (config.enableRPC) {
        rpc = std::make_unique<RpcServer>(chain, config.rpcPort);
        rpc->start();
        std::cout << "RPC ready on http://127.0.0.1:" << config.rpcPort << "\n";
    } else {
        std::cout << "RPC server disabled\n";
    }

    // 6. Start miner (optional, mine every 5 seconds)
    std::unique_ptr<ZkMiningEngine> engine;
    std::unique_ptr<Miner> miner;
    if (config.enableMining) {
        if (!node) {
            std::cout << "Warning: Mining enabled but P2P disabled - blocks won't be broadcast\n";
        }
        engine = std::make_unique<ZkMiningEngine>();
        // Note: Miner needs P2PNode reference - create dummy or handle nullptr
        if (node) {
            miner = std::make_unique<Miner>(chain, *node, *engine);
            miner->setInterval(std::chrono::seconds(5));
            miner->start();
            std::cout << "Miner started (5 second interval)\n";
        } else {
            std::cout << "Mining skipped - requires P2P node\n";
        }
    }

    // Mine N blocks if requested
    if (config.mineBlocks > 0) {
        std::cout << "Mining " << config.mineBlocks << " blocks...\n";
        for (uint32_t i = 0; i < config.mineBlocks; ++i) {
            Block block = chain.mineBlock();
            std::cout << "Mined block #" << block.index << " hash=" << block.hash << "\n";
            if (node) {
                node->broadcastNewBlock(block);
            }
            // Pause between blocks (skip on last block)
            if (i + 1 < config.mineBlocks) {
                std::this_thread::sleep_for(std::chrono::seconds(5));
            }
        }
        std::cout << "Finished mining " << config.mineBlocks << " blocks.\n";
    }

    // Print active configuration
    std::cout << "\n=== Configuration ===\n";
    std::cout << "P2P:         " << (config.enableP2P ? "enabled (port " + std::to_string(config.p2pPort) + ")" : "disabled") << "\n";
    std::cout << "RPC:         " << (config.enableRPC ? "enabled (port " + std::to_string(config.rpcPort) + ")" : "disabled") << "\n";
    std::cout << "Auto-mining: " << (config.enableMining ? "enabled" : "disabled") << "\n";
    if (config.mineBlocks > 0) {
        std::cout << "Mine blocks: " << config.mineBlocks << " (completed)\n";
    }
    std::cout << "====================\n\n";

    // Keep node alive
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
    
    // Cleanup (unreachable in current loop, but here for completeness)
    if (miner) miner->stop();
    if (rpc) rpc->stop();
    if (node) node->stop();
    return 0;
}

/**
 RPC API Examples:

 curl -X POST http://127.0.0.1:8545 \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","method":"eth_blockNumber","params":[],"id":1}'

  curl -X POST http://127.0.0.1:8545 \
  -H "Content-Type: application/json" \
  -d '{"jsonrpc":"2.0","method":"eth_getBalance","params":["<Address A here>"],"id":2}'

  curl -X POST http://127.0.0.1:8545 \
  -H "Content-Type: application/json" \
  -d '{
    "jsonrpc":"2.0",
    "method":"eth_sendRawTransaction",
    "params":["0xf86c808504a817c80082520894..."],
    "id":1
  }'
 */
