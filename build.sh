#!/usr/bin/env bash
#
# [=========================================================================]
#  OpenAA: Open Source Adaptive AUTOSAR Project
#  Author: Sherif Mohamed
#
#  File description:
#  -----------------
#  Enterprise-grade build script for AdaptiveAutosarCpp17 with advanced
#  logging, security hardening, cross-platform support, and modular design.
#
#  Features:
#  - Strict mode with comprehensive error handling
#  - Advanced logging with multiple outputs and JSON support
#  - Security hardening with input validation
#  - Cross-platform compatibility
#  - Parallel build support with resource monitoring
#  - Modular architecture with clear separation of concerns
#  - Support for batch builds (all targets/configurations)
#
#  Usage:
#    ./build.sh [options]
#    ./build.sh --all-targets --all-configs --qnx710-sdp /path/to/qnx710 --qnx800-sdp /path/to/qnx800
#    ./build.sh --clean --build-type Release --build-target qcc12_qnx800_aarch64 \
#               --qnx800-sdp /root/qnx800/qnxsdp-env.sh --exception-safety safe --jobs 16
# [=========================================================================]

# ------------------------------------------------------------------------------
# 1) Strict Mode and Safety Settings
# ------------------------------------------------------------------------------
set -euo pipefail
IFS=$'\n\t'

# Enable extended debugging features
shopt -s extdebug
shopt -s inherit_errexit
shopt -s nullglob

# Script metadata
readonly SCRIPT_NAME="$(basename "${BASH_SOURCE[0]}")"
readonly SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
readonly SCRIPT_VERSION="2.0.0"
readonly SCRIPT_PID="$$"

# ------------------------------------------------------------------------------
# 2) Global Constants and Configuration
# ------------------------------------------------------------------------------
readonly BUILD_TYPES=("Debug" "Release")
readonly SUPPORTED_TARGETS=(
    "gcc11_linux_x86_64"
    "gcc11_linux_aarch64"
    "gcc13_linux_x86_64"
    "gcc13_linux_aarch64"
    "clang21_linux_x86_64"
    "clang21_linux_aarch64"
    "qcc8_qnx710_aarch64"
    "qcc8_qnx710_x86_64"
    "qcc12_qnx800_aarch64"
    "qcc12_qnx800_x86_64"
)

# QNX target patterns for environment detection
readonly QNX710_PATTERN="qcc8_qnx710_"
readonly QNX800_PATTERN="qcc12_qnx800_"

# Exit codes
readonly EXIT_SUCCESS=0
readonly EXIT_GENERAL_ERROR=1
readonly EXIT_MISUSE=2
readonly EXIT_CANNOT_EXECUTE=126
readonly EXIT_COMMAND_NOT_FOUND=127
readonly EXIT_INVALID_ARGUMENT=128
readonly EXIT_SIGNAL_BASE=128

# ------------------------------------------------------------------------------
# 3) Default Build Options
# ------------------------------------------------------------------------------
BUILD_TYPE="Release"
BUILD_TARGET="gcc11_linux_x86_64"
NUM_JOBS=""
CLEAN_BUILD=false
QNX710_SDP_PATH=""
QNX800_SDP_PATH=""
EXCEPTION_SAFETY_MODE="conditional"
ALL_TARGETS=false
ALL_CONFIGS=false
VERBOSE=false
DRY_RUN=false
LOG_LEVEL="INFO"
LOG_FILE=""
LOG_JSON=false

# Performance monitoring
BUILD_START_TIME=""
BUILD_END_TIME=""

# ------------------------------------------------------------------------------
# 4) Advanced Logging System
# ------------------------------------------------------------------------------
# ANSI color codes
readonly COLOR_RED='\033[0;31m'
readonly COLOR_GREEN='\033[0;32m'
readonly COLOR_YELLOW='\033[0;33m'
readonly COLOR_BLUE='\033[0;34m'
readonly COLOR_MAGENTA='\033[0;35m'
readonly COLOR_CYAN='\033[0;36m'
readonly COLOR_WHITE='\033[0;37m'
readonly COLOR_RESET='\033[0m'

# Log levels
declare -A LOG_LEVELS=(
    ["TRACE"]=0
    ["DEBUG"]=1
    ["INFO"]=2
    ["WARN"]=3
    ["ERROR"]=4
    ["FATAL"]=5
)

# Initialize logging
init_logging() {
    # Create log directory if logging to file
    if [[ -n "$LOG_FILE" ]]; then
        local log_dir
        log_dir="$(dirname "$LOG_FILE")"
        mkdir -p "$log_dir"
        
        # Initialize log file with header
        {
            echo "# OpenAA Build Script Log"
            echo "# Version: $SCRIPT_VERSION"
            echo "# Started: $(date -u +"%Y-%m-%dT%H:%M:%SZ")"
            echo "# PID: $SCRIPT_PID"
            echo "# Command: $0 $*"
            echo "# ========================"
        } > "$LOG_FILE"
    fi
}

# Enhanced logging function with multiple outputs
log() {
    local level="${1:-INFO}"
    local message="${2:-}"
    local timestamp
    timestamp=$(date -u +"%Y-%m-%dT%H:%M:%SZ")
    
    # Check if we should log this level
    if [[ ${LOG_LEVELS[$level]:-2} -lt ${LOG_LEVELS[$LOG_LEVEL]:-2} ]]; then
        return 0
    fi
    
    # Console output with colors
    local color=""
    case "$level" in
        TRACE) color="$COLOR_WHITE" ;;
        DEBUG) color="$COLOR_CYAN" ;;
        INFO)  color="$COLOR_GREEN" ;;
        WARN)  color="$COLOR_YELLOW" ;;
        ERROR) color="$COLOR_RED" ;;
        FATAL) color="$COLOR_MAGENTA" ;;
    esac
    
    # Format console message
    if [[ -t 1 ]] && [[ -z "${NO_COLOR:-}" ]]; then
        printf "${color}[%-5s]${COLOR_RESET} [%s] %s: %s\n" \
            "$level" "$timestamp" "$SCRIPT_NAME" "$message" >&2
    else
        printf "[%-5s] [%s] %s: %s\n" \
            "$level" "$timestamp" "$SCRIPT_NAME" "$message" >&2
    fi
    
    # File output
    if [[ -n "$LOG_FILE" ]]; then
        if [[ "$LOG_JSON" == "true" ]]; then
            # JSON format for structured logging
            printf '{"timestamp":"%s","level":"%s","script":"%s","pid":%d,"message":"%s"}\n' \
                "$timestamp" "$level" "$SCRIPT_NAME" "$SCRIPT_PID" \
                "$(echo "$message" | sed 's/"/\\"/g')" >> "$LOG_FILE"
        else
            printf "[%-5s] [%s] %s[%d]: %s\n" \
                "$level" "$timestamp" "$SCRIPT_NAME" "$SCRIPT_PID" "$message" >> "$LOG_FILE"
        fi
    fi
    
    # Fatal errors trigger immediate exit
    if [[ "$level" == "FATAL" ]]; then
        exit "$EXIT_GENERAL_ERROR"
    fi
}

# ------------------------------------------------------------------------------
# 5) Input Validation and Security
# ------------------------------------------------------------------------------
# Validate string contains only safe characters
validate_safe_string() {
    local input="$1"
    local pattern="${2:-^[a-zA-Z0-9._/-]+$}"
    local name="${3:-input}"
    
    if [[ ! "$input" =~ $pattern ]]; then
        log ERROR "Invalid $name: '$input' contains unsafe characters"
        return "$EXIT_INVALID_ARGUMENT"
    fi
    return 0
}

# Validate numeric input
validate_number() {
    local input="$1"
    local min="${2:-1}"
    local max="${3:-9999}"
    local name="${4:-number}"
    
    if [[ ! "$input" =~ ^[0-9]+$ ]]; then
        log ERROR "Invalid $name: '$input' is not a number"
        return "$EXIT_INVALID_ARGUMENT"
    fi
    
    if [[ "$input" -lt "$min" ]] || [[ "$input" -gt "$max" ]]; then
        log ERROR "Invalid $name: '$input' is outside valid range [$min-$max]"
        return "$EXIT_INVALID_ARGUMENT"
    fi
    return 0
}

# Validate file exists and is readable
validate_file() {
    local file="$1"
    local name="${2:-file}"
    
    if [[ ! -f "$file" ]]; then
        log ERROR "$name does not exist: $file"
        return "$EXIT_INVALID_ARGUMENT"
    fi
    
    if [[ ! -r "$file" ]]; then
        log ERROR "$name is not readable: $file"
        return "$EXIT_INVALID_ARGUMENT"
    fi
    return 0
}

# Validate build target
validate_build_target() {
    local target="$1"
    local valid=false
    
    for valid_target in "${SUPPORTED_TARGETS[@]}"; do
        if [[ "$target" == "$valid_target" ]]; then
            valid=true
            break
        fi
    done
    
    if [[ "$valid" != "true" ]]; then
        log ERROR "Invalid build target: $target"
        log ERROR "Supported targets: ${SUPPORTED_TARGETS[*]}"
        return "$EXIT_INVALID_ARGUMENT"
    fi
    return 0
}

# ------------------------------------------------------------------------------
# 6) System Detection and Dependency Checking
# ------------------------------------------------------------------------------
# Detect operating system
detect_os() {
    local os=""
    if [[ -f /etc/os-release ]]; then
        # shellcheck disable=SC1091
        source /etc/os-release
        os="${ID:-unknown}"
    elif [[ "$OSTYPE" == "darwin"* ]]; then
        os="macos"
    elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
        os="linux"
    else
        os="unknown"
    fi
    echo "$os"
}

# Check for required commands
check_dependencies() {
    local deps=("cmake" "git" "make")
    local missing=()
    
    for cmd in "${deps[@]}"; do
        if ! command -v "$cmd" &>/dev/null; then
            missing+=("$cmd")
        fi
    done
    
    if [[ ${#missing[@]} -gt 0 ]]; then
        log FATAL "Missing required dependencies: ${missing[*]}"
    fi
    
    # Check cmake version
    local cmake_version
    cmake_version=$(cmake --version | head -1 | grep -oE '[0-9]+\.[0-9]+\.[0-9]+')
    log DEBUG "Found CMake version: $cmake_version"
}

# Get number of available CPU cores
get_cpu_count() {
    local cpu_count
    if command -v nproc &>/dev/null; then
        cpu_count=$(nproc)
    elif command -v sysctl &>/dev/null; then
        cpu_count=$(sysctl -n hw.ncpu 2>/dev/null || echo 1)
    else
        cpu_count=$(grep -c ^processor /proc/cpuinfo 2>/dev/null || echo 1)
    fi
    echo "$cpu_count"
}

# ------------------------------------------------------------------------------
# 7) Signal Handling and Cleanup
# ------------------------------------------------------------------------------
# Cleanup function for graceful shutdown
cleanup() {
    local exit_code=$?
    
    # Log cleanup start
    log DEBUG "Starting cleanup (exit code: $exit_code)"
    
    # Kill any background jobs
    local jobs
    jobs=$(jobs -p)
    if [[ -n "$jobs" ]]; then
        log WARN "Terminating background jobs: $jobs"
        # shellcheck disable=SC2086
        kill -TERM $jobs 2>/dev/null || true
        sleep 1
        # shellcheck disable=SC2086
        kill -KILL $jobs 2>/dev/null || true
    fi
    
    # Remove temporary files
    if [[ -n "${TEMP_DIR:-}" ]] && [[ -d "$TEMP_DIR" ]]; then
        log DEBUG "Removing temporary directory: $TEMP_DIR"
        rm -rf "$TEMP_DIR"
    fi
    
    # Calculate build duration if applicable
    if [[ -n "$BUILD_START_TIME" ]] && [[ -n "$BUILD_END_TIME" ]]; then
        local duration=$((BUILD_END_TIME - BUILD_START_TIME))
        log INFO "Total build time: $(format_duration "$duration")"
    fi
    
    # Final log message
    if [[ $exit_code -eq 0 ]]; then
        log INFO "Script completed successfully"
    else
        log ERROR "Script failed with exit code: $exit_code"
    fi
    
    exit "$exit_code"
}

# Signal handlers
handle_signal() {
    local signal="$1"
    log WARN "Received signal: $signal"
    BUILD_END_TIME=$(date +%s)
    exit $((EXIT_SIGNAL_BASE + signal))
}

# Setup signal traps
setup_signals() {
    trap cleanup EXIT
    trap 'handle_signal 1' HUP
    trap 'handle_signal 2' INT
    trap 'handle_signal 3' QUIT
    trap 'handle_signal 15' TERM
}

# ------------------------------------------------------------------------------
# 8) Utility Functions
# ------------------------------------------------------------------------------
# Format duration in human-readable format
format_duration() {
    local seconds="$1"
    local hours=$((seconds / 3600))
    local minutes=$(((seconds % 3600) / 60))
    local secs=$((seconds % 60))
    
    if [[ $hours -gt 0 ]]; then
        printf "%dh %dm %ds" "$hours" "$minutes" "$secs"
    elif [[ $minutes -gt 0 ]]; then
        printf "%dm %ds" "$minutes" "$secs"
    else
        printf "%ds" "$secs"
    fi
}

# Run command with error checking and timing
run_command() {
    local cmd=("$@")
    local start_time
    local end_time
    local duration
    local status
    
    if [[ "$DRY_RUN" == "true" ]]; then
        log INFO "[DRY RUN] Would execute: ${cmd[*]}"
        return 0
    fi
    
    log DEBUG "Executing: ${cmd[*]}"
    start_time=$(date +%s)
    
    if [[ "$VERBOSE" == "true" ]]; then
        "${cmd[@]}"
        status=$?
    else
        "${cmd[@]}" &>/dev/null
        status=$?
    fi
    
    end_time=$(date +%s)
    duration=$((end_time - start_time))
    
    if [[ $status -eq 0 ]]; then
        log DEBUG "Command completed in $(format_duration "$duration")"
    else
        log ERROR "Command failed with exit code $status after $(format_duration "$duration")"
        log ERROR "Failed command: ${cmd[*]}"
        return $status
    fi
    
    return 0
}

# Create directory with proper permissions
create_directory() {
    local dir="$1"
    local mode="${2:-755}"
    
    if [[ ! -d "$dir" ]]; then
        log DEBUG "Creating directory: $dir"
        mkdir -p "$dir"
        chmod "$mode" "$dir"
    fi
}

# ------------------------------------------------------------------------------
# 9) QNX Environment Management
# ------------------------------------------------------------------------------
# Setup QNX environment based on target
setup_qnx_environment() {
    local target="$1"
    local sdp_path=""
    
    # Determine which QNX version based on target
    if [[ "$target" =~ $QNX710_PATTERN ]]; then
        sdp_path="$QNX710_SDP_PATH"
        if [[ -z "$sdp_path" ]]; then
            log ERROR "QNX 7.10 SDP path required for target: $target"
            log ERROR "Use --qnx710-sdp option to specify the path"
            return "$EXIT_INVALID_ARGUMENT"
        fi
        log INFO "Setting up QNX 7.10 environment for target: $target"
    elif [[ "$target" =~ $QNX800_PATTERN ]]; then
        sdp_path="$QNX800_SDP_PATH"
        if [[ -z "$sdp_path" ]]; then
            log ERROR "QNX 8.0 SDP path required for target: $target"
            log ERROR "Use --qnx800-sdp option to specify the path"
            return "$EXIT_INVALID_ARGUMENT"
        fi
        log INFO "Setting up QNX 8.0 environment for target: $target"
    else
        # Not a QNX target, nothing to do
        return 0
    fi
    
    # Validate SDP script exists
    if ! validate_file "$sdp_path" "QNX SDP script"; then
        return "$EXIT_INVALID_ARGUMENT"
    fi
    
    # Source the SDP environment
    log DEBUG "Sourcing QNX SDP: $sdp_path"
    # shellcheck disable=SC1090
    source "$sdp_path"
    
    # Verify QNX environment is properly set
    if [[ -z "${QNX_HOST:-}" ]] || [[ -z "${QNX_TARGET:-}" ]]; then
        log ERROR "QNX environment not properly configured after sourcing SDP"
        log ERROR "QNX_HOST: ${QNX_HOST:-not set}"
        log ERROR "QNX_TARGET: ${QNX_TARGET:-not set}"
        return "$EXIT_GENERAL_ERROR"
    fi
    
    log DEBUG "QNX environment configured:"
    log DEBUG "  QNX_HOST: $QNX_HOST"
    log DEBUG "  QNX_TARGET: $QNX_TARGET"
    
    return 0
}

# ------------------------------------------------------------------------------
# 10) Build Configuration Management
# ------------------------------------------------------------------------------
# Generate preset name from target and build type
get_preset_name() {
    local target="$1"
    local build_type="$2"
    local preset_name=""
    
    if [[ "$build_type" == "Debug" ]]; then
        preset_name="${target}_debug"
    else
        preset_name="${target}_release"
    fi
    
    echo "$preset_name"
}

# Get configuration file path
get_config_file() {
    local preset_name="$1"
    echo "CMake/CMakeConfig/${preset_name}.cmake"
}

# ------------------------------------------------------------------------------
# 11) Build Operations
# ------------------------------------------------------------------------------
# Clean build directories
clean_build() {
    local preset_name="$1"
    local build_dir="$SCRIPT_DIR/build/$preset_name"
    local install_dir="$SCRIPT_DIR/install/$preset_name"
    
    if [[ -d "$build_dir" ]]; then
        log INFO "Removing build directory: $build_dir"
        rm -rf "$build_dir"
    fi
    
    if [[ -d "$install_dir" ]]; then
        log INFO "Removing install directory: $install_dir"
        rm -rf "$install_dir"
    fi
}

# Configure project
configure_project() {
    local preset_name="$1"
    local config_file="$2"
    local exception_mode="$3"
    
    log INFO "Configuring project: $preset_name"
    
    local cmake_args=(cmake)
    
    # Add configuration file if it exists
    if [[ -f "$config_file" ]]; then
        log DEBUG "Using configuration file: $config_file"
        cmake_args+=(-C "$config_file")
    else
        log WARN "Configuration file not found: $config_file"
    fi
    
    # Add exception safety mode
    if [[ "$exception_mode" == "conditional" ]]; then
        cmake_args+=(-DENABLE_PLATFORM_CONDITIONAL_EXCEPTION=1)
    fi
    
    # Add preset
    cmake_args+=(--preset "$preset_name")
    
    # Execute configuration
    run_command "${cmake_args[@]}"
}

# Build project
build_project() {
    local preset_name="$1"
    local num_jobs="$2"
    local build_dir="$SCRIPT_DIR/build/$preset_name"
    
    log INFO "Building project: $preset_name (jobs: $num_jobs)"
    
    # Monitor system resources during build
    if command -v free &>/dev/null; then
        local mem_before
        mem_before=$(free -m | awk '/^Mem:/ {print $3}')
        log DEBUG "Memory usage before build: ${mem_before}MB"
    fi
    
    local build_start
    build_start=$(date +%s)
    
    run_command cmake --build "$build_dir" -- -j"$num_jobs"
    
    local build_end
    build_end=$(date +%s)
    local build_duration=$((build_end - build_start))
    
    log INFO "Build completed in $(format_duration "$build_duration")"
    
    if command -v free &>/dev/null; then
        local mem_after
        mem_after=$(free -m | awk '/^Mem:/ {print $3}')
        log DEBUG "Memory usage after build: ${mem_after}MB"
    fi
}

# Install project
install_project() {
    local preset_name="$1"
    local build_dir="$SCRIPT_DIR/build/$preset_name"
    local install_dir="$SCRIPT_DIR/install/$preset_name"
    
    log INFO "Installing project: $preset_name"
    log DEBUG "Install directory: $install_dir"
    
    create_directory "$install_dir"
    run_command cmake --install "$build_dir" --prefix "$install_dir"
}

# Execute single build configuration
execute_build() {
    local target="$1"
    local build_type="$2"
    local clean="$3"
    local num_jobs="$4"
    local exception_mode="$5"
    
    log INFO "=========================================="
    log INFO "Building: $target ($build_type)"
    log INFO "=========================================="
    
    # Setup environment if needed
    if ! setup_qnx_environment "$target"; then
        log ERROR "Failed to setup environment for: $target"
        return "$EXIT_GENERAL_ERROR"
    fi
    
    # Export BUILD_TARGET for CMakePresets
    export BUILD_TARGET="$target"
    
    # Get configuration names
    local preset_name
    preset_name=$(get_preset_name "$target" "$build_type")
    
    local config_file
    config_file=$(get_config_file "$preset_name")
    
    # Clean if requested
    if [[ "$clean" == "true" ]]; then
        clean_build "$preset_name"
    fi
    
    # Execute build steps
    if ! configure_project "$preset_name" "$config_file" "$exception_mode"; then
        log ERROR "Configuration failed for: $preset_name"
        return "$EXIT_GENERAL_ERROR"
    fi
    
    if ! build_project "$preset_name" "$num_jobs"; then
        log ERROR "Build failed for: $preset_name"
        return "$EXIT_GENERAL_ERROR"
    fi
    
    if ! install_project "$preset_name"; then
        log ERROR "Installation failed for: $preset_name"
        return "$EXIT_GENERAL_ERROR"
    fi
    
    log INFO "Successfully built: $target ($build_type)"
    return 0
}

# Execute batch builds
execute_batch_builds() {
    local targets=("$@")
    local num_jobs
    num_jobs=$(get_cpu_count)
    
    # Override with user setting if provided
    if [[ -n "$NUM_JOBS" ]]; then
        num_jobs="$NUM_JOBS"
    fi
    
    local total_builds=$((${#targets[@]} * ${#BUILD_TYPES[@]}))
    local current_build=0
    local failed_builds=()
    
    log INFO "Starting batch build: ${#targets[@]} targets, ${#BUILD_TYPES[@]} configurations"
    log INFO "Total builds to execute: $total_builds"
    
    for target in "${targets[@]}"; do
        for build_type in "${BUILD_TYPES[@]}"; do
            current_build=$((current_build + 1))
            log INFO ""
            log INFO "Build $current_build of $total_builds"
            
            if execute_build "$target" "$build_type" "$CLEAN_BUILD" "$num_jobs" "$EXCEPTION_SAFETY_MODE"; then
                log INFO "✓ Build successful: $target ($build_type)"
            else
                log ERROR "✗ Build failed: $target ($build_type)"
                failed_builds+=("$target:$build_type")
            fi
        done
    done
    
    # Summary
    log INFO ""
    log INFO "=========================================="
    log INFO "Batch Build Summary"
    log INFO "=========================================="
    log INFO "Total builds: $total_builds"
    log INFO "Successful: $((total_builds - ${#failed_builds[@]}))"
    log INFO "Failed: ${#failed_builds[@]}"
    
    if [[ ${#failed_builds[@]} -gt 0 ]]; then
        log ERROR "Failed builds:"
        for build in "${failed_builds[@]}"; do
            log ERROR "  - $build"
        done
        return "$EXIT_GENERAL_ERROR"
    fi
    
    return 0
}

# ------------------------------------------------------------------------------
# 12) Usage and Help
# ------------------------------------------------------------------------------
usage() {
    cat <<EOF
OpenAA Build Script v$SCRIPT_VERSION

USAGE:
    $SCRIPT_NAME [OPTIONS]

OPTIONS:
    -h, --help                    Show this help message and exit
    -c, --clean                   Clean build directories before building
    -t, --build-type TYPE         Build type: Debug or Release (default: Release)
    -j, --jobs N                  Number of parallel jobs (default: auto-detect)
    -b, --build-target TARGET     Specific build target (see TARGETS below)
    -e, --exception-safety MODE   Exception safety: 'safe' or 'conditional' (default: conditional)
    
    --qnx710-sdp PATH            Path to QNX 7.10 SDP environment script
    --qnx800-sdp PATH            Path to QNX 8.0 SDP environment script
    
    --all-targets                Build all supported targets
    --all-configs                Build all configurations (Debug and Release)
    
    -v, --verbose                Enable verbose output
    --dry-run                    Show what would be executed without doing it
    
    --log-level LEVEL            Set log level: TRACE, DEBUG, INFO, WARN, ERROR (default: INFO)
    --log-file FILE              Write logs to file
    --log-json                   Use JSON format for log file

SUPPORTED TARGETS:
EOF
    for target in "${SUPPORTED_TARGETS[@]}"; do
        echo "    - $target"
    done
    
    cat <<EOF

EXAMPLES:
    # Build specific target with Release configuration
    $SCRIPT_NAME -b gcc11_linux_x86_64 -t Release
    
    # Build QNX target with clean build
    $SCRIPT_NAME -c -b qcc12_qnx800_aarch64 --qnx800-sdp /path/to/qnx800/qnxsdp-env.sh
    
    # Build all targets with all configurations
    $SCRIPT_NAME --all-targets --all-configs --qnx710-sdp /path/to/qnx710 --qnx800-sdp /path/to/qnx800
    
    # Debug mode with verbose logging
    $SCRIPT_NAME -b gcc13_linux_x86_64 -v --log-level DEBUG --log-file build.log

ENVIRONMENT VARIABLES:
    NO_COLOR         Disable colored output
    BUILD_TARGET     Override build target (for CMakePresets.json)

EXIT CODES:
    0   Success
    1   General error
    2   Misuse of shell command
    126 Command cannot execute
    127 Command not found
    128 Invalid argument
    130 Script terminated by Ctrl+C
    143 Script terminated by SIGTERM

EOF
}

# ------------------------------------------------------------------------------
# 13) Argument Parsing
# ------------------------------------------------------------------------------
parse_arguments() {
    while [[ $# -gt 0 ]]; do
        case "$1" in
            -h|--help)
                usage
                exit "$EXIT_SUCCESS"
                ;;
            -c|--clean)
                CLEAN_BUILD=true
                ;;
            -t|--build-type)
                shift
                BUILD_TYPE="$1"
                if [[ "$BUILD_TYPE" != "Debug" ]] && [[ "$BUILD_TYPE" != "Release" ]]; then
                    log FATAL "Invalid build type: $BUILD_TYPE (must be Debug or Release)"
                fi
                ;;
            -j|--jobs)
                shift
                NUM_JOBS="$1"
                validate_number "$NUM_JOBS" 1 999 "job count" || exit $?
                ;;
            -b|--build-target)
                shift
                BUILD_TARGET="$1"
                validate_build_target "$BUILD_TARGET" || exit $?
                ;;
            -e|--exception-safety)
                shift
                EXCEPTION_SAFETY_MODE="$1"
                if [[ "$EXCEPTION_SAFETY_MODE" != "safe" ]] && [[ "$EXCEPTION_SAFETY_MODE" != "conditional" ]]; then
                    log FATAL "Invalid exception safety mode: $EXCEPTION_SAFETY_MODE (must be 'safe' or 'conditional')"
                fi
                ;;
            --qnx710-sdp)
                shift
                QNX710_SDP_PATH="$1"
                validate_safe_string "$QNX710_SDP_PATH" '^[a-zA-Z0-9._/\-~ ]+$' "QNX 7.10 SDP path" || exit $?
                ;;
            --qnx800-sdp)
                shift
                QNX800_SDP_PATH="$1"
                validate_safe_string "$QNX800_SDP_PATH" '^[a-zA-Z0-9._/\-~ ]+$' "QNX 8.0 SDP path" || exit $?
                ;;
            --all-targets)
                ALL_TARGETS=true
                ;;
            --all-configs)
                ALL_CONFIGS=true
                ;;
            -v|--verbose)
                VERBOSE=true
                ;;
            --dry-run)
                DRY_RUN=true
                ;;
            --log-level)
                shift
                LOG_LEVEL="$1"
                if [[ ! ${LOG_LEVELS[$LOG_LEVEL]+isset} ]]; then
                    log FATAL "Invalid log level: $LOG_LEVEL"
                fi
                ;;
            --log-file)
                shift
                LOG_FILE="$1"
                validate_safe_string "$LOG_FILE" '^[a-zA-Z0-9._/\-]+$' "log file path" || exit $?
                ;;
            --log-json)
                LOG_JSON=true
                ;;
            -*)
                log FATAL "Unknown option: $1"
                ;;
            *)
                log FATAL "Unexpected argument: $1"
                ;;
        esac
        shift
    done
}

# ------------------------------------------------------------------------------
# 14) Main Function
# ------------------------------------------------------------------------------
main() {
    # Initialize
    BUILD_START_TIME=$(date +%s)
    
    # Parse arguments
    parse_arguments "$@"
    
    # Initialize logging system
    init_logging "$@"
    
    # Log startup information
    log INFO "OpenAA Build Script v$SCRIPT_VERSION"
    log INFO "Host: $(hostname)"
    log INFO "OS: $(detect_os)"
    log INFO "CPU cores: $(get_cpu_count)"
    log DEBUG "Script directory: $SCRIPT_DIR"
    
    # Setup signal handlers
    setup_signals
    
    # Check dependencies
    check_dependencies
    
    # Determine build targets
    local targets=()
    if [[ "$ALL_TARGETS" == "true" ]]; then
        targets=("${SUPPORTED_TARGETS[@]}")
    else
        targets=("$BUILD_TARGET")
    fi
    
    # Determine build types
    local build_types=()
    if [[ "$ALL_CONFIGS" == "true" ]]; then
        build_types=("${BUILD_TYPES[@]}")
    else
        build_types=("$BUILD_TYPE")
    fi
    
    # Execute builds
    if [[ "$ALL_TARGETS" == "true" ]] || [[ "$ALL_CONFIGS" == "true" ]]; then
        # Batch build mode
        execute_batch_builds "${targets[@]}"
    else
        # Single build mode
        local num_jobs
        num_jobs="${NUM_JOBS:-$(get_cpu_count)}"
        execute_build "$BUILD_TARGET" "$BUILD_TYPE" "$CLEAN_BUILD" "$num_jobs" "$EXCEPTION_SAFETY_MODE"
    fi
    
    # Record completion time
    BUILD_END_TIME=$(date +%s)
    
    # Exit with success
    exit "$EXIT_SUCCESS"
}

# ------------------------------------------------------------------------------
# 15) Script Entry Point
# ------------------------------------------------------------------------------
# Ensure we're not being sourced
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi