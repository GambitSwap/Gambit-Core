#include <gtest/gtest.h>
#include "gambit/rlp.hpp"
#include "gambit/hash.hpp"

using namespace gambit;

class RlpTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// Test encoding empty bytes
TEST_F(RlpTest, EncodeEmptyBytes) {
    Bytes empty;
    Bytes encoded = rlp::encodeBytes(empty);
    ASSERT_EQ(encoded.size(), 1u);
    EXPECT_EQ(encoded[0], 0x80);
}

// Test encoding single byte < 0x80
TEST_F(RlpTest, EncodeSingleByte) {
    Bytes single = {0x7f};
    Bytes encoded = rlp::encodeBytes(single);
    ASSERT_EQ(encoded.size(), 1u);
    EXPECT_EQ(encoded[0], 0x7f);
}

// Test encoding single byte >= 0x80
TEST_F(RlpTest, EncodeSingleByteHigh) {
    Bytes single = {0x80};
    Bytes encoded = rlp::encodeBytes(single);
    ASSERT_EQ(encoded.size(), 2u);
    EXPECT_EQ(encoded[0], 0x81);
    EXPECT_EQ(encoded[1], 0x80);
}

// Test encoding short bytes (length < 56)
TEST_F(RlpTest, EncodeShortBytes) {
    Bytes data = {0x01, 0x02, 0x03, 0x04, 0x05};
    Bytes encoded = rlp::encodeBytes(data);
    ASSERT_EQ(encoded.size(), 6u);
    EXPECT_EQ(encoded[0], 0x85); // 0x80 + 5
    for (size_t i = 0; i < 5; ++i) {
        EXPECT_EQ(encoded[i + 1], data[i]);
    }
}

// Test encoding empty string
TEST_F(RlpTest, EncodeEmptyString) {
    std::string empty;
    Bytes encoded = rlp::encodeString(empty);
    ASSERT_EQ(encoded.size(), 1u);
    EXPECT_EQ(encoded[0], 0x80);
}

// Test encoding short string
TEST_F(RlpTest, EncodeShortString) {
    std::string str = "dog";
    Bytes encoded = rlp::encodeString(str);
    ASSERT_EQ(encoded.size(), 4u);
    EXPECT_EQ(encoded[0], 0x83); // 0x80 + 3
    EXPECT_EQ(encoded[1], 'd');
    EXPECT_EQ(encoded[2], 'o');
    EXPECT_EQ(encoded[3], 'g');
}

// Test encoding uint zero
TEST_F(RlpTest, EncodeUintZero) {
    Bytes encoded = rlp::encodeUint(0);
    ASSERT_EQ(encoded.size(), 1u);
    EXPECT_EQ(encoded[0], 0x80);
}

// Test encoding small uint
TEST_F(RlpTest, EncodeUintSmall) {
    Bytes encoded = rlp::encodeUint(127);
    ASSERT_EQ(encoded.size(), 1u);
    EXPECT_EQ(encoded[0], 0x7f);
}

// Test encoding larger uint
TEST_F(RlpTest, EncodeUintLarger) {
    Bytes encoded = rlp::encodeUint(256);
    ASSERT_EQ(encoded.size(), 3u);
    EXPECT_EQ(encoded[0], 0x82); // 0x80 + 2
    EXPECT_EQ(encoded[1], 0x01);
    EXPECT_EQ(encoded[2], 0x00);
}

// Test encoding uint 1024
TEST_F(RlpTest, EncodeUint1024) {
    Bytes encoded = rlp::encodeUint(1024);
    ASSERT_EQ(encoded.size(), 3u);
    EXPECT_EQ(encoded[0], 0x82); // 0x80 + 2
    EXPECT_EQ(encoded[1], 0x04);
    EXPECT_EQ(encoded[2], 0x00);
}

// Test encoding empty list
TEST_F(RlpTest, EncodeEmptyList) {
    std::vector<Bytes> items;
    Bytes encoded = rlp::encodeList(items);
    ASSERT_EQ(encoded.size(), 1u);
    EXPECT_EQ(encoded[0], 0xc0);
}

// Test encoding list with items
TEST_F(RlpTest, EncodeListWithItems) {
    std::vector<Bytes> items;
    items.push_back(rlp::encodeString("cat"));
    items.push_back(rlp::encodeString("dog"));
    
    Bytes encoded = rlp::encodeList(items);
    // [ "cat", "dog" ] = c8 83 63 61 74 83 64 6f 67
    EXPECT_EQ(encoded[0], 0xc8); // 0xc0 + 8
}

// Test concat utility
TEST_F(RlpTest, Concat) {
    std::vector<Bytes> parts;
    parts.push_back({0x01, 0x02});
    parts.push_back({0x03, 0x04, 0x05});
    parts.push_back({0x06});
    
    Bytes result = rlp::concat(parts);
    ASSERT_EQ(result.size(), 6u);
    EXPECT_EQ(result[0], 0x01);
    EXPECT_EQ(result[1], 0x02);
    EXPECT_EQ(result[2], 0x03);
    EXPECT_EQ(result[3], 0x04);
    EXPECT_EQ(result[4], 0x05);
    EXPECT_EQ(result[5], 0x06);
}

// Test decode simple bytes
TEST_F(RlpTest, DecodeSimpleBytes) {
    Bytes encoded = {0x83, 'd', 'o', 'g'};
    ::rlp::Decoded decoded = ::rlp::decode(encoded);
    
    EXPECT_FALSE(decoded.isList);
    ASSERT_EQ(decoded.bytes.size(), 3u);
    EXPECT_EQ(decoded.bytes[0], 'd');
    EXPECT_EQ(decoded.bytes[1], 'o');
    EXPECT_EQ(decoded.bytes[2], 'g');
}

// Test decode empty bytes
TEST_F(RlpTest, DecodeEmptyBytes) {
    Bytes encoded = {0x80};
    ::rlp::Decoded decoded = ::rlp::decode(encoded);
    
    EXPECT_FALSE(decoded.isList);
    EXPECT_TRUE(decoded.bytes.empty());
}

// Test decode empty list
TEST_F(RlpTest, DecodeEmptyList) {
    Bytes encoded = {0xc0};
    ::rlp::Decoded decoded = ::rlp::decode(encoded);
    
    EXPECT_TRUE(decoded.isList);
    EXPECT_TRUE(decoded.list.empty());
}

// Test decode list with items
TEST_F(RlpTest, DecodeListWithItems) {
    // Encode then decode
    std::vector<Bytes> items;
    items.push_back(rlp::encodeString("cat"));
    items.push_back(rlp::encodeString("dog"));
    Bytes encoded = rlp::encodeList(items);
    
    ::rlp::Decoded decoded = ::rlp::decode(encoded);
    
    EXPECT_TRUE(decoded.isList);
    ASSERT_EQ(decoded.list.size(), 2u);
    
    // First item: "cat"
    EXPECT_FALSE(decoded.list[0].isList);
    ASSERT_EQ(decoded.list[0].bytes.size(), 3u);
    
    // Second item: "dog"
    EXPECT_FALSE(decoded.list[1].isList);
    ASSERT_EQ(decoded.list[1].bytes.size(), 3u);
}

// Test roundtrip encoding/decoding
TEST_F(RlpTest, RoundtripBytes) {
    Bytes original = {0x12, 0x34, 0x56, 0x78};
    Bytes encoded = rlp::encodeBytes(original);
    ::rlp::Decoded decoded = ::rlp::decode(encoded);
    
    EXPECT_FALSE(decoded.isList);
    EXPECT_EQ(decoded.bytes, original);
}

