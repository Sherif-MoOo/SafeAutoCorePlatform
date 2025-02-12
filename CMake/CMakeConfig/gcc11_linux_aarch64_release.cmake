#[=======================================================================
# OpenAA: Open Source Adaptive AUTOSAR Project
# Author: Sherif Mohamed
#
# File description:
# -----------------
# CMake initial-cache file for GCC 11 on Linux aarch64 (Release configuration).
# This file sets essential CMake variables and compiler/linker flags to streamline
# the release build process for aarch64 targets.
#=======================================================================]

#[=======================================================================[
.rst:
gcc11_linux_aarch64_release
----------------------------
CMake initial cache file for GCC 11 on Linux aarch64 (Release build).

All variables can be set as initial cache variables and passed as a file to CMake:

.. code-block:: cmake

    # Create an initial cache file (gcc11_linux_aarch64_release.cmake) and define in there:
    set(CMAKE_PREFIX_PATH "/usr/local/gcc-11" CACHE STRING "")
    set(CMAKE_TOOLCHAIN_FILE "/path/to/toolchain/gcc11_linux_aarch64.cmake" CACHE PATH "")

.. code-block:: shell-session

    $ cmake -C CMake/CMakeConfig/gcc11_linux_aarch64_release.cmake -S <project-root> -B <build-dir>

#]=======================================================================]

#[=======================================================================[
  CMake Specific Project Settings
#]=======================================================================]

message(STATUS "Using gcc11_linux_aarch64_release.cmake for initial cache setup.")

#=======================================================================
# Library Building Preferences
#=======================================================================
# Control whether to build shared or static libraries.
# Recommended: OFF for static libraries to simplify deployment.
#=======================================================================
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries")

#=======================================================================
# Enable modern link-time optimization (LTO) via CMake.
# This instructs CMake to propagate LTO flags to both the compiler and linker.
#=======================================================================
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE CACHE BOOL "Enable Link Time Optimization (LTO)")

#=======================================================================
# Compiler Configuration for C
#=======================================================================
#
# The following flags enable a wide range of warnings and security checks,
# and are optimized for aarch64 by using -march=armv8-a.
#=======================================================================
set(CMAKE_C_FLAGS_INIT "-Wall -Wextra -Wconversion -pedantic -Wshadow \
-Werror -Wstrict-overflow=5 -Wmissing-prototypes -Wstrict-aliasing=2 \
-Wundef -Wredundant-decls -Wcast-align -Wformat=2 -Wfloat-equal \
-fno-common -march=armv8-a -flto -fstack-protector-strong \
-D_FORTIFY_SOURCE=2" CACHE STRING "Initial C Compiler Flags")

# Build-Type Specific Flags for C
set(CMAKE_C_FLAGS_RELEASE_INIT "-O3 -DNDEBUG -ffunction-sections -fdata-sections \
-fno-semantic-interposition -funroll-loops" CACHE STRING "C Compiler Flags for Release")
set(CMAKE_C_FLAGS_DEBUG_INIT   "-O0 -g"       CACHE STRING "C Compiler Flags for Debug")
set(CMAKE_C_FLAGS_MINSIZEREL_INIT "-Os -DNDEBUG" CACHE STRING "C Compiler Flags for MinSizeRel")
set(CMAKE_C_FLAGS_RELWITHDEBINFO_INIT "-O2 -g -DNDEBUG" CACHE STRING "C Compiler Flags for RelWithDebInfo")

# Additional sanitizer flags for C (empty in Release by default)
set(CMAKE_C_FLAGS_ALSAN_INIT "" CACHE STRING "C Compiler Flags for Address Sanitizer")
set(CMAKE_C_FLAGS_TSAN_INIT  "" CACHE STRING "C Compiler Flags for Thread Sanitizer")
set(CMAKE_C_FLAGS_UBSAN_INIT "" CACHE STRING "C Compiler Flags for Undefined Behavior Sanitizer")
set(CMAKE_C_FLAGS_RELEASEWITHO2_INIT "-O2 -DNDEBUG" CACHE STRING "C Compiler Flags for ReleaseWithO2")

#=======================================================================
# Compiler Configuration for C++
#=======================================================================
#
# Similar flags are set for C++ as for C, with additional options to disable
# exceptions and RTTI for production code.
#=======================================================================
set(CMAKE_CXX_FLAGS_INIT "-Wall -Wextra -Wnon-virtual-dtor -Wconversion \
-Wold-style-cast -pedantic -Wshadow -Wno-error=deprecated-declarations \
-Werror -Wstrict-overflow=5 -Wstrict-aliasing=2 -Wundef -Wredundant-decls \
-Wcast-align -Wformat=2 -Wfloat-equal -fno-exceptions -fno-rtti \
-fno-common -march=armv8-a -flto -fstack-protector-strong \
-D_FORTIFY_SOURCE=2" CACHE STRING "Initial C++ Compiler Flags")

# Build-Type Specific Flags for C++
set(CMAKE_CXX_FLAGS_RELEASE_INIT "-O3 -DNDEBUG -ffunction-sections -fdata-sections \
-fno-semantic-interposition -funroll-loops" CACHE STRING "C++ Compiler Flags for Release")
set(CMAKE_CXX_FLAGS_DEBUG_INIT   "-O0 -g"       CACHE STRING "C++ Compiler Flags for Debug")
set(CMAKE_CXX_FLAGS_MINSIZEREL_INIT "-Os -DNDEBUG" CACHE STRING "C++ Compiler Flags for MinSizeRel")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO_INIT "-O2 -g -DNDEBUG" CACHE STRING "C++ Compiler Flags for RelWithDebInfo")

# Additional sanitizer flags for C++ (empty in Release by default)
set(CMAKE_CXX_FLAGS_ALSAN_INIT "" CACHE STRING "C++ Compiler Flags for Address Sanitizer")
set(CMAKE_CXX_FLAGS_TSAN_INIT  "" CACHE STRING "C++ Compiler Flags for Thread Sanitizer")
set(CMAKE_CXX_FLAGS_UBSAN_INIT "" CACHE STRING "C++ Compiler Flags for Undefined Behavior Sanitizer")
set(CMAKE_CXX_FLAGS_RELEASEWITHO2_INIT "-O2 -DNDEBUG" CACHE STRING "C++ Compiler Flags for ReleaseWithO2")

#=======================================================================
# Linker Flags Configuration
#=======================================================================
#
# These flags are used by the linker during respective build types.
# For Release builds, LTO and garbage collection of unused sections are enabled.
#=======================================================================
set(CMAKE_EXE_LINKER_FLAGS_INIT "" CACHE STRING "Initial Executable Linker Flags")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE_INIT "-flto -Wl,--gc-sections" CACHE STRING "Executable Linker Flags for Release")
set(CMAKE_EXE_LINKER_FLAGS_DEBUG_INIT   "-g" CACHE STRING "Executable Linker Flags for Debug")
set(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL_INIT "-s" CACHE STRING "Executable Linker Flags for MinSizeRel")
set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO_INIT "-g" CACHE STRING "Executable Linker Flags for RelWithDebInfo")
set(CMAKE_EXE_LINKER_FLAGS_RELEASEWITHO2_INIT "" CACHE STRING "Executable Linker Flags for ReleaseWithO2")

# (Similar settings for shared, module, and static libraries can be added as needed.)
# For example:
set(CMAKE_SHARED_LINKER_FLAGS_INIT "" CACHE STRING "Initial Shared Library Linker Flags")
set(CMAKE_SHARED_LINKER_FLAGS_RELEASE_INIT "-flto -Wl,--gc-sections" CACHE STRING "Shared Library Linker Flags for Release")
set(CMAKE_MODULE_LINKER_FLAGS_INIT "" CACHE STRING "Initial Module Linker Flags")
set(CMAKE_MODULE_LINKER_FLAGS_RELEASE_INIT "-flto -Wl,--gc-sections" CACHE STRING "Module Linker Flags for Release")
set(CMAKE_STATIC_LINKER_FLAGS_INIT "" CACHE STRING "Initial Static Library Linker Flags")
set(CMAKE_STATIC_LINKER_FLAGS_RELEASE_INIT "-flto" CACHE STRING "Static Library Linker Flags for Release")

#=======================================================================
# AUTOSAR Specific Compiler Flags
#=======================================================================
#
# Define any AUTOSAR-specific macros required by your project.
#=======================================================================
set(CMAKE_C_FLAGS_INIT "${CMAKE_C_FLAGS_INIT} -D_AUTOSAR=4.2.2 -DAUTOSAR_COMPILATION" CACHE STRING "Initial C Compiler Flags with AUTOSAR")
set(CMAKE_CXX_FLAGS_INIT "${CMAKE_CXX_FLAGS_INIT} -D_AUTOSAR=4.2.2 -DAUTOSAR_COMPILATION" CACHE STRING "Initial C++ Compiler Flags with AUTOSAR")

#=======================================================================
# Build-Type Configuration
#=======================================================================
set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type (Debug, Release, etc.)")
