#[=======================================================================
# OpenAA: Open Source Adaptive AUTOSAR Project
# Author: Sherif Mohamed
#
# File description:
# -----------------
# CMake initial-cache file for OpenAA - QNX 7.10 x86_64 using QCC-8.
# This file sets essential CMake variables and compiler/linker flags
# to streamline the build process.
#=======================================================================]

#[=======================================================================[
.rst:
QNX710_x86_64_QCC8
---------------------
CMake initial cache file for QNX 7.1.0 x86_64 using QCC-8.

All variables can be set as initial cache variables and passed as a file to CMake:

.. code-block:: cmake

    # Create an initial cache file (qcc8_qnx710_x86_64.cmake) and define in there:
    set(CMAKE_PREFIX_PATH "/opt/qnx710/host/linux/x86_64/usr" CACHE STRING "")
    set(CMAKE_TOOLCHAIN_FILE "/opt/toolchain/qcc8_qnx710_x86_64.cmake" CACHE PATH "")

.. code-block:: shell-session

    # QNX dev kit can only be used with bash!
    $ bash -i
    $ source /opt/qnx710/qnxsdp-env.sh
    $ cmake -C CMake/CMakeConfig/qcc8_qnx710_x86_64.cmake -S <project-root> -B <build-dir>

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
message(STATUS "Using qcc8_qnx710_x86_64_release.cmake for initial cache setup.")

set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries")

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
#   -D_QNX_SOURCE: Define _QNX_SOURCE macro for QNX-specific features.
#
# Added Flags:
#   -Werror: Treat warnings as errors.
#   -Wstrict-overflow=5: Warn about optimizations that assume no overflow.
#   -Wmissing-prototypes: Warn if a function is defined without a prototype.
#   -Wstrict-aliasing=2: Enable strict aliasing rules.
#   -Wundef: Warn if an undefined macro is used.
#   -Wredundant-decls: Warn about redundant declarations.
#   -Wcast-align: Warn if a pointer cast might result in misaligned access.
#   -Wformat=2: Enable format string checks.
#   -Wfloat-equal: Warn if floating-point values are compared for equality.
#   -fno-common: Prevent common symbols from being created.
#   -march=x86-64: Generate code for x86-64 architecture.
#   -mtune=generic: Optimize for generic x86-64 processors.
#   -fpic, -fpie: Generate position-independent code for shared libraries.
#   -fstack-protector-strong: Enable stack protection to prevent buffer overflows
#   -D_FORTIFY_SOURCE=2: Enable additional compile-time checks for buffer overflows.
#   -ftrapv: Trap on signed integer overflow.
#   -frecord-gcc-switches: Record GCC command-line switches in the binary.
#   -flto: Enable Link Time Optimization (LTO) for better performance.
#=======================================================================
set(CMAKE_C_FLAGS_INIT
    "-Wall -Wextra -Wconversion -pedantic -Wshadow -D_QNX_SOURCE \
     -Werror -Wstrict-overflow=5 -Wmissing-prototypes \
     -Wstrict-aliasing=2 -Wundef -Wredundant-decls \
     -Wcast-align -Wformat=2 -Wfloat-equal \
     -fno-common \
     -march=x86-64 -mtune=generic \
     -fpic -fpie \
     -fstack-protector-strong \
     -D_FORTIFY_SOURCE=2 \
     -ftrapv -frecord-gcc-switches \
     -flto"
  CACHE STRING "Initial C Compiler Flags with LTO and hardening")

#-----------------------------------------------------------------------
# Build-Type Specific C Flags
#-----------------------------------------------------------------------
# Flags for Debug build: No optimization, include debug symbols.
set(CMAKE_C_FLAGS_DEBUG_INIT "-O0 -g" CACHE STRING "C Compiler Flags for Debug")

# Flags for MinSizeRel build: Optimize for size, define NDEBUG.
set(CMAKE_C_FLAGS_MINSIZEREL_INIT "-Os -DNDEBUG" CACHE STRING "C Compiler Flags for MinSizeRel")

# Flags for Release build: Optimize for speed, define NDEBUG and integrate enhanced options.
set(CMAKE_C_FLAGS_RELEASE_INIT
    "-O3 -DNDEBUG \
     -ffunction-sections -fdata-sections \
     -flto"
  CACHE STRING "C Compiler Flags for Release with LTO")

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
# Mirror C flags and add C++-specific hardening and LTO.
#=======================================================================
set(CMAKE_CXX_FLAGS_INIT
    "-Wall -Wextra -Wnon-virtual-dtor -Wconversion -Wold-style-cast \
     -pedantic -Wshadow -D_QNX_SOURCE \
     -Werror -Wstrict-overflow=5 -Wstrict-aliasing=2 -Wundef -Wredundant-decls \
     -Wcast-align -Wformat=2 -Wfloat-equal \
     -fno-exceptions -fno-rtti -fno-common \
     -march=x86-64 -mtune=generic \
     -fpic -fpie \
     -fstack-protector-strong \
     -D_FORTIFY_SOURCE=2 \
     -ftrapv -fno-record-gcc-switches \
     -flto"
  CACHE STRING "Initial C++ Compiler Flags with LTO and hardening")

#-----------------------------------------------------------------------
# Build-Type Specific C++ Flags
#-----------------------------------------------------------------------
# Flags for Debug build: No optimization, include debug symbols.
set(CMAKE_CXX_FLAGS_DEBUG_INIT "-O0 -g" CACHE STRING "C++ Compiler Flags for Debug")

# Flags for MinSizeRel build: Optimize for size, define NDEBUG.
set(CMAKE_CXX_FLAGS_MINSIZEREL_INIT "-Os -DNDEBUG" CACHE STRING "C++ Compiler Flags for MinSizeRel")

# Flags for Release build: Optimize for speed, define NDEBUG and integrate enhanced options.
set(CMAKE_CXX_FLAGS_RELEASE_INIT
    "-O3 -DNDEBUG \
     -ffunction-sections -fdata-sections \
     -flto"
  CACHE STRING "C++ Compiler Flags for Release with LTO")

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
#=======================================================================

#-----------------------------------------------------------------------
# Executable Linker Flags
#-----------------------------------------------------------------------
# Initial flags for the executable linker applicable to all build types.
set(CMAKE_EXE_LINKER_FLAGS_INIT "-flto" CACHE STRING "Initial Executable Linker Flags with LTO")

#-----------------------------------------------------------------------
# Build-Type Specific Executable Linker Flags
#-----------------------------------------------------------------------
# Flags for Debug build: Include debug symbols.
set(CMAKE_EXE_LINKER_FLAGS_DEBUG_INIT "-g" CACHE STRING "Executable Linker Flags for Debug")

# Flags for MinSizeRel build: Strip symbols to reduce size.
set(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL_INIT "-s" CACHE STRING "Executable Linker Flags for MinSizeRel")

# Flags for Release build: Optimize binary size with garbage collection, symbol stripping, full RELRO, immediate binding.
set(CMAKE_EXE_LINKER_FLAGS_RELEASE_INIT
    "-flto -Wl,-z,relro -Wl,-z,now -Wl,-z,defs \
     -Wl,--gc-sections -s"
  CACHE STRING "Executable Linker Flags for Release with LTO")

# Flags for RelWithDebInfo build: Include debug symbols.
set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO_INIT "-g" CACHE STRING "Executable Linker Flags for RelWithDebInfo")

# Flags for ReleaseWithO2 build: No additional flags.
set(CMAKE_EXE_LINKER_FLAGS_RELEASEWITHO2_INIT "" CACHE STRING "Executable Linker Flags for ReleaseWithO2")

#-----------------------------------------------------------------------
# Shared Library Linker Flags
#-----------------------------------------------------------------------
set(CMAKE_SHARED_LINKER_FLAGS_INIT "-flto" CACHE STRING "Initial Shared Library Linker Flags with LTO")

# Flags for Release build: Optimize binary size with garbage collection.
set(CMAKE_SHARED_LINKER_FLAGS_RELEASE_INIT "-flto -Wl,--gc-sections" CACHE STRING "Shared Library Linker Flags for Release with LTO")

#-----------------------------------------------------------------------
# Static Library Linker Flags
#-----------------------------------------------------------------------
set(CMAKE_STATIC_LINKER_FLAGS_INIT "-flto" CACHE STRING "Initial Static Library Linker Flags with LTO")

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