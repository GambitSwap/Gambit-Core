cmake_minimum_required(VERSION 3.15)

project(secp256k1_external C)

set(SECP256K1_SRC
src/secp256k1.c
)

add_library(secp256k1 STATIC ${SECP256K1_SRC})

target_include_directories(secp256k1 PUBLIC
${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_definitions(secp256k1 PRIVATE
-DSECP256K1_BUILD=1
)

# You’ll vendor the upstream secp256k1 sources into external/secp256k1/src and external/secp256k1/include. This CMake just builds them.
# You may want to enable specific modules here later:
# e.g. recovery module, ecdh, etc.
# You’ll populate src/ and include/ from the official repo:
# https://github.com/bitcoin-core/secp256k1
