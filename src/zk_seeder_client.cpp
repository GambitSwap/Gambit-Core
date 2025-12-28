#include "gambit/zk_seeder_client.hpp"
#include "gambit/hash.hpp"
#include <chrono>
#include <stdexcept>

namespace gambit {

Bytes ZkSeederClient::buildSeederProof(const KeyPair& key,
                                       PeerInfo& peer,
                                       std::uint64_t chainId)
{
    // 1. Ensure nodeId is consistent with the key
    Address addrFromKey = key.address();
    if (!(peer.nodeId == Address()) && !(peer.nodeId == addrFromKey)) {
        throw std::runtime_error("buildSeederProof: peer.nodeId does not match key");
    }
    peer.nodeId = addrFromKey;

    // 2. Set lastSeen to "now" (or let caller do it; but this keeps it consistent)
    using namespace std::chrono;
    std::uint64_t ts = duration_cast<seconds>(
        system_clock::now().time_since_epoch()
    ).count();
    peer.lastSeen = ts;

    // 3. Serialize message exactly as verifyProof expects
    Bytes msg;
    Bytes nodeBytes = peer.nodeId.toBytes();
    msg.insert(msg.end(), nodeBytes.begin(), nodeBytes.end());

    msg.insert(msg.end(), peer.ip.begin(), peer.ip.end());

    msg.push_back(static_cast<std::uint8_t>(peer.port >> 8));
    msg.push_back(static_cast<std::uint8_t>(peer.port & 0xFF));

    for (int i = 7; i >= 0; --i) {
        msg.push_back(static_cast<std::uint8_t>((peer.lastSeen >> (8 * i)) & 0xFF));
    }

    Bytes32 msgHash = keccak256(msg);

    // 4. Sign with node key (secp256k1 ECDSA)
    Signature sig = key.sign(msgHash, chainId);
    // Assumes: key.sign returns { r (32 bytes), s (32 bytes), v }

    if (sig.r.size() != 32 || sig.s.size() != 32) {
        throw std::runtime_error("buildSeederProof: signer produced invalid r/s size");
    }

    // 5. Encode proof: r || s || v
    Bytes proof;
    proof.reserve(65);
    proof.insert(proof.end(), sig.r.begin(), sig.r.end());
    proof.insert(proof.end(), sig.s.begin(), sig.s.end());
    proof.push_back(sig.v);

    return proof;
}

} // namespace gambit
