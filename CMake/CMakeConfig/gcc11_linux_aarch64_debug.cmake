#[=======================================================================
# OpenAA: Open Source Adaptive AUTOSAR Project
# Author: Sherif Mohamed
#
# File description:
# -----------------
# CMake initial-cache file for GCC 11 on Linux x86_64.
# This file sets essential CMake variables and compiler/linker flags
# to streamline the build process for Linux targets.
#=======================================================================]

#[=======================================================================[
.rst:
gcc11_linux_x86_64
-------------------
CMake initial cache file for GCC 11 on Linux x86_64.

All variables can be set as initial cache variables and passed as a file to CMake:

.. code-block:: cmake

    # Create an initial cache file (gcc11_linux_x86_64.cmake) and define in there:
    set(CMAKE_PREFIX_PATH "/usr/local/gcc-11" CACHE STRING "")
    set(CMAKE_TOOLCHAIN_FILE "/path/to/toolchain/gcc11_linux_x86_64.cmake" CACHE PATH "")

.. code-block:: shell-session

    $ cmake -C CMake/CMakeConfig/gcc11_linux_x86_64.cmake -S <project-root> -B <build-dir>

#]=======================================================================]

#[=======================================================================[
  CMake Specific Project Settings
#]=======================================================================]

#=======================================================================
# Library Building Preferences
#=======================================================================
#
# Control whether to build shared or static libraries.
# Set to ON to build shared libraries, OFF to build static libraries.
#
# Recommended: OFF for static libraries to simplify deployment.
#=======================================================================

#=======================================================================
message(STATUS "Using gcc11_linux_x86_64_debug.cmake for initial cache setup.")

#=======================================================================
# Library Building Preferences
#=======================================================================
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries")

#=======================================================================
# Compiler Configuration for C
#=======================================================================
#
# The initial C compiler flags include a full suite of warnings and 
# debugging information.
#=======================================================================
set(CMAKE_C_FLAGS_INIT "-Wall -Wextra -Wconversion -pedantic -Wshadow -Wdouble-promotion -Wformat=2 \
-Wnull-dereference  -g -fno-inline -fno-omit-frame-pointer" CACHE STRING "Initial C Compiler Flags for Debug Build")

# For Debug builds, optimizations are disabled (-O0) and the highest level
# of debug information (-g3) is enabled.
set(CMAKE_C_FLAGS_DEBUG_INIT "-O0 -g3" CACHE STRING "C Compiler Flags for Debug Build")

#=======================================================================
# Compiler Configuration for C++
#=======================================================================
#
# Similar to C, the initial C++ compiler flags include strict warnings and
# extensive debugging information.
#=======================================================================
set(CMAKE_CXX_FLAGS_INIT "-Wall -Wextra -Wnon-virtual-dtor -Wconversion -Wold-style-cast -pedantic \
-Wshadow -Wno-error=deprecated-declarations -Wdouble-promotion -Wformat=2  -g -fno-inline -fno-omit-frame-pointer" CACHE STRING "Initial C++ Compiler Flags for Debug Build")

set(CMAKE_CXX_FLAGS_DEBUG_INIT "-O0 -g3" CACHE STRING "C++ Compiler Flags for Debug Build")

#=======================================================================
# Linker Flags Configuration
#=======================================================================
#
# The linker is instructed to include debugging symbols.
#=======================================================================
set(CMAKE_EXE_LINKER_FLAGS_INIT "-g" CACHE STRING "Initial Executable Linker Flags for Debug")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "-g" CACHE STRING "Initial Shared Library Linker Flags for Debug")

#=======================================================================
# Build-Type Configuration
#=======================================================================
#
# The build type is explicitly set to Debug.
#=======================================================================
set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type (Debug, Release, etc.)")

#=======================================================================
# Additional Debugging Features
#=======================================================================
#
# These sanitizer flags help catch runtime issues such as memory errors,
# data races, and undefined behavior.
#=======================================================================
set(CMAKE_C_FLAGS_ALSAN_INIT "-fsanitize=address -fno-omit-frame-pointer" CACHE STRING "Address Sanitizer Flags for C")
set(CMAKE_CXX_FLAGS_ALSAN_INIT "-fsanitize=address -fno-omit-frame-pointer" CACHE STRING "Address Sanitizer Flags for C++")

set(CMAKE_C_FLAGS_TSAN_INIT "-fsanitize=thread" CACHE STRING "Thread Sanitizer Flags for C")
set(CMAKE_CXX_FLAGS_TSAN_INIT "-fsanitize=thread" CACHE STRING "Thread Sanitizer Flags for C++")

set(CMAKE_C_FLAGS_UBSAN_INIT "-fsanitize=undefined" CACHE STRING "Undefined Behavior Sanitizer Flags for C")
set(CMAKE_CXX_FLAGS_UBSAN_INIT "-fsanitize=undefined" CACHE STRING "Undefined Behavior Sanitizer Flags for C++")

#=======================================================================
# Notes for Debugging
#=======================================================================
# 1. Debug builds include extensive debugging information with `-g3`.
# 2. Address, thread, and undefined behavior sanitizers are enabled to catch runtime issues.
# 3. Warnings are set to maximum to enforce ISO compliance and detect potential issues.
# 4. Optimizations are disabled (`-O0`) to facilitate debugging.
#=======================================================================