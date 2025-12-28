#pragma once
#include "gambit/zk_seeder.hpp"
#include "gambit/keys.hpp"

namespace gambit {

class ZkSeederClient {
public:
    // key: the node's keypair (identity)
    // peer: the PeerInfo we want to register (ip, port, nodeId may be set)
    // chainId: used for EIP-155-style v if your signer expects it
    static Bytes buildSeederProof(const KeyPair& key,
                                  PeerInfo& peer,
                                  std::uint64_t chainId);
};

} // namespace gambit
