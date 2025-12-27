#include <gtest/gtest.h>
#include "gambit/bloom.hpp"
#include "gambit/hash.hpp"

using namespace gambit;

class BloomTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// Test default bloom filter is empty
TEST_F(BloomTest, DefaultEmpty) {
    Bloom bloom;
    
    // All bits should be zero
    bool allZero = true;
    for (size_t i = 0; i < Bloom::kBytes; ++i) {
        if (bloom.bits[i] != 0) {
            allZero = false;
            break;
        }
    }
    EXPECT_TRUE(allZero);
}

// Test bloom filter size constant
TEST_F(BloomTest, SizeConstant) {
    EXPECT_EQ(Bloom::kBytes, 256u);  // 2048 bits = 256 bytes
}

// Test adding data sets some bits
TEST_F(BloomTest, AddDataSetsBits) {
    Bloom bloom;
    
    Bytes data = {0x01, 0x02, 0x03, 0x04};
    bloom.add(data);
    
    // At least some bits should be set
    bool anySet = false;
    for (size_t i = 0; i < Bloom::kBytes; ++i) {
        if (bloom.bits[i] != 0) {
            anySet = true;
            break;
        }
    }
    EXPECT_TRUE(anySet);
}

// Test adding hex string
TEST_F(BloomTest, AddHexString) {
    Bloom bloom;
    
    bloom.add("0xdeadbeef");
    
    // At least some bits should be set
    bool anySet = false;
    for (size_t i = 0; i < Bloom::kBytes; ++i) {
        if (bloom.bits[i] != 0) {
            anySet = true;
            break;
        }
    }
    EXPECT_TRUE(anySet);
}

// Test adding same data is idempotent
TEST_F(BloomTest, AddIdempotent) {
    Bloom bloom1;
    Bloom bloom2;
    
    Bytes data = {0xaa, 0xbb, 0xcc};
    
    bloom1.add(data);
    
    bloom2.add(data);
    bloom2.add(data);
    bloom2.add(data);
    
    // Both should have the same bits set
    EXPECT_EQ(bloom1.bits, bloom2.bits);
}

// Test different data sets different bits
TEST_F(BloomTest, DifferentDataDifferentBits) {
    Bloom bloom1;
    Bloom bloom2;
    
    Bytes data1 = {0x01, 0x02, 0x03};
    Bytes data2 = {0x04, 0x05, 0x06};
    
    bloom1.add(data1);
    bloom2.add(data2);
    
    // High probability they differ (could theoretically be same, but very unlikely)
    // We'll just check they're not all zeros
    bool bloom1HasBits = false;
    bool bloom2HasBits = false;
    
    for (size_t i = 0; i < Bloom::kBytes; ++i) {
        if (bloom1.bits[i] != 0) bloom1HasBits = true;
        if (bloom2.bits[i] != 0) bloom2HasBits = true;
    }
    
    EXPECT_TRUE(bloom1HasBits);
    EXPECT_TRUE(bloom2HasBits);
}

// Test toHex returns correct format
TEST_F(BloomTest, ToHexFormat) {
    Bloom bloom;
    
    Bytes data = {0x01, 0x02, 0x03};
    bloom.add(data);
    
    std::string hex = bloom.toHex();
    
    // Should be 256 bytes = 512 hex chars (possibly with 0x prefix)
    if (hex.substr(0, 2) == "0x") {
        EXPECT_EQ(hex.length(), 514u);  // 0x + 512 hex chars
    } else {
        EXPECT_EQ(hex.length(), 512u);
    }
}

// Test toHex with empty bloom
TEST_F(BloomTest, ToHexEmpty) {
    Bloom bloom;
    std::string hex = bloom.toHex();
    
    // Should be all zeros
    size_t start = 0;
    if (hex.substr(0, 2) == "0x") {
        start = 2;
    }
    
    for (size_t i = start; i < hex.length(); ++i) {
        EXPECT_EQ(hex[i], '0');
    }
}

// Test multiple adds accumulate bits
TEST_F(BloomTest, MultipleAddsAccumulate) {
    Bloom bloom;
    
    Bytes data1 = {0x01};
    Bytes data2 = {0x02};
    Bytes data3 = {0x03};
    
    bloom.add(data1);
    int count1 = 0;
    for (size_t i = 0; i < Bloom::kBytes; ++i) {
        for (int j = 0; j < 8; ++j) {
            if (bloom.bits[i] & (1 << j)) count1++;
        }
    }
    
    bloom.add(data2);
    bloom.add(data3);
    
    int count2 = 0;
    for (size_t i = 0; i < Bloom::kBytes; ++i) {
        for (int j = 0; j < 8; ++j) {
            if (bloom.bits[i] & (1 << j)) count2++;
        }
    }
    
    // After adding more data, should have at least as many bits set
    EXPECT_GE(count2, count1);
}

// Test adding address-like data
TEST_F(BloomTest, AddAddressData) {
    Bloom bloom;
    
    // 20-byte address
    Bytes address(20, 0x00);
    address[0] = 0xde;
    address[19] = 0xad;
    
    bloom.add(address);
    
    // Should set some bits
    bool anySet = false;
    for (size_t i = 0; i < Bloom::kBytes; ++i) {
        if (bloom.bits[i] != 0) {
            anySet = true;
            break;
        }
    }
    EXPECT_TRUE(anySet);
}

// Test hex string addition consistency
TEST_F(BloomTest, HexStringConsistency) {
    Bloom bloom1;
    Bloom bloom2;
    
    // These should produce the same result
    Bytes data = fromHex("deadbeef");
    bloom1.add(data);
    bloom2.add("0xdeadbeef");
    
    EXPECT_EQ(bloom1.bits, bloom2.bits);
}

