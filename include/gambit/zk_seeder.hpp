#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include <mutex>

#include "gambit/address.hpp"
#include "gambit/hash.hpp"
#include "gambit/blockchain.hpp"

namespace gambit {

struct PeerInfo {
    Address nodeId;           // identity (from pubkey)
    std::string ip;           // advertised IP (string form)
    std::uint16_t port;       // p2p port
    std::uint64_t lastSeen;   // unix time
    std::uint64_t score;      // quality metric, for later
};

class ZkSeederService {
public:
    explicit ZkSeederService(Blockchain& chain);

    // Node asks to be registered
    bool registerPeer(const PeerInfo& peer, const Bytes& proof, std::string& err);

    // Node asks for peers
    std::vector<PeerInfo> getPeers(std::size_t limit) const;

    // Node asks "what record do you have for me?"
    bool getRecordFor(const Address& nodeId, PeerInfo& out) const;

private:
    Blockchain& chain_;
    mutable std::mutex mutex_;
    std::vector<PeerInfo> peers_;

    bool verifyProof(const PeerInfo& peer, const Bytes& proof, std::string& err) const;
    std::uint64_t now() const;
};

} // namespace gambit
