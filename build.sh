#!/bin/bash
#
# [=========================================================================]
#  OpenAA: Open Source Adaptive AUTOSAR Project
#  Author: Sherif Mohamed
#
#  File description:
#  -----------------
#  Enhanced build script for AdaptiveAutosarCpp17 with modular functions,
#  improved logging, and robust error handling.
#
#  Integrations:
#  - Uses CMakePresets.json for build presets.
#  - Supports a configuration file (-C option) for initial cache variables.
#  - Enhanced logging utility for better traceability.
#  - Modular, functional design for maintainability and clarity.
#
#  Usage:
#    ./build.sh [options]
#    e.g.: ./build.sh --clean --build-type Release --build-target qcc12_qnx800_aarch64 \
#         --sdp-path /root/qnx800/qnxsdp-env.sh --exception-safety safe --jobs 16
# [=========================================================================]

SCRIPT_NAME=$(basename "$0")

# ------------------------------------------------------------------------------
# 1) Bash Settings for Safety
# ------------------------------------------------------------------------------
set -e
set -o pipefail

# ------------------------------------------------------------------------------
# 2) Default Build Options
# ------------------------------------------------------------------------------
BUILD_TYPE="Release"                  # Default build type
BUILD_TARGET="gcc11_linux_x86_64"     # Default build target
export BUILD_TARGET                   # Export for CMakePresets substitution
NUM_JOBS=$(nproc)                     # Default to number of CPU cores
CLEAN_BUILD=false                     # Clean build flag
SDP_PATH=""                           # QNX SDP path (if needed)
PRESET_NAME=""                        # Computed preset name
CONFIG_FILE=""                        # Computed config file path

# ------------------------------------------------------------------------------
# 2.1) Exception Safety Options
# ------------------------------------------------------------------------------
EXCEPTION_SAFETY_MODE="conditional"   # Default exception safety mode

# ------------------------------------------------------------------------------
# 3) Enhanced Logging Utility
# ------------------------------------------------------------------------------
# Colors: Blue for INFO, Yellow for WARN, Red for ERROR, Gray for unknown.
COLOR_INFO="\033[37m"
COLOR_WARN="\033[33m"
COLOR_ERROR="\033[31m"
COLOR_RESET="\033[0m"
LOG_TAG="[$SCRIPT_NAME]"

log() {
    local level=$1
    local message=$2
    local timestamp
    timestamp=$(date "+%Y-%m-%d %H:%M:%S")
    case "$level" in
        INFO)
            echo -e "${COLOR_INFO}[INFO] [$timestamp] ${LOG_TAG} $message${COLOR_RESET}"
            ;;
        WARN)
            echo -e "${COLOR_WARN}[WARN] [$timestamp] ${LOG_TAG} $message${COLOR_RESET}"
            ;;
        ERROR)
            echo -e "${COLOR_ERROR}[ERROR] [$timestamp] ${LOG_TAG} $message${COLOR_RESET}"
            ;;
        *)
            echo "[UNKNOWN] [$timestamp] ${LOG_TAG} $message"
            ;;
    esac
}

# ------------------------------------------------------------------------------
# 4) Run Command with Error Checking
# ------------------------------------------------------------------------------
run_command() {
    local cmd=("$@")
    log INFO "Running command: ${cmd[*]}"
    "${cmd[@]}"
    local status=$?
    if [ $status -ne 0 ]; then
        log ERROR "Command failed with exit code $status: ${cmd[*]}"
        exit $status
    fi
}

# ------------------------------------------------------------------------------
# 5) Usage / Help Function
# ------------------------------------------------------------------------------
usage() {
    log INFO "Usage: $SCRIPT_NAME [OPTIONS]"
    echo ""
    echo "Options:"
    echo "  -h, --help                          Show this help message and exit"
    echo "  -c, --clean                         Clean build by removing build and install directories"
    echo "  -t, --build-type TYPE               Specify build type (Debug, Release, etc.). Default: Release"
    echo "  -j, --jobs N                        Number of parallel jobs. Default: number of CPU cores"
    echo "  -b, --build-target TARGET           Build target (e.g. gcc11_linux_x86_64, qcc12_qnx800_aarch64, etc.)"
    echo "  -s, --sdp-path PATH                 Path to qnxsdp-env.sh for QNX builds"
    echo "  -e, --exception-safety MODE         Exception safety mode: 'safe' or 'conditional'. Default: conditional"
    echo ""
    exit 1
}

# ------------------------------------------------------------------------------
# 6) Cleanup on Interruption
# ------------------------------------------------------------------------------
cleanup() {
    log WARN "Build interrupted. Cleaning up..."
    [ -n "$TEMP_CACHE_FILE" ] && [ -f "$TEMP_CACHE_FILE" ] && {
        rm -f "$TEMP_CACHE_FILE"
        log INFO "Removed temporary config file: $TEMP_CACHE_FILE"
    }
    exit 1
}

# Trap signals for cleanup
trap cleanup SIGINT SIGTERM

# ------------------------------------------------------------------------------
# 7) Setup QNX Environment (if needed)
# ------------------------------------------------------------------------------
setup_environment() {
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
# 8) Define Build Parameters Based on Target and Build Type
# ------------------------------------------------------------------------------
define_build_parameters() {
    case $BUILD_TARGET in
        gcc11_linux_x86_64)
            if [ "$BUILD_TYPE" == "Debug" ]; then
                PRESET_NAME="gcc11_linux_x86_64_debug"
            else
                PRESET_NAME="gcc11_linux_x86_64_release"
            fi
            CONFIG_FILE="CMake/CMakeConfig/${PRESET_NAME}.cmake"
            ;;
        gcc11_linux_aarch64)
            if [ "$BUILD_TYPE" == "Debug" ]; then
                PRESET_NAME="gcc11_linux_aarch64_debug"
            else
                PRESET_NAME="gcc11_linux_aarch64_release"
            fi
            CONFIG_FILE="CMake/CMakeConfig/${PRESET_NAME}.cmake"
            ;;
        gcc13_linux_x86_64)
            if [ "$BUILD_TYPE" == "Debug" ]; then
                PRESET_NAME="gcc13_linux_x86_64_debug"
            else
                PRESET_NAME="gcc13_linux_x86_64_release"
            fi
            CONFIG_FILE="CMake/CMakeConfig/${PRESET_NAME}.cmake"
            ;;
        gcc13_linux_aarch64)
            if [ "$BUILD_TYPE" == "Debug" ]; then
                PRESET_NAME="gcc13_linux_aarch64_debug"
            else
                PRESET_NAME="gcc13_linux_aarch64_release"
            fi
            CONFIG_FILE="CMake/CMakeConfig/${PRESET_NAME}.cmake"
            ;;
        qcc12_qnx800_aarch64)
            if [ "$BUILD_TYPE" == "Debug" ]; then
                PRESET_NAME="qcc12_qnx800_aarch64_debug"
            else
                PRESET_NAME="qcc12_qnx800_aarch64_release"
            fi
            CONFIG_FILE="CMake/CMakeConfig/${PRESET_NAME}.cmake"
            ;;
        qcc12_qnx800_x86_64)
            if [ "$BUILD_TYPE" == "Debug" ]; then
                PRESET_NAME="qcc12_qnx800_x86_64_debug"
            else
                PRESET_NAME="qcc12_qnx800_x86_64_release"
            fi
            CONFIG_FILE="CMake/CMakeConfig/${PRESET_NAME}.cmake"
            ;;
        *)
            log ERROR "Invalid build target: $BUILD_TARGET"
            usage
            ;;
    esac
    log INFO "Build parameters set: PRESET_NAME=$PRESET_NAME, CONFIG_FILE=$CONFIG_FILE"
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
# 10) Configure Project (cmake)
# ------------------------------------------------------------------------------
configure_project() {
    if ! command -v cmake &>/dev/null; then
        log ERROR "cmake command not found. Please install cmake or fix your PATH."
        exit 1
    fi

    log INFO "Configuring project with preset: $PRESET_NAME and EXCEPTION_SAFETY_MODE=${EXCEPTION_SAFETY_MODE}"
    local cmd=(cmake)
    if [ -f "$CONFIG_FILE" ]; then
        log INFO "Using configuration file: $CONFIG_FILE"
        cmd+=("-C" "$CONFIG_FILE")
    else
        log WARN "Configuration file not found: $CONFIG_FILE. Proceeding without it."
    fi

    if [ "$EXCEPTION_SAFETY_MODE" = "conditional" ]; then
        cmd+=("-DARA_CORE_ARRAY_ENABLE_CONDITIONAL_EXCEPTIONS=1")
    fi

    cmd+=("--preset" "$PRESET_NAME")
    run_command "${cmd[@]}"
}

# ------------------------------------------------------------------------------
# 11) Build Project
# ------------------------------------------------------------------------------
build_project() {
    local build_dir="${PWD}/build/${PRESET_NAME}"
    log INFO "Building project in: $build_dir"
    run_command cmake --build "$build_dir" -- -j"$NUM_JOBS"
}

# ------------------------------------------------------------------------------
# 12) Install Project
# ------------------------------------------------------------------------------
install_project() {
    local build_dir="${PWD}/build/${PRESET_NAME}"
    local install_dir="${PWD}/install/${PRESET_NAME}"
    log INFO "Installing project to: $install_dir"
    run_command cmake --install "$build_dir" --prefix "$install_dir"
}

# ------------------------------------------------------------------------------
# 13) Main Function
# ------------------------------------------------------------------------------
main() {
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
                    log ERROR "Invalid value for --exception-safety: $2. Allowed values: safe or conditional."
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

    # Execute build steps in order
    setup_environment
    define_build_parameters
    clean_directories
    configure_project
    build_project
    install_project

    [ -n "$TEMP_CACHE_FILE" ] && [ -f "$TEMP_CACHE_FILE" ] && {
        rm -f "$TEMP_CACHE_FILE"
        log INFO "Removed temporary config file: $TEMP_CACHE_FILE"
    }

    log INFO "Build completed successfully."
}

# ------------------------------------------------------------------------------
# 14) Execute Main
# ------------------------------------------------------------------------------
main "$@"
