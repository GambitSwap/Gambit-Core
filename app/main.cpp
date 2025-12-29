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
#include "gambit/wallet.hpp"

using namespace gambit;

// Node configuration toggles
struct NodeConfig {
    bool enableP2P = true;
    bool enableRPC = false;
    bool enableMining = false;
    bool enableWallet = false;
    bool enableGUI = false;

    uint32_t mineBlocks = 0;  // 0 = don't mine specific blocks
    uint16_t p2pPort = 30303;
    uint16_t rpcPort = 8545;
    uint64_t chainId = 1337;
    uint64_t premineAmount = 1000000;  // Default premine amount
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
    std::cout << "  --chain-id=<id>     Set chain ID (default: 1337)\n";
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
        else if (arg.rfind("--chain-id=", 0) == 0) {
            std::string idStr = arg.substr(11);
            try {
                config.chainId = std::stoull(idStr);
            } catch (...) {
                std::cerr << "Error: Invalid chain ID: " << idStr << "\n";
                return false;
            }
        }// In parseArgs function, add:
        else if (arg == "--wallet") {
            config.enableWallet = true;
        }
        else {
            std::cerr << "Error: Unknown option: " << arg << "\n";
            std::cerr << "Use --help for usage information.\n";
            return false;
        }
    }
    return true;
}

void walletCLI() {
    std::string command;
    std::cout << "\n[GAMBIT WALLET]\n";
    std::cout << "Commands: create, load, add-account, list, export-key, export-mnemonic\n";
    std::cout << "> ";
    std::getline(std::cin, command);
    
    if (command == "create") {
        std::string path;
        std::cout << "Wallet file path: ";
        std::getline(std::cin, path);
        
        std::string password;
        std::cout << "Set password: ";
        std::getline(std::cin, password);
        
        auto w = Wallet::create(path, password);
        w->addAccount("default", "m/44'/60'/0'/0/0");
        w->save(password);
        std::cout << "✓ Wallet created and saved\n";
    }
    else if (command == "load") {
        std::string path, password;
        std::cout << "Wallet file path: ";
        std::getline(std::cin, path);
        std::cout << "Password: ";
        std::getline(std::cin, password);
        
        auto w = Wallet::load(path, password);
        auto accounts = w->listAccounts();
        
        std::cout << "Accounts:\n";
        for (const auto& acc : accounts) {
            std::cout << "  " << acc.name << ": " << acc.address.toHex() << "\n";
        }
    }
    else if (command == "add-account") {
        // Similar pattern
    }
    else {
        std::cout << "Unknown command\n";
    }
}


int main(int argc, char* argv[]) {
    // Parse command line arguments
    NodeConfig config;
    if (!parseArgs(argc, argv, config)) {
        return 1;
    }
    if (config.enableWallet) {
        walletCLI();
        return 0;
    }
    std::cout << "=== Gambit Node Starting ===\n";
    
    // ========================================
    // Genesis Configuration
    // ========================================
    // Using deterministic dev keypairs for reproducible testing.
    // In production, these would be loaded from a config file or generated once.
    
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
/*

    VMRegistry vmRegistry;
    initBuiltInVMs(vmRegistry);
    
    VMPluginLoader loader(vmRegistry);
    loader.loadFromDirectory("plugins");
    
    // now Executor can use any VM type that exists in registry
*/      

    std::cout << "\n=== Genesis Configuration ===\n";
    std::cout << "Chain ID:      " << config.chainId << "\n";
    std::cout << "Coinbase:      " << coinbase.toHex() << "\n";
    std::cout << "Premine:       " << config.premineAmount << "\n";

    // Create genesis config
    GenesisConfig genesis;
    genesis.chainId = config.chainId;
    genesis.premine.push_back({ coinbase, config.premineAmount });

    // Initialize blockchain with genesis
    Blockchain chain(genesis);
    
    const Block& genesisBlock = chain.chain().front();
    std::cout << "Genesis Hash:  " << genesisBlock.hash << "\n";
    std::cout << "State Root:    " << genesisBlock.stateAfter << "\n";
    std::cout << "==============================\n\n";

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
            std::cout << "Mined block #" << block.index 
                      << " hash=" << block.hash
                      << " proof=" << block.proof.proof
                      << " (nonce=" << block.nonce << ")\n";
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
    std::cout << "\n=== Node Status ===\n";
    std::cout << "Chain ID:    " << config.chainId << "\n";
    std::cout << "P2P:         " << (config.enableP2P ? "enabled (port " + std::to_string(config.p2pPort) + ")" : "disabled") << "\n";
    std::cout << "RPC:         " << (config.enableRPC ? "enabled (port " + std::to_string(config.rpcPort) + ")" : "disabled") << "\n";
    std::cout << "Auto-mining: " << (config.enableMining ? "enabled" : "disabled") << "\n";
    std::cout << "Block height: " << chain.chain().size() - 1 << "\n";
    if (config.mineBlocks > 0) {
        std::cout << "Mine blocks: " << config.mineBlocks << " (completed)\n";
    }
    std::cout << "===================\n\n";
    std::cout << "Node running. Press Ctrl+C to stop.\n";

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
