# FLTK should already be available from the main CMakeLists.txt
# No need to find it here again

add_executable(gambit_gui main_gui.cpp)
target_link_libraries(gambit_gui 
    gambit_core
    fltk::fltk
)

target_include_directories(gambit_gui PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../external/tiny-keccak
    ${CMAKE_CURRENT_SOURCE_DIR}/../external/secp256k1/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../external
    ${CMAKE_CURRENT_SOURCE_DIR}
)