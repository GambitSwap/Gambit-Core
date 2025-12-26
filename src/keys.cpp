#include "gambit/keys.hpp"
#include <secp256k1.h>
#include <secp256k1_recovery.h>
#include <random>
#include <stdexcept>
#include <cstring>

namespace gambit {

static secp256k1_context* g_ctx = nullptr;

secp256k1_context* KeyPair::context() {
    if (!g_ctx) {
        g_ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
    }
    return g_ctx;
}

KeyPair::KeyPair() : priv_(32), pub_(64) {}

KeyPair KeyPair::random() {
    KeyPair kp;

    std::random_device rd;
    std::mt19937_64 gen(rd());

    // Generate valid private key
    while (true) {
        for (auto& b : kp.priv_) {
            b = static_cast<std::uint8_t>(gen() & 0xFF);
        }
        if (secp256k1_ec_seckey_verify(context(), kp.priv_.data())) {
            break;
        }
    }

    // Derive public key
    secp256k1_pubkey pubkey;
    if (!secp256k1_ec_pubkey_create(context(), &pubkey, kp.priv_.data())) {
        throw std::runtime_error("Failed to create secp256k1 public key");
    }

    // Serialize uncompressed (65 bytes: 0x04 + 64 bytes)
    std::uint8_t out[65];
    size_t outlen = 65;
    secp256k1_ec_pubkey_serialize(context(), out, &outlen, &pubkey, SECP256K1_EC_UNCOMPRESSED);

    // Drop prefix 0x04
    kp.pub_.assign(out + 1, out + 65);

    return kp;
}

Address KeyPair::address() const {
    return Address::fromPublicKey(pub_);
}

Signature KeyPair::sign(const Bytes32& msgHash, std::uint64_t chainId) const {
    Signature sig;
    sig.r.resize(32);
    sig.s.resize(32);

    secp256k1_ecdsa_recoverable_signature recsig;

    if (!secp256k1_ecdsa_sign_recoverable(
            context(),
            &recsig,
            msgHash.data(),
            priv_.data(),
            nullptr,
            nullptr
        )) {
        throw std::runtime_error("Failed to sign message");
    }

    int recid = 0;
    std::uint8_t compact[64];
    secp256k1_ecdsa_recoverable_signature_serialize_compact(
        context(),
        compact,
        &recid,
        &recsig
    );

    std::memcpy(sig.r.data(), compact, 32);
    std::memcpy(sig.s.data(), compact + 32, 32);

    // EIP-155: v = recid + 35 + 2 * chainId
    sig.v = static_cast<std::uint8_t>(recid);

    return sig;
}

bool KeyPair::verify(const Bytes32& msgHash,
                     const Signature& sig,
                     const std::vector<std::uint8_t>& pubKey)
{
    if (pubKey.size() != 64) {
        throw std::runtime_error("verify: pubKey must be 64 bytes");
    }

    // Reconstruct secp256k1_pubkey
    secp256k1_pubkey pk;
    std::uint8_t full[65];
    full[0] = 0x04;
    std::memcpy(full + 1, pubKey.data(), 64);

    if (!secp256k1_ec_pubkey_parse(context(), &pk, full, 65)) {
        return false;
    }

    // Convert r||s into normal signature
    secp256k1_ecdsa_signature normsig;
    std::uint8_t compact[64];
    std::memcpy(compact, sig.r.data(), 32);
    std::memcpy(compact + 32, sig.s.data(), 32);

    if (!secp256k1_ecdsa_signature_parse_compact(context(), &normsig, compact)) {
        return false;
    }

    // Normalize signature (Ethereum requires low-S)
    secp256k1_ecdsa_signature normalized;
    secp256k1_ecdsa_signature_normalize(context(), &normalized, &normsig);

    return secp256k1_ecdsa_verify(context(), &normalized, msgHash.data(), &pk);
}

Address Keys::recoverAddress(const Bytes32& msgHash,
                             const Signature& sig,
                             std::uint64_t chainId)
{
    // EIP-155: v = recId + 35 + 2*chainId
    int recId;
    if (sig.v == 0 || sig.v == 1) {
        recId = sig.v;
    } else {
        std::uint64_t vFull = sig.v;
        if (vFull < 35) {
            throw std::runtime_error("recoverAddress: unsupported v");
        }
        std::uint64_t cid = (vFull - 35) / 2;
        if (cid != chainId) {
            throw std::runtime_error("recoverAddress: chainId mismatch");
        }
        recId = static_cast<int>(vFull - (35 + 2 * chainId));
    }

    if (sig.r.size() != 32 || sig.s.size() != 32) {
        throw std::runtime_error("recoverAddress: invalid r/s size");
    }

    unsigned char compact[64];
    std::memcpy(compact,     sig.r.data(), 32);
    std::memcpy(compact + 32, sig.s.data(), 32);

    secp256k1_ecdsa_recoverable_signature recSig;
    if (!secp256k1_ecdsa_recoverable_signature_parse_compact(ctx, &recSig, compact, recId)) {
        throw std::runtime_error("recoverAddress: parse_compact failed");
    }

    secp256k1_pubkey pubkey;
    if (!secp256k1_ecdsa_recover(ctx, &pubkey, &recSig, msgHash.data())) {
        throw std::runtime_error("recoverAddress: recover failed");
    }

    // Serialize uncompressed
    unsigned char pub[65];
    size_t pubLen = 65;
    secp256k1_ec_pubkey_serialize(ctx, pub, &pubLen, &pubkey, SECP256K1_EC_UNCOMPRESSED);

    // Ethereum address = last 20 bytes of keccak256(pubkey[1..64])
    Bytes pubBytes(pub + 1, pub + pubLen); // skip 0x04
    Bytes hash = keccak256(pubBytes);

    Bytes addrBytes(hash.end() - 20, hash.end());
    Address addr = Address::fromBytes(addrBytes);
    return addr;
}

} // namespace gambit
