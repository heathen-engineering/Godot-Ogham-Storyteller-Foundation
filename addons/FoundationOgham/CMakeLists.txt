cmake_minimum_required(VERSION 3.16)
project(FoundationOgham)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Defaults to the godot-cpp git submodule vendored at the repo root (../../godot-cpp
# relative to this file, i.e. <repo>/godot-cpp). Override with -DGODOT_CPP_PATH=...
# to build against a different checkout.
set(GODOT_CPP_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../godot-cpp" CACHE PATH "Path to Godot-cpp bindings")

# Defaults to the Game-Framework git submodule vendored at <repo>/extern/Game-Framework —
# needed for Subsystem/SubsystemManager headers and to build the gameframework::shared
# import stub this addon links against at compile time. See Godot-Game-Framework's README,
# "The linking model", for why this MUST be the shared target (not gameframework::gameframework)
# and why the resulting libgameframework.* build byproduct here must NOT be packaged.
set(GAMEFRAMEWORK_CORE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../extern/Game-Framework/Core" CACHE PATH "Path to Game-Framework's Core (engine-agnostic C++)")

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

string(TOLOWER "${CMAKE_BUILD_TYPE}" TYPE_STRING)

if(ANDROID)
    set(PLATFORM "android")
elseif(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set(PLATFORM "ios")
elseif(WIN32)
    set(PLATFORM "windows")
elseif(APPLE)
    set(PLATFORM "macos")
else()
    set(PLATFORM "linux")
endif()

if(ANDROID)
    # Godot's own arch names: arm64-v8a -> arm64, armeabi-v7a -> arm32.
    if(ANDROID_ABI STREQUAL "arm64-v8a")
        set(ARCH "arm64")
    elseif(ANDROID_ABI STREQUAL "armeabi-v7a")
        set(ARCH "arm32")
    else()
        set(ARCH "${ANDROID_ABI}")
    endif()
elseif(PLATFORM STREQUAL "ios")
    set(ARCH "arm64")
else()
    set(ARCH "x86_64")
endif()

message(STATUS "Building: ${CMAKE_BUILD_TYPE} on ${PLATFORM} ${ARCH}")
message(STATUS "GODOT_CPP_PATH: ${GODOT_CPP_PATH}")
message(STATUS "No xxHash/GameplayTags/Lexicon compile-time deps — Ogham talks to those extensions purely via Variant calls at runtime, see src/public/OghamInterop.h")

if(NOT EXISTS "${GODOT_CPP_PATH}/CMakeLists.txt")
    message(FATAL_ERROR "godot-cpp not found at ${GODOT_CPP_PATH}. Did you run 'git submodule update --init --recursive'?")
endif()
if(NOT EXISTS "${GAMEFRAMEWORK_CORE_PATH}/CMakeLists.txt")
    message(FATAL_ERROR "Game-Framework Core not found at ${GAMEFRAMEWORK_CORE_PATH}. Did you run 'git submodule update --init --recursive'?")
endif()

# Build godot-cpp as part of this build (no separate SCons pre-build step needed).
add_subdirectory(${GODOT_CPP_PATH} ${CMAKE_BINARY_DIR}/godot-cpp)

# Build Game-Framework's Core for its headers + the gameframework::shared import stub.
# OUTPUT_NAME on gameframework_shared MUST match Godot-Game-Framework's own convention
# byte-for-byte (libgameframework.<platform>.<config>.<arch>) so the DT_NEEDED/import
# name this addon embeds matches what's actually shipped at runtime.
set(GAMEFRAMEWORK_BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(${GAMEFRAMEWORK_CORE_PATH} ${CMAKE_BINARY_DIR}/gameframework-core)
if(NOT PLATFORM STREQUAL "ios")
    set_target_properties(gameframework_shared PROPERTIES
        PREFIX ""
        OUTPUT_NAME "libgameframework.${PLATFORM}.${TYPE_STRING}.${ARCH}"
    )
endif()

file(GLOB_RECURSE SOURCES "src/private/*.cpp")

# iOS does not allow dynamically-loaded GDExtensions — Godot requires them
# statically linked into the exported binary there (unlike every other
# platform this addon targets).
if(PLATFORM STREQUAL "ios")
    add_library(FoundationOgham STATIC ${SOURCES})
else()
    add_library(FoundationOgham SHARED ${SOURCES})
endif()

target_include_directories(FoundationOgham PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/public
)

if(PLATFORM STREQUAL "ios")
    target_link_libraries(FoundationOgham PRIVATE godot::cpp gameframework::gameframework)
else()
    target_link_libraries(FoundationOgham PRIVATE godot::cpp gameframework::shared)
endif()

set_target_properties(FoundationOgham
    PROPERTIES
    PREFIX ""
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin"
    OUTPUT_NAME "libFoundationOgham.${PLATFORM}.${TYPE_STRING}.${ARCH}"
)

if(NOT WIN32 AND NOT PLATFORM STREQUAL "ios")
    target_link_options(FoundationOgham PRIVATE "-rdynamic")
endif()

# Avoids CMake's default absolute build-tree RPATH — see Godot-Game-Framework's
# CMakeLists.txt for the full story (caught there via readelf). Doesn't ship its
# own libgameframework.*, so $ORIGIN won't resolve the dependency itself; the OS
# loader finds it already-loaded-by-SONAME from FoundationGameFramework.so
# instead (verified with a real two-extension test).
if(PLATFORM STREQUAL "linux" OR PLATFORM STREQUAL "android")
    set_target_properties(FoundationOgham PROPERTIES
        BUILD_WITH_INSTALL_RPATH TRUE
        INSTALL_RPATH "$ORIGIN"
    )
elseif(PLATFORM STREQUAL "macos")
    set_target_properties(FoundationOgham PROPERTIES
        BUILD_WITH_INSTALL_RPATH TRUE
        INSTALL_RPATH "@loader_path"
    )
endif()
