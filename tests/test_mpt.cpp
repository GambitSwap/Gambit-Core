#include <gtest/gtest.h>
#include "gambit/mpt.hpp"
#include "gambit/hash.hpp"

using namespace gambit;

class MptTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// Test empty trie creation
TEST_F(MptTest, EmptyTrie) {
    MptTrie trie;
    std::string root = trie.rootHash();
    
    // Empty trie should have a deterministic root
    EXPECT_FALSE(root.empty());
}

// Test put and get single value
TEST_F(MptTest, PutGetSingle) {
    MptTrie trie;
    
    Bytes key = {0x01, 0x02, 0x03};
    Bytes value = {0xaa, 0xbb, 0xcc};
    
    trie.put(key, value);
    
    auto retrieved = trie.get(key);
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved.value(), value);
}

// Test get non-existent key
TEST_F(MptTest, GetNonExistent) {
    MptTrie trie;
    
    Bytes key = {0x01, 0x02, 0x03};
    auto result = trie.get(key);
    
    EXPECT_FALSE(result.has_value());
}

// Test multiple puts
TEST_F(MptTest, MultiplePuts) {
    MptTrie trie;
    
    Bytes key1 = {0x01, 0x02, 0x03};
    Bytes value1 = {0xaa};
    
    Bytes key2 = {0x01, 0x02, 0x04};
    Bytes value2 = {0xbb};
    
    Bytes key3 = {0x05, 0x06, 0x07};
    Bytes value3 = {0xcc};
    
    trie.put(key1, value1);
    trie.put(key2, value2);
    trie.put(key3, value3);
    
    auto retrieved1 = trie.get(key1);
    auto retrieved2 = trie.get(key2);
    auto retrieved3 = trie.get(key3);
    
    ASSERT_TRUE(retrieved1.has_value());
    ASSERT_TRUE(retrieved2.has_value());
    ASSERT_TRUE(retrieved3.has_value());
    
    EXPECT_EQ(retrieved1.value(), value1);
    EXPECT_EQ(retrieved2.value(), value2);
    EXPECT_EQ(retrieved3.value(), value3);
}

// Test overwriting value
TEST_F(MptTest, OverwriteValue) {
    MptTrie trie;
    
    Bytes key = {0x01, 0x02, 0x03};
    Bytes value1 = {0xaa};
    Bytes value2 = {0xbb, 0xcc};
    
    trie.put(key, value1);
    trie.put(key, value2);
    
    auto retrieved = trie.get(key);
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved.value(), value2);
}

// Test root hash changes with modifications
TEST_F(MptTest, RootHashChanges) {
    MptTrie trie;
    
    std::string emptyRoot = trie.rootHash();
    
    Bytes key = {0x01, 0x02, 0x03};
    Bytes value = {0xaa, 0xbb};
    
    trie.put(key, value);
    
    std::string newRoot = trie.rootHash();
    
    EXPECT_NE(emptyRoot, newRoot);
}

// Test root hash is deterministic
TEST_F(MptTest, RootHashDeterministic) {
    MptTrie trie1;
    MptTrie trie2;
    
    Bytes key = {0x01, 0x02, 0x03};
    Bytes value = {0xaa, 0xbb};
    
    trie1.put(key, value);
    trie2.put(key, value);
    
    EXPECT_EQ(trie1.rootHash(), trie2.rootHash());
}

// Test with address-like keys (20 bytes)
TEST_F(MptTest, AddressLikeKeys) {
    MptTrie trie;
    
    // Create a 20-byte key (like an Ethereum address)
    Bytes key1(20, 0x00);
    key1[0] = 0xde;
    key1[19] = 0xad;
    
    Bytes key2(20, 0x00);
    key2[0] = 0xbe;
    key2[19] = 0xef;
    
    Bytes value1 = {0x01, 0x02, 0x03, 0x04};
    Bytes value2 = {0x05, 0x06, 0x07, 0x08};
    
    trie.put(key1, value1);
    trie.put(key2, value2);
    
    auto retrieved1 = trie.get(key1);
    auto retrieved2 = trie.get(key2);
    
    ASSERT_TRUE(retrieved1.has_value());
    ASSERT_TRUE(retrieved2.has_value());
    
    EXPECT_EQ(retrieved1.value(), value1);
    EXPECT_EQ(retrieved2.value(), value2);
}

// Test empty key
TEST_F(MptTest, EmptyKey) {
    MptTrie trie;
    
    Bytes emptyKey;
    Bytes value = {0xaa};
    
    trie.put(emptyKey, value);
    
    auto retrieved = trie.get(emptyKey);
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved.value(), value);
}

// Test empty value
TEST_F(MptTest, EmptyValue) {
    MptTrie trie;
    
    Bytes key = {0x01, 0x02};
    Bytes emptyValue;
    
    trie.put(key, emptyValue);
    
    auto retrieved = trie.get(key);
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_TRUE(retrieved.value().empty());
}

// Test root hash format
TEST_F(MptTest, RootHashFormat) {
    MptTrie trie;
    
    Bytes key = {0x01};
    Bytes value = {0x02};
    trie.put(key, value);
    
    std::string root = trie.rootHash();
    
    // Should be a hex string
    EXPECT_FALSE(root.empty());
    
    // Check it's valid hex (only hex characters)
    for (char c : root) {
        if (c != 'x') {  // Skip 0x prefix if present
            bool isHex = (c >= '0' && c <= '9') || 
                         (c >= 'a' && c <= 'f') || 
                         (c >= 'A' && c <= 'F');
            EXPECT_TRUE(isHex);
        }
    }
}

// Test keys with common prefixes
TEST_F(MptTest, CommonPrefixKeys) {
    MptTrie trie;
    
    Bytes key1 = {0x01, 0x02, 0x03, 0x04};
    Bytes key2 = {0x01, 0x02, 0x03, 0x05};
    Bytes key3 = {0x01, 0x02, 0x03, 0x06};
    
    Bytes value1 = {0xaa};
    Bytes value2 = {0xbb};
    Bytes value3 = {0xcc};
    
    trie.put(key1, value1);
    trie.put(key2, value2);
    trie.put(key3, value3);
    
    EXPECT_EQ(trie.get(key1).value(), value1);
    EXPECT_EQ(trie.get(key2).value(), value2);
    EXPECT_EQ(trie.get(key3).value(), value3);
}

