#include "gambit/zk_seeder.hpp"
#include "gambit/hash.hpp"
#include "gambit/keys.hpp"
#include <chrono>
#include <algorithm>

namespace gambit {
    struct SeederPublicInputs {
        Address nodeId;
        std::string ip;
        std::uint16_t port;
        std::uint64_t timestamp;
    };
    
    struct SeederWitness {
        Bytes32 sk;       // or some representation of secret key
    };

// Legacy declarations removed: ZkSeederProver/ZkSeederVerifier types are not
// present in the current public headers. Seeder service uses signature-based
// verification (see `verifyProof`) and does not require external prover APIs.

ZkSeederService::ZkSeederService(Blockchain& chain)
    : chain_(chain) {}

std::uint64_t ZkSeederService::now() const {
    using namespace std::chrono;
    return duration_cast<seconds>(
        system_clock::now().time_since_epoch()
    ).count();
}

bool ZkSeederService::verifyProof(const PeerInfo& peer, const Bytes& proof, std::string& err) const {
    // v1: Signature-based proof
    //
    // message = keccak256( nodeId || ip || port || lastSeen )
    // proof   = raw signature bytes (r||s||v or some format you define)

    // Serialize message
    Bytes msg;
    const auto& addrBytes = peer.nodeId.bytes();
    msg.insert(msg.end(), addrBytes.begin(), addrBytes.end());
    msg.insert(msg.end(), peer.ip.begin(), peer.ip.end());
    msg.push_back(static_cast<std::uint8_t>(peer.port >> 8));
    msg.push_back(static_cast<std::uint8_t>(peer.port & 0xFF));
    for (int i = 7; i >= 0; --i) {
        msg.push_back(static_cast<std::uint8_t>((peer.lastSeen >> (8 * i)) & 0xFF));
    }

    Bytes32 msgHash = keccak256_32(msg);

    // Parse proof -> Signature (you define this format)
    if (proof.size() != 65) {
        err = "Invalid proof size";
        return false;
    }

    Signature sig;
    sig.r.assign(proof.begin(), proof.begin() + 32);
    sig.s.assign(proof.begin() + 32, proof.begin() + 64);
    sig.v = proof[64]; // assume EIP-155 style full v or 27/28 etc.

    // Recover address
    try {
        Address recovered = Keys::recoverAddress(msgHash, sig, chain_.chainId());
        if (!(recovered == peer.nodeId)) {
            err = "Proof does not match nodeId";
            return false;
        }
    } catch (const std::exception& e) {
        err = e.what();
        return false;
    }

    // v2: hook zk here later (e.g., verify a zk SNARK over same public inputs)
    return true;
}

bool ZkSeederService::registerPeer(const PeerInfo& peer, const Bytes& proof, std::string& err) {
    PeerInfo p = peer;
    p.lastSeen = now();

    if (!verifyProof(p, proof, err)) {
        return false;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    // upsert by nodeId
    auto it = std::find_if(peers_.begin(), peers_.end(), [&](const PeerInfo& x) {
        return x.nodeId == p.nodeId;
    });

    if (it != peers_.end()) {
        *it = p;
    } else {
        peers_.push_back(p);
    }

    return true;
}

std::vector<PeerInfo> ZkSeederService::getPeers(std::size_t limit) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<PeerInfo> out = peers_;

    // TODO: sort by score/lastSeen; for now just truncate
    if (out.size() > limit) {
        out.resize(limit);
    }
    return out;
}

bool ZkSeederService::getRecordFor(const Address& nodeId, PeerInfo& out) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = std::find_if(peers_.begin(), peers_.end(), [&](const PeerInfo& x) {
        return x.nodeId == nodeId;
    });
    if (it == peers_.end()) {
        return false;
    }
    out = *it;
    return true;
}

} // namespace gambit
