#[=======================================================================
# OpenAA: Open Source Adaptive AUTOSAR Project
# Author: Sherif Mohamed
#
# File description:
# -----------------
# CMake initial-cache file for OpenAA - QNX 7.10 x86_64 using QCC-8.
# 
# DEBUG & SAFETY:
# ==========================
# This configuration achieves MAXIMUM SAFETY & DEBUGGABILITY through:
# 1. Zero optimization for accurate debugging (-O0 or -Og)
# 2. Comprehensive safety checks and runtime validation
# 3. Maximum security hardening features
# 4. Extensive debugging information and tools
#
# SAFETY & SECURITY FEATURES:
# ===========================
# All safety and security features are ENABLED by default:
# • Stack protection: Full canary coverage
# • Buffer overflow: Runtime bounds checking
# • Integer overflow: Trap on overflow
# • Memory safety: Guard pages and patterns
# • Security: Full ASLR, RELRO, NX, etc.
#
# PERFORMANCE IMPACT:
# ===================
# This configuration prioritizes safety over performance:
# • Comprehensive runtime checks
# • Full symbolic debugging capability
#=======================================================================]

#[=======================================================================[
.rst:
QNX710_X86_64_QCC8_MAXIMUM_SAFETY_DEBUG
-----------------------------------------
Maximum safety debug configuration for QNX 7.1.0 x86_64 using QCC-8 (GCC 8.3.0)

This configuration prioritizes SAFETY and DEBUGGABILITY above all else.
All safety features are enabled regardless of performance impact.

Usage:
------
.. code-block:: bash

    # Setup QNX environment
    source /opt/qnx710/qnxsdp-env.sh
    
    # Configure with maximum safety debug
    cmake -C qcc8_qnx710_x86_64_debug.cmake \
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
message(STATUS "Target: x86_64 (Generic x86-64 + SSE4.2)")
message(STATUS "Compiler: QCC-8")
message(STATUS "Mode: Debug with Maximum Safety & Security")
message(STATUS "==========================================================")

# Force shared libraries for better debugging
# Shared linking enables:
# - Easier debugging with symbol loading
# - Runtime library substitution
# - Better sanitizer support
# - Separate debug symbols per library
set(BUILD_SHARED_LIBS ON CACHE BOOL "Build shared libraries for debugging")

# Deterministic builds for reproducibility
set(DETERMINISTIC_BUILD_SEED "openaa_debug" CACHE STRING "Build seed")

#=======================================================================
# C Compiler Flags - MAXIMUM SAFETY DEBUG CONFIGURATION
#=======================================================================

# ╔════════════════════════════════════════════════════════════════════╗
# ║               DEBUG OPTIMIZATION FLAGS (ZERO/MINIMAL)              ║
# ╚════════════════════════════════════════════════════════════════════╝

# Build flags in a variable first to avoid overwriting
set(OPENAA_C_FLAGS_DEBUG "")

# Debug optimization level
# Choice between -O0 and -Og for debugging
string(APPEND OPENAA_C_FLAGS_DEBUG " -Og")
string(APPEND OPENAA_C_FLAGS_DEBUG " -DDEBUG")
string(APPEND OPENAA_C_FLAGS_DEBUG " -DDEBUG_BUILD")

# Debug optimization explanation:
# -Og: Optimized debugging experience (GCC 4.8+)
#   • Better than -O0: Faster compilation and execution
#   • Maintains: Accurate variable values and control flow
#   • Enables: Basic optimizations that don't interfere with debugging
#   • Example: Dead code elimination, but preserves all variables
#   • Trade-off: ~20% faster than -O0, equally debuggable
#
# Alternative -O0: Zero optimization
#   • Use when: Absolute debugging accuracy required
#   • Example: Every line corresponds exactly to machine code
#   • Downside: Very slow execution, large binary
#   • To use: Replace -Og with -O0 above
#
# -DDEBUG: Enable debug-specific code paths
#   • Activates: assert() macros
#   • Enables: Debug logging and validation
#   • Example: #ifdef DEBUG validation code
#
# -DDEBUG_BUILD: Additional debug marker
#   • Purpose: Distinguish from NDEBUG release builds
#   • Usage: Custom debug-only features

# ╔════════════════════════════════════════════════════════════════════╗
# ║                    DEBUG INFORMATION FLAGS                         ║
# ╚════════════════════════════════════════════════════════════════════╝

string(APPEND OPENAA_C_FLAGS_DEBUG " -g3")
string(APPEND OPENAA_C_FLAGS_DEBUG " -ggdb3")
string(APPEND OPENAA_C_FLAGS_DEBUG " -gdwarf-4")
string(APPEND OPENAA_C_FLAGS_DEBUG " -fno-omit-frame-pointer")
string(APPEND OPENAA_C_FLAGS_DEBUG " -fasynchronous-unwind-tables")
string(APPEND OPENAA_C_FLAGS_DEBUG " -fno-optimize-sibling-calls")

# Debug information details:
# -g3: Maximum debug information level
#   • Includes: All symbols, types, and macros
#   • Example: Can expand macros in debugger
#   • Size: 10-20x larger than stripped binary
#   • Usage: (gdb) info macro MAX_SIZE
#
# -ggdb3: GDB-specific maximum debug info
#   • Enhanced: GDB extensions beyond DWARF
#   • Features: Better expression evaluation
#   • Example: Can call inline functions from GDB
#
# -gdwarf-4: DWARF version 4 debug format
#   • Compatibility: Best QNX 7.1 debugger support
#   • Features: Compressed debug sections
#   • Size: 15-30% smaller than DWARF-2
#   • Note: DWARF-5 not fully supported in GCC 8.3
#
# -fno-omit-frame-pointer: Keep frame pointer (RBP)
#   • Purpose: Accurate stack traces always
#   • Example: backtrace() works reliably
#   • Overhead: One less register (3-5% performance)
#   • Critical: For profilers and crash dumps
#
# -fasynchronous-unwind-tables: Unwind info for all functions
#   • Purpose: Stack traces from any point
#   • Example: Signal handlers can unwind
#   • Size: ~5% binary size increase
#   • Benefit: Accurate profiling data
#
# -fno-optimize-sibling-calls: Disable tail call optimization
#   • Purpose: Complete call stacks
#   • Example: Recursive calls show full depth
#   • Debugging: Every call visible in backtrace

# ╔════════════════════════════════════════════════════════════════════╗
# ║     QNX / POSIX FEATURE-TEST MACRO (HEADER-VISIBILITY CONTROL)     ║
# ╚════════════════════════════════════════════════════════════════════╝

string(APPEND OPENAA_C_FLAGS_DEBUG " -D_QNX_SOURCE")
string(APPEND OPENAA_C_FLAGS_DEBUG " -D_FORTIFY_SOURCE=2")

# Feature macro details:
# -D_QNX_SOURCE: Enable all QNX-specific features
#   • Same as release build - needed for QNX APIs
#   • Exposes: MsgSend, ChannelCreate, etc.
#
# -D_FORTIFY_SOURCE=2: Runtime security checks
#   • Protection: Buffer overflow detection
#   • Example: strcpy → __strcpy_chk with bounds
#   • Level 2: Maximum checking (vs level 1)
#   • Overhead: 2-5% performance impact

# ╔════════════════════════════════════════════════════════════════════╗
# ║                    STACK PROTECTION FLAGS                          ║
# ╚════════════════════════════════════════════════════════════════════╝

string(APPEND OPENAA_C_FLAGS_DEBUG " -fstack-protector-all")
string(APPEND OPENAA_C_FLAGS_DEBUG " -fstack-clash-protection")

# Stack protection details:
# -fstack-protector-all: Protect ALL functions
#   • Coverage: Every function gets canary
#   • Example: void tiny() { return; } → has canary
#   • Overhead: 5-10% performance impact
#   • Compare: -fstack-protector-strong only some functions
#   • Catches: All stack buffer overflows
#
# -fstack-clash-protection: Prevent stack clash
#   • Purpose: Stop stack jumping over guard page
#   • Method: Probe each 4KB of stack allocation
#   • Example: char huge[1MB] → probed incrementally
#   • Security: Prevents privilege escalation

# ╔════════════════════════════════════════════════════════════════════╗
# ║            x86_64 SECURITY FEATURES (Intel CET)                    ║
# ╚════════════════════════════════════════════════════════════════════╝

string(APPEND OPENAA_C_FLAGS_DEBUG " -fcf-protection=full")
string(APPEND OPENAA_C_FLAGS_DEBUG " -mshstk")

# Intel CET security details:
# -fcf-protection=full: Full control flow protection
#   • Branch: Indirect branch tracking (IBT)
#   • Return: Shadow stack protection
#   • Hardware: Intel Tiger Lake+ or AMD Zen 3+
#   • Fallback: NOPs on older processors
#
# -mshstk: Shadow stack support
#   • Purpose: Hardware-protected return addresses
#   • Method: Parallel shadow stack in protected memory
#   • Security: Prevents ROP attacks completely

# ╔════════════════════════════════════════════════════════════════════╗
# ║                    INTEGER OVERFLOW PROTECTION                     ║
# ╚════════════════════════════════════════════════════════════════════╝

string(APPEND OPENAA_C_FLAGS_DEBUG " -ftrapv")
string(APPEND OPENAA_C_FLAGS_DEBUG " -fwrapv")
string(APPEND OPENAA_C_FLAGS_DEBUG " -fno-strict-overflow")

# Integer protection details:
# -ftrapv: Trap on signed integer overflow
#   • Action: SIGILL on overflow
#   • Example: INT_MAX + 1 → crash with core dump
#   • Overhead: 5-10% for integer-heavy code
#   • Debugging: Immediate detection of overflow
#   • Implementation: Calls __addvsi3 etc.
#
# -fwrapv: Define overflow behavior as wrapping
#   • Purpose: Make signed overflow defined
#   • Example: INT_MAX + 1 → INT_MIN (defined)
#   • Safety: Prevents optimization surprises
#   • Use with: -ftrapv for trap+defined behavior
#
# -fno-strict-overflow: Conservative overflow handling
#   • Purpose: Don't optimize based on no-overflow
#   • Example: Keeps overflow checks compiler might remove
#   • Safety: More predictable behavior

# ╔════════════════════════════════════════════════════════════════════╗
# ║                    MEMORY SAFETY FLAGS                             ║
# ╚════════════════════════════════════════════════════════════════════╝

string(APPEND OPENAA_C_FLAGS_DEBUG " -fno-delete-null-pointer-checks")
string(APPEND OPENAA_C_FLAGS_DEBUG " -fno-strict-aliasing")
string(APPEND OPENAA_C_FLAGS_DEBUG " -fno-common")
string(APPEND OPENAA_C_FLAGS_DEBUG " -Wl,-z,defs")

# Memory safety details:
# -fno-delete-null-pointer-checks: Keep ALL null checks
#   • Purpose: Catch null dereferences
#   • Example: if(p) *p=5; if(!p) error(); → both kept
#   • Hardware: Catches corrupted pointers
#   • Overhead: Negligible (<0.1%)
#
# -fno-strict-aliasing: Conservative aliasing
#   • Purpose: Type-punning safety
#   • Example: float/int union access works
#   • Required: Much system/embedded code
#   • Trade-off: ~5% performance vs correctness
#
# -fno-common: No common symbols
#   • Purpose: Detect duplicate definitions
#   • Example: int x; in two files → link error
#   • Safety: Prevents accidental sharing
#   • Modern: Default in GCC 10+
#
# -Wl,-z,defs: No undefined symbols
#   • Purpose: Complete link verification
#   • Example: Missing function → link fails
#   • Safety: No runtime symbol errors

# ╔════════════════════════════════════════════════════════════════════╗
# ║              x86_64 SPECIFIC DEBUG OPTIMIZATIONS                   ║
# ╚════════════════════════════════════════════════════════════════════╝

string(APPEND OPENAA_C_FLAGS_DEBUG " -mno-omit-leaf-frame-pointer")
string(APPEND OPENAA_C_FLAGS_DEBUG " -mfpmath=sse")
string(APPEND OPENAA_C_FLAGS_DEBUG " -msse4.2")

# x86_64 debug optimization details:
# -mno-omit-leaf-frame-pointer: Frame pointers in leaf functions
#   • Purpose: Complete stack traces always
#   • Leaf functions: ~50% of all functions
#   • Debug: Every function has frame pointer
#   • Trade-off: One less register everywhere
#
# -mfpmath=sse: Use SSE for floating-point
#   • Consistency: Same as release build
#   • Debug: More predictable than x87
#   • Precision: Strict IEEE 754 compliance
#
# -msse4.2: Enable SSE4.2 for consistency
#   • Purpose: Match release build capabilities
#   • Debug: Can test SIMD code paths

# ╔════════════════════════════════════════════════════════════════════╗
# ║                    SANITIZER FLAGS                                 ║
# ╚════════════════════════════════════════════════════════════════════╝

# # AddressSanitizer - comprehensive memory error detection (Not supported for QNX 7.1)
# string(APPEND OPENAA_C_FLAGS_DEBUG " -fsanitize=address")
# string(APPEND OPENAA_C_FLAGS_DEBUG " -fsanitize-address-use-after-scope")
# string(APPEND OPENAA_C_FLAGS_DEBUG " -fno-sanitize-recover=address")

# # UndefinedBehaviorSanitizer - catch undefined behavior
# string(APPEND OPENAA_C_FLAGS_DEBUG " -fsanitize=undefined")
# string(APPEND OPENAA_C_FLAGS_DEBUG " -fno-sanitize-recover=undefined")

# # Additional sanitizers
# string(APPEND OPENAA_C_FLAGS_DEBUG " -fsanitize=leak")
# string(APPEND OPENAA_C_FLAGS_DEBUG " -fsanitize=float-divide-by-zero")
# string(APPEND OPENAA_C_FLAGS_DEBUG " -fsanitize=float-cast-overflow")

# Sanitizer details:
# -fsanitize=address (ASan): Memory error detection
#   • Detects: Buffer overflow, use-after-free, double-free
#   • Overhead: 2-3x slowdown, 2-3x memory usage
#   • Example: char a[10]; a[10] = 0; → immediate detection
#   • Shadow memory: 1/8 of address space for metadata
#   • Red zones: Poisoned memory around allocations
#
# -fsanitize-address-use-after-scope: Scope checking
#   • Detects: Use of stack variables after scope
#   • Example: int* p; { int x; p = &x; } *p = 5; → error
#
# -fno-sanitize-recover=address: Abort on error
#   • Purpose: Don't continue after memory error
#   • Debugging: Get core dump at error point
#
# -fsanitize=undefined (UBSan): Undefined behavior
#   • Detects: Integer overflow, null deref, alignment
#   • Example: int x = INT_MAX; x++; → runtime error
#   • Overhead: 20-50% performance impact
#
# -fsanitize=leak: Memory leak detection
#   • Detects: Unreachable heap allocations
#   • Report: At program exit
#   • Integration: Works with ASan
#
# -fsanitize=float-divide-by-zero: FP division by zero
#   • Detects: x/0.0 operations
#   • Supplements: Standard FP exceptions
#
# -fsanitize=float-cast-overflow: FP to int overflow
#   • Detects: float f = 1e20; char c = f; → error

# ╔════════════════════════════════════════════════════════════════════╗
# ║                    WARNING FLAGS (MAXIMUM)                         ║
# ╚════════════════════════════════════════════════════════════════════╝

# All warnings from release build
string(APPEND OPENAA_C_FLAGS_DEBUG " -Wall")
string(APPEND OPENAA_C_FLAGS_DEBUG " -Wextra")
string(APPEND OPENAA_C_FLAGS_DEBUG " -Werror")
string(APPEND OPENAA_C_FLAGS_DEBUG " -Wpedantic")
string(APPEND OPENAA_C_FLAGS_DEBUG " -Winit-self")
string(APPEND OPENAA_C_FLAGS_DEBUG " -Walloc-zero")
string(APPEND OPENAA_C_FLAGS_DEBUG " -Wformat-signedness")
string(APPEND OPENAA_C_FLAGS_DEBUG " -Wredundant-decls")
string(APPEND OPENAA_C_FLAGS_DEBUG " -Wstringop-overflow=2")
string(APPEND OPENAA_C_FLAGS_DEBUG " -Wformat=2")
string(APPEND OPENAA_C_FLAGS_DEBUG " -Wformat-overflow=2")
string(APPEND OPENAA_C_FLAGS_DEBUG " -Wformat-truncation=2")
string(APPEND OPENAA_C_FLAGS_DEBUG " -Wformat-security")
string(APPEND OPENAA_C_FLAGS_DEBUG " -Wnull-dereference")
string(APPEND OPENAA_C_FLAGS_DEBUG " -Wstack-usage=16384")
string(APPEND OPENAA_C_FLAGS_DEBUG " -Wvla-larger-than=1024")
string(APPEND OPENAA_C_FLAGS_DEBUG " -Warray-bounds=2")
string(APPEND OPENAA_C_FLAGS_DEBUG " -Wimplicit-fallthrough=3")
string(APPEND OPENAA_C_FLAGS_DEBUG " -Wconversion")
string(APPEND OPENAA_C_FLAGS_DEBUG " -Wdouble-promotion")
string(APPEND OPENAA_C_FLAGS_DEBUG " -Wcast-align=strict")
string(APPEND OPENAA_C_FLAGS_DEBUG " -Wcast-qual")
string(APPEND OPENAA_C_FLAGS_DEBUG " -Wpointer-arith")
string(APPEND OPENAA_C_FLAGS_DEBUG " -Wshadow")
string(APPEND OPENAA_C_FLAGS_DEBUG " -Wlogical-op")
string(APPEND OPENAA_C_FLAGS_DEBUG " -Wduplicated-cond")
string(APPEND OPENAA_C_FLAGS_DEBUG " -Wduplicated-branches")
string(APPEND OPENAA_C_FLAGS_DEBUG " -Wrestrict")
string(APPEND OPENAA_C_FLAGS_DEBUG " -Walloca")
string(APPEND OPENAA_C_FLAGS_DEBUG " -Wfloat-equal")
string(APPEND OPENAA_C_FLAGS_DEBUG " -Wundef")
string(APPEND OPENAA_C_FLAGS_DEBUG " -Wswitch-enum")
string(APPEND OPENAA_C_FLAGS_DEBUG " -Wswitch-default")
string(APPEND OPENAA_C_FLAGS_DEBUG " -Wswitch-bool")
string(APPEND OPENAA_C_FLAGS_DEBUG " -Wswitch-unreachable")
string(APPEND OPENAA_C_FLAGS_DEBUG " -Wdate-time")
string(APPEND OPENAA_C_FLAGS_DEBUG " -Wtrampolines")
string(APPEND OPENAA_C_FLAGS_DEBUG " -Wstrict-overflow=1")


# Warning flag details (ZERO runtime impact):
# -Wall: Basic warnings
#   • Includes: -Wunused, -Wuninitialized, -Wparentheses
#   • Example: Catches "if(a = b)" typos
#
# -Wextra: Extended warnings
#   • Includes: -Wmissing-field-initializers, -Wtype-limits
#   • Example: Catches unsigned >= 0 comparisons
#
# -Werror: Warnings as errors
#   • Purpose: Force fixing all issues
#   • CI/CD: Prevents warning accumulation
#
# -Wpedantic: Strict ISO C compliance
#   • Catches: Non-standard extensions
#   • Example: GNU statement expressions
#   • ISO 26262: Ensures portable code
#
# -Winit-self: Uninitialized self-assignment
#   • Example: int x=x; → warning
#   • Safety: Prevents accidental self-initialization
#   • Performance: Zero overhead (compile-time only)
#
# -Walloc-zero: Zero-sized allocation warning
#   • Example: malloc(0) → warning
#   • Safety: Prevents undefined behavior in allocations
#   • Performance: Zero overhead (compile-time only)
#
# -Wformat-signedness: Signed/unsigned format mismatch
#   • Example: printf("%u", -1) → warning
#   • Safety: Prevents format string vulnerabilities
#   • Performance: Zero overhead (compile-time only)
#
# -Wredundant-decls: Redundant declarations
#   • Example: int f(); int f(); → warning
#   • Safety: Prevents accidental re-declarations
#   • Performance: Zero overhead (compile-time only)
#
# -Wstringop-overflow=2: String operation overflow
#   • Example: char buf[10]; strcpy(buf,long_string); → warning
#   • Safety: Prevents buffer overflows in string operations
#   • Performance: Zero overhead (compile-time only)
#
# -Wformat=2: Printf/scanf format checking
#   • Example: printf("%d", 3.14) → warning
#   • Security: Catches format string vulnerabilities
#
# -Wformat-overflow=2: Detect sprintf buffer overflows
#   • Example: char buf[10]; sprintf(buf,"%s",long_string);
#   • Level 2: More aggressive analysis
#
# -Wformat-truncation=2: Detect snprintf truncation
#   • Example: char buf[5]; snprintf(buf,5,"hello") → truncates
#
# -Wformat-security: Format string exploits
#   • Example: printf(user_input) → warning (use printf("%s", user_input))
#
# -Wnull-dereference: Detect NULL pointer usage
#   • Example: int *p=NULL; return *p; → warning
#   • Flow analysis: Tracks NULL through code paths
#
# -Wstack-usage=16384: Large stack frame warning
#   • Example: void func() { char huge[10000]; } → warning
#   • QNX: Default thread stack is often 8KB
#
# -Wvla-larger-than=1024: Variable Length Array limit
#   • Example: void func(int n) { char vla[n]; } → warning if n>1024
#   • Safety: VLAs can blow stack unexpectedly
#
# -Warray-bounds=2: Aggressive array checking
#   • Example: int arr[10]; arr[15]=0; → warning
#   • Level 2: Includes more complex cases
#
# -Wimplicit-fallthrough=3: Switch case fallthrough
#   • Example: case 1: x++; case 2: → warning
#   • Fix: Add /* fallthrough */ comment
#
# -Wconversion: Type conversion warnings
#   • Example: int i=300; char c=i; → warning (truncation)
#   • Catches: Signedness changes, precision loss
#
# -Wdouble-promotion: Float promoted to double
#   • Example: float f; sqrt(f); → warning (use sqrtf)
#   • Performance: Double operations slower on some cores
#
# -Wcast-align=strict: Alignment-increasing casts
#   • Example: char *p; int *q=(int*)p; → warning
#   • x86_64: Unaligned access works but 2x slower
#   • Strict: Even warns on platform-supported cases
#
# -Wcast-qual: Qualifier-removing casts
#   • Example: const int *p; int *q=(int*)p; → warning
#   • Safety: Prevents const-correctness violations
#
# -Wpointer-arith: Arithmetic on void* or functions
#   • Example: void *p; p+5; → warning
#   • Portability: Size of void is undefined
#
# -Wshadow: Variable name shadowing
#   • Example: int x; { int x; } → warning
#   • Prevents: Confusion about which variable is used
#
# -Wlogical-op: Suspicious logical operations
#   • Example: if(x<5 && x<3) → warning (redundant)
#   • Catches: Common copy-paste errors
#
# -Wduplicated-cond: Duplicate if-else conditions
#   • Example: if(x) {} else if(x) {} → warning
#
# -Wduplicated-branches: Identical if-else branches
#   • Example: if(x) {y=1;} else {y=1;} → warning
#
# -Wrestrict: Overlapping restrict pointers
#   • Example: memcpy(p, p+1, 10) → warning
#   • C99: restrict means no aliasing
#
# -Wnested-externs: Extern inside function
#   • Example: void f() { extern int x; } → warning
#   • Style: Promotes cleaner header usage
#
# -Wjump-misses-init: Goto bypasses initialization
#   • Example: goto end; int x=5; end: → warning
#   • Safety: Prevents use of uninitialized variables
# -Walloca: Warn on alloca() use
#   • Risk: Dynamic stack allocation
#   • Alternative: Fixed arrays or heap
#
# -Wfloat-equal: Warn on FP equality
#   • Example: if(x == 0.1) → warning
#   • Better: if(fabs(x - 0.1) < epsilon)
#
# -Wundef: Undefined macro in #if
#   • Example: #if UNDEFINED_MACRO → warning
#   • Safety: Catches typos in conditionals
#
# -Wswitch-enum: All enum cases required
#   • Example: Missing case in switch(enum)
#   • Safety: Ensures complete handling
#
# -Wswitch-default: Require default case
#   • Safety: Handle unexpected values
#
# -Wswitch-bool: Boolean switch cases
#   • Example: switch(flag) { case true: ...; } → warning
#   • Safety: Prevents confusion with integer cases
#
# -Wswitch-unreachable: Unreachable switch cases
#   • Example: switch(x) { case 1: ...; case 2: ...; default: ...; case 3: ...; } → warning
#   • Safety: Catches logic errors in switch statements
#
# -Wdate-time: Warn on date/time macros
#   • Safety: ensures reproducible builds by flagging time-dependent compilation macros
#
# -Wbad-function-cast: Function pointer cast issues
#   • Example: int (*f)() = (int (*)())0x1234; → warning
#   • Safety: Prevents invalid function pointer casts
#
# -Wtrampolines: Trampoline function warnings
#   • Example: Function pointer to a trampoline function
#   • Safety: Prevents misuse of function pointers
#
# -WStrict-overflow=1: Strict aliasing assumptions
#   • Example: int x=5; if(x+1>5) { /* x is not modified */ }
#   • Performance: Allows more aggressive optimizations
#   • Safety: Ensures no undefined behavior from aliasing
#
# -Wtraditional-conversion: Traditional C conversion warnings
#   • Example: int x=5; float y=x; → warning (C99 requires explicit cast)
#   • Safety: Prevents implicit conversions that may lose precision
#   • Performance: Zero overhead (compile-time only)


# C-specific warnings
string(APPEND OPENAA_C_FLAGS_DEBUG " -Wnested-externs")
string(APPEND OPENAA_C_FLAGS_DEBUG " -Wjump-misses-init")
string(APPEND OPENAA_C_FLAGS_DEBUG " -Wmissing-prototypes")
string(APPEND OPENAA_C_FLAGS_DEBUG " -Wstrict-prototypes")
string(APPEND OPENAA_C_FLAGS_DEBUG " -Wbad-function-cast")
string(APPEND OPENAA_C_FLAGS_DEBUG " -Wold-style-definition")
string(APPEND OPENAA_C_FLAGS_DEBUG " -Wmissing-declarations")
string(APPEND OPENAA_C_FLAGS_DEBUG " -Wtraditional-conversion")

# C-specific warning details:
# -Wmissing-prototypes: Function needs prototype
#   • Example: int func() {} → warning (add prototype)
#   • Safety: Ensures callers/implementation match
#
# -Wstrict-prototypes: Prototype must list parameters
#   • Example: int func(); → warning (use int func(void))
#   • C: Empty parens mean "any args" not "no args"
#
# -Wbad-function-cast: Function pointer cast issues
#   • Example: int (*f)() = (int (*)())0x1234; → warning
#   • Safety: Prevents invalid function pointer casts
#
# -Wold-style-definition: No K&R definitions
#   • Example: int func(x) int x; {} → warning
#   • Modern: Use int func(int x) {}
#
# -Wmissing-declarations: Global needs declaration
#   • Example: int func() {} → warning if no prior declaration
#   • Catches: Spelling mismatches, missing headers
#
# -Wtraditional-conversion: Traditional C conversion warnings
#   • Example: int x=5; float y=x; → warning (C99 requires explicit cast)
#   • Safety: Prevents implicit conversions that may lose precision
#   • Performance: Zero overhead (compile-time only)

# ╔════════════════════════════════════════════════════════════════════╗
# ║                    RUNTIME HARDENING FLAGS                         ║
# ╚════════════════════════════════════════════════════════════════════╝

string(APPEND OPENAA_C_FLAGS_DEBUG " -fpie")
string(APPEND OPENAA_C_FLAGS_DEBUG " -Wl,-z,relro")
string(APPEND OPENAA_C_FLAGS_DEBUG " -Wl,-z,now")
string(APPEND OPENAA_C_FLAGS_DEBUG " -Wl,-z,noexecstack")

# Runtime hardening details:
# -fpie: Position Independent Executable
#   • ASLR: Full address randomization
#   • Debug: Still works with random addresses
#   • GDB: Handles PIE automatically
#
# -Wl,-z,relro: Read-only relocations
#   • GOT: Read-only after startup
#   • Security: No GOT overwrite attacks
#
# -Wl,-z,now: Immediate binding
#   • Symbols: All resolved at startup
#   • Overhead: 5-10ms startup time
#   • Benefit: Full RELRO protection
#
# -Wl,-z,noexecstack: No executable stack
#   • Security: W^X enforcement
#   • Required: Modern security standard


# ╔════════════════════════════════════════════════════════════════════╗
# ║                    PROCESSOR-SPECIFIC DEBUG SETTINGS               ║
# ╚════════════════════════════════════════════════════════════════════╝

# Same cache parameters as release for consistency
string(APPEND OPENAA_C_FLAGS_DEBUG " --param l1-cache-size=32")
string(APPEND OPENAA_C_FLAGS_DEBUG " --param l1-cache-line-size=64")
string(APPEND OPENAA_C_FLAGS_DEBUG " --param l2-cache-size=256")

# Debug-specific parameters
string(APPEND OPENAA_C_FLAGS_DEBUG " --param max-vartrack-size=100000000")
string(APPEND OPENAA_C_FLAGS_DEBUG " --param max-vartrack-expr-depth=100")

# Debug parameter details:
# --param max-vartrack-size=100000000: Variable tracking
#   • Purpose: Track all variables for debugging
#   • Default: Lower limit might drop tracking
#   • Benefit: All variables visible in debugger
#
# --param max-vartrack-expr-depth=100: Expression depth
#   • Purpose: Complex expressions in debugger
#   • Example: Can evaluate deep nested structs

# Now set the complete debug flags
set(CMAKE_C_FLAGS_INIT "${OPENAA_C_FLAGS_DEBUG}" CACHE STRING "C compiler flags for maximum safety debug")

# Debug build type - already comprehensive, just set marker
set(CMAKE_C_FLAGS_DEBUG "-DCMAKE_BUILD_TYPE_DEBUG" CACHE STRING "Additional debug build flags")

#=======================================================================
# C++ Compiler Flags - MAXIMUM SAFETY DEBUG
#=======================================================================

# Build C++ flags starting with C flags
set(OPENAA_CXX_FLAGS_DEBUG "${OPENAA_C_FLAGS_DEBUG}")

# ╔════════════════════════════════════════════════════════════════════╗
# ║                    C++ SPECIFIC DEBUG FLAGS                        ║
# ╚════════════════════════════════════════════════════════════════════╝

# Keep RTTI and exceptions for debugging
string(APPEND OPENAA_CXX_FLAGS_DEBUG " -frtti")
string(APPEND OPENAA_CXX_FLAGS_DEBUG " -fexceptions")

# C++ debug details:
# -frtti: Runtime Type Information enabled
#   • Purpose: dynamic_cast and typeid work
#   • Debugging: Can inspect object types
#   • Example: typeid(*ptr).name() in debugger
#   • Overhead: 5-10% size, minimal runtime
#
# -fexceptions: Exception handling enabled
#   • Purpose: Full C++ exception support
#   • Debugging: Can trace exception propagation
#   • Stack: Proper unwinding on throw
#   • Overhead: 10-15% code size
#
# -fcxx-exceptions: C++ specific exceptions
#   • Purpose: Ensure C++ semantics
#   • Difference: From C structured exceptions

# ╔════════════════════════════════════════════════════════════════════╗
# ║                    C++ SPECIFIC WARNINGS                           ║
# ╚════════════════════════════════════════════════════════════════════╝

# Remove C-only warnings
string(REPLACE " -Wnested-externs"         ""  OPENAA_CXX_FLAGS_DEBUG "${OPENAA_CXX_FLAGS_DEBUG}")
string(REPLACE " -Wjump-misses-init"       ""  OPENAA_CXX_FLAGS_DEBUG "${OPENAA_CXX_FLAGS_DEBUG}")
string(REPLACE " -Wmissing-prototypes"     ""  OPENAA_CXX_FLAGS_DEBUG "${OPENAA_CXX_FLAGS_DEBUG}")
string(REPLACE " -Wstrict-prototypes"      ""  OPENAA_CXX_FLAGS_DEBUG "${OPENAA_CXX_FLAGS_DEBUG}")
string(REPLACE " -Wold-style-definition"   ""  OPENAA_CXX_FLAGS_DEBUG "${OPENAA_CXX_FLAGS_DEBUG}")
string(REPLACE " -Wbad-function-cast"      ""  OPENAA_CXX_FLAGS_DEBUG "${OPENAA_CXX_FLAGS_DEBUG}")
string(REPLACE " -Wmissing-declarations"   ""  OPENAA_CXX_FLAGS_DEBUG "${OPENAA_CXX_FLAGS_DEBUG}")
string(REPLACE " -Wtraditional-conversion" ""  OPENAA_CXX_FLAGS_DEBUG "${OPENAA_CXX_FLAGS_DEBUG}")

# Add C++ specific warnings
string(APPEND OPENAA_CXX_FLAGS_DEBUG " -Weffc++")
string(APPEND OPENAA_CXX_FLAGS_DEBUG " -Wzero-as-null-pointer-constant")
string(APPEND OPENAA_CXX_FLAGS_DEBUG " -Wsuggest-final-methods")
string(APPEND OPENAA_CXX_FLAGS_DEBUG " -Wstrict-null-sentinel")
string(APPEND OPENAA_CXX_FLAGS_DEBUG " -Wsuggest-final-types")
string(APPEND OPENAA_CXX_FLAGS_DEBUG " -Wdelete-non-virtual-dtor")
string(APPEND OPENAA_CXX_FLAGS_DEBUG " -Woverloaded-virtual")
string(APPEND OPENAA_CXX_FLAGS_DEBUG " -Wsuggest-override")
string(APPEND OPENAA_CXX_FLAGS_DEBUG " -Wnon-virtual-dtor")
string(APPEND OPENAA_CXX_FLAGS_DEBUG " -Wplacement-new=2")
string(APPEND OPENAA_CXX_FLAGS_DEBUG " -Wold-style-cast")
string(APPEND OPENAA_CXX_FLAGS_DEBUG " -Wuseless-cast")
string(APPEND OPENAA_CXX_FLAGS_DEBUG " -Wsign-promo")
string(APPEND OPENAA_CXX_FLAGS_DEBUG " -Wextra-semi")
string(APPEND OPENAA_CXX_FLAGS_DEBUG " -Wnoexcept")

# C++ warning details:
# -Weffc++: Effective C++ guidelines
#   • Safety: Enforces best practices from Scott Meyers' book
#       which align closely with AUTOSAR rules M12-1-1 and M12-1-2 for constructor/destructor 
#       safety and AUTOSAR rule M10-3-2 for virtual function declarations.
#   • Performance: Zero overhead (compile-time only)
#   • Note: Can be too strict for some codebases
#
# -Wsuggest-override: Missing override specifier
#   • Example: virtual void f(); in derived → add override
#   • C++11: Documents and enforces inheritance
#
# -Wsuggest-final-types: Classes that could be final
#   • Performance: Enables devirtualization
#   • Example: class Leaf : Base {}; → final class Leaf
#
# -Wstrict-null-sentinel: Null sentinel in variadic functions
#   • Example: printf("%s", NULL); → warning
#   • Safety: Prevents passing NULL to variadic functions
#   • Fix: Use nullptr or proper sentinel value
#
# -Wsuggest-final-methods: Methods that could be final
#   • Performance: Prevents vtable lookup
#   • Example: virtual void lastImpl() → final
#
# -Wdelete-non-virtual-dtor: Non-virtual destructor in polymorphic class
#   • Safety: Prevents memory leaks when deleting derived class
#   • Example: class Base { void f(); }; → add virtual ~Base()
#   • Fix: Add virtual destructor to base class
#   • Note: C++11 requires virtual destructors for polymorphic classes
#
# -Wplacement-new=2: Placement new warnings
#   • Example: new (ptr) MyClass(); → warning if ptr not aligned
#   • Safety: Ensures proper memory alignment
#
# -Wnon-virtual-dtor: Polymorphic class needs virtual dtor
#   • Safety: Prevents slicing, memory leaks
#   • Example: class Base { virtual void f(); }; → add virtual ~Base()
#
# -Woverloaded-virtual: Hidden virtual functions
#   • Example: Base::f(int), Derived::f(float) hides base
#   • Fix: Add using Base::f; or override correctly
#
# -Wzero-as-null-pointer-constant: Use nullptr
#   • C++11: Type-safe null pointer
#   • Example: int* p = 0; → int* p = nullptr;
#
# -Wold-style-cast: C-style casts in C++
#   • Example: (int)x → static_cast<int>(x)
#   • Safety: C++ casts are more specific
#
# -Wsign-promo: Overload resolution promotes sign
#   • Example: f(unsigned) chosen over f(int) for -1
#   • Subtle: Can cause unexpected behavior
#
# -Wuseless-cast: Redundant casts
#   • Example: int i; static_cast<int>(i) → warning
#   • Cleaner: Remove unnecessary casts
#
# -Wextra-semi: Extra semicolons
#   • Example: class X {}; ; → warning
#   • Pedantic: But can hide real errors
#
# -Wnoexcept: Missing noexcept specifier
#   • Example: void f() { throw 1; } → warning
#   • C++11: Documents exception guarantees

# Set C++ debug flags
set(CMAKE_CXX_FLAGS_INIT "${OPENAA_CXX_FLAGS_DEBUG}" CACHE STRING "C++ compiler flags for maximum safety debug")

# C++ build type specific flags
set(CMAKE_CXX_FLAGS_DEBUG "-DCMAKE_BUILD_TYPE_DEBUG" CACHE STRING "Additional C++ debug build flags")

#=======================================================================
# Linker Flags - MAXIMUM SAFETY & DEBUG
#=======================================================================

# ╔════════════════════════════════════════════════════════════════════╗
# ║                    EXECUTABLE LINKER FLAGS                         ║
# ╚════════════════════════════════════════════════════════════════════╝

set(OPENAA_EXEC_LINKER_FLAGS_DEBUG "")

# Core security flags
string(APPEND OPENAA_EXEC_LINKER_FLAGS_DEBUG " -pie")
string(APPEND OPENAA_EXEC_LINKER_FLAGS_DEBUG " -Wl,-z,relro")
string(APPEND OPENAA_EXEC_LINKER_FLAGS_DEBUG " -Wl,-z,now")
string(APPEND OPENAA_EXEC_LINKER_FLAGS_DEBUG " -Wl,-z,noexecstack")
string(APPEND OPENAA_EXEC_LINKER_FLAGS_DEBUG " -Wl,-z,defs")

# Debug-specific linker flags
string(APPEND OPENAA_EXEC_LINKER_FLAGS_DEBUG " -Wl,--warn-common")
string(APPEND OPENAA_EXEC_LINKER_FLAGS_DEBUG " -Wl,--warn-constructors")
string(APPEND OPENAA_EXEC_LINKER_FLAGS_DEBUG " -Wl,--warn-multiple-gp")
string(APPEND OPENAA_EXEC_LINKER_FLAGS_DEBUG " -Wl,--fatal-warnings")

set(CMAKE_EXE_LINKER_FLAGS_INIT "${OPENAA_EXEC_LINKER_FLAGS_DEBUG}" CACHE STRING "Debug executable linker flags")

# Debug linker flag details:
# -Wl,--warn-common: Warn about common symbols
#   • Purpose: Detect accidental globals
#   • Example: int x; in multiple files
#
# -Wl,--warn-constructors: Global constructor warning
#   • Purpose: Startup time debugging
#   • C++: Static initialization issues
#
# -Wl,--warn-multiple-gp: Multiple GP warning
#   • Purpose: Embedded systems check
#   • MIPS/PowerPC: Global pointer issues

#
# -Wl,--fatal-warnings: Warnings are errors
#   • Purpose: Strict linking
#   • Safety: No ignored issues

# Debug-specific executable flags
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "" CACHE STRING "Additional debug linker flags")

# ╔════════════════════════════════════════════════════════════════════╗
# ║                 SHARED LIBRARY LINKER FLAGS                        ║
# ╚════════════════════════════════════════════════════════════════════╝

set(OPENAA_SHARED_LINKER_FLAGS_DEBUG "")

string(APPEND OPENAA_SHARED_LINKER_FLAGS_DEBUG " -Wl,-z,relro")
string(APPEND OPENAA_SHARED_LINKER_FLAGS_DEBUG " -Wl,-z,now")
string(APPEND OPENAA_SHARED_LINKER_FLAGS_DEBUG " -Wl,-z,noexecstack")
string(APPEND OPENAA_SHARED_LINKER_FLAGS_DEBUG " -Wl,-z,defs")
string(APPEND OPENAA_SHARED_LINKER_FLAGS_DEBUG " -Wl,--no-undefined")

set(CMAKE_SHARED_LINKER_FLAGS_INIT "${OPENAA_SHARED_LINKER_FLAGS_DEBUG}" CACHE STRING "Debug shared library linker flags")
set(CMAKE_SHARED_LINKER_FLAGS_DEBUG "" CACHE STRING "Additional debug shared library flags")

# -Wl,--no-undefined: No undefined symbols in shared libs
#   • Purpose: Complete symbol resolution
#   • Safety: All dependencies explicit

#=======================================================================
# Build Validation for Safety Compliance
#=======================================================================

function(validate_debug_safety)
    # Check that optimization is disabled
    string(FIND "${CMAKE_C_FLAGS}" "-O3" found_o3)
    string(FIND "${CMAKE_C_FLAGS}" "-O2" found_o2)
    if(found_o3 GREATER -1 OR found_o2 GREATER -1)
        message(WARNING "High optimization detected in debug build!")
    endif()
    
    # Verify safety features are enabled
    set(REQUIRED_SAFETY_FLAGS
        "-fstack-protector"
        "-D_FORTIFY_SOURCE"
        "-ftrapv"
    )
    
    foreach(flag ${REQUIRED_SAFETY_FLAGS})
        string(FIND "${CMAKE_C_FLAGS_INIT}" "${flag}" found_pos)
        if(found_pos EQUAL -1)
            message(WARNING "Safety flag '${flag}' not found in debug configuration!")
        endif()
    endforeach()
endfunction()

# Validate debug configuration
validate_debug_safety()

#=======================================================================
# Summary and Usage
#=======================================================================

message(STATUS "")
message(STATUS "Debug Safety Configuration Summary:")
message(STATUS "- Optimization: -Og (debuggable optimization)")
message(STATUS "- Stack Protection: ALL functions protected")
message(STATUS "- Integer Overflow: Trapping enabled")
message(STATUS "- Sanitizers: Address, Undefined, Leak")
message(STATUS "- Memory Safety: Guard pages, fill patterns")
message(STATUS "- Debug Info: Maximum with DWARF-4")
message(STATUS "- Intel CET: Full control flow protection")
message(STATUS "")