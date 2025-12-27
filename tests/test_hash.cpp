#include <gtest/gtest.h>
#include "gambit/hash.hpp"

using namespace gambit;

class HashTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// Test toHex conversion for Bytes
TEST_F(HashTest, ToHexBytes) {
    Bytes data = {0x00, 0x01, 0x02, 0xab, 0xcd, 0xef};
    std::string hex = toHex(data);
    EXPECT_EQ(hex, "000102abcdef");
}

// Test toHex conversion for Bytes32
TEST_F(HashTest, ToHexBytes32) {
    Bytes32 data = {};
    std::fill(data.begin(), data.end(), 0x00);
    data[31] = 0x01;
    std::string hex = toHex(data);
    EXPECT_EQ(hex.length(), 64u);
    EXPECT_EQ(hex.substr(62), "01");
}

// Test fromHex conversion
TEST_F(HashTest, FromHex) {
    std::string hex = "abcdef";
    Bytes result = fromHex(hex);
    ASSERT_EQ(result.size(), 3u);
    EXPECT_EQ(result[0], 0xab);
    EXPECT_EQ(result[1], 0xcd);
    EXPECT_EQ(result[2], 0xef);
}

// Test fromHex with 0x prefix
TEST_F(HashTest, FromHexWithPrefix) {
    std::string hex = "0xabcdef";
    Bytes result = fromHex(hex);
    ASSERT_EQ(result.size(), 3u);
    EXPECT_EQ(result[0], 0xab);
    EXPECT_EQ(result[1], 0xcd);
    EXPECT_EQ(result[2], 0xef);
}

// Test roundtrip hex conversion
TEST_F(HashTest, HexRoundtrip) {
    Bytes original = {0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0};
    std::string hex = toHex(original);
    Bytes recovered = fromHex(hex);
    EXPECT_EQ(original, recovered);
}

// Test keccak256 with known vector
TEST_F(HashTest, Keccak256KnownVector) {
    // keccak256("") = c5d2460186f7233c927e7db2dcc703c0e500b653ca82273b7bfad8045d85a470
    Bytes empty;
    Bytes hash = keccak256(empty);
    std::string hex = toHex(hash);
    EXPECT_EQ(hex, "c5d2460186f7233c927e7db2dcc703c0e500b653ca82273b7bfad8045d85a470");
}

// Test keccak256 with string input
TEST_F(HashTest, Keccak256String) {
    // keccak256("hello") = 1c8aff950685c2ed4bc3174f3472287b56d9517b9c948127319a09a7a36deac8
    std::string input = "hello";
    Bytes hash = keccak256(input);
    std::string hex = toHex(hash);
    EXPECT_EQ(hex, "1c8aff950685c2ed4bc3174f3472287b56d9517b9c948127319a09a7a36deac8");
}

// Test keccak256_32 returns correct size
TEST_F(HashTest, Keccak256_32Size) {
    Bytes input = {0x01, 0x02, 0x03};
    Bytes32 hash = keccak256_32(input);
    EXPECT_EQ(hash.size(), 32u);
}

// Test keccak256_32 consistency with keccak256
TEST_F(HashTest, Keccak256_32Consistency) {
    Bytes input = {0x01, 0x02, 0x03, 0x04, 0x05};
    Bytes hash1 = keccak256(input);
    Bytes32 hash2 = keccak256_32(input);
    
    ASSERT_EQ(hash1.size(), 32u);
    for (size_t i = 0; i < 32; ++i) {
        EXPECT_EQ(hash1[i], hash2[i]);
    }
}

// Test empty input
TEST_F(HashTest, EmptyFromHex) {
    std::string hex = "";
    Bytes result = fromHex(hex);
    EXPECT_TRUE(result.empty());
}

// Test fromHex with uppercase
TEST_F(HashTest, FromHexUppercase) {
    std::string hex = "ABCDEF";
    Bytes result = fromHex(hex);
    ASSERT_EQ(result.size(), 3u);
    EXPECT_EQ(result[0], 0xab);
    EXPECT_EQ(result[1], 0xcd);
    EXPECT_EQ(result[2], 0xef);
}

