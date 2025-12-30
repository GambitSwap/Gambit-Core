add_executable(gambit_node main.cpp)
target_link_libraries(gambit_node gambit_core)

# Add GUI support if FLTK is available
if(GAMBIT_GUI_AVAILABLE)
    target_compile_definitions(gambit_node PRIVATE GAMBIT_GUI_ENABLED=1)
    target_link_libraries(gambit_node fltk::fltk)
    target_include_directories(gambit_node PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/../gui
    )
    # Add GUI sources for embedded mode
    target_sources(gambit_node PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../gui/main_gui.cpp
    )
endif()

# Windows-specific settings
if(WIN32)
    target_link_libraries(gambit_node ws2_32)
endif()