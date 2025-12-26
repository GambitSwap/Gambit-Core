#include <iostream>
#include <thread>

#include "gambit/blockchain.hpp"
#include "gambit/p2p_node.hpp"
#include "gambit/keys.hpp"
#include "gambit/transaction.hpp"
#include "gambit/rpc_server.hpp"
#include "gambit/miner.hpp"
#include "gambit/zk_mining_engine.hpp"

using namespace gambit;

int main() {
    std::cout << "=== Gambit Node Starting ===\n";

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

    std::cout << "Genesis block hash: " << chain.chain().back().hash << "\n";

    // 4. P2P node
    P2PNode node(chain, 30303);
    node.start();

    // 5. RPC server
    RpcServer rpc(chain, 8545);
    rpc.start();

    // 6. Start miner (mine every 5 seconds)
    ZkMiningEngine engine;
    Miner miner(chain, node, engine);
    miner.setInterval(std::chrono::seconds(5));
    miner.start();

    /*
    // 5. Create a transaction A -> B
    Transaction tx;
    tx.nonce = 0;
    tx.gasPrice = 1;
    tx.gasLimit = 21000;
    tx.to = addrB;
    tx.value = 10;
    tx.chainId = genesis.chainId;

    tx.signWith(kpA);

    std::cout << "Signed TX: " << tx.toHex() << "\n";

    chain.addTransaction(tx);
    node.broadcastNewTx(tx);

    // 6. Mine a block
    Block b1 = chain.mineBlock();
    std::cout << "Mined block #" << b1.index << " hash=" << b1.hash << "\n";

    node.broadcastNewBlock(b1);
    */

    std::cout << "RPC ready on http://127.0.0.1:8545\n";

    // Keep node alive
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
    
    miner.stop();
    rpc.stop();
    node.stop();
    return 0;
}

/**
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
