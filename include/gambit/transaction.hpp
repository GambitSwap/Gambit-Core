#pragma once
#include <string>
#include <vector>
#include <cstdint>

#include "gambit/address.hpp"
#include "gambit/keys.hpp"
#include "gambit/hash.hpp"
#include "gambit/rlp.hpp"

namespace gambit {

class Transaction {
public:
    // Core fields
    std::uint64_t nonce{0};
    std::uint64_t gasPrice{0};
    std::uint64_t gasLimit{0};
    Address to;
    std::uint64_t value{0};
    std::vector<std::uint8_t> data;
    std::uint64_t chainId{1};

    Address from;

    // Signature fields
    Signature sig;

    // Cached hash (keccak256(rlpEncodeSigned))
    std::string hash;


    // RLP encoding for signing (EIP-155)
    Bytes rlpEncodeForSigning() const;

    // RLP encoding for broadcasting (includes v,r,s)
    Bytes rlpEncodeSigned() const;

    // Hash for signing (keccak256 of RLP)
    Bytes32 signingHash() const;

    // Sign with a keypair
    void signWith(const KeyPair& key);

    // Verify signature
    bool verifySignature() const;

    // Serialize to hex for network transport
    std::string toHex() const;

    // Deserialize from hex (optional)
    static Transaction fromHex(const std::string& hex);
};

} // namespace gambit
