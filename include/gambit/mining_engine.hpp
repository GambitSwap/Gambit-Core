#pragma once
#include "gambit/block.hpp"
#include "gambit/blockchain.hpp"

namespace gambit {

class MiningEngine {
public:
    virtual ~MiningEngine() = default;

    virtual Block buildBlockTemplate(Blockchain& chain) = 0;
    virtual bool validateMinedBlock(const Block& block, Blockchain& chain) = 0;
};

} // namespace gambit
