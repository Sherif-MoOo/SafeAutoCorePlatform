#!/bin/bash
#
# [=========================================================================]
#  OpenAA: Open Source Adaptive AUTOSAR Project
#  Author: Sherif Mohamed
#
#  File description:
#  -----------------
#  Build script for AdaptiveAutosarCpp17, with error handling,
#  standardized structure, and detailed commentary.
#
#  Integrations:
#  - CMakePresets.json for build presets.
#  - Configuration file (-C option) for initial cache variables.
#  - Logging utility for better traceability.
#  - Modular, functional programming style for maintainability.
#
#  Usage:
#    ./build.sh [options]
#    e.g.: ./build.sh --clean --build-type Release --build-target qcc12_qnx800_aarch64 --exception-safety safe
# [=========================================================================]

# ------------------------------------------------------------------------------
# 1) Bash Settings for Safety
# ------------------------------------------------------------------------------
set -e          # Exit immediately if any command returns non-zero status.
set -o pipefail # If any command in a pipeline fails, the entire pipeline fails.

# ------------------------------------------------------------------------------
# 2) Default Build Options
# ------------------------------------------------------------------------------
BUILD_TYPE="Release"                   # Default build type
BUILD_TARGET="gcc11_linux_x86_64"      # Default build target
NUM_JOBS=$(nproc)                      # Default to number of CPU cores
CLEAN_BUILD=false                      # Default: do not clean
SDP_PATH=""                            # Default: no QNX SDP path
PRESET_NAME=""                         # Computed build preset name
CONFIG_FILE=""                         # Computed config file path

# -----------------------------------------------------------------------------------
# 2.1) Exception Safety Options [SWS_CORE_ARRAY_BUILD_OPTIONS]
# -----------------------------------------------------------------------------------
EXCEPTION_SAFETY_MODE="conditional"    # Default: conditional exception safety

# ------------------------------------------------------------------------------
# 3) Logging Utility
# ------------------------------------------------------------------------------
log() {
    local level=$1
    local message=$2
    local timestamp=$(date "+%Y-%m-%d %H:%M:%S")
    case $level in
        INFO) echo -e "\033[37m[INFO] [$timestamp] $message\033[0m" ;;
        WARN) echo -e "\033[33m[WARN] [$timestamp] $message\033[0m" ;;
        ERROR) echo -e "\033[31m[ERROR] [$timestamp] $message\033[0m" ;;
        *) echo "[UNKNOWN] [$timestamp] $message" ;;
    esac
}

# ------------------------------------------------------------------------------
# 4) Error Handling with run_command
# ------------------------------------------------------------------------------
# This function runs a given command (as an array), logs it, and checks status.
# If the command fails (non-zero exit), we log an error and exit.
# ------------------------------------------------------------------------------
run_command() {
  local cmd=("$@") # capture all arguments as an array
  log INFO "Running command: ${cmd[*]}"
  
  # Use bash 'command' builtin to run safely
  "${cmd[@]}"
  local status=$?
  
  if [ $status -ne 0 ]; then
    log ERROR "Command failed with exit code $status: ${cmd[*]}"
    exit $status
  fi
}

# ------------------------------------------------------------------------------
# 5) Usage / Help
# ------------------------------------------------------------------------------
usage() {
  log INFO "Usage: $0 [OPTIONS]"
  echo ""
  echo "Options:"
  echo "  -h, --help                          Show this help message and exit"
  echo "  -c, --clean                         Perform a clean build by removing build and install directories"
  echo "  -t, --build-type TYPE               Specify build type (Debug, Release, etc.). Default: Release"
  echo "  -j, --jobs N                        Specify number of parallel jobs. Default: number of CPU cores"
  echo "  -b, --build-target TARGET           Specify build target (e.g. gcc11_linux_x86_64, qcc12_qnx800_aarch64, etc.)"
  echo "  -s, --sdp-path PATH                 Specify path to qnxsdp-env.sh for QNX builds"
  echo "  -e, --exception-safety MODE          Specify exception safety mode: 'safe' or 'conditional'. Default: 'conditional'"
  echo ""
  exit 1
}

# ------------------------------------------------------------------------------
# 6) Cleanup on Interruption
# ------------------------------------------------------------------------------
cleanup() {
  log WARN "Build interrupted by user or signal. Cleaning up..."
  # Remove temporary cache file if it exists
  if [ -n "$TEMP_CACHE_FILE" ] && [ -f "$TEMP_CACHE_FILE" ]; then
    rm -f "$TEMP_CACHE_FILE"
    log INFO "Removed temporary cache file: $TEMP_CACHE_FILE"
  fi
  exit 1
}

# ------------------------------------------------------------------------------
# 7) Setup QNX Environment (if needed)
# ------------------------------------------------------------------------------
setup_environment() {
  # If user chose a QNX target, we require an SDP path.
  if [[ "$BUILD_TARGET" == qcc12_qnx800_aarch64 || "$BUILD_TARGET" == qcc12_qnx800_x86_64 ]]; then
    if [ -z "$SDP_PATH" ]; then
      log ERROR "SDP path must be provided for QNX builds."
      exit 1
    fi
    log INFO "Sourcing QNX SDP environment from: $SDP_PATH"
    source "$SDP_PATH"
  fi
}

# ------------------------------------------------------------------------------
# 8) Define Build Parameters Based on Target + Build Type
# ------------------------------------------------------------------------------
define_build_parameters() {
  case $BUILD_TARGET in
    gcc11_linux_x86_64)
      if [ "$BUILD_TYPE" == "Debug" ]; then
        PRESET_NAME="gcc11_linux_x86_64_debug"
      else
        PRESET_NAME="gcc11_linux_x86_64_release"
      fi
      CONFIG_FILE="CMake/CMakeConfig/$PRESET_NAME.cmake"
      ;;
    gcc11_linux_aarch64)
      if [ "$BUILD_TYPE" == "Debug" ]; then
        PRESET_NAME="gcc11_linux_aarch64_debug"
      else
        PRESET_NAME="gcc11_linux_aarch64_release"
      fi
      CONFIG_FILE="CMake/CMakeConfig/$PRESET_NAME.cmake"
      ;;
    qcc12_qnx800_aarch64)
      if [ "$BUILD_TYPE" == "Debug" ]; then
        PRESET_NAME="qcc12_qnx800_aarch64_debug"
      else
        PRESET_NAME="qcc12_qnx800_aarch64_release"
      fi
      CONFIG_FILE="CMake/CMakeConfig/$PRESET_NAME.cmake"
      ;;
    qcc12_qnx800_x86_64)
      if [ "$BUILD_TYPE" == "Debug" ]; then
        PRESET_NAME="qcc12_qnx800_x86_64_debug"
      else
        PRESET_NAME="qcc12_qnx800_x86_64_release"
      fi
      CONFIG_FILE="CMake/CMakeConfig/$PRESET_NAME.cmake"
      ;;
    *)
      log ERROR "Invalid build target: $BUILD_TARGET"
      usage
      ;;
  esac
}

# ------------------------------------------------------------------------------
# 9) Clean Build & Install Directories (if --clean was passed)
# ------------------------------------------------------------------------------
clean_directories() {
  if [ "$CLEAN_BUILD" = true ]; then
    local build_dir="${PWD}/build/${PRESET_NAME}"
    local install_dir="${PWD}/install/${PRESET_NAME}"

    log INFO "Cleaning build directory: $build_dir"
    rm -rf "$build_dir"

    log INFO "Cleaning install directory: $install_dir"
    rm -rf "$install_dir"
  fi
}

# ------------------------------------------------------------------------------
# 11) Configure Project (cmake)
# ------------------------------------------------------------------------------
configure_project() {
  # Verify cmake is installed and working
  if ! command -v cmake &>/dev/null; then
    log ERROR "cmake command not found. Please install cmake or fix your PATH."
    exit 1
  fi

  log INFO "Configuring the project with preset: $PRESET_NAME and EXCEPTION_SAFETY_MODE=${EXCEPTION_SAFETY_MODE}"

  # Build an array of cmake command arguments.
  cmd=(cmake)

  # If a configuration file exists, add it.
  if [ -f "$CONFIG_FILE" ]; then
    log INFO "Using configuration file: $CONFIG_FILE"
    cmd+=("-C" "$CONFIG_FILE")
  else
    if [ -n "$CONFIG_FILE" ]; then
      log WARN "Configuration file not found: $CONFIG_FILE. Proceeding without it."
    fi
  fi

  # Conditionally add the conditional exceptions flag if the mode is "conditional".
  if [ "$EXCEPTION_SAFETY_MODE" = "conditional" ]; then
    cmd+=("-DARA_CORE_ARRAY_ENABLE_CONDITIONAL_EXCEPTIONS=1")
  fi

  # Specify the preset.
  cmd+=("--preset" "$PRESET_NAME")

  # Run the command.
  run_command "${cmd[@]}"
}


# ------------------------------------------------------------------------------
# 12) Build Project
# ------------------------------------------------------------------------------
build_project() {
  local build_dir="${PWD}/build/${PRESET_NAME}"
  log INFO "Building the project in: $build_dir"

  # If you wanted to log or check anything prior to building, do it here.
  run_command cmake --build "$build_dir" -- -j"$NUM_JOBS"
}

# ------------------------------------------------------------------------------
# 13) Install Project
# ------------------------------------------------------------------------------
install_project() {
  local build_dir="${PWD}/build/${PRESET_NAME}"
  local install_dir="${PWD}/install/${PRESET_NAME}"

  log INFO "Installing the project to: $install_dir"
  run_command cmake --install "$build_dir" --prefix "$install_dir"
}

# ------------------------------------------------------------------------------
# 14) Main Function
# ------------------------------------------------------------------------------
main() {
  # Trap signals for cleanup if user presses Ctrl+C, etc.
  trap cleanup SIGINT SIGTERM

  # Parse command-line arguments
  while [[ "$#" -gt 0 ]]; do
    case $1 in
      -h|--help)
        usage
        ;;
      -c|--clean)
        CLEAN_BUILD=true
        ;;
      -t|--build-type)
        BUILD_TYPE="$2"
        shift
        ;;
      -j|--jobs)
        NUM_JOBS="$2"
        shift
        ;;
      -b|--build-target)
        BUILD_TARGET="$2"
        shift
        ;;
      -s|--sdp-path)
        SDP_PATH="$2"
        shift
        ;;
      -e|--exception-safety)
        if [[ "$2" != "safe" && "$2" != "conditional" ]]; then
          log ERROR "Invalid value for --exception-safety: $2. Allowed values are 'safe' or 'conditional'."
          usage
        fi
        EXCEPTION_SAFETY_MODE="$2"
        shift
        ;;
      *)
        log ERROR "Unknown option: $1"
        usage
        ;;
    esac
    shift
  done

  # 1) Setup environment
  setup_environment

  # 2) Define build parameters (preset name, config file path, etc.)
  define_build_parameters

  # 3) Clean if requested
  clean_directories

  # 4) Configure (cmake)
  configure_project

  # 5) Build
  build_project

  # 6) Install
  install_project

  # 7) Clean up temporary config.cmake
  if [ -n "$TEMP_CACHE_FILE" ] && [ -f "$TEMP_CACHE_FILE" ]; then
    rm -f "$TEMP_CACHE_FILE"
    log INFO "Removed temporary config.cmake: $TEMP_CACHE_FILE"
  fi

  log INFO "Build completed successfully."
}

# ------------------------------------------------------------------------------
# 15) Execute Main
# ------------------------------------------------------------------------------
main "$@"
