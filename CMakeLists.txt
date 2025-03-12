cmake_minimum_required(VERSION 3.16)
project(Coincidence)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# First, we'll add the CMake folder, in case we'll need to find_package later:
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake")

# Compile commands, useful for some IDEs like VS-Code
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)
enable_language(OBJC)
enable_language(OBJCXX)

# Enable ccache if available for faster rebuilds
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    set(CMAKE_OBJC_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    set(CMAKE_OBJCXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    message(STATUS "Using ccache: ${CCACHE_PROGRAM}")
endif()

# Parallel build settings
include(ProcessorCount)
ProcessorCount(CPU_COUNT)
if(NOT CPU_COUNT EQUAL 0)
    set(CMAKE_BUILD_PARALLEL_LEVEL ${CPU_COUNT})
    message(STATUS "Parallel build enabled with ${CPU_COUNT} cores")
else()
    set(CMAKE_BUILD_PARALLEL_LEVEL 8)
    message(STATUS "Parallel build enabled with default 8 cores")
endif()

# For Ninja generator, explicitly set job count
if(CMAKE_GENERATOR STREQUAL "Ninja")
    set(CMAKE_MAKE_PROGRAM_ARGS "${CMAKE_MAKE_PROGRAM_ARGS} -j${CMAKE_BUILD_PARALLEL_LEVEL}")
endif()

# Configure unity builds for faster compilation
set(CMAKE_UNITY_BUILD ON)
set(CMAKE_UNITY_BUILD_BATCH_SIZE 8)

# Ensure consistent symbol visibility for all targets
if (APPLE)
    set(CMAKE_CXX_VISIBILITY_PRESET hidden)
    set(CMAKE_C_VISIBILITY_PRESET hidden)
    set(CMAKE_VISIBILITY_INLINES_HIDDEN YES)
elseif(UNIX)
    set(CMAKE_CXX_VISIBILITY_PRESET hidden)
    set(CMAKE_C_VISIBILITY_PRESET hidden)
    set(CMAKE_VISIBILITY_INLINES_HIDDEN YES)
endif()

# Minimum MacOS target, set globally
if(${CMAKE_SYSTEM_NAME} STREQUAL "iOS")
    set(CMAKE_OSX_DEPLOYMENT_TARGET 11.0 CACHE STRING "Minimum OS X deployment version" FORCE)
else()
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.11" CACHE STRING "Minimum OS X deployment version" FORCE)
endif()

option(UniversalBinary "Build universal binary for mac" OFF)
if(UniversalBinary)
    set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE INTERNAL "")
endif()

# Set cache directory for build artifacts
if(NOT DEFINED CMAKE_CACHEFILE_DIR)
    set(CMAKE_CACHEFILE_DIR "${CMAKE_BINARY_DIR}/CMakeCache" CACHE PATH "Directory for CMake cache files")
endif()

# We 'require' that we need juce. If JUCE isn't found, it will revert to what's in
# CMake/Findjuce.cmake, where you can see how JUCE is brought in/configured
find_package(juce REQUIRED)

# Add compiler flags to handle weak symbol visibility consistently
if(APPLE)
    add_compile_options(-fvisibility=hidden -fvisibility-inlines-hidden)
    # For Clang on macOS, use these options to fix weak symbol warnings
    add_link_options(-Wl,-weak_reference_mismatches,weak)
endif()

# Disable unity builds for JUCE modules by excluding their source files
set(_juce_modules_dir "${CMAKE_CURRENT_SOURCE_DIR}/cmake-build-debug/_deps/juce-src/modules")

if(EXISTS "${_juce_modules_dir}")
    message(STATUS "Excluding JUCE modules from unity builds: ${_juce_modules_dir}")
    file(GLOB_RECURSE _juce_module_files CONFIGURE_DEPENDS
        "${_juce_modules_dir}/*.cpp"
        "${_juce_modules_dir}/*.c"
        "${_juce_modules_dir}/*.h"
        "${_juce_modules_dir}/*.mm"
        "${_juce_modules_dir}/*.m")
    
    list(LENGTH _juce_module_files _juce_file_count)
    message(STATUS "Found ${_juce_file_count} files to exclude from unity builds")
    foreach(_file IN LISTS _juce_module_files)
        set_property(SOURCE ${_file} PROPERTY SKIP_UNITY_BUILD_INCLUSION ON)
    endforeach()
else()
    message(WARNING "Could not locate JUCE modules directory at: ${_juce_modules_dir}")
endif()

Include(FetchContent)
FetchContent_Declare(melatonin_inspector
        GIT_REPOSITORY https://github.com/sudara/melatonin_inspector.git
        GIT_TAG origin/main
        SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/melatonin_inspector)
FetchContent_MakeAvailable(melatonin_inspector)

add_definitions(-w)

add_subdirectory(Source)