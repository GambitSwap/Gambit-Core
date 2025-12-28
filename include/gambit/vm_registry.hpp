#pragma once
#include <array>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <stdexcept>

#include "gambit/vm.hpp"

namespace gambit {

class VMRegistry {
public:
    VMRegistry();

    // Register a VM instance for a given type.
    // Takes ownership of vm.
    void registerVM(ContractType type, std::unique_ptr<IVM> vm);

    // Get VM for a given type. Returns nullptr if not found.
    IVM* get(ContractType type) const;

    // Check if a VM exists for a type.
    bool has(ContractType type) const;

private:
    // Fast path: array index by underlying enum value (0–255)
    std::array<IVM*, 256> table_;
    // Ownership storage so instances live for the process lifetime
    std::unordered_map<ContractType, std::unique_ptr<IVM>> storage_;

    mutable std::mutex mutex_;
};

} // namespace gambit
