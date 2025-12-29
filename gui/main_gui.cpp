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

#include "gambit/blockchain.hpp"
#include "gambit/p2p_node.hpp"
#include "gambit/rpc_server.hpp"
#include "gambit/miner.hpp"
#include "gambit/zk_mining_engine.hpp"
#include "gambit/wallet.hpp"

using namespace gambit;

class GambitGUI {
public:
    GambitGUI(uint64_t chainId = 1337)
        : chainId_(chainId), miningActive_(false) {
        
        // Initialize genesis (same as CLI version)
        Bytes32 devPrivKey = {
            0xac, 0x09, 0x74, 0xbe, 0xc3, 0x9a, 0x17, 0xe3,
            0x6b, 0xa4, 0xa6, 0xb4, 0xd2, 0x38, 0xff, 0x94,
            0x4b, 0xac, 0xb4, 0x78, 0xcb, 0xed, 0x5e, 0xfc,
            0xae, 0x78, 0x4d, 0x7b, 0xf4, 0xf2, 0xff, 0x80
        };
        
        KeyPair devKey = KeyPair::fromPrivateKey(devPrivKey);
        Address coinbase = devKey.address();
        
        GenesisConfig genesis;
        genesis.chainId = chainId_;
        genesis.premine.push_back({ coinbase, 1000000 });
        
        chain_ = std::make_unique<Blockchain>(genesis);
        
        createGUI();
        updateStatus();
    }
    
    void run() {
        window_->show();
        Fl::run();
    }
    
private:
    void createGUI() {
        window_ = new Fl_Window(1200, 800, "Gambit Blockchain Node");
        
        tabs_ = new Fl_Tabs(10, 10, 1180, 780);
        
        // Dashboard Tab
        createDashboardTab();
        
        // Blocks Tab  
        createBlocksTab();
        
        // Wallet Tab
        createWalletTab();
        
        // Transactions Tab
        createTransactionsTab();
        
        tabs_->end();
        window_->end();
    }
    
    void createDashboardTab() {
        Fl_Group* dashboard = new Fl_Group(10, 35, 1180, 755, "Dashboard");
        
        Fl_Pack* pack = new Fl_Pack(20, 50, 1140, 700);
        pack->type(Fl_Pack::VERTICAL);
        pack->spacing(10);
        
        // Title
        Fl_Output* title = new Fl_Output(0, 0, 1140, 30);
        title->value("Gambit Node Status");
        title->textsize(24);
        
        // Status displays
        Fl_Group* statusGroup = new Fl_Group(0, 0, 1140, 150);
        statusGroup->box(FL_DOWN_BOX);

        blockHeightDisplay_ = new Fl_Output(20, 20, 300, 25, "Block Height:");
        chainIdDisplay_ = new Fl_Output(20, 55, 300, 25, "Chain ID:");
        p2pStatusDisplay_ = new Fl_Output(20, 90, 300, 25, "P2P Status:");
        rpcStatusDisplay_ = new Fl_Output(20, 125, 300, 25, "RPC Status:");
        
        statusGroup->end();
        
        // Buttons
        Fl_Group* buttonGroup = new Fl_Group(0, 0, 1140, 50);
        miningButton_ = new Fl_Button(20, 10, 150, 30, "Start Mining");
        miningButton_->callback(miningCallback, this);
        
        mineBlockButton_ = new Fl_Button(180, 10, 150, 30, "Mine Block");
        mineBlockButton_->callback(mineBlockCallback, this);
        
        buttonGroup->end();
        
        pack->end();
        dashboard->end();
    }
    
    void createBlocksTab() {
        Fl_Group* blocksTab = new Fl_Group(10, 35, 1180, 755, "Blocks");
        
        // Search controls
        blockSearchInput_ = new Fl_Input(20, 50, 200, 25, "Block Number:");
        blockSearchButton_ = new Fl_Button(230, 50, 80, 25, "Search");
        blockSearchButton_->callback(searchBlockCallback, this);
        
        // Block list
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
    
    void updateStatus() {
        blockHeightDisplay_->value(std::to_string(chain_->chain().size() - 1).c_str());
        chainIdDisplay_->value(std::to_string(chainId_).c_str());
        p2pStatusDisplay_->value(p2pNode_ ? "Connected" : "Disconnected");
        rpcStatusDisplay_->value(rpcServer_ ? "Running" : "Stopped");
        
        miningButton_->label(miningActive_ ? "Stop Mining" : "Start Mining");
        
        refreshBlocks();
    }
    
    void refreshBlocks() {
        std::stringstream ss;
        const auto& chain = chain_->chain();
        
        ss << "Recent Blocks:\n\n";
        
        // Show last 10 blocks
        size_t start = chain.size() > 10 ? chain.size() - 10 : 0;
        for (size_t i = start; i < chain.size(); ++i) {
            const Block& block = chain[i];
            ss << "Block #" << block.index 
               << " | Hash: " << block.hash.substr(0, 20) << "..."
               << " | Tx: " << block.transactions.size() << "\n";
        }
        
        blockBuffer_->text(ss.str().c_str());
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
    
    void toggleMining() {
        if (miningActive_) {
            stopMining();
        } else {
            startMining();
        }
    }
    
    void startMining() {
        if (!miningActive_) {
            miningActive_ = true;
            
            if (!p2pNode_) {
                p2pNode_ = std::make_unique<P2PNode>(*chain_, 30303);
                p2pNode_->start();
            }
            
            miningEngine_ = std::make_unique<ZkMiningEngine>();
            miner_ = std::make_unique<Miner>(*chain_, *p2pNode_, *miningEngine_);
            miner_->setInterval(std::chrono::seconds(5));
            miner_->start();
            
            updateStatus();
        }
    }
    
    void stopMining() {
        if (miningActive_) {
            miningActive_ = false;
            if (miner_) {
                miner_->stop();
                miner_.reset();
            }
            miningEngine_.reset();
            updateStatus();
        }
    }
    
    void mineBlock() {
        try {
            Block block = chain_->mineBlock();
            std::cout << "Mined block #" << block.index << " hash=" << block.hash << std::endl;
            
            if (p2pNode_) {
                p2pNode_->broadcastNewBlock(block);
            }
            
            updateStatus();
        } catch (const std::exception& e) {
            std::cerr << "Failed to mine block: " << e.what() << std::endl;
        }
    }
    
    void searchBlock() {
        std::string blockNumStr = blockSearchInput_->value();
        try {
            uint64_t blockNum = std::stoull(blockNumStr);
            const auto& chain = chain_->chain();
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
        } catch (...) {
            blockBuffer_->text("Invalid block number");
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
            
            std::cout << "Wallet created successfully" << std::endl;
        } catch (const std::exception& e) {
            std::stringstream ss;
            ss << "Failed to create wallet: " << e.what();
            walletBuffer_->text(ss.str().c_str());
            std::cerr << "Failed to create wallet: " << e.what() << std::endl;
        }
    }
    
    void loadWallet() {
        // Implementation for wallet loading
        walletBuffer_->text("Load wallet functionality not yet implemented");
    }
    
    void sendTransaction() {
        // Implementation for sending transactions
        txBuffer_->text("Send transaction functionality not yet implemented");
    }
    
    uint64_t chainId_;
    std::atomic<bool> miningActive_;
    
    std::unique_ptr<Blockchain> chain_;
    std::unique_ptr<P2PNode> p2pNode_;
    std::unique_ptr<RpcServer> rpcServer_;
    std::unique_ptr<ZkMiningEngine> miningEngine_;
    std::unique_ptr<Miner> miner_;
    
    // FLTK widgets
    Fl_Window* window_;
    Fl_Tabs* tabs_;
    
    Fl_Output* blockHeightDisplay_;
    Fl_Output* chainIdDisplay_;
    Fl_Output* p2pStatusDisplay_;
    Fl_Output* rpcStatusDisplay_;
    
    Fl_Button* miningButton_;
    Fl_Button* mineBlockButton_;
    
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
};

int main(int argc, char* argv[]) {
    uint64_t chainId = 1337;
    
    if (argc > 1) {
        chainId = std::stoull(argv[1]);
    }
    
    Fl::scheme("gtk+");
    GambitGUI gui(chainId);
    gui.run();
    
    return 0;
}