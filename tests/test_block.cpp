#include <gtest/gtest.h>
#include "gambit/block.hpp"
#include "gambit/transaction.hpp"
#include "gambit/keys.hpp"
#include "gambit/hash.hpp"

using namespace gambit;

class BlockTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
    
    Transaction createTestTransaction() {
        KeyPair kp = KeyPair::random();
        Address recipient = Address::fromHex("0x1234567890123456789012345678901234567890");
        
        Transaction tx;
        tx.nonce = 0;
        tx.gasPrice = 20000000000;
        tx.gasLimit = 21000;
        tx.to = recipient;
        tx.value = 1000000000000000000;
        tx.chainId = 1;
        tx.signWith(kp);
        
        return tx;
    }
};

// Test default block construction
TEST_F(BlockTest, DefaultConstruction) {
    Block block;
    
    EXPECT_EQ(block.index, 0u);
    EXPECT_TRUE(block.prevHash.empty());
    EXPECT_EQ(block.timestamp, 0u);
    EXPECT_TRUE(block.transactions.empty());
}

// Test block construction with parameters
TEST_F(BlockTest, ParameterizedConstruction) {
    std::string prevHash = "0x1234";
    std::string stateBefore = "0xabcd";
    std::string stateAfter = "0xef01";
    std::string txRoot = "0x5678";
    ZkProof proof;
    
    Block block(1, prevHash, stateBefore, stateAfter, txRoot, proof);
    
    EXPECT_EQ(block.index, 1u);
    EXPECT_EQ(block.prevHash, prevHash);
    EXPECT_EQ(block.stateBefore, stateBefore);
    EXPECT_EQ(block.stateAfter, stateAfter);
    EXPECT_EQ(block.txRoot, txRoot);
}

// Test compute hash
TEST_F(BlockTest, ComputeHash) {
    Block block;
    block.index = 1;
    block.prevHash = "0x0000000000000000000000000000000000000000000000000000000000000000";
    block.timestamp = 1234567890;
    
    std::string hash = block.computeHash();
    
    // Hash should not be empty
    EXPECT_FALSE(hash.empty());
    
    // Hash should be deterministic
    std::string hash2 = block.computeHash();
    EXPECT_EQ(hash, hash2);
}

// Test different blocks have different hashes
TEST_F(BlockTest, DifferentHashes) {
    Block block1;
    block1.index = 1;
    block1.prevHash = "0x0000";
    block1.timestamp = 1000;
    
    Block block2;
    block2.index = 2;
    block2.prevHash = "0x0000";
    block2.timestamp = 1000;
    
    EXPECT_NE(block1.computeHash(), block2.computeHash());
}

// Test RLP encoding
TEST_F(BlockTest, RlpEncode) {
    Block block;
    block.index = 1;
    block.prevHash = "0x0000";
    block.timestamp = 1234567890;
    
    Bytes encoded = block.rlpEncode();
    
    // Should produce non-empty bytes
    EXPECT_FALSE(encoded.empty());
    
    // First byte should be list prefix
    EXPECT_GE(encoded[0], 0xc0);
}

// Test RLP decode
TEST_F(BlockTest, RlpDecode) {
    Block original;
    original.index = 42;
    original.prevHash = "0xdeadbeef";
    original.stateBefore = "0xaabb";
    original.stateAfter = "0xccdd";
    original.timestamp = 1234567890;
    
    Bytes encoded = original.rlpEncode();
    Block decoded = Block::rlpDecode(encoded);
    
    EXPECT_EQ(decoded.index, original.index);
    EXPECT_EQ(decoded.prevHash, original.prevHash);
    EXPECT_EQ(decoded.timestamp, original.timestamp);
}

// Test toHex serialization
TEST_F(BlockTest, ToHex) {
    Block block;
    block.index = 1;
    block.prevHash = "0x0000";
    block.timestamp = 1234567890;
    
    std::string hex = block.toHex();
    
    // Should start with 0x
    EXPECT_EQ(hex.substr(0, 2), "0x");
    
    // Should be even length
    EXPECT_EQ(hex.length() % 2, 0u);
}

// Test fromHex deserialization
TEST_F(BlockTest, FromHex) {
    Block original;
    original.index = 42;
    original.prevHash = "0xdeadbeef";
    original.timestamp = 1234567890;
    
    std::string hex = original.toHex();
    Block recovered = Block::fromHex(hex);
    
    EXPECT_EQ(recovered.index, original.index);
    EXPECT_EQ(recovered.prevHash, original.prevHash);
    EXPECT_EQ(recovered.timestamp, original.timestamp);
}

// Test block with transactions
TEST_F(BlockTest, BlockWithTransactions) {
    Block block;
    block.index = 1;
    block.prevHash = "0x0000";
    block.timestamp = 1234567890;
    
    // Add some test transactions
    block.transactions.push_back(createTestTransaction());
    block.transactions.push_back(createTestTransaction());
    
    EXPECT_EQ(block.transactions.size(), 2u);
    
    // Compute hash should still work
    std::string hash = block.computeHash();
    EXPECT_FALSE(hash.empty());
}

// Test genesis block (index 0)
TEST_F(BlockTest, GenesisBlock) {
    Block genesis;
    genesis.index = 0;
    genesis.prevHash = "0x0000000000000000000000000000000000000000000000000000000000000000";
    genesis.timestamp = 0;
    
    EXPECT_EQ(genesis.index, 0u);
    
    std::string hash = genesis.computeHash();
    EXPECT_FALSE(hash.empty());
}

// Test block hash format
TEST_F(BlockTest, HashFormat) {
    Block block;
    block.index = 1;
    block.prevHash = "0x0000";
    block.timestamp = 1234567890;
    
    std::string hash = block.computeHash();
    
    // Should be hex format
    for (char c : hash) {
        if (c != 'x') {
            bool isHex = (c >= '0' && c <= '9') || 
                         (c >= 'a' && c <= 'f') || 
                         (c >= 'A' && c <= 'F');
            EXPECT_TRUE(isHex);
        }
    }
}

// Test logs bloom in block
TEST_F(BlockTest, LogsBloom) {
    Block block;
    
    // Default bloom should be all zeros
    bool allZero = true;
    for (size_t i = 0; i < Bloom::kBytes; ++i) {
        if (block.logsBloom.bits[i] != 0) {
            allZero = false;
            break;
        }
    }
    EXPECT_TRUE(allZero);
}

// Test block with receipts
TEST_F(BlockTest, BlockWithReceipts) {
    Block block;
    block.index = 1;
    block.prevHash = "0x0000";
    block.timestamp = 1234567890;
    
    // Receipts vector should be available
    EXPECT_TRUE(block.receipts.empty());
}

