# Gambit — Minimal Ethereum-Style zk-Blockchain in Modern C++

Gambit is a compact, modular, production-grade prototype blockchain client written in C++17.

It includes:

- Real Keccak-256 (tiny-keccak)
- Real secp256k1 signing (libsecp256k1)
- Ethereum-style addresses (EIP-55)
- Ethereum-style transactions (EIP-155)
- RLP encoding
- Deterministic state model
- Genesis configuration
- zk validity layer (mocked but pluggable)
- Block construction + hashing
- Blockchain engine
- P2P networking (TCP)
- Runnable node

## Directory Structure

Gambit/
  CMakeLists.txt
  external/
    tiny-keccak/
      keccak.hpp          # vendored (single-header) keccak-256
    secp256k1/
      CMakeLists.txt      # builds libsecp256k1
      src/...             # upstream secp256k1 source (you'll drop it here)
      include/...         # upstream headers
  include/
    gambit/
      hash.hpp
      address.hpp
      keys.hpp
      rlp.hpp
      transaction.hpp
      account.hpp
      state.hpp
      genesis.hpp
      zk.hpp
      block.hpp
      blockchain.hpp
      p2p_message.hpp
      p2p_peer.hpp
      p2p_node.hpp
  src/
    hash.cpp
    address.cpp
    keys.cpp
    rlp.cpp
    transaction.cpp
    state.cpp
    genesis.cpp
    zk.cpp
    block.cpp
    blockchain.cpp
    p2p_message.cpp
    p2p_peer.cpp
    p2p_node.cpp
  app/
    CMakeLists.txt
    main.cpp
  README.md


## Build Instructions

### 1. Clone repo

> https://github.com

### 2. Build secp256k1 (vendored)

The project includes a vendored copy of secp256k1.  
CMake will build it automatically.

### 3. Build Gambit

> mkdir build cd build cmake .. make -j8

### 4. Run node

> ./gambit_node

## Quick build & run

Prerequisites
- CMake >= 3.10
- A C++17-compatible compiler (gcc/clang or MSVC)
- Build tools: make / ninja / Visual Studio
- Optional: git, Python (for auxiliary scripts)

This repository includes required crypto helpers under `external/`:
- [secp256k1](external/secp256k1)
- [tiny-keccak](external/tiny-keccak/keccak.hpp)
- [openssl](external/openssl) — built from source during CMake configuration

## Build prerequisites

### All platforms
- CMake 3.15 or higher
- C++17 compatible compiler (MSVC 2019+, GCC 9+, Clang 10+)

### Windows (MSVC)
- **Perl** (required to build OpenSSL from source)
  - Install [Strawberry Perl](https://strawberryperl.com/) or [ActivePerl](https://www.activestate.com/products/perl/)
  - Or run `install_deps.bat` (requires Administrator or Chocolatey)
- Visual Studio 2019 Build Tools or later (for MSVC compiler)

### macOS
- Xcode Command Line Tools (`xcode-select --install`)

### Linux
- GCC or Clang development tools (`build-essential` on Ubuntu/Debian)

## Build

Build (Unix: Linux / macOS)
```sh
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -- -j$(nproc)
```

Build (Windows — MSVC)
```powershell
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

Alternate generator (Ninja)
```sh
cmake -G Ninja .. -DCMAKE_BUILD_TYPE=Release
ninja
```

Where the binary is
- The main application entry point is [`main`](app/main.cpp) — after a successful build you'll find the app target in the build output (typically `build/app/` or `build/` depending on generator). Run the produced executable, e.g.:
```sh
./build/app/<executable-name>   # or ./build/<executable-name>
```

Development notes
- Core blockchain logic: [src/blockchain.cpp](src/blockchain.cpp)  
- Zero-knowledge mining / related code: [src/zk.cpp](src/zk.cpp), [src/zk_mining_engine.cpp](src/zk_mining_engine.cpp)  
- Key management: [src/keys.cpp](src/keys.cpp) and headers under [include/gambit](include/gambit)

Common issues
- If CMake cannot find a compiler, ensure `CC`/`CXX` are set or install build tools (e.g., `build-essential` on Debian/Ubuntu).
- If you hit link errors for secp256k1, the project embeds the library under [external/secp256k1](external/secp256k1); ensure the subdirectory is intact and CMake ran from a clean `build/`.

Contributing
- Follow the existing code layout: headers in `include/gambit/`, implementation in `src/`, executable target in `app/`.
- Add CMake target entries to [app/CMakeLists.txt](app/CMakeLists.txt) or top-level [CMakeLists.txt](CMakeLists.txt) as appropriate.

License
---

Gambit is designed to be a clean, readable, remixable foundation for building real blockchain systems.


## What's next

```cpp
enum class ContractType {
    EVM,        // Solidity-style bytecode
    WASM,       // EOSIO/WAX-style C++ compiled to WASM
    CORE        // Built-in system contracts
};
```

-- Raw Dialog Dump
First byte of contract code = VM selector
Example:
- 0x01 → EVM bytecode
- 0x02 → WASM module
- 0x03 → Core contract reference

Store VM type in contract metadata
state[contractAddress].vmType = ContractType::WASM;

- Use metadata to store the VM type
- Use opcode prefix to validate the code
- Use domain separation internally so the VM dispatcher is trivial
This gives you flexibility, safety, and speed.

🏗️ Execution Pipeline
Here’s how Gambit executes a transaction:
tx → decode → find contract → read vmType → dispatch to VM → run → return

Dispatcher pseudocode:
ExecutionResult Executor::execute(const Transaction& tx) {
    Address to = tx.to;

    ContractType vm = state_.getContractType(to);

    switch (vm) {
        case ContractType::EVM:
            return evm_.execute(tx, state_);
        case ContractType::WASM:
            return wasm_.execute(tx, state_);
        case ContractType::CORE:
            return core_.execute(tx, state_);
        default:
            throw std::runtime_error("Unknown VM type");
    }
}



🔥 VM #1 — Solidity / EVM Contracts
This is your existing EVM interpreter:
- Stack machine
- 256‑bit words
- Opcodes 0x00–0xFF
- Gas model
- Storage trie
- Calldata → ABI decode
You already have most of this in Gambit.

🔥 VM #2 — C++ / EOSIO‑Style WASM Contracts
This is a WASM runtime with:
- Deterministic WASM subset
- No floating point
- No syscalls except what you expose
- Memory sandbox
- Host functions for:
- read_action_data
- write_action_data
- get_balance
- set_balance
- db_get, db_set, etc.
Contract deployment:
- User uploads WASM module
- First byte = 0x02
- Node stores:
- WASM bytecode
- ABI (optional)
- vmType = WASM
Execution:
wasmRuntime.execute(wasmModule, actionName, actionData);



🔥 VM #3 — Core Contracts (Native Gambit Logic)
These are built‑in C++ classes, not user‑uploaded code.
Examples:
- Token system
- Governance
- Staking
- Seeding service
- Fee market
- System upgrades
Execution:
coreContracts[contractAddress]->invoke(method, args, state_);


These are:
- Fast
- Safe
- No gas metering needed (or minimal)
- Upgradable via governance

🧬 How Contracts Declare Their VM Type
On deployment:
When a contract is created:
- Node inspects the first byte of the code
- Sets vmType in state
- Stores the code
Example:
if (code[0] == 0x01) vmType = EVM;
else if (code[0] == 0x02) vmType = WASM;
else if (code[0] == 0x03) vmType = CORE;
else throw InvalidContractFormat;



🧩 How Calls Work Between VMs
This is where Gambit becomes next‑level.
EVM → WASM
- EVM contract calls a WASM contract
- Dispatcher switches VM
- Calldata is passed raw
- WASM contract returns bytes
- EVM receives bytes
WASM → EVM
- WASM host function call_contract(address, data)
- Dispatcher routes to EVM
- Returns bytes
CORE → anything
- Core contracts can call any VM
- Useful for governance, staking, etc.
Anything → CORE
- Core contracts are just addresses
- They behave like system contracts in Ethereum

🧱 Storage Model
All VMs share the same state trie:
- EVM uses 256‑bit storage slots
- WASM uses key/value DB
- Core uses structured C++ objects
You unify them by giving each VM a namespace:
state[contract][vmNamespace][key] = value


Example:
- EVM: state[addr]["evm"][slot] = value
- WASM: state[addr]["wasm"][key] = value
- CORE: state[addr]["core"][field] = value
This prevents collisions and keeps everything clean.

🚀 Why This Design Is Powerful
You get:
- EVM compatibility (Solidity ecosystem)
- EOSIO/WAX compatibility (C++ smart contracts)
- Native system contracts (fast, safe, upgradable)
- Cross‑VM calls
- Unified state
- Unified transaction format
- Unified gas model (or per‑VM gas rules)
This is better than:
- Polkadot (multiple runtimes but isolated)
- Cosmos (WASM only)
- Ethereum (EVM only)
- EOSIO (WASM only)
Gambit becomes a multi‑contract‑language superchain.
