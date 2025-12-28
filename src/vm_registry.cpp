#include "gambit/vm_registry.hpp"

namespace gambit {

VMRegistry::VMRegistry() {
    table_.fill(nullptr);
}

void VMRegistry::registerVM(ContractType type, std::unique_ptr<IVM> vm) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto key = type;
    auto raw = static_cast<std::uint8_t>(key);

    // Store ownership
    auto it = storage_.find(key);
    if (it != storage_.end()) {
        // Replace existing
        it->second = std::move(vm);
    } else {
        storage_.emplace(key, std::move(vm));
    }

    // Update table pointer
    table_[raw] = storage_.at(key).get();
}

IVM* VMRegistry::get(ContractType type) const {
    auto raw = static_cast<std::uint8_t>(type);
    // No lock needed: table_ is updated atomically from registerVM
    return table_[raw];
}

bool VMRegistry::has(ContractType type) const {
    return get(type) != nullptr;
}

} // namespace gambit


/*

ExecutionResult Executor::execute(const Transaction& tx) {
    ContractType vmType = state_.getContractType(tx.to);

    IVM* vm = vmRegistry_.get(vmType);
    if (!vm) {
        throw std::runtime_error("Unknown VM type");
    }
    return vm->execute(tx, state_);
}

*/