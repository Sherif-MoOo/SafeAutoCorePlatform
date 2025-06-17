#[======================================================================]
# OpenAA: Open Source Adaptive AUTOSAR Project
# Author: Sherif Mohamed
#
# File description:
# -----------------
# Root CMake configuration file for the AdaptiveAutosarCpp17 project.
# Supports Linux and QNX aarch64 builds.
#[======================================================================]

cmake_minimum_required(VERSION 3.27)

# -------------------------------------------------------------------------------------------------
# Project Definition
# -------------------------------------------------------------------------------------------------
project(AdaptiveAutosarCpp17
    VERSION 1.0
    LANGUAGES C CXX
)

# -------------------------------------------------------------------------------------------------
# Global Options (for modular enabling/disabling)
# -------------------------------------------------------------------------------------------------
option(ENABLE_OS_LIBS       "Build OS Abstraction Libraries (ara_os_process, etc.)" ON)
option(ENABLE_EXAMPLE_APPS  "Build Example apps (demo, etc.)"                       ON)
option(ENABLE_TESTS         "Build Tests (core_platform, unit tests, etc.)"         ON)

# -------------------------------------------------------------------------------------------------
# Language Standards
# -------------------------------------------------------------------------------------------------
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# -------------------------------------------------------------------------------------------------
# Use GNUInstallDirs for cross-platform installation directories
# -------------------------------------------------------------------------------------------------
include(GNUInstallDirs)  
# Now we can use:
#   ${CMAKE_INSTALL_LIBDIR}   => lib or lib64, etc.
#   ${CMAKE_INSTALL_INCLUDEDIR} => include
#   ${CMAKE_INSTALL_BINDIR}   => bin

# -------------------------------------------------------------------------------------------------
# Logging or Additional Policies
# -------------------------------------------------------------------------------------------------
if(POLICY CMP0177)
    cmake_policy(SET CMP0177 NEW)
endif()

if(DEFINED ENABLE_PLATFORM_CONDITIONAL_EXCEPTION)
  message(STATUS "Conditional exceptions flag is enabled: ${ENABLE_PLATFORM_CONDITIONAL_EXCEPTION}")
  # Add it as a compile definition to all targets:
  add_compile_definitions(ENABLE_PLATFORM_CONDITIONAL_EXCEPTION)
endif()


# -------------------------------------------------------------------------------------------------
# Subdirectories for main libraries/components
# -------------------------------------------------------------------------------------------------

# 1) The "core" library (header-only): e.g., ara::core with array.h, etc.
add_subdirectory(components/open-aa-std-adaptive-autosar-libs)

# 2) The "OS Abstraction" libraries, only if enabled
if(ENABLE_OS_LIBS)
    add_subdirectory(components/open-aa-platform-os-abstraction-libs)
endif()

# 3) Examples, only if enabled
if(ENABLE_EXAMPLE_APPS)
    add_subdirectory(components/open-aa-example-apps)
endif()

# 4) Tests, only if enabled
if(ENABLE_TESTS)
    add_subdirectory(open-aa-tests/core_platform)
endif()

# -------------------------------------------------------------------------------------------------
# Optional: custom log config or toolchain logging
# -------------------------------------------------------------------------------------------------
include("${CMAKE_CURRENT_LIST_DIR}/CMake/Toolchain/CMakeLogging/tool_chain_log_config.cmake" OPTIONAL)

# End of root CMakeLists.txt
