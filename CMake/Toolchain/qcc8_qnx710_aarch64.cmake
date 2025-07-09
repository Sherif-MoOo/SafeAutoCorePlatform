#[=======================================================================
# OpenAA: Open Source Adaptive AUTOSAR Project
# Author: Sherif Mohamed
#
# CMake Toolchain File
# --------------------
# Purpose:
#   This file configures the CMake build system to use qcc/q++ version 8.3.0 compilers
#   specifically targeting qnx710 aarch64le architectures. It also sets up various
#   build tools and utilities required during the build process, ensuring
#   compatibility and proper configuration.
#
# Key Features:
#   - Configures the qcc 8.3.0 toolchain for C and C++.
#   - Specifies tools for archiving, linking, and inspecting object files.
#   - Configures find behavior for locating libraries, includes, and programs.
#
# Target Environment:
#   - Operating System: qnx 7.10
#   - Architecture: aarch64le
#   - Compiler: qcc/q++ version 8.3.0
#==========================================================================================]

#[======================================================================
# CMake Toolchain File for QNX 7.1.0 aarch64le
#]======================================================================]

# Ensure QNX environment variables are set
if((NOT DEFINED ENV{QNX_HOST}) OR (NOT DEFINED ENV{QNX_TARGET}))
    message(FATAL_ERROR "Environment variables QNX_HOST and QNX_TARGET missing!!
$ source /path/to/qnxsdp-env.sh\n")
endif()

#==========================================================================================
# System Properties
# ------------------
# Defines the system version, name, and target processor architecture. This is critical
# for ensuring that the build system generates code optimized for the specified platform.
#==========================================================================================
set(CMAKE_SYSTEM_VERSION   7.1.0)
set(CMAKE_SYSTEM_NAME      QNX)
set(CMAKE_SYSTEM_PROCESSOR aarch64le)

#==========================================================================================
# Find Root Path Modes
# --------------------
# Controls how CMake searches for programs, libraries, and includes during the build.
# These settings are particularly useful for isolating the build environment in
# cross-compilation scenarios or controlled build setups.
#==========================================================================================
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

#==========================================================================================
# Compiler Settings
# -----------------
# Specifies the C and C++ compilers to be used. Explicitly setting the qcc version
# ensures consistency, especially in environments with multiple compiler versions.
#==========================================================================================
set(CMAKE_C_COMPILER   qcc)
set(CMAKE_CXX_COMPILER q++)
set(CMAKE_C_COMPILER_TARGET   "8.3.0,gcc_ntoaarch64le")
set(CMAKE_CXX_COMPILER_TARGET "8.3.0,gcc_ntoaarch64le_cxx")
set(CMAKE_ASM_COMPILER        ${CMAKE_C_COMPILER}            CACHE FILEPATH "" FORCE)
set(CMAKE_ASM_COMPILER_TARGET ${CMAKE_C_COMPILER_TARGET}     CACHE STRING   "" FORCE)


#==========================================================================================
# Toolchain Utilities
# -------------------
# Define paths for nm, objdump, ar, ranlib and enable LTO.
#==========================================================================================
find_program(CMAKE_NM   nm HINTS $ENV{QNX_HOST}/usr/bin)
find_program(CMAKE_OBJDUMP objdump HINTS $ENV{QNX_HOST}/usr/bin)

# Use QNX archiver and ranlib if available
find_program(CMAKE_AR    ar HINTS $ENV{QNX_HOST}/usr/bin)
find_program(CMAKE_RANLIB ranlib HINTS $ENV{QNX_HOST}/usr/bin)

# Disable response files for object files if CMake version >= 3.23
if(CMAKE_VERSION GREATER_EQUAL 3.23)
    set(CMAKE_C_USE_RESPONSE_FILE_FOR_OBJECTS FALSE)
    set(CMAKE_CXX_USE_RESPONSE_FILE_FOR_OBJECTS FALSE)
endif()

# Set the system root to QNX target
set(CMAKE_SYSROOT $ENV{QNX_TARGET})