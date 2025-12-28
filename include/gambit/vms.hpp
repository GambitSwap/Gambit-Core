
namespace gambit {

class EvmVM : public IVM {
    public:
        ExecutionResult execute(const Transaction& tx, State& state) override {
            // EVM execution logic...
        }
        std::string name() const override { return "EVM"; }
        ContractType type() const override { return ContractType::EVM; }
};
    
class WasmVM : public IVM {
    public:
        ExecutionResult execute(const Transaction& tx, State& state) override {
            // WASM execution logic...
        }
        std::string name() const override { return "WASM"; }
        ContractType type() const override { return ContractType::WASM; }
};
    
class CoreVM : public IVM {
    public:
        ExecutionResult execute(const Transaction& tx, State& state) override {
            // Core logic...
        }
        std::string name() const override { return "CORE"; }
        ContractType type() const override { return ContractType::CORE; }
};
}


/*

void initBuiltInVMs(VMRegistry& registry) {
    registry.registerVM(ContractType::EVM,  std::make_unique<EvmVM>());
    registry.registerVM(ContractType::WASM, std::make_unique<WasmVM>());
    registry.registerVM(ContractType::CORE, std::make_unique/CoreVM>());
}


*/    
