# OpenSSL CMake wrapper
# This provides OpenSSL as a CMake target for the Gambit project

message(STATUS "Setting up OpenSSL 3.2.1...")

set(OPENSSL_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(OPENSSL_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}")

if(WIN32)
    # For Windows, we need to build OpenSSL using its native build system
    # This requires Perl, but we'll provide clear instructions if missing
    
    find_package(Perl)
    
    if(NOT PERL_FOUND)
        message(FATAL_ERROR 
            "OpenSSL requires Perl to build on Windows.\n"
            "Please install one of:\n"
            "  1. Strawberry Perl: https://strawberryperl.com/\n"
            "  2. ActivePerl: https://www.activestate.com/products/perl/\n"
            "Or set SKIP_OPENSSL_BUILD=ON to use system OpenSSL instead")
    endif()
    
    # Create build directory
    file(MAKE_DIRECTORY "${OPENSSL_BUILD_DIR}")
    
    # Check if OpenSSL is already built
    if(NOT EXISTS "${OPENSSL_BUILD_DIR}/lib/libcrypto.lib")
        message(STATUS "Configuring OpenSSL (this may take a moment)...")
        
        # On Windows with MSVC
        execute_process(
            COMMAND perl Configure VC-WIN64A no-shared --prefix=${OPENSSL_BUILD_DIR}
            WORKING_DIRECTORY ${OPENSSL_SOURCE_DIR}
            RESULT_VARIABLE OPENSSL_CONFIG_RESULT
            OUTPUT_VARIABLE OPENSSL_CONFIG_OUTPUT
        )
        
        if(NOT OPENSSL_CONFIG_RESULT EQUAL 0)
            message(FATAL_ERROR "OpenSSL configuration failed:\n${OPENSSL_CONFIG_OUTPUT}")
        endif()
        
        message(STATUS "Building OpenSSL (this may take several minutes)...")
        
        execute_process(
            COMMAND nmake
            WORKING_DIRECTORY ${OPENSSL_SOURCE_DIR}
            RESULT_VARIABLE OPENSSL_BUILD_RESULT
            OUTPUT_VARIABLE OPENSSL_BUILD_OUTPUT
        )
        
        if(NOT OPENSSL_BUILD_RESULT EQUAL 0)
            message(FATAL_ERROR "OpenSSL build failed:\n${OPENSSL_BUILD_OUTPUT}")
        endif()
        
        message(STATUS "Installing OpenSSL...")
        
        execute_process(
            COMMAND nmake install
            WORKING_DIRECTORY ${OPENSSL_SOURCE_DIR}
            RESULT_VARIABLE OPENSSL_INSTALL_RESULT
        )
        
        if(NOT OPENSSL_INSTALL_RESULT EQUAL 0)
            message(FATAL_ERROR "OpenSSL installation failed")
        endif()
        
        message(STATUS "OpenSSL built successfully")
    else()
        message(STATUS "OpenSSL already built - skipping build step")
    endif()
    
elseif(APPLE)
    # macOS - can use Homebrew or native build
    message(STATUS "Building OpenSSL for macOS...")
    
    file(MAKE_DIRECTORY "${OPENSSL_BUILD_DIR}")
    
    if(NOT EXISTS "${OPENSSL_BUILD_DIR}/lib/libcrypto.a")
        execute_process(
            COMMAND ./Configure darwin64-arm64-cc no-shared --prefix=${OPENSSL_BUILD_DIR}
            WORKING_DIRECTORY ${OPENSSL_SOURCE_DIR}
        )
        
        execute_process(
            COMMAND make
            WORKING_DIRECTORY ${OPENSSL_SOURCE_DIR}
        )
        
        execute_process(
            COMMAND make install
            WORKING_DIRECTORY ${OPENSSL_SOURCE_DIR}
        )
    endif()
    
else()
    # Linux - assume standard build tools available
    message(STATUS "Building OpenSSL for Linux...")
    
    file(MAKE_DIRECTORY "${OPENSSL_BUILD_DIR}")
    
    if(NOT EXISTS "${OPENSSL_BUILD_DIR}/lib/libcrypto.a")
        execute_process(
            COMMAND ./config no-shared --prefix=${OPENSSL_BUILD_DIR}
            WORKING_DIRECTORY ${OPENSSL_SOURCE_DIR}
        )
        
        execute_process(
            COMMAND make
            WORKING_DIRECTORY ${OPENSSL_SOURCE_DIR}
        )
        
        execute_process(
            COMMAND make install
            WORKING_DIRECTORY ${OPENSSL_SOURCE_DIR}
        )
    endif()
endif()

# Create imported targets for OpenSSL
add_library(OpenSSL::Crypto STATIC IMPORTED GLOBAL)
set_target_properties(OpenSSL::Crypto PROPERTIES
    IMPORTED_LOCATION "${OPENSSL_BUILD_DIR}/lib/libcrypto${CMAKE_STATIC_LIBRARY_SUFFIX}"
    INTERFACE_INCLUDE_DIRECTORIES "${OPENSSL_BUILD_DIR}/include"
)

add_library(OpenSSL::SSL STATIC IMPORTED GLOBAL)
set_target_properties(OpenSSL::SSL PROPERTIES
    IMPORTED_LOCATION "${OPENSSL_BUILD_DIR}/lib/libssl${CMAKE_STATIC_LIBRARY_SUFFIX}"
    INTERFACE_INCLUDE_DIRECTORIES "${OPENSSL_BUILD_DIR}/include"
    INTERFACE_LINK_LIBRARIES "OpenSSL::Crypto"
)

message(STATUS "OpenSSL targets created")
message(STATUS "  Crypto: ${OPENSSL_BUILD_DIR}/lib/libcrypto${CMAKE_STATIC_LIBRARY_SUFFIX}")
message(STATUS "  SSL: ${OPENSSL_BUILD_DIR}/lib/libssl${CMAKE_STATIC_LIBRARY_SUFFIX}")

