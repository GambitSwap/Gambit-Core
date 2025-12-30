#include <iostream>
#include <thread>
#include <memory>
#include <atomic>
#include <vector>
#include <sstream>

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Tabs.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Output.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Text_Buffer.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Scroll.H>
#include <FL/Fl_Pack.H>

#include "gambit_gui.hpp"
#include "gambit/node.hpp"
#include "gambit/wallet.hpp"

// Forward declare RPC client for remote mode
#include "rpc_client.hpp"

using namespace gambit;

class GambitGUI {
public:
    // Embedded mode - uses Node directly
    explicit GambitGUI(Node* node)
        : node_(node), rpcClient_(nullptr), isRemoteMode_(false) {
        createGUI();
        updateStatus();
        
        // Set up node callbacks for real-time updates
        if (node_) {
            node_->setStatusCallback([this](const std::string& msg) {
                Fl::lock();
                appendLog(msg);
                Fl::unlock();
                Fl::awake();
            });
            
            node_->setBlockCallback([this](const Block& block) {
                Fl::lock();
                updateStatus();
                Fl::unlock();
                Fl::awake();
            });
        }
    }
    
    // RPC client mode - connects to remote node
    explicit GambitGUI(const std::string& rpcUrl)
        : node_(nullptr), rpcClient_(std::make_unique<RpcClient>(rpcUrl)), isRemoteMode_(true) {
        createGUI();
        updateStatus();
        
        // Start polling for updates in remote mode
        startPolling();
    }
    
    ~GambitGUI() {
        stopPolling();
    }
    
    void run() {
        window_->show();
        Fl::run();
    }
    
private:
    void createGUI() {
        window_ = new Fl_Window(1200, 800, "Gambit Blockchain Node");
        
        tabs_ = new Fl_Tabs(10, 10, 1180, 780);
        
        createDashboardTab();
        createBlocksTab();
        createWalletTab();
        createTransactionsTab();
        createLogTab();
        
        tabs_->end();
        window_->end();
    }
    
    void createDashboardTab() {
        Fl_Group* dashboard = new Fl_Group(10, 35, 1180, 755, "Dashboard");
        
        // Title
        Fl_Output* title = new Fl_Output(20, 50, 400, 30);
        title->value("Gambit Node Status");
        title->textsize(20);
        title->box(FL_FLAT_BOX);
        
        // Mode indicator
        modeDisplay_ = new Fl_Output(150, 90, 300, 25, "Mode:");
        modeDisplay_->value(isRemoteMode_ ? "Remote (RPC Client)" : "Embedded");
        
        // Status displays
        blockHeightDisplay_ = new Fl_Output(150, 125, 300, 25, "Block Height:");
        chainIdDisplay_ = new Fl_Output(150, 160, 300, 25, "Chain ID:");
        p2pStatusDisplay_ = new Fl_Output(150, 195, 300, 25, "P2P Status:");
        rpcStatusDisplay_ = new Fl_Output(150, 230, 300, 25, "RPC Status:");
        
        // Buttons (only in embedded mode)
        if (!isRemoteMode_) {
            miningButton_ = new Fl_Button(20, 280, 150, 30, "Start Mining");
            miningButton_->callback(miningCallback, this);
            
            mineBlockButton_ = new Fl_Button(180, 280, 150, 30, "Mine Block");
            mineBlockButton_->callback(mineBlockCallback, this);
        }
        
        // Refresh button (for remote mode)
        refreshButton_ = new Fl_Button(340, 280, 100, 30, "Refresh");
        refreshButton_->callback(refreshCallback, this);
        
        dashboard->end();
    }
    
    void createBlocksTab() {
        Fl_Group* blocksTab = new Fl_Group(10, 35, 1180, 755, "Blocks");
        
        blockSearchInput_ = new Fl_Input(130, 50, 200, 25, "Block Number:");
        blockSearchButton_ = new Fl_Button(340, 50, 80, 25, "Search");
        blockSearchButton_->callback(searchBlockCallback, this);
        
        blockDisplay_ = new Fl_Text_Display(20, 85, 1140, 650);
        blockBuffer_ = new Fl_Text_Buffer();
        blockDisplay_->buffer(blockBuffer_);
        
        blocksTab->end();
    }
    
    void createWalletTab() {
        Fl_Group* walletTab = new Fl_Group(10, 35, 1180, 755, "Wallet");
        
        createWalletButton_ = new Fl_Button(20, 50, 120, 30, "Create Wallet");
        createWalletButton_->callback(createWalletCallback, this);
        
        loadWalletButton_ = new Fl_Button(150, 50, 120, 30, "Load Wallet");
        loadWalletButton_->callback(loadWalletCallback, this);
        
        walletDisplay_ = new Fl_Text_Display(20, 90, 1140, 650);
        walletBuffer_ = new Fl_Text_Buffer();
        walletDisplay_->buffer(walletBuffer_);
        
        walletTab->end();
    }
    
    void createTransactionsTab() {
        Fl_Group* txTab = new Fl_Group(10, 35, 1180, 755, "Transactions");
        
        sendTxButton_ = new Fl_Button(20, 50, 150, 30, "Send Transaction");
        sendTxButton_->callback(sendTxCallback, this);
        
        txDisplay_ = new Fl_Text_Display(20, 90, 1140, 650);
        txBuffer_ = new Fl_Text_Buffer();
        txDisplay_->buffer(txBuffer_);
        
        txTab->end();
    }
    
    void createLogTab() {
        Fl_Group* logTab = new Fl_Group(10, 35, 1180, 755, "Log");
        
        logDisplay_ = new Fl_Text_Display(20, 50, 1140, 700);
        logBuffer_ = new Fl_Text_Buffer();
        logDisplay_->buffer(logBuffer_);
        
        logTab->end();
    }
    
    void updateStatus() {
        if (isRemoteMode_) {
            updateStatusFromRPC();
        } else {
            updateStatusFromNode();
        }
        refreshBlocks();
    }
    
    void updateStatusFromNode() {
        if (!node_) return;
        
        blockHeightDisplay_->value(std::to_string(node_->blockHeight()).c_str());
        chainIdDisplay_->value(std::to_string(node_->chainId()).c_str());
        p2pStatusDisplay_->value(node_->isP2PConnected() ? "Connected" : "Disconnected");
        rpcStatusDisplay_->value(node_->isRPCRunning() ? "Running" : "Stopped");
        
        if (miningButton_) {
            miningButton_->label(node_->isMining() ? "Stop Mining" : "Start Mining");
        }
    }
    
    void updateStatusFromRPC() {
        if (!rpcClient_) return;
        
        try {
            uint64_t blockNum = rpcClient_->getBlockNumber();
            blockHeightDisplay_->value(std::to_string(blockNum).c_str());
            
            uint64_t chainId = rpcClient_->getChainId();
            chainIdDisplay_->value(std::to_string(chainId).c_str());
            
            p2pStatusDisplay_->value("N/A (Remote)");
            rpcStatusDisplay_->value("Connected");
        } catch (const std::exception& e) {
            rpcStatusDisplay_->value("Connection Error");
            appendLog(std::string("RPC Error: ") + e.what());
        }
    }
    
    void refreshBlocks() {
        std::stringstream ss;
        ss << "Recent Blocks:\n\n";
        
        if (isRemoteMode_) {
            refreshBlocksFromRPC(ss);
        } else {
            refreshBlocksFromNode(ss);
        }
        
        blockBuffer_->text(ss.str().c_str());
    }
    
    void refreshBlocksFromNode(std::stringstream& ss) {
        if (!node_) return;
        
        const auto& chain = node_->blockchain().chain();
        size_t start = chain.size() > 10 ? chain.size() - 10 : 0;
        
        for (size_t i = start; i < chain.size(); ++i) {
            const Block& block = chain[i];
            ss << "Block #" << block.index 
               << " | Hash: " << block.hash.substr(0, 20) << "..."
               << " | Tx: " << block.transactions.size() << "\n";
        }
    }
    
    void refreshBlocksFromRPC(std::stringstream& ss) {
        if (!rpcClient_) return;
        
        try {
            uint64_t height = rpcClient_->getBlockNumber();
            uint64_t start = height > 10 ? height - 10 : 0;
            
            for (uint64_t i = start; i <= height; ++i) {
                auto blockInfo = rpcClient_->getBlockByNumber(i);
                ss << "Block #" << i 
                   << " | Hash: " << blockInfo.hash.substr(0, 20) << "..."
                   << " | Tx: " << blockInfo.txCount << "\n";
            }
        } catch (const std::exception& e) {
            ss << "Error fetching blocks: " << e.what() << "\n";
        }
    }
    
    void appendLog(const std::string& msg) {
        if (logBuffer_) {
            logBuffer_->append((msg + "\n").c_str());
            // Scroll to bottom
            if (logDisplay_) {
                logDisplay_->scroll(logBuffer_->count_lines(0, logBuffer_->length()), 0);
            }
        }
    }
    
    // Polling for remote mode
    void startPolling() {
        if (!isRemoteMode_) return;
        
        polling_ = true;
        pollThread_ = std::thread([this]() {
            while (polling_) {
                std::this_thread::sleep_for(std::chrono::seconds(5));
                if (polling_) {
                    Fl::lock();
                    updateStatus();
                    Fl::unlock();
                    Fl::awake();
                }
            }
        });
    }
    
    void stopPolling() {
        polling_ = false;
        if (pollThread_.joinable()) {
            pollThread_.join();
        }
    }
    
    // Static callback functions
    static void miningCallback(Fl_Widget*, void* data) {
        static_cast<GambitGUI*>(data)->toggleMining();
    }
    
    static void mineBlockCallback(Fl_Widget*, void* data) {
        static_cast<GambitGUI*>(data)->mineBlock();
    }
    
    static void searchBlockCallback(Fl_Widget*, void* data) {
        static_cast<GambitGUI*>(data)->searchBlock();
    }
    
    static void createWalletCallback(Fl_Widget*, void* data) {
        static_cast<GambitGUI*>(data)->createWallet();
    }
    
    static void loadWalletCallback(Fl_Widget*, void* data) {
        static_cast<GambitGUI*>(data)->loadWallet();
    }
    
    static void sendTxCallback(Fl_Widget*, void* data) {
        static_cast<GambitGUI*>(data)->sendTransaction();
    }
    
    static void refreshCallback(Fl_Widget*, void* data) {
        static_cast<GambitGUI*>(data)->updateStatus();
    }
    
    void toggleMining() {
        if (!node_) return;
        
        if (node_->isMining()) {
            node_->stopMining();
        } else {
            node_->startMining();
        }
        updateStatus();
    }
    
    void mineBlock() {
        if (!node_) return;
        
        try {
            Block block = node_->mineOneBlock();
            appendLog("Mined block #" + std::to_string(block.index) + " hash=" + block.hash);
            updateStatus();
        } catch (const std::exception& e) {
            appendLog(std::string("Failed to mine block: ") + e.what());
        }
    }
    
    void searchBlock() {
        std::string blockNumStr = blockSearchInput_->value();
        try {
            uint64_t blockNum = std::stoull(blockNumStr);
            
            if (isRemoteMode_) {
                searchBlockFromRPC(blockNum);
            } else {
                searchBlockFromNode(blockNum);
            }
        } catch (...) {
            blockBuffer_->text("Invalid block number");
        }
    }
    
    void searchBlockFromNode(uint64_t blockNum) {
        if (!node_) return;
        
        const auto& chain = node_->blockchain().chain();
        if (blockNum < chain.size()) {
            const Block& block = chain[blockNum];
            std::stringstream ss;
            ss << "Block #" << block.index << "\n";
            ss << "Hash: " << block.hash << "\n";
            ss << "Prev Hash: " << block.prevHash << "\n";
            ss << "Transactions: " << block.transactions.size() << "\n";
            blockBuffer_->text(ss.str().c_str());
        } else {
            blockBuffer_->text("Block not found");
        }
    }
    
    void searchBlockFromRPC(uint64_t blockNum) {
        if (!rpcClient_) return;
        
        try {
            auto blockInfo = rpcClient_->getBlockByNumber(blockNum);
            std::stringstream ss;
            ss << "Block #" << blockNum << "\n";
            ss << "Hash: " << blockInfo.hash << "\n";
            ss << "Prev Hash: " << blockInfo.parentHash << "\n";
            ss << "Transactions: " << blockInfo.txCount << "\n";
            blockBuffer_->text(ss.str().c_str());
        } catch (const std::exception& e) {
            blockBuffer_->text(("Error: " + std::string(e.what())).c_str());
        }
    }
    
    void createWallet() {
        try {
            std::string path = "wallet.json";
            std::string password = "password";
            
            auto w = Wallet::create(path, password);
            w->addAccount("default", "m/44'/60'/0'/0/0");
            w->save(password);
            
            std::stringstream ss;
            ss << "Wallet created successfully\n\n";
            auto accounts = w->listAccounts();
            for (const auto& acc : accounts) {
                ss << "Account: " << acc.name << "\n";
                ss << "Address: " << acc.address.toHex() << "\n\n";
            }
            walletBuffer_->text(ss.str().c_str());
            appendLog("Wallet created successfully");
        } catch (const std::exception& e) {
            std::stringstream ss;
            ss << "Failed to create wallet: " << e.what();
            walletBuffer_->text(ss.str().c_str());
            appendLog(ss.str());
        }
    }
    
    void loadWallet() {
        walletBuffer_->text("Load wallet functionality not yet implemented");
    }
    
    void sendTransaction() {
        txBuffer_->text("Send transaction functionality not yet implemented");
    }
    
    // Node reference (embedded mode)
    Node* node_;
    
    // RPC client (remote mode)
    std::unique_ptr<RpcClient> rpcClient_;
    bool isRemoteMode_;
    
    // Polling thread for remote mode
    std::atomic<bool> polling_{false};
    std::thread pollThread_;
    
    // FLTK widgets
    Fl_Window* window_;
    Fl_Tabs* tabs_;
    
    Fl_Output* modeDisplay_;
    Fl_Output* blockHeightDisplay_;
    Fl_Output* chainIdDisplay_;
    Fl_Output* p2pStatusDisplay_;
    Fl_Output* rpcStatusDisplay_;
    
    Fl_Button* miningButton_ = nullptr;
    Fl_Button* mineBlockButton_ = nullptr;
    Fl_Button* refreshButton_;
    
    Fl_Input* blockSearchInput_;
    Fl_Button* blockSearchButton_;
    Fl_Text_Display* blockDisplay_;
    Fl_Text_Buffer* blockBuffer_;
    
    Fl_Button* createWalletButton_;
    Fl_Button* loadWalletButton_;
    Fl_Text_Display* walletDisplay_;
    Fl_Text_Buffer* walletBuffer_;
    
    Fl_Button* sendTxButton_;
    Fl_Text_Display* txDisplay_;
    Fl_Text_Buffer* txBuffer_;
    
    Fl_Text_Display* logDisplay_;
    Fl_Text_Buffer* logBuffer_;
};

namespace gambit {

void runGUI(Node& node) {
    Fl::lock();  // Enable thread-safe FLTK
    Fl::scheme("gtk+");
    GambitGUI gui(&node);
    gui.run();
}

void runGUIWithRPC(const std::string& rpcUrl) {
    Fl::lock();  // Enable thread-safe FLTK
    Fl::scheme("gtk+");
    GambitGUI gui(rpcUrl);
    gui.run();
}

} // namespace gambit

// Standalone main - for gambit_gui executable (RPC client mode only)
// When building as part of gambit_node, this is excluded
#ifndef GAMBIT_GUI_ENABLED
int main(int argc, char* argv[]) {
    std::string rpcUrl = "http://127.0.0.1:8545";
    
    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg.rfind("--rpc-url=", 0) == 0) {
            rpcUrl = arg.substr(10);
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Gambit GUI - Graphical interface for Gambit blockchain\n\n";
            std::cout << "Usage: " << argv[0] << " [options]\n\n";
            std::cout << "Options:\n";
            std::cout << "  --rpc-url=<url>  Connect to node via RPC (default: http://127.0.0.1:8545)\n";
            std::cout << "  --help           Show this help message\n";
            std::cout << "\nThe standalone GUI connects to a running Gambit node via RPC.\n";
            std::cout << "For embedded mode, run: gambit_node --gui\n";
            return 0;
        }
    }
    
    std::cout << "Connecting to Gambit node at " << rpcUrl << "...\n";
    gambit::runGUIWithRPC(rpcUrl);
    
    return 0;
}
#endif
