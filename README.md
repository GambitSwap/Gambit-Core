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
