#pragma once
#include "gambit/mining_engine.hpp"
#include "gambit/zk.hpp"

namespace gambit {

class ZkMiningEngine : public MiningEngine {
public:
    Block buildBlockTemplate(Blockchain& chain) override;
    bool validateMinedBlock(const Block& block, Blockchain& chain) override;
};

} // namespace gambit
