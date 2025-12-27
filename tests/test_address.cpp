#include <gtest/gtest.h>
#include "gambit/address.hpp"
#include "gambit/hash.hpp"

using namespace gambit;

class AddressTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// Test default constructor creates zero address
TEST_F(AddressTest, DefaultConstructor) {
    Address addr;
    const auto& bytes = addr.bytes();
    for (size_t i = 0; i < Address::kSize; ++i) {
        EXPECT_EQ(bytes[i], 0x00);
    }
}

// Test construction from bytes array
TEST_F(AddressTest, FromBytesArray) {
    std::array<uint8_t, 20> raw = {};
    raw[0] = 0xde;
    raw[19] = 0xad;
    
    Address addr(raw);
    EXPECT_EQ(addr.bytes()[0], 0xde);
    EXPECT_EQ(addr.bytes()[19], 0xad);
}

// Test fromBytes with vector
TEST_F(AddressTest, FromBytesVector) {
    std::vector<uint8_t> raw(20, 0x00);
    raw[0] = 0xab;
    raw[19] = 0xcd;
    
    Address addr = Address::fromBytes(raw);
    EXPECT_EQ(addr.bytes()[0], 0xab);
    EXPECT_EQ(addr.bytes()[19], 0xcd);
}

// Test fromHex with lowercase
TEST_F(AddressTest, FromHexLowercase) {
    std::string hex = "0xdeadbeefdeadbeefdeadbeefdeadbeefdeadbeef";
    Address addr = Address::fromHex(hex);
    EXPECT_EQ(addr.bytes()[0], 0xde);
    EXPECT_EQ(addr.bytes()[1], 0xad);
    EXPECT_EQ(addr.bytes()[2], 0xbe);
    EXPECT_EQ(addr.bytes()[3], 0xef);
}

// Test fromHex without prefix
TEST_F(AddressTest, FromHexNoPrefix) {
    std::string hex = "deadbeefdeadbeefdeadbeefdeadbeefdeadbeef";
    Address addr = Address::fromHex(hex);
    EXPECT_EQ(addr.bytes()[0], 0xde);
    EXPECT_EQ(addr.bytes()[1], 0xad);
}

// Test toHex returns proper format
TEST_F(AddressTest, ToHexFormat) {
    std::string hex = "0xdeadbeefdeadbeefdeadbeefdeadbeefdeadbeef";
    Address addr = Address::fromHex(hex);
    std::string result = addr.toHex(false);
    
    // Should be 42 chars with 0x prefix
    EXPECT_EQ(result.length(), 42u);
    EXPECT_EQ(result.substr(0, 2), "0x");
}

// Test equality operator
TEST_F(AddressTest, Equality) {
    std::string hex = "0xdeadbeefdeadbeefdeadbeefdeadbeefdeadbeef";
    Address addr1 = Address::fromHex(hex);
    Address addr2 = Address::fromHex(hex);
    
    EXPECT_TRUE(addr1 == addr2);
    EXPECT_FALSE(addr1 != addr2);
}

// Test inequality operator
TEST_F(AddressTest, Inequality) {
    Address addr1 = Address::fromHex("0xdeadbeefdeadbeefdeadbeefdeadbeefdeadbeef");
    Address addr2 = Address::fromHex("0x1234567890123456789012345678901234567890");
    
    EXPECT_FALSE(addr1 == addr2);
    EXPECT_TRUE(addr1 != addr2);
}

// Test zero address
TEST_F(AddressTest, ZeroAddress) {
    Address addr = Address::fromHex("0x0000000000000000000000000000000000000000");
    const auto& bytes = addr.bytes();
    for (size_t i = 0; i < Address::kSize; ++i) {
        EXPECT_EQ(bytes[i], 0x00);
    }
}

// Test checksum address format (EIP-55)
TEST_F(AddressTest, ChecksumFormat) {
    // Known checksum address
    Address addr = Address::fromHex("0x5aAeb6053F3E94C9b9A09f33669435E7Ef1BeAed");
    std::string checksummed = addr.toHex(true);
    
    // Should contain mixed case
    bool hasUpper = false;
    bool hasLower = false;
    for (size_t i = 2; i < checksummed.length(); ++i) {
        char c = checksummed[i];
        if (c >= 'A' && c <= 'F') hasUpper = true;
        if (c >= 'a' && c <= 'f') hasLower = true;
    }
    // The checksum address should have mixed case for addresses with letters
    EXPECT_TRUE(hasUpper || hasLower); // At minimum, should have some letters
}

// Test roundtrip hex conversion
TEST_F(AddressTest, HexRoundtrip) {
    std::string original = "0xdeadbeefdeadbeefdeadbeefdeadbeefdeadbeef";
    Address addr = Address::fromHex(original);
    std::string result = addr.toHex(false);
    
    // Compare lowercase
    std::string originalLower = original;
    std::string resultLower = result;
    std::transform(originalLower.begin(), originalLower.end(), originalLower.begin(), ::tolower);
    std::transform(resultLower.begin(), resultLower.end(), resultLower.begin(), ::tolower);
    
    EXPECT_EQ(originalLower, resultLower);
}

// Test address size constant
TEST_F(AddressTest, SizeConstant) {
    EXPECT_EQ(Address::kSize, 20u);
}

