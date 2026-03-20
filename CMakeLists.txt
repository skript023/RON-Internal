cmake_minimum_required(VERSION 3.20)

project(Strinova CXX ASM_MASM)

set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

# Source Directory
set(SRC_DIR "${PROJECT_SOURCE_DIR}/src")

# OpenSSL configuration
set(OPENSSL_ROOT_DIR "${PROJECT_SOURCE_DIR}/openssl")
set(OPENSSL_INCLUDE_DIR "${OPENSSL_ROOT_DIR}/include")   # opsional
set(OPENSSL_LIBRARIES "${OPENSSL_ROOT_DIR}/lib")         # opsional
set(ZLIB_LIBRARIES "${OPENSSL_LIBRARIES}/zlib.lib")

find_package(OpenSSL REQUIRED)

get_target_property(ssl_loc OpenSSL::SSL LOCATION)
get_target_property(crypto_loc OpenSSL::Crypto LOCATION)

message(STATUS "OpenSSL::SSL    -> ${ssl_loc}")
message(STATUS "OpenSSL::Crypto -> ${crypto_loc}")

# Fetch modules
message("\nFetching modules")
include(scripts/minhook.cmake)
include(scripts/json.cmake)

message("\nFetching custom modules")
include(scripts/imgui.cmake)
include(scripts/stbi.cmake)
include(scripts/websocket.cmake)
include(scripts/asio.cmake)
include(scripts/discord.cmake)
include(scripts/g3log.cmake)
include(scripts/sdk.cmake)

# Strinova source files
message(STATUS "Strinova")
file(GLOB_RECURSE SRC_MAIN
    "${SRC_DIR}/**.hpp"
    "${SRC_DIR}/**.h"
    "${SRC_DIR}/**.cpp"
    "${SRC_DIR}/**.cc"
    "${SRC_DIR}/**.cxx"
    "${SRC_DIR}/**.asm"
)

add_library(Strinova MODULE "${SRC_MAIN}")  # Use STATIC or SHARED depending on requirements

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(TARGET Strinova PROPERTY CXX_STANDARD 26)

# Organize the source tree in IDE
source_group(TREE ${SRC_DIR} PREFIX "src" FILES ${SRC_MAIN})

# Include directories for the target
target_include_directories(Strinova PRIVATE 
    "${SRC_DIR}"
    "${SRC_DIR}/lib"
    "${SRC_DIR}/menu"
    "${SRC_DIR}/logger"
    "${SRC_DIR}/hooking"
    "${SRC_DIR}/services"
    "${SRC_DIR}/unreal_engine"
    "${OPENSSL_INCLUDE_DIR}"
    "${json_SOURCE_DIR}/single_include"
    "${imgui_SOURCE_DIR}"
    "${stbi_SOURCE_DIR}"
    "${websocket_SOURCE_DIR}"
    "${asio_SOURCE_DIR}/asio/include"
    "${minhook_SOURCE_DIR}/src/hde"
    "${discord_SOURCE_DIR}/include"
    "${sdk_SOURCE_DIR}"
)

# Precompiled headers
target_precompile_headers(Strinova PRIVATE "${SRC_DIR}/common.hpp")

# Link with external libraries
target_link_libraries(Strinova PRIVATE sdk minhook AsyncLogger imgui dbghelp discord ${ZLIB_LIBRARIES} OpenSSL::SSL OpenSSL::Crypto Crypt32)

# Compiler options for MSVC and others
if (MSVC)
    set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} /LTCG /OPT:REF,ICF /GUARD:NO")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /O2 /Oi /Ot /Oy /Ob3 /sdl- /GL /GF /GS- /Gw")
    string(REPLACE "/Ob1" "/Ob3" CMAKE_CXX_FLAGS_RELWITHDEBINFO ${CMAKE_CXX_FLAGS_RELWITHDEBINFO})
else ()
    target_compile_options(Ellohim PRIVATE -Wa,-mbig-obj)
endif ()

# Disable warnings as errors (optional)
set_property(TARGET Strinova PROPERTY COMPILE_WARNING_AS_ERROR OFF)

# Add compiler definitions for Windows specifics
add_compile_definitions(Strinova 
    "_CRT_SECURE_NO_WARNINGS"
    "NOMINMAX"
    "WIN32_LEAN_AND_MEAN"
)

set_target_properties(Strinova PROPERTIES OUTPUT_NAME "version")
# Add the 'injector' subdirectory
add_subdirectory(injector)