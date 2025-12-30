#include <iostream>
#include <thread>
#include <memory>
#include <string>
#include <cstring>

#include "gambit/node.hpp"
#include "gambit/wallet.hpp"

// GUI support - conditionally included
#ifdef GAMBIT_GUI_ENABLED
#include "gambit_gui.hpp"
#endif

using namespace gambit;

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
    std::cout << "  --wallet            Launch wallet CLI\n";
#ifdef GAMBIT_GUI_ENABLED
    std::cout << "  --gui               Launch graphical user interface\n";
#endif
    std::cout << "\nExamples:\n";
    std::cout << "  " << programName << " --mine-blocks=10 --enable-rpc\n";
    std::cout << "  " << programName << " --auto-mining --rpc-port=8080\n";
    std::cout << "  " << programName << " --no-p2p --mine-blocks=5\n";
#ifdef GAMBIT_GUI_ENABLED
    std::cout << "  " << programName << " --gui --enable-rpc\n";
#endif
}

bool parseArgs(int argc, char* argv[], NodeConfig& config, bool& enableWallet) {
    enableWallet = false;
    
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
        else if (arg == "--gui") {
#ifdef GAMBIT_GUI_ENABLED
            config.enableGUI = true;
#else
            std::cerr << "Error: GUI support not compiled. Rebuild with GAMBIT_BUILD_GUI=ON\n";
            return false;
#endif
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
        }
        else if (arg == "--wallet") {
            enableWallet = true;
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
        std::cout << "Wallet created and saved\n";
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

void runCLI(Node& node) {
    std::cout << "\n=== Node Status ===\n";
    std::cout << "Chain ID:    " << node.config().chainId << "\n";
    std::cout << "P2P:         " << (node.config().enableP2P ? "enabled (port " + std::to_string(node.config().p2pPort) + ")" : "disabled") << "\n";
    std::cout << "RPC:         " << (node.config().enableRPC ? "enabled (port " + std::to_string(node.config().rpcPort) + ")" : "disabled") << "\n";
    std::cout << "Auto-mining: " << (node.config().enableMining ? "enabled" : "disabled") << "\n";
    std::cout << "Block height: " << node.blockHeight() << "\n";
    if (node.config().mineBlocks > 0) {
        std::cout << "Mine blocks: " << node.config().mineBlocks << " (completed)\n";
    }
    std::cout << "===================\n\n";
    std::cout << "Node running. Press Ctrl+C to stop.\n";

    // Keep node alive
    while (node.isRunning()) {
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}

int main(int argc, char* argv[]) {
    // Parse command line arguments
    NodeConfig config;
    bool enableWallet = false;
    
    if (!parseArgs(argc, argv, config, enableWallet)) {
        return 1;
    }
    
    // Wallet mode
    if (enableWallet) {
        walletCLI();
        return 0;
    }
    
    std::cout << "=== Gambit Node Starting ===\n";

    // Create and start node
    Node node(config);
    node.start();

#ifdef GAMBIT_GUI_ENABLED
    if (config.enableGUI) {
        // Run GUI mode - FLTK event loop takes over
        runGUI(node);
        return 0;
    }
#endif

    // CLI mode - blocking loop
    runCLI(node);
    
    // Cleanup
    node.stop();
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
