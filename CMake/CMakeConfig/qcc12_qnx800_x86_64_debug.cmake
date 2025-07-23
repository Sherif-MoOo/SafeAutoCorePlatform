#[=======================================================================
# OpenAA: Open Source Adaptive AUTOSAR Project
# Author: Sherif Mohamed
#
# File description:
# -----------------
# CMake initial-cache file for OpenAA - QNX 8.0 x86_64 using QCC-12.
#
# DEBUG:
# ================
# Safety-critical systems require thorough validation. This configuration
# enables ALL safety features to catch issues during development.
#=======================================================================]

#[=======================================================================[
.rst:
QNX80_X86_64_QCC12_MAXIMUM_SAFETY_DEBUG
----------------------------------------
Maximum safety and security configuration for QNX 8.0 x86_64 using QCC-12 (GCC 12.2.0)

This configuration prioritizes SAFETY and DEBUGGABILITY above all else.
All safety features are enabled, including those with significant overhead.
This is intended for development, testing, and validation phases.

All linker flags have been verified against QNX 8.0's 
x86_64-pc-nto-qnx8.0.0-ld linker.

Usage:
------
.. code-block:: bash

    # Setup QNX environment
    source /opt/qnx800/qnxsdp-env.sh
    
    # Configure with maximum safety
    cmake -C qcc12_qnx80_x86_64_debug.cmake \
          -DCMAKE_BUILD_TYPE=Debug \
          -S <source> -B <build>
    
    # Build with parallel jobs
    cmake --build <build> -j$(nproc)

#]=======================================================================]

#=======================================================================
# Build System Configuration
#=======================================================================
message(STATUS "==========================================================")
message(STATUS "QNX 8.0 MAXIMUM SAFETY DEBUG Configuration")
message(STATUS "Target: x86_64 Generic (x86-64-v1 baseline)")
message(STATUS "Compiler: QCC-12")
message(STATUS "Focus: Safety, Security, and Debuggability")
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
string(APPEND OPENAA_C_FLAGS " -march=x86-64")
string(APPEND OPENAA_C_FLAGS " -mtune=generic")
string(APPEND OPENAA_C_FLAGS " -mfpmath=sse")

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
# -march=x86-64: Generic x86-64 baseline (x86-64-v1)
#   • Baseline ISA: SSE2 required by x86-64 ABI
#   • Features: MMX, SSE, SSE2, FXSR, CMPXCHG8B
#   • Compatible with: All x86-64 processors (2003+)
#   • Example: Core 2, Athlon 64, and newer
#   • Trade-off: Misses newer instructions but runs everywhere
#
# -mtune=generic: Optimize for common x86_64 processors
#   • Balances: Performance across Intel/AMD architectures
#   • Targets: Mix of recent processors (Haswell to Zen 3)
#   • Scheduling: Average latencies and throughputs
#   • Cache: Assumes 64-byte lines, moderate sizes
#   • Example: Good performance on both Xeon and EPYC
#
# -mfpmath=sse: Use SSE for floating-point (default on x86_64)
#   • Performance: 2-4x faster than x87 FPU
#   • Precision: IEEE-754 compliant single/double
#   • Registers: 16 XMM registers vs 8 x87 stack
#   • Example: addsd xmm0, xmm1 vs fadd st(1), st
#   • Consistency: Same as release builds

# Additional x86_64 baseline features
string(APPEND OPENAA_C_FLAGS " -msse2")
string(APPEND OPENAA_C_FLAGS " -mfxsr")
string(APPEND OPENAA_C_FLAGS " -mcx16")

# x86_64 baseline feature details:
# -msse2: Streaming SIMD Extensions 2 (required by ABI)
#   • Vectors: 128-bit operations on integers/floats
#   • Instructions: MOVDQA, PADDD, PMULLW, etc.
#   • Example: Process 4 floats or 2 doubles at once
#   • Debug benefit: Consistent with production
#
# -mfxsr: Fast save/restore of FPU state
#   • Performance: Faster context switches
#   • Instructions: FXSAVE/FXRSTOR
#   • Saves: FPU, MMX, SSE state in one operation
#   • Debug: Better signal handler debugging
#
# -mcx16: CMPXCHG16B instruction
#   • Atomics: 128-bit compare-and-swap
#   • Example: Lock-free data structures
#   • Available: All x86-64 processors except early AMD
#   • Debug: Atomic operation debugging

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
string(APPEND OPENAA_C_FLAGS " -mno-omit-leaf-frame-pointer")
string(APPEND OPENAA_C_FLAGS " -fno-optimize-sibling-calls")
string(APPEND OPENAA_C_FLAGS " -fasynchronous-unwind-tables")
string(APPEND OPENAA_C_FLAGS " -funwind-tables")
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
#   • x86_64: Preserves RBP for frame walking
#   • Overhead: 5-7% performance (worth it for safety)
#   • Required: For reliable crash analysis
#   • Critical: x86_64 has fewer registers than ARM
#
# -mno-omit-leaf-frame-pointer: Frame pointer in leaf functions
#   • x86_64 specific: Even simple functions get frame
#   • Complete: No missing frames in traces
#   • Tools: Better profiler support
#
# -fno-optimize-sibling-calls: Disable tail call optimization
#   • Debugging: Preserves full call stacks
#   • Example: Recursive calls remain visible
#   • Safety: Better error diagnosis
#   • x86_64: JMP → CALL for debugging
#
# -fasynchronous-unwind-tables: Always generate unwind info
#   • Safety: Stack traces even from signal handlers
#   • Size: ~10% binary size increase
#   • Critical: For post-mortem debugging
#   • x86_64: .eh_frame sections
#
# -funwind-tables: Generate unwind tables
#   • Complement: Works with async tables
#   • Exception: C++ exception support
#   • Backtrace: From any point
#
# -fvar-tracking: Track variable locations
#   • Debugging: Variables visible throughout lifetime
#   • Example: Optimized variables still inspectable
#   • x86_64: Through register moves
#
# -fvar-tracking-assignments: Track through assignments
#   • Enhanced: Better variable value tracking
#   • Quality: Improves debugger experience
#   • Example: Track through SSA form
#
# -fverbose-asm: Annotated assembly output
#   • Validation: Source correlation in .s files
#   • ISO 26262: Aids in code inspection
#   • x86_64: Intel syntax option available
#
# -frecord-gcc-switches: Embed compilation flags
#   • Traceability: Build options in binary
#   • Audit: Can verify safety flags were used
#   • Command: readelf -p .GCC.command.line <binary>
#
# -fdiagnostics-color=always: Colored diagnostics
#   • Usability: Easier to spot warnings/errors
#   • CI/CD: Works with build servers
#   • Terminal: ANSI color codes
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
string(APPEND OPENAA_C_FLAGS " -fcf-protection=full")
string(APPEND OPENAA_C_FLAGS " -fpie")
string(APPEND OPENAA_C_FLAGS " -frandom-seed=${DETERMINISTIC_BUILD_SEED}")
string(APPEND OPENAA_C_FLAGS " -fno-delete-null-pointer-checks")
string(APPEND OPENAA_C_FLAGS " -ftrivial-auto-var-init=pattern")
string(APPEND OPENAA_C_FLAGS " -fno-strict-aliasing")
string(APPEND OPENAA_C_FLAGS " -fno-strict-overflow")
string(APPEND OPENAA_C_FLAGS " -fwrapv")
string(APPEND OPENAA_C_FLAGS " -fwrapv-pointer")
string(APPEND OPENAA_C_FLAGS " -D_FORTIFY_SOURCE=2")
string(APPEND OPENAA_C_FLAGS " -D_GLIBCXX_ASSERTIONS")
string(APPEND OPENAA_C_FLAGS " -D_GLIBCXX_DEBUG")
string(APPEND OPENAA_C_FLAGS " -D_GLIBCXX_DEBUG_PEDANTIC")

# Maximum safety flag details:
# -ftrapv: Trap on signed integer overflow
#   • Safety: SIGILL on overflow instead of wraparound
#   • Example: INT_MAX + 1 → program abort
#   • Overhead: 5-10% for integer arithmetic
#   • ISO 26262: Detects arithmetic errors immediately
#   • Use case: Critical calculations must be correct
#   • x86_64: Uses INTO instruction or JO branches
#
# -fstack-protector-all: Protect ALL functions
#   • Safety: Stack canary for every function
#   • Coverage: 100% of functions (vs -strong: ~80%)
#   • Overhead: 8-12% performance impact
#   • Example: Even main() gets canary protection
#   • Detection: Stack buffer overflows caught
#   • x86_64: FS segment for canary storage
#
# -fstack-clash-protection: Additional stack safety
#   • Safety: Prevent stack clash attacks
#   • Method: Probe stack allocations >4KB
#   • Security: Prevents jumping guard pages
#   • Overhead: <1% (only large allocations)
#   • x86_64: OR/TEST probing pattern
#
# -fcf-protection=full: Intel CET protection
#   • Hardware: Shadow stack + indirect branch tracking
#   • Shadow Stack: Return address protection
#   • IBT: ENDBR64 at valid targets
#   • Compatibility: Safe NOP on older CPUs
#   • Tiger Lake+: Hardware acceleration
#   • Debug: Compatible with debuggers
#
# -fpie: Position Independent Executable
#   • Security: Full ASLR randomization
#   • Every run: Different memory layout
#   • Exploits: Much harder to develop
#   • x86_64: ~3% overhead (GOT/PLT usage)
#   • Debug: Addresses change each run
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
# -ftrivial-auto-var-init=pattern: Pattern initialization
#   • Safety: All variables initialized to 0xFE pattern
#   • Debug: Uninitialized use obvious (not zero)
#   • Example: int x; // Contains 0xFEFEFEFE
#   • Overhead: 1-3% runtime
#   • Better than =zero: Catches more bugs
#   • x86_64: REP STOSB for init
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
# -fwrapv-pointer: NEW in GCC 12 - Pointer wrapping
#   • Safety: Pointer arithmetic wraps predictably
#   • Prevents: Optimization assuming no pointer wrap
#   • Security: More predictable behavior
#
# _FORTIFY_SOURCE= 2 
#   • Runtime: Dynamic object size checking
#   • Level 2: Comprehensive checking
#   • Covers: String, memory, I/O operations
#   • Example: strcpy → __strcpy_chk with size validation
#   • Overhead: 5-10% for string operations
#   • x86_64: Uses __builtin_object_size
#
# _GLIBCXX_ASSERTIONS: STL assertions
#   • Containers: Bounds checking
#   • Iterators: Basic validity
#   • Overhead: 5-10% for STL code
#
# _GLIBCXX_DEBUG: Full STL debug mode
#   • Complete: All STL safety checks
#   • Iterator: Full validity tracking
#   • Overhead: 50-100% for STL
#
# _GLIBCXX_DEBUG_PEDANTIC: Extra pedantic
#   • Strictest: Most thorough checks
#   • Standard: Full conformance

# ╔════════════════════════════════════════════════════════════════════╗
# ║         HARDWARE SECURITY FEATURES (x86_64 Intel/AMD)              ║
# ╚════════════════════════════════════════════════════════════════════╝

string(APPEND OPENAA_C_FLAGS " -mshstk")
string(APPEND OPENAA_C_FLAGS " -mindirect-branch-register")

# Hardware security details:
# -mshstk: Shadow stack instructions
#   • Hardware: Shadow stack operations
#   • WRSS: Write shadow stack
#   • INCSSP: Increment shadow pointer
#
# -mindirect-branch-register: Force register
#   • Variant: Additional protection
#   • No memory: Indirect through register
#   • Harder: To exploit

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
string(APPEND OPENAA_C_FLAGS " -Wstringop-overread")
string(APPEND OPENAA_C_FLAGS " -Warray-bounds=2")
string(APPEND OPENAA_C_FLAGS " -Warray-parameter=2")

# String/array operation details:
# -Wstringop-overflow=4: String overflow
#   • Level 4: Maximum checking (NEW)
#   • Coverage: All string operations
#   • Catches: Subtle overflows
#
# -Wstringop-overread: Reading past end
#   • NEW: GCC 11+ feature
#   • Example: strlen() on unterminated string
#
# -Warray-bounds=2: Array access checks
#   • Level 2: More aggressive checking
#   • Includes: VLA bounds checking
#
# -Warray-parameter=2: Parameter mismatches
#   • Level 2: Strict array parameter checking
#   • Example: void f(int a[10]) called with int[5]

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
string(APPEND OPENAA_C_FLAGS " -Wvla-larger-than=0")
string(APPEND OPENAA_C_FLAGS " -Wframe-larger-than=16384")

# Stack usage details:
# -Wstack-usage=16384: Stack size limit
#   • Limit: 4KB maximum stack frame
#   • QNX: Threads have limited stacks
#   • Safety: Prevents stack overflow
#
# -Wvla-larger-than=0: Ban all VLAs
#   • Zero: Prohibits any VLA
#   • ISO 26262: VLAs forbidden in ASIL-D
#   • Alternative: Use fixed arrays
#
# -Wframe-larger-than=16384: Frame size limit
#   • x86_64: Additional frame check
#   • Complement: With stack-usage
#   • Stricter: For critical functions

# Type conversion warnings
string(APPEND OPENAA_C_FLAGS " -Wconversion")
string(APPEND OPENAA_C_FLAGS " -Wsign-conversion")
string(APPEND OPENAA_C_FLAGS " -Wdouble-promotion")
string(APPEND OPENAA_C_FLAGS " -Wcast-align=strict")
string(APPEND OPENAA_C_FLAGS " -Wcast-qual")

# Type conversion details:
# -Wconversion: Implicit conversions
#   • Example: int to short assignment
#   • Data loss: Catches truncation
#   • x86_64: 32→16, 64→32 conversions
#
# -Wsign-conversion: Signedness changes
#   • Example: unsigned = signed
#   • Subtle: Can cause bugs
#   • Separate: From -Wconversion
#
# -Wdouble-promotion: Float→double promotion
#   • Performance: Unintended promotion
#   • x86_64: SSE vs x87 issues
#   • Example: Use sqrtf not sqrt
#
# -Wcast-align=strict: Alignment issues
#   • Strict: Most restrictive level
#   • x86_64: Unaligned access slower
#   • SIMD: Critical for SSE/AVX
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
string(APPEND OPENAA_C_FLAGS " -Wjump-misses-init")

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
#
# -Wjump-misses-init: Goto skips init
#   • C specific: Important for C
#   • Example: goto over initialization

# Overflow and arithmetic warnings
string(APPEND OPENAA_C_FLAGS " -Wstrict-overflow=5")
string(APPEND OPENAA_C_FLAGS " -Wfloat-equal")
string(APPEND OPENAA_C_FLAGS " -Wfloat-conversion")

# Arithmetic warning details:
# -Wstrict-overflow=5: Overflow assumptions
#   • Level 5: Maximum sensitivity
#   • May be noisy but catches bugs
#   • Works with: -fno-strict-overflow
#
# -Wfloat-equal: Float comparison
#   • Example: if(f == 1.0) → warning
#   • Floating point: Use epsilon comparison
#   • x86_64: x87 vs SSE precision
#
# -Wfloat-conversion: Float to int
#   • Example: int = 3.14 → warning
#   • Precision: Loss of fractional

# Miscellaneous safety warnings
string(APPEND OPENAA_C_FLAGS " -Wundef")
string(APPEND OPENAA_C_FLAGS " -Wdate-time")
string(APPEND OPENAA_C_FLAGS " -Wtrampolines")
string(APPEND OPENAA_C_FLAGS " -Wattribute-alias=2")
string(APPEND OPENAA_C_FLAGS " -Wbidi-chars=any")
string(APPEND OPENAA_C_FLAGS " -Wopenacc-parallelism")
string(APPEND OPENAA_C_FLAGS " -Wzero-length-bounds")
string(APPEND OPENAA_C_FLAGS " -Wstringop-truncation")

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
#   • x86_64: More common than ARM
#
# -Wattribute-alias=2: Alias attribute issues
#   • Level 2: Stricter checking
#   • Linkage: Catches ABI mismatches
#
# -Wbidi-chars=any: Bidirectional Unicode
#   • Security: Trojan source attacks
#   • NEW: GCC 12 security feature
#
# -Wopenacc-parallelism: OpenACC issues
#   • Parallel: Race condition warnings
#   • GPU: Relevant for accelerators
#
# -Wzero-length-bounds: Zero-length arrays
#   • NEW: GCC 12 feature
#   • Bounds: Common overflow source
#
# -Wstringop-truncation: String truncation
#   • strncpy: Common source of bugs
#   • Buffer: Unterminated strings

# Debug-specific additional warnings
string(APPEND OPENAA_C_FLAGS " -Wunused")
string(APPEND OPENAA_C_FLAGS " -Wunused-macros")
string(APPEND OPENAA_C_FLAGS " -Wunused-result")
string(APPEND OPENAA_C_FLAGS " -Wunused-parameter")
string(APPEND OPENAA_C_FLAGS " -Wunused-but-set-parameter")
string(APPEND OPENAA_C_FLAGS " -Wunused-but-set-variable")
string(APPEND OPENAA_C_FLAGS " -Wunused-local-typedefs")
string(APPEND OPENAA_C_FLAGS " -Wunused-function")
string(APPEND OPENAA_C_FLAGS " -Wunused-label")
string(APPEND OPENAA_C_FLAGS " -Wunused-value")
string(APPEND OPENAA_C_FLAGS " -Wunused-variable")

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
#
# -Wunused-local-typedefs: Unused typedefs
#   • C++11: Can use [[maybe_unused]]
#
# -Wunused-function: Unused static functions
#   • Dead code: Remove or use
#
# -Wunused-label: Unused goto labels
#   • Cleanup: Remove dead labels
#
# -Wunused-value: Computed but not used
#   • Example: x + 1; (no assignment)
#
# -Wunused-variable: Unused variables
#   • Most common: Dead variable

# Additional debug-specific warnings
string(APPEND OPENAA_C_FLAGS " -Wvariadic-macros")
string(APPEND OPENAA_C_FLAGS " -Wvolatile-register-var")
string(APPEND OPENAA_C_FLAGS " -Wwrite-strings")
string(APPEND OPENAA_C_FLAGS " -Whsa")
string(APPEND OPENAA_C_FLAGS " -Waggressive-loop-optimizations")
string(APPEND OPENAA_C_FLAGS " -Wattribute-warning")
string(APPEND OPENAA_C_FLAGS " -Wdisabled-optimization")
string(APPEND OPENAA_C_FLAGS " -Winvalid-pch")
string(APPEND OPENAA_C_FLAGS " -Wmissing-include-dirs")
string(APPEND OPENAA_C_FLAGS " -Wpacked")
string(APPEND OPENAA_C_FLAGS " -Wunsafe-loop-optimizations")
string(APPEND OPENAA_C_FLAGS " -Wvector-operation-performance")
string(APPEND OPENAA_C_FLAGS " -Wdangling-else")
string(APPEND OPENAA_C_FLAGS " -Wmemset-elt-size")
string(APPEND OPENAA_C_FLAGS " -Wmemset-transposed-args")
string(APPEND OPENAA_C_FLAGS " -Wmisleading-indentation")
string(APPEND OPENAA_C_FLAGS " -Wnonnull-compare")
string(APPEND OPENAA_C_FLAGS " -Wshift-negative-value")
string(APPEND OPENAA_C_FLAGS " -Wshift-overflow=2")
string(APPEND OPENAA_C_FLAGS " -Wtautological-compare")

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
# -Wattribute-warning: Custom warnings
#   • User: [[gnu::warning("msg")]]
#   • Deprecation: Custom messages
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
#   • x86_64: Unaligned access penalty
#
# -Wunsafe-loop-optimizations: Loop safety
#   • Bounds: May access out of bounds
#   • Complement: Use with sanitizers
#
# -Wvector-operation-performance: Vector ops
#   • SIMD: Suboptimal vectorization
#   • x86_64: SSE/AVX issues
#
# -Wdangling-else: Ambiguous else
#   • Example: if(a) if(b) x; else y;
#   • Clarity: Use braces
#
# -Wmemset-elt-size: memset element size
#   • Example: memset(arr, 0, n) vs n*sizeof
#   • Common: Bug source
#
# -Wmemset-transposed-args: memset args
#   • Example: memset(p, size, value)
#   • Common: Argument order bug
#
# -Wmisleading-indentation: Bad indentation
#   • Python-like: Indentation matters
#   • Readability: Match logic
#
# -Wnonnull-compare: Nonnull comparison
#   • Attribute: [[gnu::nonnull]]
#   • Redundant: Comparison warned
#
# -Wshift-negative-value: Negative shift
#   • Example: -1 << 1
#   • Undefined: In C
#
# -Wshift-overflow=2: Shift too far
#   • Level 2: Stricter checking
#   • UB: Undefined behavior
#
# -Wtautological-compare: Always true/false
#   • Example: unsigned >= 0
#   • Logic: Redundant checks

# C-specific warnings
string(APPEND OPENAA_C_FLAGS " -Wnested-externs")
string(APPEND OPENAA_C_FLAGS " -Wmissing-prototypes")
string(APPEND OPENAA_C_FLAGS " -Wstrict-prototypes")
string(APPEND OPENAA_C_FLAGS " -Wbad-function-cast")
string(APPEND OPENAA_C_FLAGS " -Wold-style-definition")
string(APPEND OPENAA_C_FLAGS " -Wmissing-declarations")
string(APPEND OPENAA_C_FLAGS " -Wmissing-parameter-type")
string(APPEND OPENAA_C_FLAGS " -Wold-style-declaration")
string(APPEND OPENAA_C_FLAGS " -Wtraditional-conversion")
string(APPEND OPENAA_C_FLAGS " -Wdeclaration-after-statement")
string(APPEND OPENAA_C_FLAGS " -Wpointer-sign")

# C-specific warning details:
# -Wnested-externs: Nested extern declarations
#   • Scope: extern inside functions
#   • Style: Put at file scope
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
#
# -Wtraditional-conversion: Traditional C
#   • Prototype: Changes argument promotion
#   • Compatibility: With K&R C
#
# -Wdeclaration-after-statement: C89 compat
#   • C99: Mixed declarations allowed
#   • Style: Some prefer C89 style
#
# -Wpointer-sign: Pointer signedness
#   • Example: char* = unsigned char*
#   • Common: String handling issue

# ╔════════════════════════════════════════════════════════════════════╗
# ║                 STATIC ANALYSIS INTEGRATION                        ║
# ╚════════════════════════════════════════════════════════════════════╝

string(APPEND OPENAA_C_FLAGS " -fanalyzer")
string(APPEND OPENAA_C_FLAGS " -fanalyzer-checker=taint")
string(APPEND OPENAA_C_FLAGS " -Wanalyzer-double-fclose")
string(APPEND OPENAA_C_FLAGS " -Wanalyzer-double-free")
string(APPEND OPENAA_C_FLAGS " -Wanalyzer-exposure-through-output-file")
string(APPEND OPENAA_C_FLAGS " -Wanalyzer-file-leak")
string(APPEND OPENAA_C_FLAGS " -Wanalyzer-free-of-non-heap")
string(APPEND OPENAA_C_FLAGS " -Wanalyzer-malloc-leak")
string(APPEND OPENAA_C_FLAGS " -Wanalyzer-mismatching-deallocation")
string(APPEND OPENAA_C_FLAGS " -Wanalyzer-null-argument")
string(APPEND OPENAA_C_FLAGS " -Wanalyzer-null-dereference")
string(APPEND OPENAA_C_FLAGS " -Wanalyzer-possible-null-argument")
string(APPEND OPENAA_C_FLAGS " -Wanalyzer-possible-null-dereference")
string(APPEND OPENAA_C_FLAGS " -Wanalyzer-shift-count-negative")
string(APPEND OPENAA_C_FLAGS " -Wanalyzer-shift-count-overflow")
string(APPEND OPENAA_C_FLAGS " -Wanalyzer-stale-setjmp-buffer")
string(APPEND OPENAA_C_FLAGS " -Wanalyzer-tainted-allocation-size")
string(APPEND OPENAA_C_FLAGS " -Wanalyzer-tainted-array-index")
string(APPEND OPENAA_C_FLAGS " -Wanalyzer-tainted-divisor")
string(APPEND OPENAA_C_FLAGS " -Wanalyzer-tainted-offset")
string(APPEND OPENAA_C_FLAGS " -Wanalyzer-tainted-size")
string(APPEND OPENAA_C_FLAGS " -Wanalyzer-unsafe-call-within-signal-handler")
string(APPEND OPENAA_C_FLAGS " -Wanalyzer-use-after-free")
string(APPEND OPENAA_C_FLAGS " -Wanalyzer-use-of-pointer-in-stale-stack-frame")
string(APPEND OPENAA_C_FLAGS " -Wno-analyzer-use-of-uninitialized-value")
string(APPEND OPENAA_C_FLAGS " -Wanalyzer-write-to-const")
string(APPEND OPENAA_C_FLAGS " -Wanalyzer-write-to-string-literal")

# GCC Static Analyzer details:
# -fanalyzer: Enable static analysis
#   • Path-sensitive: Follows execution paths
#   • Interprocedural: Across functions
#   • State: Tracks program state
#   • Cost: 2-5x compile time
#   • x86_64: Full support
#
# -fanalyzer-checker=taint: Taint tracking
#   • Input: Tracks untrusted data
#   • Flow: Through program paths
#   • Security: Find injection bugs
#
# Resource management warnings:
# -Wanalyzer-double-fclose: Double fclose()
#   • Resource: File closed twice
#   • Undefined: Behavior undefined
#
# -Wanalyzer-double-free: Double free()
#   • Memory: Freed twice
#   • Security: Exploitable bug
#   • x86_64: Heap corruption
#
# -Wanalyzer-exposure-through-output-file: Info leak
#   • Security: Sensitive data in output
#   • Privacy: Unintended disclosure
#
# -Wanalyzer-file-leak: File descriptor leak
#   • Resource: Unclosed files
#   • Exhaustion: FD limit reached
#
# -Wanalyzer-free-of-non-heap: Invalid free
#   • Example: free(stack_var)
#   • Crash: Corrupts memory
#
# Memory safety warnings:
# -Wanalyzer-malloc-leak: Memory leak
#   • Resource: Allocated not freed
#   • Growth: Memory exhaustion
#
# -Wanalyzer-mismatching-deallocation: Wrong free
#   • Example: new/free mismatch
#   • C++: new/delete vs malloc/free
#
# -Wanalyzer-null-argument: NULL passed
#   • Example: strcpy(NULL, src)
#   • Crash: Segmentation fault
#
# -Wanalyzer-null-dereference: NULL deref
#   • Example: p->member when p=NULL
#   • Common: Most common C bug
#
# -Wanalyzer-possible-null-argument: Maybe NULL
#   • Conditional: Path-dependent NULL
#   • Example: After failed malloc
#
# -Wanalyzer-possible-null-dereference: Maybe NULL
#   • Path: Some paths have NULL
#   • Defense: Add NULL checks
#
# Arithmetic warnings:
# -Wanalyzer-shift-count-negative: Negative shift
#   • Example: x << -1
#   • Undefined: Behavior undefined
#
# -Wanalyzer-shift-count-overflow: Shift too large
#   • Example: x << 33 (32-bit int)
#   • Undefined: Behavior undefined
#
# -Wanalyzer-stale-setjmp-buffer: Invalid longjmp
#   • Stack: Buffer no longer valid
#   • Complex: setjmp/longjmp issues
#
# Taint analysis warnings:
# -Wanalyzer-tainted-allocation-size: Untrusted size
#   • Example: malloc(user_input)
#   • DoS: Denial of service
#
# -Wanalyzer-tainted-array-index: Untrusted index
#   • Example: array[user_input]
#   • Overflow: Buffer overflow
#
# -Wanalyzer-tainted-divisor: Division by tainted
#   • Example: x / user_input
#   • Crash: Division by zero
#
# -Wanalyzer-tainted-offset: Untrusted offset
#   • Example: ptr + user_input
#   • Memory: Out of bounds
#
# -Wanalyzer-tainted-size: Untrusted size
#   # Example: memcpy size from user
#   • Overflow: Buffer overflow
#
# Signal and special warnings:
# -Wanalyzer-unsafe-call-within-signal-handler: Signal safety
#   • Async: Non-async-safe function
#   • Example: printf() in handler
#
# -Wanalyzer-use-after-free: Use after free
#   • Memory: Accessing freed memory
#   • Security: Exploitable bug
#
# -Wanalyzer-use-of-pointer-in-stale-stack-frame: Stack pointer
#   • Return: Pointer to local
#   • Lifetime: Stack frame gone
#
# -Wno-analyzer-use-of-uninitialized-value: Disabled
#   • Noisy: Too many false positives
#   • Alternative: Use -ftrivial-auto-var-init
#
# -Wanalyzer-write-to-const: Const write
#   • Example: Modifying const data
#   • Undefined: Behavior undefined
#
# -Wanalyzer-write-to-string-literal: Literal write
#   • Example: "hello"[0] = 'H'
#   • Crash: Read-only memory

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
string(APPEND OPENAA_C_FLAGS_DEBUG " -D_LIBCPP_ENABLE_ASSERTIONS=1")

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
#
# -D_LIBCPP_ENABLE_ASSERTIONS=1: libc++ asserts
#   • Lightweight: Basic checks
#   • Alternative: To full debug

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
string(APPEND OPENAA_CXX_FLAGS " -fconstexpr-depth=512")
string(APPEND OPENAA_CXX_FLAGS " -fconstexpr-loop-limit=1048576")
string(APPEND OPENAA_CXX_FLAGS " -fconstexpr-ops-limit=100000000")
string(APPEND OPENAA_CXX_FLAGS " -ftemplate-depth=1024")

# C++ template diagnostics:
# -ftemplate-backtrace-limit=0: Unlimited template traces
#   • Templates: Full instantiation chain
#   • Debugging: Complete error context
#   • Default: Limited to 10 levels
#   • Trade-off: Longer error messages
#
# -fconstexpr-depth=512: Constexpr recursion
#   • Limit: 512 levels deep
#   • Default: Often 256
#   • Complex: Meta-programming
#
# -fconstexpr-loop-limit=1048576: Loop iterations
#   • Compile-time: Loops in constexpr
#   • Large: Algorithms possible
#
# -fconstexpr-ops-limit=100000000: Operations
#   • Compile-time: Operation count
#   • Complex: Computations allowed
#
# -ftemplate-depth=1024: Template instantiation
#   • Deep: Template recursion
#   • Meta: Programming support

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
string(REPLACE " -Wtraditional-conversion"  ""  OPENAA_CXX_FLAGS  "${OPENAA_CXX_FLAGS}")
string(REPLACE " -Wdeclaration-after-statement" ""  OPENAA_CXX_FLAGS  "${OPENAA_CXX_FLAGS}")
string(REPLACE " -Wpointer-sign"            ""  OPENAA_CXX_FLAGS  "${OPENAA_CXX_FLAGS}")

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
#   • AUTOSAR: Alignment with rules
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
string(APPEND OPENAA_CXX_FLAGS " -Wnoexcept-type")
string(APPEND OPENAA_CXX_FLAGS " -Wctad-maybe-unsupported")
string(APPEND OPENAA_CXX_FLAGS " -Wdeprecated-enum-enum-conversion")
string(APPEND OPENAA_CXX_FLAGS " -Wdeprecated-enum-float-conversion")
string(APPEND OPENAA_CXX_FLAGS " -Wvolatile")

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
#   • Alignment: x86_64 critical
#
# -Wnoexcept: Missing noexcept
#   • C++11: Exception specification
#   • Performance: Move optimization
#
# -Wnoexcept-type: Noexcept in type
#   • C++17: Part of type system
#   • Function pointers: Changed
#
# -Wctad-maybe-unsupported: CTAD issues
#   • C++17: Class template deduction
#   • Explicit: May need guides
#
# -Wdeprecated-enum-enum-conversion: Enum conversion
#   • C++20: Deprecated conversion
#   • Type safety: Enum class better
#
# -Wdeprecated-enum-float-conversion: Enum to float
#   • C++20: Deprecated conversion
#   • Suspicious: Rarely intended
#
# -Wvolatile: Volatile deprecations
#   • C++20: Many uses deprecated
#   • Modern: Use atomics instead

# Additional C++ safety warnings
string(APPEND OPENAA_CXX_FLAGS " -Wstrict-null-sentinel")
string(APPEND OPENAA_CXX_FLAGS " -Wextra-semi")
string(APPEND OPENAA_CXX_FLAGS " -Winvalid-imported-macros")
string(APPEND OPENAA_CXX_FLAGS " -Wmultiple-inheritance")
string(APPEND OPENAA_CXX_FLAGS " -Wvirtual-inheritance")
string(APPEND OPENAA_CXX_FLAGS " -Wctor-dtor-privacy")
string(APPEND OPENAA_CXX_FLAGS " -Winherited-variadic-ctor")
string(APPEND OPENAA_CXX_FLAGS " -Wvirtual-move-assign")
string(APPEND OPENAA_CXX_FLAGS " -Wregister")
string(APPEND OPENAA_CXX_FLAGS " -Wmismatched-tags")
string(APPEND OPENAA_CXX_FLAGS " -Wredundant-tags")
string(APPEND OPENAA_CXX_FLAGS " -Wdeprecated-copy")
string(APPEND OPENAA_CXX_FLAGS " -Wdeprecated-copy-dtor")
string(APPEND OPENAA_CXX_FLAGS " -Wpessimizing-move")
string(APPEND OPENAA_CXX_FLAGS " -Wredundant-move")
string(APPEND OPENAA_CXX_FLAGS " -Wrange-loop-construct")
string(APPEND OPENAA_CXX_FLAGS " -Waligned-new=all")
string(APPEND OPENAA_CXX_FLAGS " -Wcatch-value=3")
string(APPEND OPENAA_CXX_FLAGS " -Wsized-deallocation")
string(APPEND OPENAA_CXX_FLAGS " -Wcomma-subscript")
string(APPEND OPENAA_CXX_FLAGS " -Wclass-conversion")
string(APPEND OPENAA_CXX_FLAGS " -Wexceptions")
string(APPEND OPENAA_CXX_FLAGS " -Winit-list-lifetime")
string(APPEND OPENAA_CXX_FLAGS " -Wliteral-suffix")
string(APPEND OPENAA_CXX_FLAGS " -Wmismatched-new-delete")
string(APPEND OPENAA_CXX_FLAGS " -Wnarrowing")
string(APPEND OPENAA_CXX_FLAGS " -Wplacement-new")
string(APPEND OPENAA_CXX_FLAGS " -Wterminate")
string(APPEND OPENAA_CXX_FLAGS " -Wvexing-parse")

# Additional C++ warning details:
# -Wstrict-null-sentinel: NULL sentinel
#   • Varargs: Requires NULL terminator
#   • Portability: 64-bit issues
#
# -Wextra-semi: Extra semicolons
#   • Style: Unnecessary semicolons
#   • Pedantic: Clean code
#
# -Winvalid-imported-macros: Module macros
#   • C++20: Module macro issues
#   • Modules: Import problems
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
#   • C++17: Removed
#   • Error: In C++17+
#
# -Wmismatched-tags: struct/class
#   • Example: class X; struct X {}
#   • Consistency: Pick one
#
# -Wredundant-tags: Unnecessary tags
#   • Example: struct X x; → X x;
#   • C++: Tags not needed
#
# -Wdeprecated-copy: Copy operations
#   • Rule of 5: Define all or none
#   • C++11: Deprecated pattern
#
# -Wdeprecated-copy-dtor: Related to above
#   • Destructor: With copy ops
#   • Modern: Follow rule of 5
#
# -Wpessimizing-move: Bad std::move
#   • Example: return std::move(local)
#   • NRVO: Prevents optimization
#
# -Wredundant-move: Unnecessary move
#   • Example: return std::move(temp)
#   • Automatic: Already moved
#
# -Wrange-loop-construct: Range-for issues
#   • Example: for(auto x : vec) copies
#   • Better: for(const auto& x : vec)
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
#
# -Wcomma-subscript: Comma in []
#   • C++20: Deprecated
#   • Confusing: a[1,2] → a[2]
#
# Additional C++20+ warnings:
# -Wclass-conversion: Class conversions
#   • Implicit: User-defined
#   • Explicit: Better practice
#
# -Wexceptions: Exception issues
#   • Spec: Mismatches
#   • noexcept: Violations
#
# -Winit-list-lifetime: Initializer lists
#   • Dangling: References
#   • Temporary: Lifetime issues
#
# -Wliteral-suffix: User literals
#   • Spacing: Required
#   • Example: 123s vs 123 s
#
# -Wmismatched-new-delete: Allocation
#   • new[]: Needs delete[]
#   • Mismatch: UB
#
# -Wnarrowing: Narrowing conversions
#   • C++11: In {} init
#   • Example: int{3.14}
#
# -Wplacement-new: Placement new
#   • Buffer: Size checks
#   • Alignment: Requirements
#
# -Wterminate: std::terminate calls
#   • noexcept: Violations
#   • Unexpected: Paths
#
# -Wvexing-parse: Most vexing parse
#   • Example: T x(); is declaration
#   • Clarity: Use {} or =

# ╔════════════════════════════════════════════════════════════════════╗
# ║                 C++ COROUTINES AND MODULES                         ║
# ╚════════════════════════════════════════════════════════════════════╝

string(APPEND OPENAA_CXX_FLAGS " -fcoroutines")
string(APPEND OPENAA_CXX_FLAGS " -fmodules-ts")
string(APPEND OPENAA_CXX_FLAGS " -fconcepts-diagnostics-depth=10")

# C++20 feature flags:
# -fcoroutines: Enable coroutines
#   • C++20: Async without callbacks
#   • co_await: Suspension points
#   • Generators: co_yield support
#   • Debug: Frame inspection
#
# -fmodules-ts: Enable modules
#   • C++20: Module support
#   • Experimental: Still evolving
#   • Faster: Compilation speedup
#   • Isolation: Better encapsulation
#
# -fconcepts-diagnostics-depth=10: Concept errors
#   • Templates: Better error messages
#   • Depth 10: Very detailed for debug
#   • C++20: Constraint debugging

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
string(APPEND OPENAA_EXEC_LINKER_FLAGS " -Wl,--warn-once")
string(APPEND OPENAA_EXEC_LINKER_FLAGS " -Wl,--warn-execstack")

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
#
# -Wl,--warn-once: Warn once per symbol
#   • Noise: Reduce duplicates
#   • Clarity: Cleaner output
#
# -Wl,--warn-execstack: Executable stack
#   • Security: W^X violation
#   • Trampolines: Detection

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

# -fcf-protection is not compatible with -mindirect-branch and -mfunction-return
# -mindirect-branch=thunk: Spectre v2 mitigation
#   • Retpoline: Indirect branch protection
#   • Performance: 5-10% overhead
#   • Security: Prevents BTB poisoning
#   • x86_64: Critical for security

# -mfunction-return=thunk: Return thunking
#   • Spectre: Return address protection
#   • Complement: With indirect branch
#   • Pattern: Similar overhead

#=======================================================================
# Summary and Usage
#=======================================================================

message(STATUS "")
message(STATUS "Safety Configuration Summary:")
message(STATUS "- Optimization: -Og (debug-friendly)")
message(STATUS "- Debug Info: Maximum (-g3 -ggdb3)")
message(STATUS "- Static Analysis: GCC analyzer enabled")
message(STATUS "")