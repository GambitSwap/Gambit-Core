#pragma once
#include <vector>
#include <cstdint>
#include <string>
#include "gambit/address.hpp"
#include "gambit/hash.hpp"

struct secp256k1_context_struct;
typedef secp256k1_context_struct secp256k1_context;

namespace gambit
{

    class Keys
    {
    public:
        // Recover address from msgHash and signature (EIP-155 aware)
        static Address recoverAddress(const Bytes32 &msgHash,
                                      const Signature &sig,
                                      std::uint64_t chainId);
    };

    struct Signature
    {
        std::vector<std::uint8_t> r; // 32 bytes
        std::vector<std::uint8_t> s; // 32 bytes
        std::uint8_t v;              // recovery id (0 or 1)
    };

    class KeyPair
    {
    public:
        KeyPair();
        static KeyPair random();

        const std::vector<std::uint8_t> &privateKey() const { return priv_; }
        const std::vector<std::uint8_t> &publicKey() const { return pub_; } // 64 bytes (x||y)
        Address address() const;

        Signature sign(const Bytes32 &msgHash, std::uint64_t chainId) const;

        static bool verify(const Bytes32 &msgHash,
                           const Signature &sig,
                           const std::vector<std::uint8_t> &pubKey);

    private:
        std::vector<std::uint8_t> priv_; // 32 bytes
        std::vector<std::uint8_t> pub_;  // 64 bytes (uncompressed, no prefix)

        static secp256k1_context *context();
    };

} // namespace gambit
