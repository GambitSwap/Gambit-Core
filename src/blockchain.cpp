#include "gambit/blockchain.hpp"
#include "gambit/hash.hpp"
#include "gambit/zk.hpp"
#include <algorithm>
#include <stdexcept>

namespace gambit
{

    Blockchain::Blockchain(const GenesisConfig &genesis)
        : state_(genesis), chainId_(genesis.chainId)
    {
        initGenesis(genesis);
    }

    void Blockchain::initGenesis(const GenesisConfig &genesis)
    {
        Block genesisBlock(
            0,
            "0x00",
            state_.root(),
            state_.root(),
            "0x00",
            ZkProver::generate(state_.root(), state_.root(), "0x00"));

        chain_.push_back(genesisBlock);
    }
    
    bool Blockchain::validateTransaction(const Transaction &tx, std::string &err) const
    {
        // 1. chainId
        if (tx.chainId != chainId_)
        {
            err = "Invalid chainId";
            return false;
        }

        // 2. signature (if we want to double check)
        if (!tx.verifySignature())
        {
            err = "Invalid signature";
            return false;
        }

        // 3. account existence & nonce
        const Account *acc = state_.get(tx.from);
        std::uint64_t expectedNonce = acc ? acc->nonce : 0;
        if (tx.nonce != expectedNonce)
        {
            err = "Invalid nonce";
            return false;
        }

        // 4. balance >= value + gasPrice * gasLimit (simplified)
        // Check for multiplication overflow (portable, MSVC-compatible)
        std::uint64_t gasCost = tx.gasPrice * tx.gasLimit;
        if (tx.gasPrice != 0 && gasCost / tx.gasPrice != tx.gasLimit)
        {
            err = "Gas cost overflow";
            return false;
        }
        // Check for addition overflow
        std::uint64_t needed = gasCost + tx.value;
        if (needed < gasCost)
        {
            err = "Total cost overflow";
            return false;
        }

        std::uint64_t bal = acc ? acc->balance : 0;
        if (bal < needed)
        {
            err = "Insufficient funds";
            return false;
        }

        return true;
    }

    void Blockchain::addTransaction(const Transaction &tx)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        mempool_.push_back(tx);
    }

    std::string Blockchain::computeTxRoot(const std::vector<Transaction> &txs) const
    {
        if (txs.empty())
        {
            return "0x00";
        }

        // Simple Merkle root: hash(concat(txHex...))
        std::string concat;
        for (const auto &tx : txs)
        {
            concat += tx.toHex() + "|";
        }

        return toHex(keccak256(concat));
    }

    Block Blockchain::mineBlock()
    {
        std::lock_guard<std::mutex> lock(mutex_);

        std::string before = state_.root();

        // Apply transactions (if any)
        for (const auto &tx : mempool_)
        {
            state_.applyTransaction(tx.from, tx);
        }

        std::string after = state_.root();
        std::string txRoot = computeTxRoot(mempool_);

        std::vector<Receipt> receipts;
        Receipt rc;
        uint64_t cumulativeGas = 0;
        for (const auto &tx : mempool_)
        {
            cumulativeGas += tx.gasLimit;
            rc.status = true;
            rc.cumulativeGasUsed = cumulativeGas;
            rc.logs.clear(); // no logs yet
            receipts.push_back(rc);
        }

        // Compute receiptsRoot
        MptTrie receiptsTrie;
        for (size_t i = 0; i < receipts.size(); ++i)
        {
            Bytes key{static_cast<uint8_t>(i)};
            receiptsTrie.put(key, receipts[i].rlpEncode());
        }
        std::string receiptsRoot = receiptsTrie.rootHash();

        ZkProof proof = ZkProver::generate(before, after, txRoot);

        Block block(
            chain_.size(),
            chain_.back().hash,
            before,
            after,
            txRoot,
            proof);

        block.transactions = mempool_; // attach txn (may be empty)
        block.receipts = receipts;
        block.receiptsRoot = receiptsRoot;

        chain_.push_back(block);
        mempool_.clear();

        return block;
    }

    bool Blockchain::addBlock(const Block &block)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        // Basic checks
        if (block.index != chain_.size())
        {
            return false;
        }
        if (block.prevHash != chain_.back().hash)
        {
            return false;
        }
        if (!ZkVerifier::verify(block.proof))
        {
            return false;
        }

        // Apply block transactions (not included in block struct yet)
        // For now, assume stateAfter is authoritative
        // In a real chain, you'd re-execute txs and compare roots.

        chain_.push_back(block);
        return true;
    }

} // namespace gambit
