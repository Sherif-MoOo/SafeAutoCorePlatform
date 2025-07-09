#[=======================================================================
# OpenAA: Open Source Adaptive AUTOSAR Project
# Author: Sherif Mohamed
#
# File description:
# -----------------
# CMake initial-cache file for OpenAA - QNX 8.0 x86_64 using QCC-12.
# Debug build configuration with enhanced warnings, debug options, and ISO compliance.
#=======================================================================]

#[=======================================================================[
.rst:
QNX800_x86_64_QCC12_DEBUG
---------------------------
CMake initial cache file for QNX 8.0 x86_64 debug builds using QCC-12.

This file sets essential CMake variables and compiler/linker flags to support
debugging by including extensive debug symbols, disabling optimizations, disabling
inlining to improve backtraces, and enabling maximum warnings for ISO compliance.

.. code-block:: shell-session

    # QNX dev kit can only be used with bash!
    $ bash -i
    $ source /opt/qnx800/qnxsdp-env.sh
    $ cmake -C CMake/CMakeConfig/qcc12_x86_64_debug.cmake -S <project-root> -B <build-dir>

#]=======================================================================]

message(STATUS "Using qcc12_x86_64_debug.cmake for debug build configuration.")

#=======================================================================
# Library Building Preferences
#=======================================================================
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries")

#=======================================================================
# Compiler Configuration for C
#=======================================================================
#
# The following flags enable extensive warnings and ISO compliance.
# Additional debug-specific options are included:
#   -DDEBUG: Defines a debug macro.
#   -fno-inline: Disables function inlining for improved backtrace accuracy.
#   -fno-omit-frame-pointer: Retains frame pointers for better debugging.
#   -g: Enables debug symbols.
#=======================================================================
set(CMAKE_C_FLAGS_INIT "-Wall -Wextra -Wconversion -pedantic -Wshadow -Wdouble-promotion -Wformat=2 \
-Wnull-dereference -D_QNX_SOURCE -DDEBUG -g -fno-inline -fno-omit-frame-pointer" CACHE STRING "Initial C Compiler Flags for Debug Build")

# Debug build specific C flags: No optimization, maximum debug symbols.
set(CMAKE_C_FLAGS_DEBUG_INIT "-O0 -g3" CACHE STRING "C Compiler Flags for Debug Build")

#=======================================================================
# Compiler Configuration for C++
#=======================================================================
#
# Similar to the C flags, these options enable extensive warnings,
# debug symbols, and disable inlining for improved backtraces.
#=======================================================================
set(CMAKE_CXX_FLAGS_INIT "-Wall -Wextra -Wnon-virtual-dtor -Wconversion -Wold-style-cast -pedantic \
-Wshadow -Wdouble-promotion -Wformat=2 -D_QNX_SOURCE -DDEBUG -g -fno-inline -fno-omit-frame-pointer" CACHE STRING "Initial C++ Compiler Flags for Debug Build")

# Debug build specific C++ flags.
set(CMAKE_CXX_FLAGS_DEBUG_INIT "-O0 -g3" CACHE STRING "C++ Compiler Flags for Debug Build")

#=======================================================================
# Linker Flags Configuration
#=======================================================================
#
# The following linker flags ensure that debug symbols are preserved.
#=======================================================================
set(CMAKE_EXE_LINKER_FLAGS_INIT "-g" CACHE STRING "Initial Executable Linker Flags for Debug")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "-g" CACHE STRING "Initial Shared Library Linker Flags for Debug")

#=======================================================================
# Build-Type Configuration
#=======================================================================
set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type (Debug, Release, etc.)")

#=======================================================================
# Additional Debugging Features
#=======================================================================
#
# The following sanitizer flags are available to help catch runtime issues.
# They can be enabled as needed.
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
# 1. Debug builds include extensive debugging information using `-g3`.
# 2. The `-DDEBUG` macro is defined to enable conditional compilation of debug code.
# 3. Inlining is disabled and frame pointers are retained (`-fno-inline -fno-omit-frame-pointer`)
#    to ensure more accurate backtraces.
# 4. Address, thread, and undefined behavior sanitizers are enabled to help catch runtime issues.
# 5. Optimizations are disabled (`-O0`) to create a predictable debugging environment.
#=======================================================================
