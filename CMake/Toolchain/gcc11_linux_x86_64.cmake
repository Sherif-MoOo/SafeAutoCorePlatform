#[=======================================================================
# OpenAA: Open Source Adaptive AUTOSAR Project
# Author: Sherif Mohamed
#
# CMake Toolchain File
# --------------------
# Purpose:
#   This file configures the CMake build system to use GCC/G++ version 11 compilers
#   specifically targeting Linux x86_64 architectures. It also sets up various
#   build tools and utilities required during the build process, ensuring
#   compatibility and proper configuration.
#
# Key Features:
#   - Configures the GCC 11 toolchain for C and C++.
#   - Specifies tools for archiving, linking, and inspecting object files.
#   - Configures `find` behavior for locating libraries, includes, and programs.
#
# Target Environment:
#   - Operating System: Linux
#   - Architecture: x86_64
#   - Compiler: GCC/G++ version 11
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
# Specifies the C and C++ compilers to be used. Explicitly setting the GCC version
# ensures consistency, especially in environments with multiple compiler versions.
#==========================================================================================
set(CMAKE_C_COMPILER   gcc-11)       # C compiler (GCC version 11)
set(CMAKE_CXX_COMPILER g++-11)       # C++ compiler (G++ version 11)

#==========================================================================================
# Archiver and Ranlib Settings
# ----------------------------
# Specifies the tools for creating and indexing static library archives. These tools
# are crucial for managing `.a` files and ensuring fast linking during the build process.
#==========================================================================================
set(CMAKE_AR     ar     CACHE FILEPATH "Archiver for creating static libraries")
set(CMAKE_RANLIB ranlib CACHE FILEPATH "Ranlib for indexing static libraries")

#==========================================================================================
# Utility Tools
# -------------
# Specifies additional tools used for inspecting, copying, and dumping object files.
# These tools are helpful for debugging and analyzing compiled code.
#==========================================================================================
set(CMAKE_NM      nm      CACHE FILEPATH "Tool for listing symbols in object files")
set(CMAKE_OBJCOPY objcopy CACHE FILEPATH "Tool for copying and translating object files")
set(CMAKE_OBJDUMP objdump CACHE FILEPATH "Tool for displaying object file details")

#==========================================================================================
# GCC-Specific Tools for LTO (Link-Time Optimization)
# ---------------------------------------------------
# Specifies GCC-specific versions of archiver and ranlib. These tools are used to support
# advanced optimization techniques like Link-Time Optimization (LTO), ensuring compatibility
# with GCC's intermediate representations.
#==========================================================================================
set(CMAKE_C_COMPILER_AR       gcc-ar-11     CACHE FILEPATH "GCC-specific archiver for C")
set(CMAKE_C_COMPILER_RANLIB   gcc-ranlib-11 CACHE FILEPATH "GCC-specific ranlib for C")
set(CMAKE_CXX_COMPILER_AR     gcc-ar-11     CACHE FILEPATH "GCC-specific archiver for C++")
set(CMAKE_CXX_COMPILER_RANLIB gcc-ranlib-11 CACHE FILEPATH "GCC-specific ranlib for C++")
