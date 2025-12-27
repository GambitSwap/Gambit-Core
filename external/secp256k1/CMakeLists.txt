cmake_minimum_required(VERSION 3.15)

project(secp256k1_external C)

set(SECP256K1_SRC
    src/secp256k1.c
    src/precomputed_ecmult.c
    src/precomputed_ecmult_gen.c
)

add_library(secp256k1 STATIC ${SECP256K1_SRC})

target_include_directories(secp256k1 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(secp256k1 PRIVATE
    SECP256K1_BUILD=1
    ENABLE_MODULE_RECOVERY=1
    ECMULT_WINDOW_SIZE=15
    ECMULT_GEN_PREC_BITS=4
)

# Using 8x32 scalar on all platforms for simplicity
# For 64-bit platforms, you could use 4x64 for better performance
