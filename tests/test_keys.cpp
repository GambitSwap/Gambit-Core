#include <gtest/gtest.h>
#include "gambit/keys.hpp"
#include "gambit/hash.hpp"

using namespace gambit;

class KeysTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// Test KeyPair generation
TEST_F(KeysTest, GenerateKeyPair) {
    KeyPair kp = KeyPair::random();
    
    // Private key should be 32 bytes
    EXPECT_EQ(kp.privateKey().size(), 32u);
    
    // Public key should be 64 bytes (uncompressed, no prefix)
    EXPECT_EQ(kp.publicKey().size(), 64u);
    
    // Keys should not be all zeros
    bool privNonZero = false;
    for (auto b : kp.privateKey()) {
        if (b != 0) { privNonZero = true; break; }
    }
    EXPECT_TRUE(privNonZero);
    
    bool pubNonZero = false;
    for (auto b : kp.publicKey()) {
        if (b != 0) { pubNonZero = true; break; }
    }
    EXPECT_TRUE(pubNonZero);
}

// Test each key generation produces different keys
TEST_F(KeysTest, UniquenessOfKeys) {
    KeyPair kp1 = KeyPair::random();
    KeyPair kp2 = KeyPair::random();
    
    EXPECT_NE(kp1.privateKey(), kp2.privateKey());
    EXPECT_NE(kp1.publicKey(), kp2.publicKey());
}

// Test address derivation
TEST_F(KeysTest, AddressDerivation) {
    KeyPair kp = KeyPair::random();
    Address addr = kp.address();
    
    // Address should be 20 bytes
    EXPECT_EQ(addr.bytes().size(), 20u);
    
    // Address should be deterministic from same keypair
    Address addr2 = kp.address();
    EXPECT_EQ(addr, addr2);
}

// Test signing produces valid signature
TEST_F(KeysTest, SignatureGeneration) {
    KeyPair kp = KeyPair::random();
    
    // Create a message hash
    Bytes32 msgHash = keccak256_32("test message");
    uint64_t chainId = 1;
    
    Signature sig = kp.sign(msgHash, chainId);
    
    // r and s should be 32 bytes each
    EXPECT_EQ(sig.r.size(), 32u);
    EXPECT_EQ(sig.s.size(), 32u);
    
    // v should be valid recovery id
    EXPECT_TRUE(sig.v == 0 || sig.v == 1);
}

// Test signature verification
TEST_F(KeysTest, SignatureVerification) {
    KeyPair kp = KeyPair::random();
    
    Bytes32 msgHash = keccak256_32("verify this message");
    uint64_t chainId = 1;
    
    Signature sig = kp.sign(msgHash, chainId);
    
    // Verify with the correct public key
    bool valid = KeyPair::verify(msgHash, sig, kp.publicKey());
    EXPECT_TRUE(valid);
}

// Test signature verification fails with wrong message
TEST_F(KeysTest, SignatureVerificationWrongMessage) {
    KeyPair kp = KeyPair::random();
    
    Bytes32 msgHash1 = keccak256_32("original message");
    Bytes32 msgHash2 = keccak256_32("different message");
    uint64_t chainId = 1;
    
    Signature sig = kp.sign(msgHash1, chainId);
    
    // Verify with different message hash should fail
    bool valid = KeyPair::verify(msgHash2, sig, kp.publicKey());
    EXPECT_FALSE(valid);
}

// Test signature verification fails with wrong key
TEST_F(KeysTest, SignatureVerificationWrongKey) {
    KeyPair kp1 = KeyPair::random();
    KeyPair kp2 = KeyPair::random();
    
    Bytes32 msgHash = keccak256_32("test message");
    uint64_t chainId = 1;
    
    Signature sig = kp1.sign(msgHash, chainId);
    
    // Verify with wrong public key should fail
    bool valid = KeyPair::verify(msgHash, sig, kp2.publicKey());
    EXPECT_FALSE(valid);
}

// Test address recovery from signature
TEST_F(KeysTest, AddressRecovery) {
    KeyPair kp = KeyPair::random();
    Address expectedAddr = kp.address();
    
    Bytes32 msgHash = keccak256_32("recover address from this");
    uint64_t chainId = 1;
    
    Signature sig = kp.sign(msgHash, chainId);
    
    // Recover address from signature
    Address recoveredAddr = Keys::recoverAddress(msgHash, sig, chainId);
    
    EXPECT_EQ(expectedAddr, recoveredAddr);
}

// Test signing with different chain IDs
TEST_F(KeysTest, DifferentChainIds) {
    KeyPair kp = KeyPair::random();
    Bytes32 msgHash = keccak256_32("chain id test");
    
    Signature sig1 = kp.sign(msgHash, 1);    // Mainnet
    Signature sig2 = kp.sign(msgHash, 137);  // Polygon
    
    // Signatures should be valid for their respective chain IDs
    bool valid1 = KeyPair::verify(msgHash, sig1, kp.publicKey());
    bool valid2 = KeyPair::verify(msgHash, sig2, kp.publicKey());
    
    EXPECT_TRUE(valid1);
    EXPECT_TRUE(valid2);
}

// Test signature r and s are non-zero
TEST_F(KeysTest, SignatureComponentsNonZero) {
    KeyPair kp = KeyPair::random();
    Bytes32 msgHash = keccak256_32("non-zero test");
    
    Signature sig = kp.sign(msgHash, 1);
    
    bool rNonZero = false;
    for (auto b : sig.r) {
        if (b != 0) { rNonZero = true; break; }
    }
    EXPECT_TRUE(rNonZero);
    
    bool sNonZero = false;
    for (auto b : sig.s) {
        if (b != 0) { sNonZero = true; break; }
    }
    EXPECT_TRUE(sNonZero);
}

