#include <gtest/gtest.h>
#include "gambit/transaction.hpp"
#include "gambit/keys.hpp"
#include "gambit/hash.hpp"

using namespace gambit;

class TransactionTest : public ::testing::Test {
protected:
    KeyPair senderKey;
    Address recipientAddr;
    
    void SetUp() override {
        senderKey = KeyPair::random();
        recipientAddr = Address::fromHex("0x1234567890123456789012345678901234567890");
    }
    void TearDown() override {}
};

// Test default transaction construction
TEST_F(TransactionTest, DefaultConstruction) {
    Transaction tx;
    
    EXPECT_EQ(tx.nonce, 0u);
    EXPECT_EQ(tx.gasPrice, 0u);
    EXPECT_EQ(tx.gasLimit, 0u);
    EXPECT_EQ(tx.value, 0u);
    EXPECT_EQ(tx.chainId, 1u);
    EXPECT_TRUE(tx.data.empty());
}

// Test transaction field assignment
TEST_F(TransactionTest, FieldAssignment) {
    Transaction tx;
    tx.nonce = 42;
    tx.gasPrice = 20000000000;  // 20 gwei
    tx.gasLimit = 21000;
    tx.to = recipientAddr;
    tx.value = 1000000000000000000;  // 1 ETH in wei
    tx.chainId = 1;
    
    EXPECT_EQ(tx.nonce, 42u);
    EXPECT_EQ(tx.gasPrice, 20000000000u);
    EXPECT_EQ(tx.gasLimit, 21000u);
    EXPECT_EQ(tx.to, recipientAddr);
    EXPECT_EQ(tx.value, 1000000000000000000u);
    EXPECT_EQ(tx.chainId, 1u);
}

// Test RLP encoding for signing
TEST_F(TransactionTest, RlpEncodeForSigning) {
    Transaction tx;
    tx.nonce = 0;
    tx.gasPrice = 1;
    tx.gasLimit = 21000;
    tx.to = recipientAddr;
    tx.value = 100;
    tx.chainId = 1;
    
    Bytes encoded = tx.rlpEncodeForSigning();
    
    // Should produce non-empty bytes
    EXPECT_FALSE(encoded.empty());
    
    // First byte should be list prefix (0xc0 or higher)
    EXPECT_GE(encoded[0], 0xc0);
}

// Test signing hash generation
TEST_F(TransactionTest, SigningHash) {
    Transaction tx;
    tx.nonce = 0;
    tx.gasPrice = 1;
    tx.gasLimit = 21000;
    tx.to = recipientAddr;
    tx.value = 100;
    tx.chainId = 1;
    
    Bytes32 hash = tx.signingHash();
    
    // Hash should be 32 bytes
    EXPECT_EQ(hash.size(), 32u);
    
    // Hash should be deterministic
    Bytes32 hash2 = tx.signingHash();
    EXPECT_EQ(hash, hash2);
}

// Test transaction signing
TEST_F(TransactionTest, SignTransaction) {
    Transaction tx;
    tx.nonce = 0;
    tx.gasPrice = 20000000000;
    tx.gasLimit = 21000;
    tx.to = recipientAddr;
    tx.value = 1000000000000000000;
    tx.chainId = 1;
    
    tx.signWith(senderKey);
    
    // Signature should be populated
    EXPECT_EQ(tx.sig.r.size(), 32u);
    EXPECT_EQ(tx.sig.s.size(), 32u);
    
    // From address should be set
    EXPECT_EQ(tx.from, senderKey.address());
}

// Test signature verification
TEST_F(TransactionTest, VerifySignature) {
    Transaction tx;
    tx.nonce = 0;
    tx.gasPrice = 20000000000;
    tx.gasLimit = 21000;
    tx.to = recipientAddr;
    tx.value = 1000000000000000000;
    tx.chainId = 1;
    
    tx.signWith(senderKey);
    
    EXPECT_TRUE(tx.verifySignature());
}

// Test RLP encoding of signed transaction
TEST_F(TransactionTest, RlpEncodeSigned) {
    Transaction tx;
    tx.nonce = 0;
    tx.gasPrice = 20000000000;
    tx.gasLimit = 21000;
    tx.to = recipientAddr;
    tx.value = 1000000000000000000;
    tx.chainId = 1;
    
    tx.signWith(senderKey);
    
    Bytes encoded = tx.rlpEncodeSigned();
    
    // Should include signature data
    EXPECT_FALSE(encoded.empty());
    
    // Should be larger than unsigned encoding
    Bytes unsignedEncoded = tx.rlpEncodeForSigning();
    EXPECT_GT(encoded.size(), unsignedEncoded.size());
}

// Test toHex serialization
TEST_F(TransactionTest, ToHex) {
    Transaction tx;
    tx.nonce = 0;
    tx.gasPrice = 20000000000;
    tx.gasLimit = 21000;
    tx.to = recipientAddr;
    tx.value = 1000000000000000000;
    tx.chainId = 1;
    
    tx.signWith(senderKey);
    
    std::string hex = tx.toHex();
    
    // Should start with 0x
    EXPECT_EQ(hex.substr(0, 2), "0x");
    
    // Should be even length (hex encoding)
    EXPECT_EQ(hex.length() % 2, 0u);
}

// Test fromHex deserialization
TEST_F(TransactionTest, FromHex) {
    Transaction original;
    original.nonce = 42;
    original.gasPrice = 20000000000;
    original.gasLimit = 21000;
    original.to = recipientAddr;
    original.value = 1000000000000000000;
    original.chainId = 1;
    
    original.signWith(senderKey);
    
    std::string hex = original.toHex();
    Transaction recovered = Transaction::fromHex(hex);
    
    EXPECT_EQ(recovered.nonce, original.nonce);
    EXPECT_EQ(recovered.gasPrice, original.gasPrice);
    EXPECT_EQ(recovered.gasLimit, original.gasLimit);
    EXPECT_EQ(recovered.to, original.to);
    EXPECT_EQ(recovered.value, original.value);
}

// Test transaction with data
TEST_F(TransactionTest, TransactionWithData) {
    Transaction tx;
    tx.nonce = 0;
    tx.gasPrice = 20000000000;
    tx.gasLimit = 100000;
    tx.to = recipientAddr;
    tx.value = 0;
    tx.data = {0xa9, 0x05, 0x9c, 0xbb};  // Example ERC20 transfer selector
    tx.chainId = 1;
    
    tx.signWith(senderKey);
    
    EXPECT_TRUE(tx.verifySignature());
    EXPECT_EQ(tx.data.size(), 4u);
}

// Test different chain IDs
TEST_F(TransactionTest, DifferentChainIds) {
    Transaction tx1;
    tx1.nonce = 0;
    tx1.gasPrice = 1;
    tx1.gasLimit = 21000;
    tx1.to = recipientAddr;
    tx1.value = 100;
    tx1.chainId = 1;  // Mainnet
    
    Transaction tx2 = tx1;
    tx2.chainId = 137;  // Polygon
    
    // Different chain IDs should produce different signing hashes
    EXPECT_NE(tx1.signingHash(), tx2.signingHash());
}

// Test hash is computed after signing
TEST_F(TransactionTest, HashComputed) {
    Transaction tx;
    tx.nonce = 0;
    tx.gasPrice = 20000000000;
    tx.gasLimit = 21000;
    tx.to = recipientAddr;
    tx.value = 1000000000000000000;
    tx.chainId = 1;
    
    tx.signWith(senderKey);
    
    // Hash should be populated
    EXPECT_FALSE(tx.hash.empty());
    
    // Hash should be hex string of 66 chars (0x + 64 hex digits)
    EXPECT_EQ(tx.hash.length(), 66u);
    EXPECT_EQ(tx.hash.substr(0, 2), "0x");
}

