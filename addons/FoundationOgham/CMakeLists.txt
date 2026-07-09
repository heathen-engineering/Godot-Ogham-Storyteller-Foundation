cmake_minimum_required(VERSION 3.16)
project(FoundationOgham)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Defaults to the godot-cpp git submodule vendored at the repo root (../../godot-cpp
# relative to this file, i.e. <repo>/godot-cpp). Override with -DGODOT_CPP_PATH=...
# to build against a different checkout.
set(GODOT_CPP_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../godot-cpp" CACHE PATH "Path to Godot-cpp bindings")

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

string(TOLOWER "${CMAKE_BUILD_TYPE}" TYPE_STRING)

if(WIN32)
    set(PLATFORM "windows")
elseif(APPLE)
    set(PLATFORM "macos")
else()
    set(PLATFORM "linux")
endif()

set(ARCH "x86_64")

message(STATUS "Building: ${CMAKE_BUILD_TYPE} on ${PLATFORM} ${ARCH}")
message(STATUS "GODOT_CPP_PATH: ${GODOT_CPP_PATH}")
message(STATUS "No xxHash/GameplayTags/Lexicon compile-time deps — Ogham talks to those extensions purely via Variant calls at runtime, see src/public/OghamInterop.h")

if(NOT EXISTS "${GODOT_CPP_PATH}/CMakeLists.txt")
    message(FATAL_ERROR "godot-cpp not found at ${GODOT_CPP_PATH}. Did you run 'git submodule update --init --recursive'?")
endif()

# Build godot-cpp as part of this build (no separate SCons pre-build step needed).
add_subdirectory(${GODOT_CPP_PATH} ${CMAKE_BINARY_DIR}/godot-cpp)

file(GLOB_RECURSE SOURCES "src/private/*.cpp")

add_library(FoundationOgham SHARED ${SOURCES})

target_include_directories(FoundationOgham PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/public
)

target_link_libraries(FoundationOgham PRIVATE godot::cpp)

set_target_properties(FoundationOgham
    PROPERTIES
    PREFIX ""
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin"
    OUTPUT_NAME "libFoundationOgham.${PLATFORM}.${TYPE_STRING}.${ARCH}"
)

if(NOT WIN32)
    target_link_options(FoundationOgham PRIVATE "-rdynamic")
endif()
