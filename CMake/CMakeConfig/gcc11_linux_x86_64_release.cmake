#[=======================================================================
# OpenAA: Open Source Adaptive AUTOSAR Project
# Author: Sherif Mohamed
#
# This software is copyright protected and proprietary to Your
# Company. You are granted only those rights as set out in the
# license conditions. All other rights remain with Your Company.
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
message(STATUS "Using gcc11_linux_x86_64_release.cmake for initial cache setup.")

set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries")

#=======================================================================
# Enable modern link-time optimization (LTO) via CMake.
# This (available in CMake 3.9+) tells CMake to propagate the proper LTO
# flags to both the compiler and linker. (It complements the manually added -flto.)
#=======================================================================
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE CACHE BOOL "Enable Link Time Optimization (LTO)")

#=======================================================================
# Compiler Configuration for C
#=======================================================================
#
# These flags are used by the C compiler during respective build types.
# Adjust them according to your project's requirements and target environment.
#
# Flags:
#   -Wall, -Wextra: Enable most compiler warnings.
#   -Wconversion: Warn for implicit type conversions that may alter a value.
#   -pedantic: Enforce strict ISO compliance.
#   -Wshadow: Warn when a variable shadows another variable.
#
# Added Flags:
#   -Werror: Treat all warnings as errors.
#   -Wstrict-overflow=5: Warn about cases where the compiler assumes that signed overflow does not occur.
#   -Wmissing-prototypes: Warn if a global function is defined without a previous prototype declaration.
#   -Wstrict-aliasing=2: Enforce strict aliasing rules.
#   -Wundef: Warn if an undefined identifier is evaluated in an `#if` directive.
#   -Wredundant-decls: Warn about redundant declarations.
#   -Wcast-align: Warn about potentially unsafe alignment casts.
#   -Wformat=2: Check printf/scanf format strings.
#   -Wfloat-equal: Warn if floating-point values are used in equality comparisons.
#   -fno-common: Prevent multiple definitions.
#   -march=native: Optimize for the local machine architecture.
#   -flto: Enable Link Time Optimization.
#   -fstack-protector-strong: Enable stack protection.
#   -D_FORTIFY_SOURCE=2: Enable additional compile-time and run-time checks for buffer overflows.
#=======================================================================
set(CMAKE_C_FLAGS_INIT "-Wall -Wextra -Wconversion -pedantic -Wshadow \
-Werror -Wstrict-overflow=5 -Wmissing-prototypes \
-Wstrict-aliasing=2 -Wundef -Wredundant-decls \
-Wcast-align -Wformat=2 -Wfloat-equal \
-fno-common -march=native -flto -fstack-protector-strong \
-D_FORTIFY_SOURCE=2" CACHE STRING "Initial C Compiler Flags")

#-----------------------------------------------------------------------
# Build-Type Specific C Flags
#-----------------------------------------------------------------------
# Flags for Debug build: No optimization, include debug symbols.
set(CMAKE_C_FLAGS_DEBUG_INIT "-O0 -g" CACHE STRING "C Compiler Flags for Debug")

# Flags for MinSizeRel build: Optimize for size, define NDEBUG.
set(CMAKE_C_FLAGS_MINSIZEREL_INIT "-Os -DNDEBUG" CACHE STRING "C Compiler Flags for MinSizeRel")

# Flags for Release build: Optimize for speed, define NDEBUG.
# Additional production flags are added:
#  -ffunction-sections and -fdata-sections: Allow removal of unused sections.
#  -fno-semantic-interposition: Enable more aggressive inlining and optimization.
#  -funroll-loops: Unroll loops where beneficial.
set(CMAKE_C_FLAGS_RELEASE_INIT "-O3 -DNDEBUG -ffunction-sections -fdata-sections -fno-semantic-interposition -funroll-loops" CACHE STRING "C Compiler Flags for Release")

# Flags for RelWithDebInfo build: Optimize with debug symbols.
set(CMAKE_C_FLAGS_RELWITHDEBINFO_INIT "-O2 -g -DNDEBUG" CACHE STRING "C Compiler Flags for RelWithDebInfo")

# Additional sanitizer flags for special build types.
set(CMAKE_C_FLAGS_ALSAN_INIT "-fsanitize=address -fno-omit-frame-pointer" CACHE STRING "C Compiler Flags for Address Sanitizer")
set(CMAKE_C_FLAGS_TSAN_INIT "-fsanitize=thread" CACHE STRING "C Compiler Flags for Thread Sanitizer")
set(CMAKE_C_FLAGS_UBSAN_INIT "-fsanitize=undefined" CACHE STRING "C Compiler Flags for Undefined Behavior Sanitizer")
set(CMAKE_C_FLAGS_RELEASEWITHO2_INIT "-O2 -DNDEBUG" CACHE STRING "C Compiler Flags for ReleaseWithO2")

#=======================================================================
# Compiler Configuration for C++
#=======================================================================
#
# These flags are used by the C++ compiler during respective build types.
# Adjust them according to your project's requirements and target environment.
#
# Flags:
#   -Wall, -Wextra: Enable most compiler warnings.
#   -Wnon-virtual-dtor: Warn if a class with virtual functions has a non-virtual destructor.
#   -Wconversion: Warn for implicit type conversions that may alter a value.
#   -Wold-style-cast: Warn about C-style casts.
#   -pedantic: Enforce strict ISO compliance.
#   -Wshadow: Warn when a variable shadows another variable.
#  : Do not treat deprecated declarations as errors.
#
# Added Flags:
#   -Werror: Treat all warnings as errors.
#   -Wstrict-overflow=5: Warn about cases where the compiler assumes that signed overflow does not occur.
#   -Wstrict-aliasing=2: Enforce strict aliasing rules.
#   -Wundef: Warn if an undefined identifier is evaluated in an `#if` directive.
#   -Wredundant-decls: Warn about redundant declarations.
#   -Wcast-align: Warn about potentially unsafe alignment casts.
#   -Wformat=2: Check printf/scanf format strings.
#   -Wfloat-equal: Warn if floating-point values are used in equality comparisons.
#   -fno-exceptions: Disable C++ exception handling.
#   -fno-rtti: Disable Run-Time Type Information.
#   -fno-common: Prevent multiple definitions.
#   -march=native: Optimize for the local machine architecture.
#   -flto: Enable Link Time Optimization.
#   -fstack-protector-strong: Enable stack protection.
#   -D_FORTIFY_SOURCE=2: Enable additional compile-time and run-time checks for buffer overflows.
#=======================================================================
set(CMAKE_CXX_FLAGS_INIT "-Wall -Wextra -Wnon-virtual-dtor -Wconversion -Wold-style-cast \
-pedantic -Wshadow \
-Werror -Wstrict-overflow=5 \
-Wstrict-aliasing=2 -Wundef -Wredundant-decls \
-Wcast-align -Wformat=2 -Wfloat-equal \
-fno-exceptions -fno-rtti -fno-common -march=native -flto -fstack-protector-strong \
-D_FORTIFY_SOURCE=2" CACHE STRING "Initial C++ Compiler Flags")

#-----------------------------------------------------------------------
# Build-Type Specific C++ Flags
#-----------------------------------------------------------------------
# Flags for Debug build: No optimization, include debug symbols.
set(CMAKE_CXX_FLAGS_DEBUG_INIT "-O0 -g" CACHE STRING "C++ Compiler Flags for Debug")

# Flags for MinSizeRel build: Optimize for size, define NDEBUG.
set(CMAKE_CXX_FLAGS_MINSIZEREL_INIT "-Os -DNDEBUG" CACHE STRING "C++ Compiler Flags for MinSizeRel")

# Flags for Release build: Optimize for speed, define NDEBUG.
# Additional production flags are added:
#  -ffunction-sections and -fdata-sections
#  -fno-semantic-interposition
#  -funroll-loops
set(CMAKE_CXX_FLAGS_RELEASE_INIT "-O3 -DNDEBUG -ffunction-sections -fdata-sections -fno-semantic-interposition -funroll-loops" CACHE STRING "C++ Compiler Flags for Release")

# Flags for RelWithDebInfo build: Optimize with debug symbols.
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO_INIT "-O2 -g -DNDEBUG" CACHE STRING "C++ Compiler Flags for RelWithDebInfo")

# Additional sanitizer flags for special build types.
set(CMAKE_CXX_FLAGS_ALSAN_INIT "-fsanitize=address -fno-omit-frame-pointer" CACHE STRING "C++ Compiler Flags for Address Sanitizer")
set(CMAKE_CXX_FLAGS_TSAN_INIT "-fsanitize=thread" CACHE STRING "C++ Compiler Flags for Thread Sanitizer")
set(CMAKE_CXX_FLAGS_UBSAN_INIT "-fsanitize=undefined" CACHE STRING "C++ Compiler Flags for Undefined Behavior Sanitizer")
set(CMAKE_CXX_FLAGS_RELEASEWITHO2_INIT "-O2 -DNDEBUG" CACHE STRING "C++ Compiler Flags for ReleaseWithO2")

#=======================================================================
# Linker Flags Configuration
#=======================================================================
#
# These flags are used by the linker during respective build types.
# Adjust them based on your project's requirements and target environment.
# For production builds, we now add -flto and instruct the linker to remove
# unused sections with -Wl,--gc-sections.
#=======================================================================

#=======================================================================
# Executable Linker Flags
#=======================================================================
#
# Initial flags for the executable linker applicable to all build types.
#=======================================================================
set(CMAKE_EXE_LINKER_FLAGS_INIT "" CACHE STRING "Initial Executable Linker Flags")

#-----------------------------------------------------------------------
# Build-Type Specific Executable Linker Flags
#-----------------------------------------------------------------------
# Flags for Debug build: Include debug symbols.
set(CMAKE_EXE_LINKER_FLAGS_DEBUG_INIT "-g" CACHE STRING "Executable Linker Flags for Debug")

# Flags for MinSizeRel build: Strip symbols to reduce size.
set(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL_INIT "-s" CACHE STRING "Executable Linker Flags for MinSizeRel")

# Flags for Release build: Enable LTO and garbage collect unused sections.
set(CMAKE_EXE_LINKER_FLAGS_RELEASE_INIT "-flto -Wl,--gc-sections" CACHE STRING "Executable Linker Flags for Release")

# Flags for RelWithDebInfo build: Include debug symbols.
set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO_INIT "-g" CACHE STRING "Executable Linker Flags for RelWithDebInfo")

# Flags for ReleaseWithO2 build: No additional flags.
set(CMAKE_EXE_LINKER_FLAGS_RELEASEWITHO2_INIT "" CACHE STRING "Executable Linker Flags for ReleaseWithO2")

#=======================================================================
# Module Linker Flags
#=======================================================================
#
# Initial flags for the module linker applicable to all build types.
#=======================================================================
set(CMAKE_MODULE_LINKER_FLAGS_INIT "" CACHE STRING "Initial Module Linker Flags")

#-----------------------------------------------------------------------
# Build-Type Specific Module Linker Flags
#-----------------------------------------------------------------------
# Flags for Debug build: No additional flags.
set(CMAKE_MODULE_LINKER_FLAGS_DEBUG_INIT "" CACHE STRING "Module Linker Flags for Debug")

# Flags for MinSizeRel build: No additional flags.
set(CMAKE_MODULE_LINKER_FLAGS_MINSIZEREL_INIT "" CACHE STRING "Module Linker Flags for MinSizeRel")

# Flags for Release build: Enable LTO and section garbage collection.
set(CMAKE_MODULE_LINKER_FLAGS_RELEASE_INIT "-flto -Wl,--gc-sections" CACHE STRING "Module Linker Flags for Release")

# Flags for RelWithDebInfo build: No additional flags.
set(CMAKE_MODULE_LINKER_FLAGS_RELWITHDEBINFO_INIT "" CACHE STRING "Module Linker Flags for RelWithDebInfo")

# Flags for ReleaseWithO2 build: No additional flags.
set(CMAKE_MODULE_LINKER_FLAGS_RELEASEWITHO2_INIT "" CACHE STRING "Module Linker Flags for ReleaseWithO2")

#=======================================================================
# Shared Library Linker Flags
#=======================================================================
#
# Initial flags for the shared library linker applicable to all build types.
#=======================================================================
set(CMAKE_SHARED_LINKER_FLAGS_INIT "" CACHE STRING "Initial Shared Library Linker Flags")

#-----------------------------------------------------------------------
# Build-Type Specific Shared Library Linker Flags
#-----------------------------------------------------------------------
# Flags for Debug build: No additional flags.
set(CMAKE_SHARED_LINKER_FLAGS_DEBUG_INIT "" CACHE STRING "Shared Library Linker Flags for Debug")

# Flags for MinSizeRel build: No additional flags.
set(CMAKE_SHARED_LINKER_FLAGS_MINSIZEREL_INIT "" CACHE STRING "Shared Library Linker Flags for MinSizeRel")

# Flags for Release build: Enable LTO and section garbage collection.
set(CMAKE_SHARED_LINKER_FLAGS_RELEASE_INIT "-flto -Wl,--gc-sections" CACHE STRING "Shared Library Linker Flags for Release")

# Flags for RelWithDebInfo build: No additional flags.
set(CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO_INIT "" CACHE STRING "Shared Library Linker Flags for RelWithDebInfo")

# Flags for ReleaseWithO2 build: No additional flags.
set(CMAKE_SHARED_LINKER_FLAGS_RELEASEWITHO2_INIT "" CACHE STRING "Shared Library Linker Flags for ReleaseWithO2")

#=======================================================================
# Static Library Linker Flags
#=======================================================================
#
# Initial flags for the static library linker applicable to all build types.
# For static libraries, we do not need linker flags like -Wl,--gc-sections.
#=======================================================================
set(CMAKE_STATIC_LINKER_FLAGS_INIT "" CACHE STRING "Initial Static Library Linker Flags")

#-----------------------------------------------------------------------
# Build-Type Specific Static Library Linker Flags
#-----------------------------------------------------------------------
# Flags for Debug build: No additional flags.
set(CMAKE_STATIC_LINKER_FLAGS_DEBUG_INIT "" CACHE STRING "Static Library Linker Flags for Debug")

# Flags for MinSizeRel build: No additional flags.
set(CMAKE_STATIC_LINKER_FLAGS_MINSIZEREL_INIT "" CACHE STRING "Static Library Linker Flags for MinSizeRel")

# Flags for Release build: Remove linker flags not applicable to static libraries.
set(CMAKE_STATIC_LINKER_FLAGS_RELEASE_INIT "-flto" CACHE STRING "Static Library Linker Flags for Release")

# Flags for RelWithDebInfo build: No additional flags.
set(CMAKE_STATIC_LINKER_FLAGS_RELWITHDEBINFO_INIT "" CACHE STRING "Static Library Linker Flags for RelWithDebInfo")

# Flags for ReleaseWithO2 build: No additional flags.
set(CMAKE_STATIC_LINKER_FLAGS_RELEASEWITHO2_INIT "" CACHE STRING "Static Library Linker Flags for ReleaseWithO2")

#=======================================================================
# AUTOSAR Specific Compiler Flags
#=======================================================================
#
# AUTOSAR provides specific guidelines and macros. Depending on the GCC
# compiler support, you may need to define additional macros or flags.
#
# Example:
#   -D_AUTOSAR=4.2.2: Define AUTOSAR version.
#   -DAUTOSAR_COMPILATION: Enable AUTOSAR-specific compilation paths.
#=======================================================================
set(CMAKE_C_FLAGS_INIT "${CMAKE_C_FLAGS_INIT} -D_AUTOSAR=4.2.2 -DAUTOSAR_COMPILATION" CACHE STRING "Initial C Compiler Flags with AUTOSAR")
set(CMAKE_CXX_FLAGS_INIT "${CMAKE_CXX_FLAGS_INIT} -D_AUTOSAR=4.2.2 -DAUTOSAR_COMPILATION" CACHE STRING "Initial C++ Compiler Flags with AUTOSAR")

#=======================================================================
# Additional CMake Settings
#=======================================================================
#
# Add any additional CMake variables and configurations below as needed.
# This section is reserved for any project-specific settings that do not
# fit into the categories above.
#
# Example:
# set(SOME_PROJECT_VARIABLE "value" CACHE STRING "Description of the variable")
#
#=======================================================================
