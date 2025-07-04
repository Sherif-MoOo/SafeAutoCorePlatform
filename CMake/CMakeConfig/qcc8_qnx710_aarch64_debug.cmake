#[=======================================================================
# OpenAA: Open Source Adaptive AUTOSAR Project
# Author: Sherif Mohamed
#
# File description:
# -----------------
# CMake initial-cache file for OpenAA - QNX 7.10 aarch64 using QCC-8.
# This file sets essential CMake variables and compiler/linker flags
# to streamline the build process.
#=======================================================================]

#[=======================================================================[
.rst:
QNX710_AARCH64_QCC8
---------------------
CMake initial cache file for QNX 7.1.0 aarch64 using QCC-8.

All variables can be set as initial cache variables and passed as a file to CMake:

.. code-block:: cmake

    # Create an initial cache file (qcc8_qnx710_aarch64.cmake) and define in there:
    set(CMAKE_PREFIX_PATH "/opt/qnx710/host/linux/x86_64/usr" CACHE STRING "")
    set(CMAKE_TOOLCHAIN_FILE "/opt/toolchain/qcc8_qnx710_aarch64.cmake" CACHE PATH "")

.. code-block:: shell-session

    # QNX dev kit can only be used with bash!
    $ bash -i
    $ source /opt/qnx710/qnxsdp-env.sh
    $ cmake -C CMake/CMakeConfig/qcc8_qnx710_aarch64.cmake -S <project-root> -B <build-dir>

Platform, quality, and product-specific build caches can be defined externally without
unnecessarily inflating or patching CMakeLists.txt files contained in the project.

Fixed development processes for construction, testing, and packaging can be generalized
independently of the project.

.. note::

    Set the environment variables before you initialize the cache in respect to the
    actual paths on your machine! To use the QNX dev kit, one must use bash and
    source the file /opt/qnx710/qnxsdp-env.sh!
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
message(STATUS "Using qcc8_qnx710_aarch64_debug.cmake for debug build configuration.")

#=======================================================================
# Library Building Preferences
#=======================================================================
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries")

#=======================================================================
# Compiler Configuration for C
#=======================================================================
#
# The following flags enable extensive warnings and ISO compliance.
# Additional options for debugging are included:
#   -DDEBUG: Defines a debug macro.
#   -fno-inline: Disables inlining to improve function call traceability.
#   -fno-omit-frame-pointer: Retains frame pointers for better backtraces.
#   -g: Enables debug symbols.
#=======================================================================
set(CMAKE_C_FLAGS_INIT "-Wall -Wextra -Wconversion -pedantic -Wshadow -Wdouble-promotion -Wformat=2 \
-Wnull-dereference -D_QNX_SOURCE -DDEBUG -g -fno-inline -fno-omit-frame-pointer" CACHE STRING "Initial C Compiler Flags for Debug Build")

# Debug build specific C flags: Disable optimization and use maximum debug symbols.
set(CMAKE_C_FLAGS_DEBUG_INIT "-O0 -g3" CACHE STRING "C Compiler Flags for Debug Build")

#=======================================================================
# Compiler Configuration for C++
#=======================================================================
#
# Similar to the C flags, the C++ options include:
#   -DDEBUG: Enables debug-specific code.
#   -fno-inline and -fno-omit-frame-pointer for improved backtraces.
#   -g: Includes debug symbols.
#=======================================================================
set(CMAKE_CXX_FLAGS_INIT "-Wall -Wextra -Wnon-virtual-dtor -Wconversion -Wold-style-cast -pedantic \
-Wshadow -Wdouble-promotion -Wformat=2 -D_QNX_SOURCE -DDEBUG -g -fno-inline -fno-omit-frame-pointer" CACHE STRING "Initial C++ Compiler Flags for Debug Build")

# Debug build specific C++ flags.
set(CMAKE_CXX_FLAGS_DEBUG_INIT "-O0 -g3" CACHE STRING "C++ Compiler Flags for Debug Build")

#=======================================================================
# Linker Flags Configuration
#=======================================================================
#
# The linker flags ensure that debug symbols are retained in the binaries.
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
# They can be enabled as needed by appending the appropriate flag.
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
# 1. Debug builds include extensive debugging information with -g3.
# 2. The -DDEBUG macro is defined to enable conditional debug code.
# 3. Inlining is disabled and frame pointers are retained (-fno-inline -fno-omit-frame-pointer)
#    to ensure accurate backtraces.
# 4. Address, thread, and undefined behavior sanitizers are enabled to help catch runtime issues.
# 5. Optimizations are disabled (-O0) to facilitate a more predictable debugging environment.
#=======================================================================