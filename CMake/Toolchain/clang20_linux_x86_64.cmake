#[=======================================================================
# OpenAA: Open Source Adaptive AUTOSAR Project
# Author: Sherif Mohamed
#
# CMake Toolchain File
# --------------------
# Purpose:
#   This file configures the CMake build system to use clang/clang++ version 20 compilers
#   specifically targeting Linux x86_64 architectures. It also sets up various
#   build tools and utilities required during the build process, ensuring
#   compatibility and proper configuration.
#
# Key Features:
#   - Configures the Clang 20 toolchain for C and C++.
#   - Specifies tools for archiving, linking, and inspecting object files.
#   - Configures `find` behavior for locating libraries, includes, and programs.
#
# Target Environment:
#   - Operating System: Linux
#   - Architecture: x86_64
#   - Compiler: Clang/Clang++ version 20
#==========================================================================================]

#==========================================================================================
# System Properties
# ------------------
# Defines the system version, name, and target processor architecture. This is critical
# for ensuring that the build system generates code optimized for the specified platform.
#==========================================================================================
set(CMAKE_SYSTEM_VERSION   1)         # Placeholder system version, adjust as needed
set(CMAKE_SYSTEM_NAME      Linux)     # Target system name
set(CMAKE_SYSTEM_PROCESSOR x86_64)    # Target processor architecture

#==========================================================================================
# Compiler Target
# -----------------
# Specifies the target architecture for the compiler. This is important for ensuring
# that the generated code is compatible with the specified architecture.
# The target architecture is set to x86_64-unknown-linux-gnu, which is a common
# target for 64-bit Linux systems.
#==========================================================================================
set(CMAKE_C_COMPILER_TARGET   x86_64-unknown-linux-gnu)
set(CMAKE_CXX_COMPILER_TARGET x86_64-unknown-linux-gnu)
set(CMAKE_ASM_COMPILER_TARGET x86_64-unknown-linux-gnu)

#==========================================================================================
# Find Root Path Modes
# --------------------
# Controls how CMake searches for programs, libraries, and includes during the build.
# These settings are particularly useful for isolating the build environment in
# cross-compilation scenarios or controlled build setups.
#==========================================================================================
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)  # Do not search programs in the root path
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)   # Search libraries only in the root path
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)   # Search includes only in the root path

#==========================================================================================
# Compiler Settings
# -----------------
# Specifies the C and C++ compilers to be used. Explicitly setting the clang version
# ensures consistency, especially in environments with multiple compiler versions.
#==========================================================================================
set(CMAKE_C_COMPILER   clang-20)       # C compiler (Clang version 20)
set(CMAKE_CXX_COMPILER clang++-20)     # C++ compiler (Clang++ version 20)
set(CMAKE_ASM_COMPILER clang-20)       # ASM compiler (Clang version 20)

#==========================================================================================
# Archiver and Ranlib Settings
# ----------------------------
# Specifies the tools for creating and indexing static library archives. These tools
# are crucial for managing `.a` files and ensuring fast linking during the build process.
#==========================================================================================
set(CMAKE_AR     llvm-ar-20     CACHE FILEPATH "Archiver for creating static libraries")
set(CMAKE_RANLIB llvm-ranlib-20 CACHE FILEPATH "Ranlib for indexing static libraries")

#==========================================================================================
# Utility Tools
# -------------
# Specifies additional tools used for inspecting, copying, and dumping object files.
# These tools are helpful for debugging and analyzing compiled code.
#==========================================================================================
set(CMAKE_NM      llvm-nm-20      CACHE FILEPATH "Tool for listing symbols in object files")
set(CMAKE_OBJCOPY llvm-objcopy-20 CACHE FILEPATH "Tool for copying and translating object files")
set(CMAKE_OBJDUMP llvm-objdump-20 CACHE FILEPATH "Tool for displaying object file details")

#==========================================================================================
# Clang-Specific Tools for LTO (Link-Time Optimization)
# ---------------------------------------------------
# Specifies Clang-specific versions of archiver and ranlib. These tools are used to support
# advanced optimization techniques like Link-Time Optimization (LTO), ensuring compatibility
# with Clang's intermediate representations.
#==========================================================================================
set(CMAKE_C_COMPILER_AR       llvm-ar-20     CACHE FILEPATH "Clang-specific archiver for C")
set(CMAKE_C_COMPILER_RANLIB   llvm-ranlib-20 CACHE FILEPATH "Clang-specific ranlib for C")
set(CMAKE_CXX_COMPILER_AR     llvm-ar-20     CACHE FILEPATH "Clang-specific archiver for C++")
set(CMAKE_CXX_COMPILER_RANLIB llvm-ranlib-20 CACHE FILEPATH "Clang-specific ranlib for C++")
