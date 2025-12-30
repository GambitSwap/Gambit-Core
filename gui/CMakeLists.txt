# Standalone GUI executable (RPC client mode only)
# For embedded mode, use: gambit_node --gui

add_executable(gambit_gui main_gui.cpp)

# Link with FLTK
target_link_libraries(gambit_gui 
    fltk::fltk
    gambit_core
)

target_include_directories(gambit_gui PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../external/tiny-keccak
    ${CMAKE_CURRENT_SOURCE_DIR}/../external/secp256k1/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../external
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Windows-specific settings for networking
if(WIN32)
    target_link_libraries(gambit_gui ws2_32)
endif()
