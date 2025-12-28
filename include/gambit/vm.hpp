#pragma once
#include <string>

#include "gambit/transaction.hpp"
#include "gambit/state.hpp"
#include "gambit/execution_result.hpp"

namespace gambit {

enum class ContractType : std::uint8_t {
    EVM   = 0,
    WASM  = 1,
    CORE  = 2,
    // Plugin types start from 128+ for safety
    PLUGIN_BASE = 128
};

class IVM {
public:
    virtual ~IVM() = default;

    virtual ExecutionResult execute(const Transaction& tx, State& state) = 0;

    // Optional: identity / metadata
    virtual std::string name() const = 0;
    virtual ContractType type() const = 0;
};

} // namespace gambit
