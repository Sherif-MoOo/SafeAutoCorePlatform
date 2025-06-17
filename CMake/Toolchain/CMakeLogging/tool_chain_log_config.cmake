# =======================================================================================
#  OpenAA: Open Source Adaptive AUTOSAR Project
#  Author: Sherif Mohamed
#  File: tool_chain_log_config.cmake
#
#  Purpose:
#    - Centralize all logging macros (including log_decorator).
#    - Dump environment variables, CMake variables, initial flags, final flags, etc.
#    - Provide a scalable approach for logging across multiple toolchains.
#    - Includes a "Project Options" section to log all relevant options.
#    - **Added logging for Exception Safety Mode and macro definition status.**
#
#  Usage:
#    Include this file at the END of your toolchain file, for example:
#      include("tool_chain_log_config.cmake")
# =======================================================================================

if(TOOLCHAIN_LOGGING_DONE)
  # Already logged. Exit without redefinition or repeated logs.
  return()
endif()
set(TOOLCHAIN_LOGGING_DONE TRUE CACHE INTERNAL "" FORCE)

# ------------------------------------------------------------------------------
# 0) Optional “first-run” detection logic
# ------------------------------------------------------------------------------
if(NOT FIRST_RUN_COMPLETED AND FIRST_RUN_DETECTED)
  set(FIRST_RUN_COMPLETED TRUE CACHE INTERNAL "" FORCE)
  set(FIRST_RUN_DETECTED FALSE CACHE INTERNAL "" FORCE)
endif()
set(FIRST_RUN_DETECTED TRUE CACHE INTERNAL "" FORCE)

# ------------------------------------------------------------------------------
# 1) Toggle color and verbosity
# ------------------------------------------------------------------------------
if(NOT DEFINED ENABLE_LOG_COLORS)
  set(ENABLE_LOG_COLORS ON)
endif()

if(NOT DEFINED VERBOSE_TOOLCHAIN_LOG)
  set(VERBOSE_TOOLCHAIN_LOG OFF)
endif()

# ------------------------------------------------------------------------------
# 2) Define color-logging macros
#    - log_info, log_warn, log_error, log_debug, log_decorator
#    - We store them here so everything is "centralized" in one place.
# ------------------------------------------------------------------------------
string(ASCII 27 ESC_CHAR)

function(_give_shape color text)
  if(ENABLE_LOG_COLORS)
    message(STATUS "${ESC_CHAR}[1;${color}m${text}${ESC_CHAR}[0m")
  else()
    message(STATUS "${text}")
  endif()
endfunction()

macro(log_info msg)
  # White text
  _give_shape("37" "[INFO] ${msg}")
endmacro()

macro(log_warn msg)
  # Yellow text
  _give_shape("33" "[WARN] ${msg}")
endmacro()

macro(log_error msg)
  # Red text
  _give_shape("31" "[ERROR] ${msg}")
endmacro()

macro(log_debug msg)
  # Cyan text
  if(VERBOSE_TOOLCHAIN_LOG)
    _give_shape("36" "[DEBUG] ${msg}")
  endif()
endmacro()

macro(log_decorator msg)
  # Blue text
  _give_shape("34" "${msg}")
endmacro()

# ------------------------------------------------------------------------------
# 3) Distinguish QNX vs. Linux vs. other system
# ------------------------------------------------------------------------------
if(CMAKE_SYSTEM_NAME STREQUAL "QNX")
  log_decorator("====== Target-compiling for QNX ==============================")

  if(DEFINED ENV{QNX_HOST})
    log_info("QNX_HOST            = $ENV{QNX_HOST}")
  endif()
  if(DEFINED ENV{QNX_TARGET})
    log_info("QNX_TARGET          = $ENV{QNX_TARGET}")
  endif()
  if(DEFINED ENV{QNX_CONFIGURATION})
    log_info("QNX_CONFIGURATION   = $ENV{QNX_CONFIGURATION}")
  endif()

  if(CMAKE_C_COMPILER_TARGET)
    log_info("C_COMPILER_TARGET   = ${CMAKE_C_COMPILER_TARGET}")
  endif()
  if(CMAKE_CXX_COMPILER_TARGET)
    log_info("CXX_COMPILER_TARGET = ${CMAKE_CXX_COMPILER_TARGET}")
  endif()

else()
  log_decorator("====== Target-compiling for LINUX/OTHER ======================")

  if(CMAKE_SYSTEM_NAME)
    log_info("PLATFORM_TARGET     = ${CMAKE_SYSTEM_NAME}")
  endif()
  if(CMAKE_SYSTEM_PROCESSOR)
    log_info("Target arch         = ${CMAKE_SYSTEM_PROCESSOR}")
  endif()
endif()

# ------------------------------------------------------------------------------
# 4) General CMake info
# ------------------------------------------------------------------------------
log_decorator("================ GENERAL CONFIG =============================")
log_info("CMAKE_TOOLCHAIN_FILE  : ${CMAKE_TOOLCHAIN_FILE}")
log_info("CMAKE_VERBOSE_MAKEFILE: ${CMAKE_VERBOSE_MAKEFILE}")
log_decorator("-------------------------------------------------------------")
log_info("CMAKE_VERSION         : ${CMAKE_VERSION}")
log_info("CMAKE_BUILD_TYPE      : ${CMAKE_BUILD_TYPE}")
log_info("CMAKE_SYSTEM_NAME     : ${CMAKE_SYSTEM_NAME}")

# ------------------------------------------------------------------------------
# 4a) Fallback logic for compiler versions (if empty)
# ------------------------------------------------------------------------------
if(NOT DEFINED CMAKE_C_COMPILER_VERSION OR "${CMAKE_C_COMPILER_VERSION}" STREQUAL "")
  execute_process(
    COMMAND "${CMAKE_C_COMPILER}" --version
    OUTPUT_VARIABLE _c_compiler_out
    ERROR_VARIABLE  _c_compiler_err
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_STRIP_TRAILING_WHITESPACE
  )
  string(REGEX REPLACE "\n.*" "" _c_compiler_line "${_c_compiler_out}")
  if(_c_compiler_line STREQUAL "" AND NOT _c_compiler_err STREQUAL "")
    set(CMAKE_C_COMPILER_VERSION "(unknown, --version leads to error)")
  else()
    set(CMAKE_C_COMPILER_VERSION "${_c_compiler_line}")
  endif()
endif()

if(NOT DEFINED CMAKE_CXX_COMPILER_VERSION OR "${CMAKE_CXX_COMPILER_VERSION}" STREQUAL "")
  execute_process(
    COMMAND "${CMAKE_CXX_COMPILER}" --version
    OUTPUT_VARIABLE _cxx_compiler_out
    ERROR_VARIABLE  _cxx_compiler_err
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_STRIP_TRAILING_WHITESPACE
  )
  string(REGEX REPLACE "\n.*" "" _cxx_compiler_line "${_cxx_compiler_out}")
  if(_cxx_compiler_line STREQUAL "" AND NOT _cxx_compiler_err STREQUAL "")
    set(CMAKE_CXX_COMPILER_VERSION "(unknown, --version leads to error)")
  else()
    set(CMAKE_CXX_COMPILER_VERSION "${_cxx_compiler_line}")
  endif()
endif()

if(CMAKE_C_COMPILER)
  log_info("C Compiler used       : ${CMAKE_C_COMPILER}")
endif()
if(CMAKE_C_COMPILER_VERSION)
  log_info("C Compiler version    : ${CMAKE_C_COMPILER_VERSION}")
endif()

if(CMAKE_CXX_COMPILER)
  log_info("CXX Compiler used     : ${CMAKE_CXX_COMPILER}")
endif()
if(CMAKE_CXX_COMPILER_VERSION)
  log_info("CXX Compiler version  : ${CMAKE_CXX_COMPILER_VERSION}")
endif()

# If you want to show the standard, do so here (if they are set)
if(CMAKE_C_STANDARD)
  log_info("C Standard used       : ${CMAKE_C_STANDARD}")
endif()
if(CMAKE_CXX_STANDARD)
  log_info("CXX Standard used     : ${CMAKE_CXX_STANDARD}")
endif()

# ------------------------------------------------------------------------------
# 5) _INIT Variables from the -C file
# ------------------------------------------------------------------------------
log_decorator("====== _INIT VARIABLES FROM -C FILE (if any) ================")

macro(log_init_variable var)
  if(DEFINED ${var})
    log_info("${var}: ${${var}}")
  endif()
endmacro()

# For C Flags:
log_init_variable("CMAKE_C_FLAGS_INIT")
log_init_variable("CMAKE_C_FLAGS_DEBUG_INIT")
log_init_variable("CMAKE_C_FLAGS_RELEASE_INIT")
log_init_variable("CMAKE_C_FLAGS_MINSIZEREL_INIT")
log_init_variable("CMAKE_C_FLAGS_RELWITHDEBINFO_INIT")
log_init_variable("CMAKE_C_FLAGS_ALSAN_INIT")
log_init_variable("CMAKE_C_FLAGS_TSAN_INIT")
log_init_variable("CMAKE_C_FLAGS_UBSAN_INIT")

# For C++ Flags:
log_init_variable("CMAKE_CXX_FLAGS_INIT")
log_init_variable("CMAKE_CXX_FLAGS_DEBUG_INIT")
log_init_variable("CMAKE_CXX_FLAGS_RELEASE_INIT")
log_init_variable("CMAKE_CXX_FLAGS_MINSIZEREL_INIT")
log_init_variable("CMAKE_CXX_FLAGS_RELWITHDEBINFO_INIT")
log_init_variable("CMAKE_CXX_FLAGS_ALSAN_INIT")
log_init_variable("CMAKE_CXX_FLAGS_TSAN_INIT")
log_init_variable("CMAKE_CXX_FLAGS_UBSAN_INIT")

# For linker flags, if your -C file sets them:
log_init_variable("CMAKE_EXE_LINKER_FLAGS_INIT")
log_init_variable("CMAKE_MODULE_LINKER_FLAGS_INIT")
log_init_variable("CMAKE_SHARED_LINKER_FLAGS_INIT")
log_init_variable("CMAKE_STATIC_LINKER_FLAGS_INIT")

# ------------------------------------------------------------------------------
# 6) EFFECTIVE FLAGS
# ------------------------------------------------------------------------------
log_decorator("====== IN ACTION CMAKE FLAGS ================================")
log_info("CMAKE_C_FLAGS                   : ${CMAKE_C_FLAGS}")
log_info("CMAKE_CXX_FLAGS                 : ${CMAKE_CXX_FLAGS}")
log_decorator("-------------------------------------------------------------")
log_info("CMAKE_C_FLAGS_DEBUG             : ${CMAKE_C_FLAGS_DEBUG}")
log_info("CMAKE_C_FLAGS_RELEASE           : ${CMAKE_C_FLAGS_RELEASE}")
log_info("CMAKE_C_FLAGS_MINSIZEREL        : ${CMAKE_C_FLAGS_MINSIZEREL}")
log_info("CMAKE_C_FLAGS_RELWITHDEBINFO    : ${CMAKE_C_FLAGS_RELWITHDEBINFO}")
log_decorator("-------------------------------------------------------------")
log_info("CMAKE_CXX_FLAGS_DEBUG           : ${CMAKE_CXX_FLAGS_DEBUG}")
log_info("CMAKE_CXX_FLAGS_RELEASE         : ${CMAKE_CXX_FLAGS_RELEASE}")
log_info("CMAKE_CXX_FLAGS_MINSIZEREL      : ${CMAKE_CXX_FLAGS_MINSIZEREL}")
log_info("CMAKE_CXX_FLAGS_RELWITHDEBINFO  : ${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
log_decorator("-------------------------------------------------------------")
log_info("CMAKE_EXE_LINKER_FLAGS          : ${CMAKE_EXE_LINKER_FLAGS}")
log_info("CMAKE_MODULE_LINKER_FLAGS       : ${CMAKE_MODULE_LINKER_FLAGS}")
log_info("CMAKE_SHARED_LINKER_FLAGS       : ${CMAKE_SHARED_LINKER_FLAGS}")
log_info("CMAKE_STATIC_LINKER_FLAGS       : ${CMAKE_STATIC_LINKER_FLAGS}")
log_decorator("-------------------------------------------------------------")

# ------------------------------------------------------------------------------
# 7) PROJECT OPTIONS SECTION
# ------------------------------------------------------------------------------
log_decorator("=================== PROJECT OPTIONS =========================")

macro(log_option var)
  if(DEFINED ${var})
    log_info("option -D${var}=${${var}}")
  else()
    log_info("option -D${var}= (not defined)")
  endif()
endmacro()

# Example project options to log:
log_option("ENABLE_PROFILING")
log_option("ENABLE_STRICT")
log_option("ENABLE_SANITIZER")
log_option("ENABLE_SANITIZER_RECOVER")
log_option("ENABLE_COMPILE_TIME_TRACE")
log_option("ENABLE_SYNTAX_ONLY")
log_option("RUN_CLANG_TIDY_ENABLE")
log_option("BUILD_TESTS")
log_option("ENABLE_DOXYGEN")
log_option("ENABLE_EXCEPTIONS")
log_option("ENABLE_RTTI")
log_option("ENABLE_CCACHE")
log_option("ENABLE_SCCACHE")
log_option("DEBUG_LEVEL")
log_option("OPTIMIZATION_LEVEL")
log_option("ENABLE_SPLIT_DWARF")
log_option("BUILD_TEST2020")
log_option("VERBOSE_TOOLCHAIN_LOG")
log_option("CACHE_ALL_FLAG_VARS")
log_option("ENABLE_COLOR")
log_option("SET_LINKER")

log_decorator("-------------------------------------------------------------")

# ------------------------------------------------------------------------------
# 8) EXCEPTION SAFETY MODE LOGGING
# ------------------------------------------------------------------------------
log_decorator("================ EXCEPTION SAFETY MODE =======================")

# Log the EXCEPTION_SAFETY_MODE variable
if(DEFINED EXCEPTION_SAFETY_MODE)
  log_info("EXCEPTION_SAFETY_MODE: ${EXCEPTION_SAFETY_MODE}")
else()
  log_info("EXCEPTION_SAFETY_MODE: (not defined)")
endif()

# Check if ENABLE_PLATFORM_CONDITIONAL_EXCEPTION is defined in CMAKE_CXX_FLAGS
string(FIND "${CMAKE_CXX_FLAGS}" "-DENABLE_PLATFORM_CONDITIONAL_EXCEPTION=1" macro_pos)
if(macro_pos GREATER_EQUAL 0)
  log_info("ENABLE_PLATFORM_CONDITIONAL_EXCEPTION is defined as 1")
else()
  log_info("ENABLE_PLATFORM_CONDITIONAL_EXCEPTION is NOT defined")
endif()

log_decorator("-------------------------------------------------------------")

# ------------------------------------------------------------------------------
# 9) Optional: Dump ALL cache variables
# ------------------------------------------------------------------------------
if(VERBOSE_TOOLCHAIN_LOG)
  log_decorator("====== ALL CACHE VARIABLES (VERBOSE) ========================")
  get_cmake_property(_cacheVars CACHE_VARIABLES)
  foreach(_cv ${_cacheVars})
    get_property(_val CACHE "${_cv}" PROPERTY VALUE)
    log_debug(" [${_cv}] = ${_val}")
  endforeach()
  log_decorator("-------------------------------------------------------------")
endif()

# ------------------------------------------------------------------------------
# 10) Summary Report
# ------------------------------------------------------------------------------
log_decorator("=================== BUILD SUMMARY ===========================")

log_info("Build Type          : ${CMAKE_BUILD_TYPE}")
log_info("C Compiler          : ${CMAKE_C_COMPILER} (${CMAKE_C_COMPILER_VERSION})")
log_info("C++ Compiler        : ${CMAKE_CXX_COMPILER} (${CMAKE_CXX_COMPILER_VERSION})")
log_info("Build Shared Libs   : ${BUILD_SHARED_LIBS}")
log_info("Optimization Flags  : ${CMAKE_C_FLAGS_RELEASE} ${CMAKE_CXX_FLAGS_RELEASE}")

log_decorator("=============================================================")
