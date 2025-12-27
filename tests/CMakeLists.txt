# Gambit Unit Tests

set(TEST_SOURCES
    test_hash.cpp
    test_rlp.cpp
    test_address.cpp
    test_keys.cpp
    test_transaction.cpp
    test_mpt.cpp
    test_bloom.cpp
    test_block.cpp
)

add_executable(gambit_tests ${TEST_SOURCES})

target_link_libraries(gambit_tests
    gambit_core
    GTest::gtest_main
)

target_include_directories(gambit_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/external/tiny-keccak
    ${CMAKE_SOURCE_DIR}/external/secp256k1/include
)

include(GoogleTest)
gtest_discover_tests(gambit_tests)

