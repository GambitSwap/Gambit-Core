cmake_minimum_required(VERSION 3.15)
project(Gambit VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(GAMBIT_BUILD_TESTS "Build Gambit tests" OFF)

# External deps
add_subdirectory(external/secp256k1)

# Include dirs
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/external/tiny-keccak
    ${CMAKE_CURRENT_SOURCE_DIR}/external/secp256k1/include
)

# Core library
add_library(gambit_core
    src/hash.cpp
    src/address.cpp
    src/keys.cpp
    src/rlp.cpp
    src/transaction.cpp
    src/state.cpp
    src/genesis.cpp
    src/zk.cpp
    src/zk_mining_engine.cpp
    src/zk_seeder.cpp
    src/zk_seeder_client.cpp
    src/block.cpp
    src/blockchain.cpp
    src/miner.cpp
    src/p2p_message.cpp
    src/p2p_peer.cpp
    src/p2p_node.cpp
    src/dns_seed.cpp
    src/rpc_server.cpp
    src/mpt.cpp
    src/receipt.cpp
    src/bloom.cpp
    external/tiny-keccak/keccak.cpp
)

target_link_libraries(gambit_core
    secp256k1
)

target_compile_definitions(gambit_core PRIVATE -DSECP256K1_BUILD=1)

# App
add_subdirectory(app)

# Tests
if(GAMBIT_BUILD_TESTS)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    )
    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    enable_testing()
    add_subdirectory(tests)
endif()