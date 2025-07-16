#[=======================================================================
# OpenAA: Open Source Adaptive AUTOSAR Project
# Author: Sherif Mohamed
#
# CMake Toolchain File
# --------------------
# Purpose:
#   Configure CMake to use qcc/q++ 8.3.0 to build *target binaries for QNX 7.1.0 aarch64le*
#   using the *host-side* cross toolchain shipped in the QNX SDP.
#
# Key Features:
#   - Selects qcc/q++ frontends with gcc_ntoaarch64le specs.
#   - Points CMake binutils (ar, ranlib, nm, etc.) to the correct QNX *host* tools.
#   - Establishes cross-compilation search behavior and sysroot.
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

# ---------------------------------------------------------------------------
# Import environment -> cache vars (MUST precede any use of QNX_HOST/QNX_TARGET)
# ---------------------------------------------------------------------------
set(QNX_HOST   "$ENV{QNX_HOST}"   CACHE PATH "QNX SDP host directory"   FORCE)
set(QNX_TARGET "$ENV{QNX_TARGET}" CACHE PATH "QNX SDP target directory" FORCE)
message(STATUS "QNX_HOST   = ${QNX_HOST}")
message(STATUS "QNX_TARGET = ${QNX_TARGET}")

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

# Use qcc front-end for ASM as well (common for QNX)
set(CMAKE_ASM_COMPILER        ${CMAKE_C_COMPILER}        CACHE FILEPATH "" FORCE)
set(CMAKE_ASM_COMPILER_TARGET ${CMAKE_C_COMPILER_TARGET} CACHE STRING   "" FORCE)

#==========================================================================================
# Toolchain Utilities (host-side cross tools that produce target objects)
# TOOL ROLES (quick ref):
#   • CMAKE_MAKE_PROGRAM : Build driver (GNU make from QNX host; optional but explicit).
#   • CMAKE_AR           : Create/update static archives (.a).
#   • CMAKE_RANLIB       : Generate archive symbol index (ranlib table) for fast linking.
#   • CMAKE_NM           : List/inspect symbols (used by tooling, diagnostics, LTO steps).
#   • CMAKE_OBJCOPY      : Copy/transform objects; strip sections; binary extraction.
#   • CMAKE_OBJDUMP      : Disassemble / dump ELF headers & reloc info (debug builds).
#   • CMAKE_LINKER       : Low-level linker (usually invoked by qcc, but exposed for edge cases).
#   • CMAKE_STRIP        : Remove symbols/debug info from target binaries when size matters.
#==========================================================================================
set(_QNX_ARCH "aarch64")
set(_QNX_BIN  "${QNX_HOST}/usr/bin")

set(CMAKE_MAKE_PROGRAM "${_QNX_BIN}/make"                        CACHE FILEPATH "QNX make Program"    FORCE)
set(CMAKE_AR           "${_QNX_BIN}/nto${_QNX_ARCH}-ar"          CACHE FILEPATH "QNX ar Program"      FORCE)
set(CMAKE_RANLIB       "${_QNX_BIN}/nto${_QNX_ARCH}-ranlib"      CACHE FILEPATH "QNX ranlib Program"  FORCE)
set(CMAKE_NM           "${_QNX_BIN}/nto${_QNX_ARCH}-nm"          CACHE FILEPATH "QNX nm Program"      FORCE)
set(CMAKE_OBJCOPY      "${_QNX_BIN}/nto${_QNX_ARCH}-objcopy"     CACHE FILEPATH "QNX objcopy Program" FORCE)
set(CMAKE_OBJDUMP      "${_QNX_BIN}/nto${_QNX_ARCH}-objdump"     CACHE FILEPATH "QNX objdump Program" FORCE)
set(CMAKE_LINKER       "${_QNX_BIN}/nto${_QNX_ARCH}-ld"          CACHE FILEPATH "QNX linker Program"  FORCE)
set(CMAKE_STRIP        "${_QNX_BIN}/nto${_QNX_ARCH}-strip"       CACHE FILEPATH "QNX strip Program"   FORCE)

#==========================================================================================
# FATAL VALIDATION OF TOOL PATHS
#==========================================================================================
set(_QNX_MISSING_TOOLS "")
foreach(_t MAKE_PROGRAM AR RANLIB NM OBJCOPY OBJDUMP LINKER STRIP)
    set(_tool_path "${CMAKE_${_t}}")
    if(NOT _tool_path)
        string(APPEND _QNX_MISSING_TOOLS "  - CMAKE_${_t} is empty (expected QNX host tool)\n")
    elseif(NOT EXISTS "${_tool_path}")
        string(APPEND _QNX_MISSING_TOOLS "  - ${_tool_path} (CMAKE_${_t}) does NOT exist.\n")
    elseif(IS_DIRECTORY "${_tool_path}")
        string(APPEND _QNX_MISSING_TOOLS "  - ${_tool_path} is a directory (CMAKE_${_t}).\n")
    endif()
endforeach()

if(_QNX_MISSING_TOOLS)
    message(FATAL_ERROR
        "QNX toolchain verification failed.\n"
        "Missing/invalid host-side cross tools:\n"
        "${_QNX_MISSING_TOOLS}\n"
        "Environment:\n"
        "  QNX_HOST   = ${QNX_HOST}\n"
        "  QNX_TARGET = ${QNX_TARGET}\n"
        "Arch prefix: _QNX_ARCH='${_QNX_ARCH}'\n"
        "Did you source the correct qnxsdp-env.sh? Wrong arch?\n"
    )
endif()
unset(_QNX_MISSING_TOOLS)

#==========================================================================================
# Response file suppression (optional)
#==========================================================================================
if(CMAKE_VERSION GREATER_EQUAL 3.23)
    set(CMAKE_C_USE_RESPONSE_FILE_FOR_OBJECTS FALSE)
    set(CMAKE_CXX_USE_RESPONSE_FILE_FOR_OBJECTS FALSE)
endif()

#==========================================================================================
# Sysroot
#==========================================================================================
set(CMAKE_SYSROOT "${QNX_TARGET}")