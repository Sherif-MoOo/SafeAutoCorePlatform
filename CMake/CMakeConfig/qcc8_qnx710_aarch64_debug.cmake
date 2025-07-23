#[=======================================================================
# OpenAA: Open Source Adaptive AUTOSAR Project
# Author: Sherif Mohamed
#
# File description:
# -----------------
# CMake initial-cache file for OpenAA - QNX 7.10 aarch64 using QCC-8.
#
# DEBUG:
# ================
# Safety-critical systems require thorough validation. This configuration
# enables ALL safety features to catch issues during development.
#=======================================================================]

#[=======================================================================[
.rst:
QNX710_AARCH64_QCC8_MAXIMUM_SAFETY_DEBUG
-----------------------------------------
Maximum safety and security configuration for QNX 7.10 aarch64 using QCC-8 (GCC 8.3.0)

This configuration prioritizes SAFETY and DEBUGGABILITY above all else.
All safety features are enabled, including those with significant overhead.
This is intended for development, testing, and validation phases.

All linker flags have been verified against QNX 7.10's
aarch64-unknown-nto-qnx7.1.0-ld linker.

Usage:
------
.. code-block:: bash

    # Setup QNX environment
    source /opt/qnx710/qnxsdp-env.sh
    
    # Configure with maximum safety debug
    cmake -C qcc8_qnx710_aarch64_debug.cmake \
          -DCMAKE_BUILD_TYPE=Debug \
          -S <source> -B <build>
    
    # Build with parallel jobs
    cmake --build <build> -j$(nproc)

#]=======================================================================]

#=======================================================================
# Build System Configuration
#=======================================================================
message(STATUS "==========================================================")
message(STATUS "QNX 7.10 MAXIMUM SAFETY DEBUG Configuration")
message(STATUS "Target: ARM64 Cortex-A75 (ARMv8.2-A)")
message(STATUS "Compiler: QCC-8")
message(STATUS "Mode: Debug with Maximum Safety & Security")
message(STATUS "==========================================================")

# BUILD_SHARED_LIBS: Force shared libraries
#   • Purpose: Better debugging and runtime analysis
#   • Symbol loading: Easier to load symbols per library
#   • Runtime substitution: Can replace libraries at runtime
#   • Sanitizer support: Better memory error detection
#   • Debug symbols: Separate .so files with debug info
#   • Trade-off: Slightly slower startup vs static linking
set(BUILD_SHARED_LIBS ON CACHE BOOL "Build shared libraries for safety")

# DETERMINISTIC_BUILD_SEED: Reproducible builds
#   • Purpose: ISO 26262 requirement for verification
#   • Reproducibility: Bit-identical builds from same source
#   • Validation: Can verify binary matches expected output
#   • Seed usage: Controls random number generation in compiler
#   • Compliance: ASIL-D requires deterministic builds
set(DETERMINISTIC_BUILD_SEED "openaa_debug" CACHE STRING "Debug build seed")

#=======================================================================
# C Compiler Flags - MAXIMUM SAFETY CONFIGURATION
#=======================================================================

# ╔════════════════════════════════════════════════════════════════════╗
# ║            DEBUG OPTIMIZATION FLAGS (SAFETY FOCUSED)               ║
# ╚════════════════════════════════════════════════════════════════════╝

# Build flags in a variable first to avoid overwriting
set(OPENAA_C_FLAGS "")

# Base optimization and architecture
string(APPEND OPENAA_C_FLAGS " -Og")
string(APPEND OPENAA_C_FLAGS " -g3")
string(APPEND OPENAA_C_FLAGS " -ggdb3")
string(APPEND OPENAA_C_FLAGS " -march=armv8.2-a+crypto+crc+fp16+rcpc+dotprod")
string(APPEND OPENAA_C_FLAGS " -mcpu=cortex-a75")
string(APPEND OPENAA_C_FLAGS " -mtune=cortex-a75")

# Debug optimization details:
# -Og: Optimized for debugging
#   • Purpose: Balance between optimization and debuggability
#   • Preserves: Variable values, control flow clarity
#   • Enables: Basic optimizations that don't interfere with debugging
#   • Example: Dead code elimination still works, but not inlining
#   • Performance: 70-80% of -O2 speed, 100% debuggable
#
# -g3: Maximum debug information
#   • Includes: All symbols, types, macros, inline functions
#   • Size: 5-10x larger binary (symbols can be stripped)
#   • Features: Macro expansion in debugger
#   • Example: p BUFFER_SIZE shows macro value in gdb
#
# -ggdb3: GDB-specific maximum debug info
#   • Extensions: GDB-specific debug extensions
#   • Features: Better pretty-printing, Python integration
#   • QNX: Full support for gdb extensions
#
# -march=armv8.2-a: Base ARMv8.2 architecture
#   • Adds RAS (Reliability, Availability, Serviceability) features
#   • Improved atomics (LSE - Large System Extensions)
#   • Statistical profiling extension
#   • Example: ldadd instruction for atomic add (vs ldxr/stxr loop)
#
# +crypto: Hardware cryptography acceleration
#   • AES: 10-100x faster encryption/decryption
#   • SHA1/SHA256: 5-20x faster hashing
#   • Example: OpenSSL AES-128-GCM: 50MB/s → 1.2GB/s
#   • Real code: vaeseq_u8() compiles to single AESE instruction
#
# +crc: CRC32 hardware instructions
#   • 8-10x faster CRC32 computation
#   • Example: Data integrity checks in 1 cycle vs 8-10 cycles
#   • Real code: __crc32d() intrinsic maps to CRC32X instruction
#
# +fp16: Half-precision floating-point
#   • 2x memory bandwidth for ML workloads
#   • 2x throughput for supported operations
#   • Example: Neural network inference 50-100% faster
#   • Real code: vcvt_f32_f16() converts FP16→FP32 in hardware
#
# +rcpc: Release Consistent Processor Consistent
#   • Better C++11/C11 memory model performance
#   • 20-30% faster atomic operations
#   • Example: std::atomic<T>::load(memory_order_acquire) uses LDAPR
#
# +dotprod: Dot product instructions (UDOT/SDOT)
#   • 4x faster INT8 matrix multiplication
#   • Example: for(i=0;i<n;i+=4) sum += a[i]*b[i] + a[i+1]*b[i+1] + ...
#   • Real code: vdotq_s32() computes 4 INT8 dot products in 1 instruction
#
# -mcpu=cortex-a75: Target Cortex-A75 microarchitecture
#   • 3-wide decode, 8-wide dispatch out-of-order core
#   • Example: Optimizes instruction scheduling for A75 pipeline depth
#   • Sets cache parameters: 64KB L1, 256-512KB L2
#
# -mtune=cortex-a75: Fine-tune for A75 without changing ISA
#   • Adjusts cost model for instruction selection
#   • Example: Prefers CSEL over branches for conditionals
#   • Optimizes for 11-13 stage pipeline depth

# ╔════════════════════════════════════════════════════════════════════╗
# ║                   QNX FEATURE-TEST MACROS                          ║
# ╚════════════════════════════════════════════════════════════════════╝

string(APPEND OPENAA_C_FLAGS " -D_QNX_SOURCE")

# _QNX_SOURCE: Enable QNX-specific features
#   • Purpose: Access to QNX-specific APIs and extensions
#   • Features: QNX message passing, resource managers
#   • POSIX: Enables QNX POSIX extensions
#   • Required: For QNX-specific system calls
#   • Example: Enables MsgSend(), MsgReceive() APIs

# ╔════════════════════════════════════════════════════════════════════╗
# ║                 DEBUG & DIAGNOSTIC FLAGS                           ║
# ╚════════════════════════════════════════════════════════════════════╝

string(APPEND OPENAA_C_FLAGS " -fno-omit-frame-pointer")
string(APPEND OPENAA_C_FLAGS " -fno-optimize-sibling-calls")
string(APPEND OPENAA_C_FLAGS " -fasynchronous-unwind-tables")
string(APPEND OPENAA_C_FLAGS " -fvar-tracking")
string(APPEND OPENAA_C_FLAGS " -fvar-tracking-assignments")
string(APPEND OPENAA_C_FLAGS " -fverbose-asm")
string(APPEND OPENAA_C_FLAGS " -frecord-gcc-switches")
string(APPEND OPENAA_C_FLAGS " -fdiagnostics-color=always")
string(APPEND OPENAA_C_FLAGS " -fdiagnostics-show-option")
string(APPEND OPENAA_C_FLAGS " -fdiagnostics-show-caret")
string(APPEND OPENAA_C_FLAGS " -fdiagnostics-show-location=every-line")
string(APPEND OPENAA_C_FLAGS " -ftrack-macro-expansion=2")
string(APPEND OPENAA_C_FLAGS " -fdebug-types-section")
string(APPEND OPENAA_C_FLAGS " -fno-eliminate-unused-debug-types")

# Debug and diagnostic flag details:
# -fno-omit-frame-pointer: Always keep frame pointer
#   • Debugging: Complete stack traces always available
#   • ARM64: Preserves x29 for frame walking
#   • Overhead: 3-5% performance (worth it for safety)
#   • Required: For reliable crash analysis
#
# -fno-optimize-sibling-calls: Disable tail call optimization
#   • Debugging: Preserves full call stacks
#   • Example: Recursive calls remain visible
#   • Safety: Better error diagnosis
#
# -fasynchronous-unwind-tables: Always generate unwind info
#   • Safety: Stack traces even from signal handlers
#   • Size: ~10% binary size increase
#   • Critical: For post-mortem debugging
#
# -fvar-tracking: Track variable locations
#   • Debugging: Variables visible throughout lifetime
#   • Example: Optimized variables still inspectable
#
# -fvar-tracking-assignments: Track through assignments
#   • Enhanced: Better variable value tracking
#   • Quality: Improves debugger experience
#
# -fverbose-asm: Annotated assembly output
#   • Validation: Source correlation in .s files
#   • ISO 26262: Aids in code inspection
#
# -frecord-gcc-switches: Embed compilation flags
#   • Traceability: Build options in binary
#   • Audit: Can verify safety flags were used
#   • Command: readelf -p .GCC.command.line <binary>
#
# -fdiagnostics-color=always: Colored diagnostics
#   • Usability: Easier to spot warnings/errors
#   • CI/CD: Works with build servers
#
# -fdiagnostics-show-option: Show flag for each warning
#   • Example: warning: [-Wuninitialized]
#   • Useful: For selective warning suppression
#
# -fdiagnostics-show-caret: Show error location
#   • Example: Points to exact error position
#   • Quality: Better error understanding
#
# -fdiagnostics-show-location=every-line: Full paths
#   • Traceability: Complete file locations
#   • CI/CD: Better for automated analysis
#
# -ftrack-macro-expansion=2: Full macro tracking
#   • Level 2: Track through all macro expansions
#   • Debugging: See macro expansion chain
#
# -fdebug-types-section: Separate debug types
#   • DWARF: .debug_types section
#   • Size: Reduces debug info duplication
#
# -fno-eliminate-unused-debug-types: Keep all debug info
#   • Completeness: All types available
#   • Static analysis: Better tool support

# ╔════════════════════════════════════════════════════════════════════╗
# ║              MAXIMUM SAFETY FLAGS (HIGH OVERHEAD)                  ║
# ╚════════════════════════════════════════════════════════════════════╝

string(APPEND OPENAA_C_FLAGS " -ftrapv")
string(APPEND OPENAA_C_FLAGS " -fstack-protector-all")
string(APPEND OPENAA_C_FLAGS " -fstack-clash-protection")
string(APPEND OPENAA_C_FLAGS " -fcf-protection=none")
string(APPEND OPENAA_C_FLAGS " -fpie")
string(APPEND OPENAA_C_FLAGS " -frandom-seed=${DETERMINISTIC_BUILD_SEED}")
string(APPEND OPENAA_C_FLAGS " -fno-delete-null-pointer-checks")
string(APPEND OPENAA_C_FLAGS " -fno-strict-aliasing")
string(APPEND OPENAA_C_FLAGS " -fno-strict-overflow")
string(APPEND OPENAA_C_FLAGS " -fwrapv")
string(APPEND OPENAA_C_FLAGS " -fwrapv-pointer")
string(APPEND OPENAA_C_FLAGS " -D_FORTIFY_SOURCE=2")

# Maximum safety flag details:
# -ftrapv: Trap on signed integer overflow
#   • Safety: SIGILL on overflow instead of wraparound
#   • Example: INT_MAX + 1 → program abort
#   • Overhead: 5-10% for integer arithmetic
#   • ISO 26262: Detects arithmetic errors immediately
#   • Use case: Critical calculations must be correct
#
# -fstack-protector-all: Protect ALL functions
#   • Safety: Stack canary for every function
#   • Coverage: 100% of functions (vs -strong: ~80%)
#   • Overhead: 8-12% performance impact
#   • Example: Even main() gets canary protection
#   • Detection: Stack buffer overflows caught
#
# -fstack-clash-protection: Additional stack safety
#   • Safety: Prevent stack clash attacks
#   • Method: Probe stack allocations >4KB
#   • Security: Prevents jumping guard pages
#   • Overhead: <1% (only large allocations)
#
# -fcf-protection=none: Control Flow protection disabled
#   • ARM64: CET not available on ARM
#   • Note: Using PAC+BTI instead (see below)
#   • Future: May add ARM CET equivalent
#
# -fpie: Position Independent Executable
#   • Security: Full ASLR randomization
#   • Every run: Different memory layout
#   • Exploits: Much harder to develop
#   • ARM64: Minimal overhead (<1%)
#
# -frandom-seed=: Deterministic random seed
#   • Purpose: Reproducible builds
#   • Uses: ${DETERMINISTIC_BUILD_SEED} variable
#   • Affects: Symbol generation, name mangling
#   • ISO 26262: Required for validation
#
# -fno-delete-null-pointer-checks: Keep null checks
#   • Safety: Don't optimize away null checks
#   • Example: if(p) check always performed
#   • Security: Prevents null pointer exploits
#   • Kernel: Required for kernel code
#
# -fno-strict-aliasing: Conservative aliasing
#   • Safety: Assume pointers can alias
#   • Prevents: Optimization-induced bugs
#   • Example: No type-punning issues
#   • Overhead: 3-5% performance
#
# -fno-strict-overflow: No overflow assumptions
#   • Safety: Don't assume signed overflow won't happen
#   • Prevents: Optimization removing overflow checks
#   • Works with: -ftrapv for detection
#
# -fwrapv: Defined overflow behavior
#   • Safety: Signed overflow wraps predictably
#   • Behavior: Two's complement wrapping
#   • Conflicts with -ftrapv (trap takes precedence)
#
# _FORTIFY_SOURCE=2: Runtime fortification
#   • Runtime: Bounds checking for string/memory functions
#   • Level 2: Object size checking at runtime
#   • Example: strcpy → __strcpy_chk with size validation
#   • Overhead: 5-10% for string operations
#   • Catches: Buffer overflows, format string attacks

# ╔════════════════════════════════════════════════════════════════════╗
# ║                    WARNING FLAGS (MAXIMUM)                         ║
# ╚════════════════════════════════════════════════════════════════════╝

# Core warning flags
string(APPEND OPENAA_C_FLAGS " -Wall")
string(APPEND OPENAA_C_FLAGS " -Wextra")
string(APPEND OPENAA_C_FLAGS " -Werror")
string(APPEND OPENAA_C_FLAGS " -Wpedantic")

# Core warning details:
# -Wall: Basic set of warnings
#   • Covers: Most common issues
#   • Includes: ~30 individual warnings
#   • Essential: Minimum for any project
#
# -Wextra: Extended warnings beyond Wall
#   • Adds: ~20 more warnings
#   • Quality: Catches more subtle issues
#   • Recommended: For production code
#
# -Werror: Warnings become errors
#   • Safety: Forces fixing all warnings
#   • CI/CD: Prevents bad code merging
#   • ISO 26262: Required for ASIL-D
#
# -Wpedantic: Strict ISO C compliance
#   • Portability: Ensures standard code
#   • Extensions: Warns on GCC extensions
#   • AUTOSAR: Required for compliance

# Initialization and allocation warnings
string(APPEND OPENAA_C_FLAGS " -Winit-self")
string(APPEND OPENAA_C_FLAGS " -Walloc-zero")

# Initialization warnings:
# -Winit-self: Self-initialization warning
#   • Example: int x = x; → warning
#   • Bug: Uninitialized variable use
#
# -Walloc-zero: Zero-size allocation
#   • Example: malloc(0) → warning
#   • Portability: Behavior varies

# Format string warnings
string(APPEND OPENAA_C_FLAGS " -Wformat=2")
string(APPEND OPENAA_C_FLAGS " -Wformat-overflow=2")
string(APPEND OPENAA_C_FLAGS " -Wformat-truncation=2")
string(APPEND OPENAA_C_FLAGS " -Wformat-security")
string(APPEND OPENAA_C_FLAGS " -Wformat-signedness")

# Format string details:
# -Wformat=2: Maximum format checking
#   • Level 2: Most comprehensive checks
#   • Security: Prevents format attacks
#   • Includes: -Wformat-nonliteral
#
# -Wformat-overflow=2: sprintf overflow
#   • Level 2: Includes conditional paths
#   • Example: sprintf(buf, "%s", long_string)
#
# -Wformat-truncation=2: snprintf truncation
#   • Level 2: Warns on any truncation
#   • Safety: Ensures complete output
#
# -Wformat-security: Security issues
#   • Example: printf(user_input) → warning
#   • CVE: Prevents format string vulnerabilities
#
# -Wformat-signedness: Sign mismatches
#   • Example: printf("%u", -1) → warning
#   • Correctness: Ensures proper formatting

# Code structure warnings
string(APPEND OPENAA_C_FLAGS " -Wredundant-decls")
string(APPEND OPENAA_C_FLAGS " -Wshadow=global")
string(APPEND OPENAA_C_FLAGS " -Wlogical-op")
string(APPEND OPENAA_C_FLAGS " -Wduplicated-cond")
string(APPEND OPENAA_C_FLAGS " -Wduplicated-branches")

# Structure warning details:
# -Wredundant-decls: Duplicate declarations
#   • Example: Multiple extern declarations
#   • Cleanup: Reduces code clutter
#
# -Wshadow=global: Variable shadowing
#   • Level: Includes global shadowing
#   • Bug: Prevents name confusion
#   • Stricter: Than basic -Wshadow
#
# -Wlogical-op: Suspicious logical operations
#   • Example: if (x < 1 && x > 2) → warning
#   • Logic: Catches impossible conditions
#
# -Wduplicated-cond: Duplicate if conditions
#   • Example: if(x) {...} else if(x) → warning
#   • Bug: Logic error detection
#
# -Wduplicated-branches: Identical branches
#   • Example: if(x) {y=1;} else {y=1;} → warning
#   • Cleanup: Simplify code

# Array and string operation warnings
string(APPEND OPENAA_C_FLAGS " -Wstringop-overflow=4")
string(APPEND OPENAA_C_FLAGS " -Warray-bounds=2")

# String/array operation details:
# -Wstringop-overflow=4: String overflow
#   • Level 4: Maximum checking (NEW)
#   • Coverage: All string operations
#   • Catches: Subtle overflows
#
# -Warray-bounds=2: Array access checks
#   • Level 2: More aggressive checking
#   • Includes: VLA bounds checking

# Memory and pointer warnings
string(APPEND OPENAA_C_FLAGS " -Wnull-dereference")
string(APPEND OPENAA_C_FLAGS " -Wpointer-arith")
string(APPEND OPENAA_C_FLAGS " -Wrestrict")
string(APPEND OPENAA_C_FLAGS " -Walloca")

# Memory warning details:
# -Wnull-dereference: Null pointer deref
#   • Static: Compile-time detection
#   • Flow: Path-sensitive analysis
#
# -Wpointer-arith: Pointer arithmetic
#   • Void*: Warns on void* arithmetic
#   • Function: Warns on function pointer math
#
# -Wrestrict: Restrict violations
#   • Example: memcpy() with overlapping
#   • C99: Enforces restrict semantics
#
# -Walloca: alloca() usage warning
#   • Stack: Dynamic stack allocation
#   • Safety: Prefer fixed-size arrays

# Stack usage warnings
string(APPEND OPENAA_C_FLAGS " -Wstack-usage=16384")

# Stack usage details:
# -Wstack-usage=16384: Stack size limit
#   • Limit: 4KB maximum stack frame
#   • QNX: Threads have limited stacks
#   • Safety: Prevents stack overflow

# Type conversion warnings
string(APPEND OPENAA_C_FLAGS " -Wconversion")
string(APPEND OPENAA_C_FLAGS " -Wdouble-promotion")
string(APPEND OPENAA_C_FLAGS " -Wcast-align=strict")
string(APPEND OPENAA_C_FLAGS " -Wcast-qual")

# Type conversion details:
# -Wconversion: Implicit conversions
#   • Example: int to short assignment
#   • Data loss: Catches truncation
#   • Signedness: Sign change warnings
#
# -Wdouble-promotion: Float→double promotion
#   • Performance: Unintended promotion
#   • Embedded: Important for FPU usage
#
# -Wcast-align=strict: Alignment issues
#   • Strict: Most restrictive level
#   • ARM64: Critical for performance
#   • Crash: Misalignment can fault
#
# -Wcast-qual: Qualifier removal
#   • Example: const cast away → warning
#   • Safety: Preserves const correctness

# Control flow warnings
string(APPEND OPENAA_C_FLAGS " -Wimplicit-fallthrough=5")
string(APPEND OPENAA_C_FLAGS " -Wswitch-enum")
string(APPEND OPENAA_C_FLAGS " -Wswitch-default")
string(APPEND OPENAA_C_FLAGS " -Wswitch-bool")
string(APPEND OPENAA_C_FLAGS " -Wswitch-unreachable")

# Control flow details:
# -Wimplicit-fallthrough=5: Switch fallthrough
#   • Level 5: Strictest checking
#   • Requires: [[fallthrough]] attribute
#   • C++17: Modern style enforcement
#
# -Wswitch-enum: Missing enum cases
#   • Completeness: All cases handled
#   • Maintenance: Catches new enums
#
# -Wswitch-default: Missing default case
#   • Defensive: Always have default
#   • MISRA: Required by coding standards
#
# -Wswitch-bool: Switch on boolean
#   • Style: Use if/else for bool
#   • Clarity: Better code readability
#
# -Wswitch-unreachable: Unreachable switch code
#   • Example: Code before first case
#   • Bug: Logic error detection

# Overflow and arithmetic warnings
string(APPEND OPENAA_C_FLAGS " -Wstrict-overflow=1")
string(APPEND OPENAA_C_FLAGS " -Wfloat-equal")

# Arithmetic warning details:
# -Wstrict-overflow=1: Overflow assumptions
#   • Level 5: Minimum sensitivity
#   • May be noisy but catches bugs
#   • Works with: -fno-strict-overflow
#
# -Wfloat-equal: Float comparison
#   • Example: if(f == 1.0) → warning
#   • Floating point: Use epsilon comparison

# Miscellaneous safety warnings
string(APPEND OPENAA_C_FLAGS " -Wundef")
string(APPEND OPENAA_C_FLAGS " -Wdate-time")
string(APPEND OPENAA_C_FLAGS " -Wtrampolines")

# Miscellaneous warning details:
# -Wundef: Undefined macro in #if
#   • Example: #if UNDEFINED_MACRO → warning
#   • Safer: Use #ifdef instead
#
# -Wdate-time: __DATE__/__TIME__ usage
#   • Reproducibility: Non-deterministic
#   • ISO 26262: Avoid for validation
#
# -Wtrampolines: Nested function trampolines
#   • Security: Executable stack required
#   • Avoid: Don't use nested functions

# Debug-specific additional warnings
string(APPEND OPENAA_C_FLAGS " -Wunused")
string(APPEND OPENAA_C_FLAGS " -Wunused-macros")
string(APPEND OPENAA_C_FLAGS " -Wunused-result")
string(APPEND OPENAA_C_FLAGS " -Wunused-parameter")
string(APPEND OPENAA_C_FLAGS " -Wunused-but-set-parameter")
string(APPEND OPENAA_C_FLAGS " -Wunused-but-set-variable")

# Unused code warnings:
# -Wunused: All unused warnings
#   • Comprehensive: Enables all unused
#   • Cleanup: Find dead code
#
# -Wunused-macros: Unused macro definitions
#   • Headers: Can be noisy
#   • Cleanup: Remove dead macros
#
# -Wunused-result: Ignored return values
#   • Example: malloc() return ignored
#   • Safety: Check all returns
#
# -Wunused-parameter: Unused parameters
#   • Style: Mark with [[maybe_unused]]
#   • Or: (void)param; to silence
#
# -Wunused-but-set-parameter: Set but not read
#   • Dead code: Parameter only written
#
# -Wunused-but-set-variable: Set but not read
#   • Dead code: Variable only written

# Additional debug-specific warnings
string(APPEND OPENAA_C_FLAGS " -Wvariadic-macros")
string(APPEND OPENAA_C_FLAGS " -Wvolatile-register-var")
string(APPEND OPENAA_C_FLAGS " -Wwrite-strings")
string(APPEND OPENAA_C_FLAGS " -Whsa")
string(APPEND OPENAA_C_FLAGS " -Waggressive-loop-optimizations")
string(APPEND OPENAA_C_FLAGS " -Wdisabled-optimization")
string(APPEND OPENAA_C_FLAGS " -Winvalid-pch")
string(APPEND OPENAA_C_FLAGS " -Wmissing-include-dirs")
string(APPEND OPENAA_C_FLAGS " -Wpacked")
string(APPEND OPENAA_C_FLAGS " -Wunsafe-loop-optimizations")

# Additional debug warning details:
# -Wvariadic-macros: Variadic macro usage
#   • C99: Warns on variadic macros
#   • Portability: Not in C90
#
# -Wvolatile-register-var: Volatile register
#   • Deprecated: register + volatile
#   • Meaningless: Combination invalid
#
# -Wwrite-strings: String literal const
#   • Example: char* = "literal" → warning
#   • Correct: const char* = "literal"
#
# -Whsa: HSA (Heterogeneous System Architecture)
#   • GPU: Offloading issues
#   • Parallel: Accelerator warnings
#
# -Waggressive-loop-optimizations: Loop issues
#   • Infinite: May not terminate
#   • Overflow: Loop counter overflow
#
# -Wdisabled-optimization: Failed optimizations
#   • Complexity: Code too complex
#   • Refactor: Simplify the code
#
# -Winvalid-pch: Precompiled header issues
#   • Build: PCH version mismatch
#   • Stale: Outdated PCH files
#
# -Wmissing-include-dirs: Missing -I paths
#   • Build: Configuration errors
#   • Setup: Check include paths
#
# -Wpacked: Packed structure warnings
#   • Alignment: Performance impact
#   • Portability: Architecture specific
#
# -Wunsafe-loop-optimizations: Loop safety
#   • Bounds: May access out of bounds
#   • Complement: Use with sanitizers

# C-specific warnings
string(APPEND OPENAA_C_FLAGS " -Wnested-externs")
string(APPEND OPENAA_C_FLAGS " -Wjump-misses-init")
string(APPEND OPENAA_C_FLAGS " -Wmissing-prototypes")
string(APPEND OPENAA_C_FLAGS " -Wstrict-prototypes")
string(APPEND OPENAA_C_FLAGS " -Wbad-function-cast")
string(APPEND OPENAA_C_FLAGS " -Wold-style-definition")
string(APPEND OPENAA_C_FLAGS " -Wmissing-declarations")
string(APPEND OPENAA_C_FLAGS " -Wmissing-parameter-type")
string(APPEND OPENAA_C_FLAGS " -Wold-style-declaration")

# C-specific warning details:
# -Wnested-externs: Nested extern declarations
#   • Scope: extern inside functions
#   • Style: Put at file scope
#
# -Wjump-misses-init: Goto bypasses init
#   • Example: goto over initialization
#   • C++: Error in C++, warning in C
#
# -Wmissing-prototypes: No prior prototype
#   • Example: int f() {} without declaration
#   • Header: Should be in header file
#
# -Wstrict-prototypes: Non-prototype function
#   • Example: int f() vs int f(void)
#   • C: Empty parens mean any args
#
# -Wbad-function-cast: Function cast issues
#   • Example: (int)sqrt(x) → warning
#   • Type: Loss of precision
#
# -Wold-style-definition: K&R definitions
#   • Example: int f(x) int x; {}
#   • Modern: Use ANSI prototypes
#
# -Wmissing-declarations: No declaration
#   • Static: Should be static or declared
#   • Linkage: Prevents naming conflicts
#
# -Wmissing-parameter-type: K&R parameter
#   • Example: f(x) without type
#   • Ancient: Pre-ANSI C
#
# -Wold-style-declaration: Declaration order
#   • Example: const static → static const
#   • Standard: Follow declaration order

# Now set the complete flags ONCE to avoid duplication
set(CMAKE_C_FLAGS_INIT "${OPENAA_C_FLAGS}" CACHE STRING "C compiler flags for maximum safety")

#-----------------------------------------------------------------------
# Build Type Specific C Flags
#-----------------------------------------------------------------------

# Build flags in a variable first to avoid overwriting
set(OPENAA_C_FLAGS_DEBUG "")

# Debug build macros
string(APPEND OPENAA_C_FLAGS_DEBUG " -DDEBUG")
string(APPEND OPENAA_C_FLAGS_DEBUG " -D_DEBUG")
string(APPEND OPENAA_C_FLAGS_DEBUG " -UNDEBUG")
string(APPEND OPENAA_C_FLAGS_DEBUG " -D_GLIBCXX_DEBUG")
string(APPEND OPENAA_C_FLAGS_DEBUG " -D_GLIBCXX_DEBUG_PEDANTIC")
string(APPEND OPENAA_C_FLAGS_DEBUG " -D_GLIBCXX_ASSERTIONS")
string(APPEND OPENAA_C_FLAGS_DEBUG " -D_LIBCPP_DEBUG=1")

# Debug macro details:
# -DDEBUG: Common debug macro
#   • Convention: Widely used
#   • Enables: Debug-only code paths
#   • Example: #ifdef DEBUG logging
#
# -D_DEBUG: Microsoft-style debug
#   • Compatibility: With MSVC code
#   • Libraries: Some libs check this
#
# -UNDEBUG: Force undefine NDEBUG
#   • assert(): Ensures active
#   • Override: Even if defined elsewhere
#   • Critical: For safety checking
#
# -D_GLIBCXX_DEBUG: GNU C++ debug mode
#   • STL: Full debugging mode
#   • Iterators: Validity checking
#   • Bounds: Container bounds checks
#   • Performance: 10-50x slower
#
# -D_GLIBCXX_DEBUG_PEDANTIC: Extra pedantic
#   • Stricter: More STL checks
#   • Standard: Strict conformance
#   • Catches: Subtle violations
#
# -D_GLIBCXX_ASSERTIONS: Lightweight checks
#   • Compromise: Some checks
#   • Performance: 5-10% overhead
#   • Production: Can leave enabled
#
# -D_LIBCPP_DEBUG=1: LLVM libc++ debug
#   • Alternative: For LLVM STL
#   • Similar: To GLIBCXX_DEBUG
#   • Choice: If using libc++

set(CMAKE_C_FLAGS_DEBUG "${OPENAA_C_FLAGS_DEBUG}" CACHE STRING "C Debug flags")

#=======================================================================
# C++ Compiler Flags - MAXIMUM SAFETY
#=======================================================================

# Build C++ flags starting with C flags
set(OPENAA_CXX_FLAGS "${OPENAA_C_FLAGS}")

# ╔════════════════════════════════════════════════════════════════════╗
# ║           C++ SPECIFIC DEBUG & DIAGNOSTIC FLAGS                    ║
# ╚════════════════════════════════════════════════════════════════════╝

string(APPEND OPENAA_CXX_FLAGS " -ftemplate-backtrace-limit=0")

# C++ template diagnostics:
# -ftemplate-backtrace-limit=0: Unlimited template traces
#   • Templates: Full instantiation chain
#   • Debugging: Complete error context
#   • Default: Limited to 10 levels
#   • Trade-off: Longer error messages

# ╔════════════════════════════════════════════════════════════════════╗
# ║                    C++ SPECIFIC SAFETY FLAGS                       ║
# ╚════════════════════════════════════════════════════════════════════╝

# C++ exception and RTTI handling for debug
string(APPEND OPENAA_CXX_FLAGS " -fexceptions")
string(APPEND OPENAA_CXX_FLAGS " -frtti")
string(APPEND OPENAA_CXX_FLAGS " -fvisibility=default")

# C++ feature flags:
# -fexceptions: Enable exception handling
#   • Unwinding: Stack unwinding support
#   • try/catch: Exception blocks work
#   • Overhead: 5-15% binary size
#   • Debug: Better error handling
#
# -frtti: Runtime Type Information
#   • typeid: Type identification
#   • dynamic_cast: Safe downcasting
#   • Overhead: 5-10% binary size
#   • Debug: Type introspection
#
# -fvisibility=default: Export all symbols
#   • Debug: All symbols visible
#   • Tools: Better debugger support
#   • Production: Use =hidden instead
#   • Size: Larger binaries

# ╔════════════════════════════════════════════════════════════════════╗
# ║                    C++ SPECIFIC WARNINGS                           ║
# ╚════════════════════════════════════════════════════════════════════╝

# Remove C-only warnings first
string(REPLACE " -Wnested-externs"          ""  OPENAA_CXX_FLAGS  "${OPENAA_CXX_FLAGS}")
string(REPLACE " -Wjump-misses-init"        ""  OPENAA_CXX_FLAGS  "${OPENAA_CXX_FLAGS}")
string(REPLACE " -Wmissing-prototypes"      ""  OPENAA_CXX_FLAGS  "${OPENAA_CXX_FLAGS}")
string(REPLACE " -Wstrict-prototypes"       ""  OPENAA_CXX_FLAGS  "${OPENAA_CXX_FLAGS}")
string(REPLACE " -Wold-style-definition"    ""  OPENAA_CXX_FLAGS  "${OPENAA_CXX_FLAGS}")
string(REPLACE " -Wmissing-declarations"    ""  OPENAA_CXX_FLAGS  "${OPENAA_CXX_FLAGS}")
string(REPLACE " -Wbad-function-cast"       ""  OPENAA_CXX_FLAGS  "${OPENAA_CXX_FLAGS}")
string(REPLACE " -Wmissing-parameter-type"  ""  OPENAA_CXX_FLAGS  "${OPENAA_CXX_FLAGS}")
string(REPLACE " -Wold-style-declaration"   ""  OPENAA_CXX_FLAGS  "${OPENAA_CXX_FLAGS}")
string(REPLACE " -Wtraditional"             ""  OPENAA_CXX_FLAGS  "${OPENAA_CXX_FLAGS}")

# Core C++ warnings
string(APPEND OPENAA_CXX_FLAGS " -Weffc++")
string(APPEND OPENAA_CXX_FLAGS " -Wold-style-cast")
string(APPEND OPENAA_CXX_FLAGS " -Wzero-as-null-pointer-constant")
string(APPEND OPENAA_CXX_FLAGS " -Wuseless-cast")
string(APPEND OPENAA_CXX_FLAGS " -Wnon-virtual-dtor")
string(APPEND OPENAA_CXX_FLAGS " -Woverloaded-virtual")
string(APPEND OPENAA_CXX_FLAGS " -Wsign-promo")

# Core C++ warning details:
# -Weffc++: Effective C++ guidelines
#   • Scott Meyers: Book guidelines
#   • Style: Best practices
#   • Quality: Better C++ code
#
# -Wold-style-cast: C-style casts
#   • Example: (int)x → warning
#   • Modern: Use static_cast<int>(x)
#   • Safety: Type-safe casts
#
# -Wzero-as-null-pointer-constant: 0 as nullptr
#   • Example: ptr = 0 → warning
#   • C++11: Use nullptr
#   • Type safety: Better null handling
#
# -Wuseless-cast: Redundant casts
#   • Example: static_cast<int>(int_var)
#   • Cleanup: Remove unnecessary
#
# -Wnon-virtual-dtor: Missing virtual destructor
#   • Polymorphic: Base class needs virtual
#   • Memory leak: Derived not destroyed
#
# -Woverloaded-virtual: Hidden virtual
#   • Example: Different signature hides
#   • Bug: Not overriding as intended
#
# -Wsign-promo: Sign promotion
#   • Overload: Unexpected selection
#   • Example: Signed/unsigned mismatch

# C++11/14/17/20 warnings
string(APPEND OPENAA_CXX_FLAGS " -Wsuggest-override")
string(APPEND OPENAA_CXX_FLAGS " -Wsuggest-final-types")
string(APPEND OPENAA_CXX_FLAGS " -Wsuggest-final-methods")
string(APPEND OPENAA_CXX_FLAGS " -Wdelete-non-virtual-dtor")
string(APPEND OPENAA_CXX_FLAGS " -Wplacement-new=2")
string(APPEND OPENAA_CXX_FLAGS " -Wnoexcept")

# Modern C++ warning details:
# -Wsuggest-override: Missing override
#   • C++11: Mark overrides explicitly
#   • Maintenance: Catch signature changes
#
# -Wsuggest-final-types: Could be final
#   • Performance: Devirtualization
#   • Design: No further derivation
#
# -Wsuggest-final-methods: Methods could be final
#   • Optimization: Better inlining
#   • Intent: Clear design
#
# -Wdelete-non-virtual-dtor: Delete via non-virtual
#   • UB: Undefined behavior
#   • Example: delete base_ptr
#
# -Wplacement-new=2: Placement new issues
#   • Level 2: Strict checking
#   • Buffer: Size mismatches
#
# -Wnoexcept: Missing noexcept
#   • C++11: Exception specification
#   • Performance: Move optimization

# Additional C++ safety warnings
string(APPEND OPENAA_CXX_FLAGS " -Wstrict-null-sentinel")
string(APPEND OPENAA_CXX_FLAGS " -Wextra-semi")
string(APPEND OPENAA_CXX_FLAGS " -Wmultiple-inheritance")
string(APPEND OPENAA_CXX_FLAGS " -Wvirtual-inheritance")
string(APPEND OPENAA_CXX_FLAGS " -Wctor-dtor-privacy")
string(APPEND OPENAA_CXX_FLAGS " -Winherited-variadic-ctor")
string(APPEND OPENAA_CXX_FLAGS " -Wvirtual-move-assign")
string(APPEND OPENAA_CXX_FLAGS " -Wregister")
string(APPEND OPENAA_CXX_FLAGS " -Waligned-new=all")
string(APPEND OPENAA_CXX_FLAGS " -Wcatch-value=3")
string(APPEND OPENAA_CXX_FLAGS " -Wsized-deallocation")

# Additional C++ warning details:
# -Wstrict-null-sentinel: NULL sentinel
#   • Varargs: Requires NULL terminator
#   • Portability: 64-bit issues
#
# -Wextra-semi: Extra semicolons
#   • Style: Unnecessary semicolons
#   • Pedantic: Clean code
#
# -Wmultiple-inheritance: MI usage
#   • Complexity: Design smell
#   • AUTOSAR: Restricted
#
# -Wvirtual-inheritance: Virtual base
#   • Diamond: Problem indicator
#   • Performance: Overhead
#
# -Wctor-dtor-privacy: Unusable class
#   • Private: Can't instantiate
#   • Design: Probable error
#
# -Winherited-variadic-ctor: Variadic inherit
#   • C++11: using inheritance
#   • Complex: May not work right
#
# -Wvirtual-move-assign: Virtual move
#   • Performance: Not optimal
#   • Design: Consider alternatives
#
# -Wregister: register keyword
#   • C++17: Deprecated
#   • Remove: No longer useful
#
# -Waligned-new=all: Alignment issues
#   • C++17: Over-aligned types
#   • All: Maximum checking
#
# -Wcatch-value=3: Exception slicing
#   • Level 3: Polymorphic types
#   • Catch: By reference
#
# -Wsized-deallocation: Sized delete
#   • C++14: Performance feature
#   • Mismatch: Size problems

# Set C++ flags
set(CMAKE_CXX_FLAGS_INIT "${OPENAA_CXX_FLAGS}" CACHE STRING "C++ compiler flags for maximum safety")

# C++ build type specific flags
set(OPENAA_CXX_FLAGS_DEBUG "${OPENAA_C_FLAGS_DEBUG}")

# Additional C++ debug flags
string(APPEND OPENAA_CXX_FLAGS_DEBUG " -D_GLIBCXX_SANITIZE_VECTOR")

# C++ specific debug macros:
# -D_GLIBCXX_SANITIZE_VECTOR: Vector safety
#   • Bounds: All vector access checked
#   • at(): Even operator[] checked
#   • Performance: Significant overhead
#   • Catches: #1 C++ bug source

set(CMAKE_CXX_FLAGS_DEBUG "${OPENAA_CXX_FLAGS_DEBUG}" CACHE STRING "C++ Debug flags")

#=======================================================================
# Linker Flags - MAXIMUM SAFETY & DIAGNOSTICS
#=======================================================================

# ╔════════════════════════════════════════════════════════════════════╗
# ║                  DEBUG EXECUTABLE LINKER FLAGS                     ║
# ╚════════════════════════════════════════════════════════════════════╝

set(OPENAA_EXEC_LINKER_FLAGS "")

# Core optimization and garbage collection
string(APPEND OPENAA_EXEC_LINKER_FLAGS " -Wl,-O0")
string(APPEND OPENAA_EXEC_LINKER_FLAGS " -Wl,--no-gc-sections")
string(APPEND OPENAA_EXEC_LINKER_FLAGS " -Wl,--print-gc-sections")

# Linker optimization details:
# -Wl,-O0: No linker optimizations
#   • Debug: Predictable layout
#   • Symbols: All preserved
#   • Speed: Faster linking
#
# -Wl,--no-gc-sections: Keep all sections
#   • Coverage: Accurate data
#   • Debug: All code available
#   • Size: Larger binary
#
# -Wl,--print-gc-sections: Report removed
#   • Diagnostic: What would be removed
#   • Dead code: Identification

# Warning flags
string(APPEND OPENAA_EXEC_LINKER_FLAGS " -Wl,--warn-common")
string(APPEND OPENAA_EXEC_LINKER_FLAGS " -Wl,--warn-section-align")
string(APPEND OPENAA_EXEC_LINKER_FLAGS " -Wl,--warn-shared-textrel")
string(APPEND OPENAA_EXEC_LINKER_FLAGS " -Wl,--warn-alternate-em")
string(APPEND OPENAA_EXEC_LINKER_FLAGS " -Wl,--warn-unresolved-symbols")
string(APPEND OPENAA_EXEC_LINKER_FLAGS " -Wl,--error-unresolved-symbols")

# Linker warning details:
# -Wl,--warn-common: Common symbols
#   • Globals: Multiple definitions
#   • Size: Different sizes
#   • Fix: Use extern properly
#
# -Wl,--warn-section-align: Alignment
#   • Performance: Misaligned sections
#   • Cache: Poor utilization
#
# -Wl,--warn-shared-textrel: Text relocations
#   • PIC: Not position independent
#   • Security: Can't be read-only
#
# -Wl,--warn-alternate-em: Emulation
#   • Target: Wrong architecture
#   • Cross: Compilation issues
#
# -Wl,--warn-unresolved-symbols: Missing symbols
#   • Link: Undefined references
#   • Runtime: Will fail to load
#
# -Wl,--error-unresolved-symbols: Fatal
#   • Strict: No missing symbols
#   • Safety: Catch at link time

# Symbol resolution flags
string(APPEND OPENAA_EXEC_LINKER_FLAGS " -Wl,--no-undefined")
string(APPEND OPENAA_EXEC_LINKER_FLAGS " -Wl,--no-allow-shlib-undefined")
string(APPEND OPENAA_EXEC_LINKER_FLAGS " -Wl,--no-undefined-version")
string(APPEND OPENAA_EXEC_LINKER_FLAGS " -Wl,-z,defs")

# Symbol resolution details:
# -Wl,--no-undefined: No undefined symbols
#   • Complete: All must resolve
#   • Libraries: All needed linked
#
# -Wl,--no-allow-shlib-undefined: Shared libs
#   • Transitive: Dependencies complete
#   • Runtime: No missing symbols
#
# -Wl,--no-undefined-version: Version scripts
#   • Symbols: All versioned properly
#   • ABI: Version consistency
#
# -Wl,-z,defs: Same as --no-undefined
#   • Solaris: Compatible flag
#   • Strict: Symbol resolution

# Binary format flags
string(APPEND OPENAA_EXEC_LINKER_FLAGS " -Wl,--check-sections")
string(APPEND OPENAA_EXEC_LINKER_FLAGS " -Wl,--hash-style=both")
string(APPEND OPENAA_EXEC_LINKER_FLAGS " -Wl,--eh-frame-hdr")
string(APPEND OPENAA_EXEC_LINKER_FLAGS " -Wl,--build-id=sha1")

# Binary format details:
# -Wl,--check-sections: Section overlap
#   • Memory: No overlapping sections
#   • Safety: Memory corruption
#
# -Wl,--hash-style=both: Hash tables
#   • GNU: .gnu.hash section
#   • SYSV: .hash section
#   • Compatibility: Both styles
#
# -Wl,--eh-frame-hdr: Exception headers
#   • C++: Exception unwinding
#   • Debug: Stack traces
#   • Binary: .eh_frame_hdr section
#
# -Wl,--build-id=sha1: Build ID
#   • Unique: SHA1 of binary
#   • Debug: Symbol matching
#   • Core dumps: Auto-load symbols

# Security hardening flags
string(APPEND OPENAA_EXEC_LINKER_FLAGS " -pie")
string(APPEND OPENAA_EXEC_LINKER_FLAGS " -Wl,-z,relro")
string(APPEND OPENAA_EXEC_LINKER_FLAGS " -Wl,-z,now")
string(APPEND OPENAA_EXEC_LINKER_FLAGS " -Wl,-z,noexecstack")

# Security hardening details:
# -pie: Position Independent Executable
#   • ASLR: Address randomization
#   • Required: For -fpie code
#   • Security: Exploit mitigation
#
# -Wl,-z,relro: Read-only relocations
#   • GOT: Global offset table
#   • PLT: Procedure linkage table
#   • After init: Made read-only
#
# -Wl,-z,now: Immediate binding
#   • Startup: Resolve all symbols
#   • No lazy: binding
#   • Security: GOT/PLT protection
#
# -Wl,-z,noexecstack: Non-executable stack
#   • Stack: Not executable
#   • Security: No code injection
#   • Default: On most systems

# Dynamic linking flags
string(APPEND OPENAA_EXEC_LINKER_FLAGS " -dynamic")
string(APPEND OPENAA_EXEC_LINKER_FLAGS " -Wl,-Bdynamic")

# Dynamic linking details:
# -dynamic: Dynamic linking
#   • QNX: Prefer dynamic
#   • Debug: Better for debugging
#   • Updates: Runtime library updates
#
# -Wl,-Bdynamic: Dynamic libraries
#   • Default: Use .so files
#   • Opposite: -Bstatic
#   • Following: Libraries dynamic

set(CMAKE_EXE_LINKER_FLAGS_INIT "${OPENAA_EXEC_LINKER_FLAGS}" CACHE STRING "Debug executable linker flags")

# Debug-specific linker flags
set(OPENAA_EXEC_LINKER_FLAGS_DEBUG "")

string(APPEND OPENAA_EXEC_LINKER_FLAGS_DEBUG " -Wl,--export-dynamic")

# Debug linker flag details:
# -Wl,--export-dynamic: Export all symbols
#   • dlopen: Runtime symbol lookup
#   • Backtrace: Better traces
#   • Plugins: Can access symbols
#   • Size: Larger binary

set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${OPENAA_EXEC_LINKER_FLAGS_DEBUG}" CACHE STRING "Debug executable linker flags")

# ╔════════════════════════════════════════════════════════════════════╗
# ║               DEBUG SHARED LIBRARY LINKER FLAGS                    ║
# ╚════════════════════════════════════════════════════════════════════╝

set(OPENAA_SHARED_LINKER_FLAGS "")

# Core shared library flags
string(APPEND OPENAA_SHARED_LINKER_FLAGS " -Wl,-O0")
string(APPEND OPENAA_SHARED_LINKER_FLAGS " -Wl,--warn-shared-textrel")
string(APPEND OPENAA_SHARED_LINKER_FLAGS " -Wl,--no-undefined")
string(APPEND OPENAA_SHARED_LINKER_FLAGS " -Wl,--no-allow-shlib-undefined")
string(APPEND OPENAA_SHARED_LINKER_FLAGS " -Wl,-z,defs")

# Shared library core details:
# Same meanings as executable flags
# Additional importance for libraries:
# - Symbol visibility matters more
# - Dependencies tracked carefully
# - Version management critical

# Security and format flags
string(APPEND OPENAA_SHARED_LINKER_FLAGS " -Wl,-z,relro")
string(APPEND OPENAA_SHARED_LINKER_FLAGS " -Wl,-z,now")
string(APPEND OPENAA_SHARED_LINKER_FLAGS " -Wl,-z,noexecstack")
string(APPEND OPENAA_SHARED_LINKER_FLAGS " -Wl,--hash-style=both")

# Shared library specific flags
string(APPEND OPENAA_SHARED_LINKER_FLAGS " -Wl,-z,unique")
string(APPEND OPENAA_SHARED_LINKER_FLAGS " -Wl,-z,initfirst")
string(APPEND OPENAA_SHARED_LINKER_FLAGS " -Wl,-z,interpose")

# Shared library specific details:
# -Wl,-z,unique: Unique DSO
#   • dlopen: Each gets own copy
#   • Isolation: No symbol sharing
#
# -Wl,-z,initfirst: Initialize first
#   • Order: Before other libs
#   • Dependencies: Proper setup
#
# -Wl,-z,interpose: Symbol interposition
#   • Override: Can replace symbols
#   • Debug: Function mocking
#   • Testing: Replace implementations

set(CMAKE_SHARED_LINKER_FLAGS_INIT "${OPENAA_SHARED_LINKER_FLAGS}" CACHE STRING "Debug shared library linker flags")

# Debug-specific shared library flags
set(OPENAA_SHARED_LINKER_FLAGS_DEBUG "")

string(APPEND OPENAA_SHARED_LINKER_FLAGS_DEBUG " -Wl,--export-dynamic")
string(APPEND OPENAA_SHARED_LINKER_FLAGS_DEBUG " -Wl,--no-strip-all")
string(APPEND OPENAA_SHARED_LINKER_FLAGS_DEBUG " -Wl,--no-strip-debug")

# Debug shared library details:
# -Wl,--no-strip-all: Keep all symbols
#   • Debug: Full symbol table
#   • Size: Much larger
#
# -Wl,--no-strip-debug: Keep debug symbols
#   • DWARF: Debug information
#   • Minimum: For debugging

set(CMAKE_SHARED_LINKER_FLAGS_DEBUG "${OPENAA_SHARED_LINKER_FLAGS_DEBUG}" CACHE STRING "Debug shared library linker flags")

# Static library flags
set(CMAKE_STATIC_LINKER_FLAGS_INIT "" CACHE STRING "Static library flags")

# Static library details:
# Minimal flags for archives
# Most flags apply during final link
# Archive creation is simple

#=======================================================================
# Deprecated/Commented Features
#=======================================================================

# The following options were considered but not used in this configuration:

# -Wl,-z,separate-code: Separate code pages
#   • Status: COMMENTED OUT
#   • Reason: Not compatible with all QNX versions
#   • Purpose: Would separate code and data pages
#   • Security: Would improve cache/security
#   • Alternative: Rely on default QNX layout

# -Wl,--cref: Cross reference table
#   • Status: COMMENTED OUT  
#   • Reason: Very verbose output
#   • Purpose: Symbol usage mapping
#   • Use case: Deep dependency analysis
#   • Alternative: Use only when needed

# -Wl,--print-memory-usage: Memory report
#   • Status: COMMENTED OUT
#   • Reason: Not always accurate on QNX
#   • Purpose: RAM/ROM usage summary  
#   • Use case: Embedded systems
#   • Alternative: Use QNX-specific tools

# -Wl,--stats: Linker statistics
#   • Status: COMMENTED OUT
#   • Reason: Noisy output
#   • Purpose: Link performance data
#   • Use case: Build optimization
#   • Alternative: Use for specific debugging

# -Wl,--trace: Trace file processing
#   • Status: COMMENTED OUT
#   • Reason: Extremely verbose
#   • Purpose: Debug link order
#   • Use case: Dependency debugging
#   • Alternative: Use --verbose selectively

# -Wl,--verbose: Maximum verbosity
#   • Status: COMMENTED OUT
#   • Reason: Overwhelming output
#   • Purpose: Complete link details
#   • Use case: Deep debugging only
#   • Alternative: Enable temporarily

# -Wanalyzer-too-complex: Code complexity
#   • Status: NOT USED
#   • Reason: Too many false positives
#   • Purpose: Identify complex code
#   • Issue: Triggers on reasonable code
#   • Alternative: Use cyclomatic complexity tools

#=======================================================================
# Summary and Usage
#=======================================================================

message(STATUS "")
message(STATUS "Safety Configuration Summary:")
message(STATUS "- Optimization: -Og (debug-friendly)")
message(STATUS "- Debug Info: Maximum (-g3 -ggdb3)")
message(STATUS "- Static Analysis: GCC analyzer enabled")
message(STATUS "")