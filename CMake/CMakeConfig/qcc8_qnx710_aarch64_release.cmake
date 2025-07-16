#[=======================================================================
# OpenAA: Open Source Adaptive AUTOSAR Project
# Author: Sherif Mohamed
#
# File description:
# -----------------
# CMake initial-cache file for OpenAA - QNX 7.10 aarch64 using QCC-8.
# 
# OPTIMIZATION:
# ========================
# This configuration achieves MAXIMUM PERFORMANCE through:
# 1. Aggressive compiler optimizations that maintain correctness
# 2. Architecture-specific ARM64/Cortex-A75 optimizations
# 3. Minimal safety overhead (documented but optional)
# 4. Smart security features with negligible performance impact
#
# SAFETY APPROACH:
# ================
# High-overhead safety features are DOCUMENTED but DISABLED by default.
# Enable them selectively based on your requirements.
#=======================================================================]

#[=======================================================================[
.rst:
QNX710_AARCH64_QCC8_MAXIMUM_PERFORMANCE
----------------------------------------
Maximum performance configuration for QNX 7.1.0 aarch64 using QCC-8 (GCC 8.3.0)

This configuration prioritizes PERFORMANCE above all else while maintaining
program correctness. Safety features with significant overhead are disabled
but thoroughly documented.

Usage:
------
.. code-block:: bash

    # Setup QNX environment
    source /opt/qnx710/qnxsdp-env.sh
    
    # Configure with maximum performance
    cmake -C qcc8_qnx710_aarch64_release.cmake \
          -DCMAKE_BUILD_TYPE=Release \
          -S <source> -B <build>
    
    # Build with parallel jobs
    cmake --build <build> -j$(nproc)

#]=======================================================================]

#=======================================================================
# Build System Configuration
#=======================================================================
message(STATUS "==========================================================")
message(STATUS "QNX 7.10 MAXIMUM PERFORMANCE Configuration")
message(STATUS "Target: ARM64 Cortex-A75 (ARMv8.2-A)")
message(STATUS "Compiler: QCC-8")
message(STATUS "==========================================================")

# Force static libraries for maximum optimization opportunities
# Static linking enables:
# - Whole program optimization
# - Better dead code elimination
# - No PLT/GOT overhead (saves 3-4 instructions per call)
# - 10-20% typical performance improvement
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static libraries for performance")

# Deterministic builds for reproducibility (zero performance impact)
# ISO 26262 requires reproducible builds for safety-critical systems.
set(DETERMINISTIC_BUILD_SEED "openaa_seed" CACHE STRING "Build seed")

#=======================================================================
# C Compiler Flags - MAXIMUM PERFORMANCE CONFIGURATION
#=======================================================================

# ╔════════════════════════════════════════════════════════════════════╗
# ║               CORE OPTIMIZATION FLAGS (HIGHEST IMPACT)             ║
# ╚════════════════════════════════════════════════════════════════════╝

# Build flags in a variable first to avoid overwriting
set(OPENAA_C_FLAGS "")

# Base optimization level and architecture
string(APPEND OPENAA_C_FLAGS " -O3")
string(APPEND OPENAA_C_FLAGS " -march=armv8.2-a+crypto+crc+fp16+rcpc+dotprod")
string(APPEND OPENAA_C_FLAGS " -mcpu=cortex-a75")
string(APPEND OPENAA_C_FLAGS " -mtune=cortex-a75")

# Detailed architecture feature explanation:
# -O3: Maximum standard optimization level
#   • Enables all -O2 optimizations plus aggressive inlining
#   • Example: Small functions completely disappear into callers
#   • Performance: 15-25% faster than -O2 on average
#   • Trade-off: 10-30% larger binary size
#   • Not suitable for ASIL-D components it enables non-deterministic optimizations
#   • O2: Maximum safe optimization for ASIL-D (industry standard)
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
# ║     QNX / POSIX FEATURE-TEST MACRO (HEADER-VISIBILITY CONTROL)     ║
# ╚════════════════════════════════════════════════════════════════════╝

string(APPEND OPENAA_C_FLAGS " -D_QNX_SOURCE")

# _QNX_SOURCE macro details:
# QNX Neutrino's "expose EVERYTHING" feature-test macro
# When defined, system headers reveal ALL interfaces:
#   • Complete POSIX.1-2008 & XSI interfaces (pthread_*, clock_gettime64, etc.)
#   • Legacy BSD/GNU helpers (strdup, getline, daemon, etc.)
#   • QNX-specific APIs (MsgSend, MsgReceive, ChannelCreate, etc.)
#   • Example: Without it, clock_gettime64() is hidden with -std=c11
#   • The QNX toolchain defines _QNX_SOURCE by default, but it's
#     undefined when using strict language modes (-std=c11, -std=c++17)
#   • Zero runtime cost (pure preprocessor control)

# ╔════════════════════════════════════════════════════════════════════╗
# ║                    LOOP OPTIMIZATION FLAGS                         ║
# ╚════════════════════════════════════════════════════════════════════╝

string(APPEND OPENAA_C_FLAGS " -funroll-loops")
string(APPEND OPENAA_C_FLAGS " -fpeel-loops")
string(APPEND OPENAA_C_FLAGS " -fsplit-loops")
string(APPEND OPENAA_C_FLAGS " -ftree-loop-distribution")
string(APPEND OPENAA_C_FLAGS " -ftree-loop-if-convert")
string(APPEND OPENAA_C_FLAGS " -ftree-loop-im")
string(APPEND OPENAA_C_FLAGS " -fivopts")

# Loop optimization details:
# -funroll-loops: Unroll loops with known bounds
#   • Performance: 20-30% improvement for small loops
#   • Example: 
#     // Before: 4 iterations, 4 branches, 4 condition checks
#     for(i=0; i<4; i++) sum += array[i];
#     // After: 0 branches, straight-line code
#     sum += array[0] + array[1] + array[2] + array[3];
#   • Trade-off: 2-4x code size increase for unrolled loops
#
# -fpeel-loops: Peel iterations for alignment/optimization
#   • Performance: 10-15% for memory-bound loops
#   • Example:
#     // Before: All iterations check alignment
#     for(i=0; i<n; i++) process(ptr[i]);
#     // After: First iteration peeled if misaligned
#     if(ptr & 15) { process(*ptr++); i=1; }
#     for(; i<n; i++) process_aligned(ptr[i]);
#
# -fsplit-loops: Split loops with multiple exit conditions
#   • Performance: Better branch prediction, 10-15% faster
#   • Example:
#     // Before: Two conditions checked each iteration
#     for(i=0; i<n && !found; i++) { if(arr[i]==x) found=1; }
#     // After: Inner loop has single exit
#     for(i=0; i<n; i++) { if(arr[i]==x) { found=1; break; } }
#
# -ftree-loop-distribution: Distribute loop statements
#   • Performance: Better cache usage and vectorization
#   • Example:
#     // Before: Poor cache locality, hard to vectorize
#     for(i=0; i<n; i++) { a[i] = 0; b[i] = c[i] * 2; }
#     // After: Each loop vectorizable, better cache usage
#     for(i=0; i<n; i++) a[i] = 0;        // memset
#     for(i=0; i<n; i++) b[i] = c[i] * 2; // vectorized
#
# -ftree-loop-if-convert: Convert branches to conditional moves
#   • Performance: Eliminates branch mispredictions
#   • Example:
#     // Before: Unpredictable branch
#     for(i=0; i<n; i++) { if(a[i]>0) b[i]=a[i]; else b[i]=0; }
#     // After: Branchless with CSEL instruction
#     for(i=0; i<n; i++) { b[i] = a[i]>0 ? a[i] : 0; }
#
# -ftree-loop-im: Loop Invariant Motion
#   • Performance: Moves computation out of loops
#   • Example:
#     // Before: Multiplication done n times
#     for(i=0; i<n; i++) arr[i] = x * y * arr[i];
#     // After: Multiplication done once
#     temp = x * y;
#     for(i=0; i<n; i++) arr[i] = temp * arr[i];
#
# -fivopts: Induction Variable Optimizations
#   • Performance: Reduces arithmetic in loops
#   • Example:
#     // Before: Two variables updated
#     for(i=0, p=arr; i<n; i++, p++) *p = i;
#     // After: Only pointer updated
#     for(p=arr, end=arr+n; p<end; p++) *p = p-arr;

# ╔════════════════════════════════════════════════════════════════════╗
# ║              INTERPROCEDURAL OPTIMIZATION (LTO)                    ║
# ╚════════════════════════════════════════════════════════════════════╝

string(APPEND OPENAA_C_FLAGS " -flto=auto")
string(APPEND OPENAA_C_FLAGS " -fuse-linker-plugin")
string(APPEND OPENAA_C_FLAGS " -ffat-lto-objects")
string(APPEND OPENAA_C_FLAGS " -fipa-pta")
string(APPEND OPENAA_C_FLAGS " -fipa-cp-clone")
string(APPEND OPENAA_C_FLAGS " -fipa-icf")

# LTO and IPA optimization details:
# -flto=auto: Link Time Optimization with all CPU cores
#   • Performance: 10-20% improvement typical
#   • Example: Inlines functions across translation units
#   • Real case: json_parser() from lib.a inlined into main.c
#   • Build time: Uses jobserver for parallel compilation
#
# -fuse-linker-plugin: LTO linker plugin integration
#   • Performance: Additional 2-3% over basic LTO
#   • Example: Linker provides symbol resolution info to compiler
#   • Enables more aggressive dead code elimination
#
# -ffat-lto-objects: Dual-mode object files
#   • Compatibility: Works with non-LTO static libraries
#   • Example: libjson.a works with both LTO and non-LTO builds
#   • Size overhead: 30-50% larger .o files (stripped at link)
#
# -fipa-pta: Interprocedural Pointer Analysis
#   • Performance: Enables more optimizations
#   • Example:
#     // file1.c: void process(int *a, int *b);
#     // file2.c: process(arr1, arr2); // Proves no alias
#   • Result: Vectorizes loops in process() function
#
# -fipa-cp-clone: Function cloning for constants
#   • Performance: Specialized fast paths
#   • Example:
#     // Many calls: compute(x, y, FAST_MODE);
#     // Creates: compute.clone.FAST_MODE() with optimizations
#   • Real case: 30% faster for constant parameters
#
# -fipa-icf: Identical Code Folding
#   • Size/Performance: Merges duplicate functions
#   • Example:
#     int max_int(int a, int b) { return a>b?a:b; }
#     int max_int2(int x, int y) { return x>y?x:y; }
#     // Merged into single function

# ╔════════════════════════════════════════════════════════════════════╗
# ║                  VECTORIZATION OPTIMIZATIONS                       ║
# ╚════════════════════════════════════════════════════════════════════╝

string(APPEND OPENAA_C_FLAGS " -ftree-vectorize")
string(APPEND OPENAA_C_FLAGS " -ftree-slp-vectorize")
string(APPEND OPENAA_C_FLAGS " -fvect-cost-model=dynamic")
string(APPEND OPENAA_C_FLAGS " -fopenmp-simd")

# ARM NEON vectorization details:
# -ftree-vectorize: Auto-vectorize loops
#   • Performance: 2-8x speedup for array operations
#   • Example:
#     // Scalar: 16 loads, 16 adds, 16 stores
#     for(i=0; i<16; i++) a[i] = b[i] + c[i];
#     // Vectorized: 4 vector loads, 4 vector adds, 4 vector stores
#     // Compiles to: LD1 {v0.4s}, [x1], #16; ADD v0.4s, v0.4s, v1.4s
#   • Real performance: 1.6GB/s → 6.4GB/s for array addition
#
# -ftree-slp-vectorize: Straight-Line Program vectorization
#   • Performance: Vectorizes non-loop code
#   • Example:
#     // Before: 4 scalar operations
#     a = b + c; d = e + f; g = h + i; j = k + l;
#     // After: 1 NEON operation
#     // Compiles to single: ADD v0.4s, v1.4s, v2.4s
#
# -fvect-cost-model=dynamic: Adaptive vectorization
#   • Performance: Only vectorizes when profitable
#   • Example: Won't vectorize loop with 3 iterations (overhead > benefit)
#   • Considers: Alignment, trip count, data dependencies
#
# -fopenmp-simd: OpenMP SIMD directive support
#   • Performance: Explicit vectorization control
#   • Example:
#     #pragma omp simd aligned(a,b,c:64)
#     for(i=0; i<n; i++) a[i] = b[i] * c[i];
#   • Guarantees vectorization even for complex loops

# ╔════════════════════════════════════════════════════════════════════╗
# ║                    MEMORY OPTIMIZATIONS                            ║
# ╚════════════════════════════════════════════════════════════════════╝

string(APPEND OPENAA_C_FLAGS " -fprefetch-loop-arrays")
string(APPEND OPENAA_C_FLAGS " -foptimize-strlen")
string(APPEND OPENAA_C_FLAGS " -ftree-switch-conversion")
string(APPEND OPENAA_C_FLAGS " -fmodulo-sched")
string(APPEND OPENAA_C_FLAGS " -fmodulo-sched-allow-regmoves")

# Memory optimization details:
# -fprefetch-loop-arrays: Hardware prefetch insertion
#   • Performance: 20-40% for memory-bound code
#   • Example:
#     // Compiler inserts prefetch for next cache line
#     for(i=0; i<n; i++) {
#       __builtin_prefetch(&arr[i+8], 0, 1); // Added by compiler
#       process(arr[i]);
#     }
#   • Tuned for: A75's 64-byte cache lines, 150-cycle memory latency
#
# -foptimize-strlen: String function optimization
#   • Performance: Eliminates redundant strlen calls
#   • Example:
#     // Before: 3 strlen calls
#     if(strlen(s)>0 && strlen(s)<100) return strlen(s);
#     // After: 1 strlen call
#     len=strlen(s); if(len>0 && len<100) return len;
#   • Also optimizes: strcpy, strcat, strcmp chains
#
# -ftree-switch-conversion: Convert switch to lookup table
#   • Performance: O(1) instead of O(n) comparisons
#   • Example:
#     // Before: Up to 10 comparisons
#     switch(x) { case 0:return 5; case 1:return 3; ... }
#     // After: Single array lookup
#     static const int table[]={5,3,...}; return table[x];
#   • Works when: Dense case values, no side effects
#
# -fmodulo-sched: Software pipelining for loops
#   • Performance: Hides instruction latencies
#   • Example: Overlaps iterations of loops
#     // Iteration N+1 loads while iteration N computes
#   • Most effective on in-order cores (A55)
#
# -fmodulo-sched-allow-regmoves: Enhanced modulo scheduling
#   • Performance: Better register allocation in pipelined loops
#   • Allows moving registers between stages of pipeline

# ╔════════════════════════════════════════════════════════════════════╗
# ║                  CODE GENERATION OPTIMIZATIONS                     ║
# ╚════════════════════════════════════════════════════════════════════╝

string(APPEND OPENAA_C_FLAGS " -fomit-frame-pointer")
string(APPEND OPENAA_C_FLAGS " -ffunction-sections")
string(APPEND OPENAA_C_FLAGS " -fdata-sections")
string(APPEND OPENAA_C_FLAGS " -falign-functions=64")
string(APPEND OPENAA_C_FLAGS " -falign-jumps=32")
string(APPEND OPENAA_C_FLAGS " -falign-loops=32")
string(APPEND OPENAA_C_FLAGS " -falign-labels=16")
string(APPEND OPENAA_C_FLAGS " -fno-stack-protector")
string(APPEND OPENAA_C_FLAGS " -fmerge-all-constants")

# Code generation optimization details:
# -fomit-frame-pointer: Free up x29 register
#   • Performance: 3-5% improvement (extra register)
#   • Example: One more variable in register vs memory
#   • Debug: Still works with -fasynchronous-unwind-tables
#   • ARM64: Frees x29 for general use
#
# -ffunction-sections: Each function in own section
#   • Size: Enables --gc-sections to remove unused code
#   • Example:
#     // Instead of everything in .text:
#     .text.main, .text.helper, .text.unused_func
#   • Result: Linker removes .text.unused_func
#
# -fdata-sections: Each global in own section
#   • Size: Removes unused global variables
#   • Example:
#     // Instead of everything in .data:
#     .data.config, .data.buffer, .data.unused_table
#
# -falign-functions=64: Align to cache line
#   • Performance: No cache line splits for hot functions
#   • Example: main() starts at 0x400040 instead of 0x400038
#   • A75 I-cache: 64-byte lines, critical for hot paths
#   • Trade-off: Up to 63 bytes padding per function
#
# -falign-jumps=32: Align jump targets
#   • Performance: Better branch prediction
#   • Example: Loop headers aligned for BTB efficiency
#   • A75: 32-byte fetch window for branch predictor
#
# -falign-loops=32: Align loop entry points
#   • Performance: Loops start at efficient fetch boundary
#   • Example: Hot loops don't span prediction boundaries
#
# -falign-labels=16: Align other labels
#   • Performance: Minor improvement for goto targets
#   • Smaller alignment for less critical code paths
#
# -fno-stack-protector: Disable stack canaries
#   • Performance: Save 3-5% overhead
#   • Example: No __stack_chk_fail checks
#   • Trade-off: No buffer overflow detection
#   • Alternative: Enable selectively with attributes
#
# -fmerge-all-constants: Merge identical constants
#   • Size/Performance: Better cache usage
#   • Example:
#     const char s1[]="hello"; const char s2[]="hello";
#     // Merged into single copy in .rodata
#   • Warning: Don't take addresses of string literals

# ╔════════════════════════════════════════════════════════════════════╗
# ║             MINIMAL SAFETY FLAGS (Low Overhead)                    ║
# ╚════════════════════════════════════════════════════════════════════╝

string(APPEND OPENAA_C_FLAGS " -frandom-seed=${DETERMINISTIC_BUILD_SEED}")
string(APPEND OPENAA_C_FLAGS " -fno-delete-null-pointer-checks")
string(APPEND OPENAA_C_FLAGS " -fstack-clash-protection")
string(APPEND OPENAA_C_FLAGS " -fcf-protection=none")
string(APPEND OPENAA_C_FLAGS " -fpie")

# Minimal safety feature details:
# -frandom-seed=${DETERMINISTIC_BUILD_SEED}: Reproducible builds
#   • Purpose: Same input → identical binary output
#   • Example: Symbol names, hash tables deterministic
#   • Required: ISO 26262 traceability requirements
#   • Performance: Zero runtime impact
#
# -fno-delete-null-pointer-checks: Keep null checks
#   • Purpose: Preserve checks compiler thinks unnecessary
#   • Example:
#     if(p) { *p = 5; if(!p) error(); } // Second check kept
#   • Safety: Catches hardware-induced pointer corruption
#   • Performance: <0.1% overhead (few extra comparisons)
#
# -fstack-clash-protection: Probe large stack allocations
#   • Purpose: Prevent stack clash attacks
#   • Example:
#     char big[1048576]; // Probed in 4KB chunks
#   • Implementation: Touch each page during allocation
#   • Performance: <0.5% (only affects large allocations)
#
# -fcf-protection=none: Disable Intel CET (not ARM64)
#   • Purpose: Explicitly disable x86-only feature
#   • ARM64: No effect, but prevents warnings
#
# -fpie: Position Independent Executable
#   • Purpose: Enable ASLR (Address Space Layout Randomization)
#   • Example: Program loaded at random address each run
#   • ARM64 overhead: <1% (PC-relative addressing)
#   • Security: Prevents hardcoded address exploits

# ╔════════════════════════════════════════════════════════════════════╗
# ║                    WARNING FLAGS (Zero Overhead)                   ║
# ╚════════════════════════════════════════════════════════════════════╝

string(APPEND OPENAA_C_FLAGS " -Wall")
string(APPEND OPENAA_C_FLAGS " -Wextra")
string(APPEND OPENAA_C_FLAGS " -Werror")
string(APPEND OPENAA_C_FLAGS " -Winit-self")
string(APPEND OPENAA_C_FLAGS " -Walloc-zero")
string(APPEND OPENAA_C_FLAGS " -Wformat-signedness")
string(APPEND OPENAA_C_FLAGS " -Wredundant-decls")
string(APPEND OPENAA_C_FLAGS " -Wstringop-overflow=2")
string(APPEND OPENAA_C_FLAGS " -Wformat=2")
string(APPEND OPENAA_C_FLAGS " -Wformat-overflow=2")
string(APPEND OPENAA_C_FLAGS " -Wformat-truncation=2")
string(APPEND OPENAA_C_FLAGS " -Wformat-security")
string(APPEND OPENAA_C_FLAGS " -Wnull-dereference")
string(APPEND OPENAA_C_FLAGS " -Wstack-usage=8192")
string(APPEND OPENAA_C_FLAGS " -Wvla-larger-than=1024")
string(APPEND OPENAA_C_FLAGS " -Warray-bounds=2")
string(APPEND OPENAA_C_FLAGS " -Wimplicit-fallthrough=3")
string(APPEND OPENAA_C_FLAGS " -Wconversion")
string(APPEND OPENAA_C_FLAGS " -Wdouble-promotion")
string(APPEND OPENAA_C_FLAGS " -Wcast-align=strict")
string(APPEND OPENAA_C_FLAGS " -Wcast-qual")
string(APPEND OPENAA_C_FLAGS " -Wpointer-arith")
string(APPEND OPENAA_C_FLAGS " -Wshadow")
string(APPEND OPENAA_C_FLAGS " -Wlogical-op")
string(APPEND OPENAA_C_FLAGS " -Wduplicated-cond")
string(APPEND OPENAA_C_FLAGS " -Wduplicated-branches")
string(APPEND OPENAA_C_FLAGS " -Wrestrict")
string(APPEND OPENAA_C_FLAGS " -Walloca")
string(APPEND OPENAA_C_FLAGS " -Wfloat-equal")
string(APPEND OPENAA_C_FLAGS " -Wundef")
string(APPEND OPENAA_C_FLAGS " -Wswitch-enum")
string(APPEND OPENAA_C_FLAGS " -Wswitch-default")
string(APPEND OPENAA_C_FLAGS " -Wswitch-bool")
string(APPEND OPENAA_C_FLAGS " -Wswitch-unreachable")
string(APPEND OPENAA_C_FLAGS " -Wdate-time")
string(APPEND OPENAA_C_FLAGS " -Wtrampolines")
string(APPEND OPENAA_C_FLAGS " -Wstrict-overflow=1")


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
# -Wstack-usage=8192: Large stack frame warning
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
#   • ARM64: Misaligned access works but slower
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
# -Wtrampolines: Trampoline function warnings
#   • Example: Function pointer to a trampoline function
#   • Safety: Prevents misuse of function pointers
#
# -WStrict-overflow=1: Strict aliasing assumptions
#   • Example: int x=5; if(x+1>5) { /* x is not modified */ }
#   • Performance: Allows more aggressive optimizations
#   • Safety: Ensures no undefined behavior from aliasing

# C-specific warnings
string(APPEND OPENAA_C_FLAGS " -Wnested-externs")
string(APPEND OPENAA_C_FLAGS " -Wjump-misses-init")
string(APPEND OPENAA_C_FLAGS " -Wmissing-prototypes")
string(APPEND OPENAA_C_FLAGS " -Wstrict-prototypes")
string(APPEND OPENAA_C_FLAGS " -Wbad-function-cast")
string(APPEND OPENAA_C_FLAGS " -Wold-style-definition")
string(APPEND OPENAA_C_FLAGS " -Wmissing-declarations")
string(APPEND OPENAA_C_FLAGS " -Wtraditional-conversion")

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
# ║                 PROCESSOR-SPECIFIC TUNING                          ║
# ╚════════════════════════════════════════════════════════════════════╝

string(APPEND OPENAA_C_FLAGS " --param l1-cache-size=64")
string(APPEND OPENAA_C_FLAGS " --param l1-cache-line-size=64")
string(APPEND OPENAA_C_FLAGS " --param l2-cache-size=512")
string(APPEND OPENAA_C_FLAGS " --param prefetch-latency=150")
string(APPEND OPENAA_C_FLAGS " --param simultaneous-prefetches=6")
string(APPEND OPENAA_C_FLAGS " --param prefetch-min-insn-to-mem-ratio=3")
string(APPEND OPENAA_C_FLAGS " --param max-inline-insns-auto=100")
string(APPEND OPENAA_C_FLAGS " --param max-inline-insns-single=1000")
string(APPEND OPENAA_C_FLAGS " --param inline-unit-growth=30")
string(APPEND OPENAA_C_FLAGS " --param large-function-growth=200")

# Processor tuning parameter details:
# --param l1-cache-size=64: L1 data cache size (KB)
#   • A75: 64KB 4-way set-associative L1D
#   • Affects: Loop tiling, blocking decisions
#
# --param l1-cache-line-size=64: Cache line size
#   • A75: 64-byte cache lines throughout
#   • Affects: Prefetch distances, alignment
#
# --param l2-cache-size=512: L2 unified cache (KB)
#   • A75: 256KB or 512KB options
#   • Affects: Function/loop size decisions
#
# --param prefetch-latency=150: Memory latency cycles
#   • A75: ~150 cycles to DRAM
#   • Affects: How far ahead to prefetch
#
# --param simultaneous-prefetches=6: Concurrent prefetches
#   • A75: Can track 6 outstanding prefetches
#   • Prevents: Prefetch queue overflow
#
# --param prefetch-min-insn-to-mem-ratio=3: When to prefetch
#   • Heuristic: Need 3 instructions per memory access
#   • Prevents: Prefetching memory-bound code
#
# --param max-inline-insns-auto=100: Auto-inline limit
#   • Functions ≤100 instructions inlined at -O3
#   • Example: Small getters/setters always inlined
#
# --param max-inline-insns-single=1000: Single call inline
#   • Functions ≤1000 instructions if called once
#   • Example: Static init functions fully inlined
#
# --param inline-unit-growth=30: Unit growth limit (%)
#   • Translation unit can grow 30% from inlining
#   • Prevents: Excessive code bloat
#
# --param large-function-growth=200: Large function growth
#   • Large functions can double from inlining
#   • Hot paths: Aggressive inlining allowed

# Now set the complete flags ONCE to avoid duplication
set(CMAKE_C_FLAGS_INIT "${OPENAA_C_FLAGS}" CACHE STRING "C compiler flags for maximum performance")

#-----------------------------------------------------------------------
# Build Type Specific C Flags
#-----------------------------------------------------------------------

# Build flags in a variable first to avoid overwriting
set(OPENAA_C_FLAGS_RELEASE "")

# Release build - Maximum performance additional flags
string(APPEND OPENAA_C_FLAGS_RELEASE " -DNDEBUG")
string(APPEND OPENAA_C_FLAGS_RELEASE " -fwhole-program")
string(APPEND OPENAA_C_FLAGS_RELEASE " -fgcse-sm")
string(APPEND OPENAA_C_FLAGS_RELEASE " -fgcse-las")
string(APPEND OPENAA_C_FLAGS_RELEASE " -fira-loop-pressure")
string(APPEND OPENAA_C_FLAGS_RELEASE " -ftree-partial-pre")

# Release build - Maximum performance (additional flags beyond base)
set(CMAKE_C_FLAGS_RELEASE "${OPENAA_C_FLAGS_RELEASE}" CACHE STRING "C Release flags")

# Release flag details:
# -DNDEBUG: Disable debug assertions
#   • Performance: Removes assert() checks
#   • Example: assert(x > 0); → no runtime check
#   • Safety: No runtime checks, use carefully
#
# -fwhole-program: Assume single compilation unit
#   • Performance: More aggressive optimization
#   • Requirement: All code in one binary
#
# -fgcse-sm: Global common subexpression elimination
#   • Example: Store motion out of loops
#
# -fgcse-las: GCSE load after store
#   • Example: Redundant load elimination
#
# -fira-loop-pressure: Register pressure aware scheduling
#   • Reduces spills in tight loops
#
# -ftree-partial-pre: Partial redundancy elimination
#   • Hoists expressions from some paths

#=======================================================================
# C++ Compiler Flags - MAXIMUM PERFORMANCE
#=======================================================================

# Build C++ flags starting with C flags
set(OPENAA_CXX_FLAGS "${OPENAA_C_FLAGS}")

# ╔════════════════════════════════════════════════════════════════════╗
# ║                    C++ SPECIFIC OPTIMIZATIONS                      ║
# ╚════════════════════════════════════════════════════════════════════╝

string(APPEND OPENAA_CXX_FLAGS " -fno-exceptions")
string(APPEND OPENAA_CXX_FLAGS " -fno-rtti")
string(APPEND OPENAA_CXX_FLAGS " -fno-enforce-eh-specs")
string(APPEND OPENAA_CXX_FLAGS " -fvisibility-inlines-hidden")
string(APPEND OPENAA_CXX_FLAGS " -fdevirtualize")
string(APPEND OPENAA_CXX_FLAGS " -fdevirtualize-speculatively")
string(APPEND OPENAA_CXX_FLAGS " -fdevirtualize-at-ltrans")

# C++ optimization details:
# -fno-exceptions: Remove exception handling
#   • Performance: 5-15% improvement typical
#   • Code size: 10-20% smaller binary
#   • Example: No .eh_frame section, no unwind tables
#   • Requirement: No throw/try/catch in codebase
#   • Stack usage: Reduced by ~20% (no exception state)
#
# -fno-rtti: Remove Runtime Type Information
#   • Performance: 5-10% improvement
#   • Memory: Saves 4-8 bytes per polymorphic class
#   • Example: No .rodata._ZTI* typeinfo sections
#   • Restriction: No dynamic_cast<>, typeid()
#   • Alternative: Use enum/virtual function for type
#
# -fno-enforce-eh-specs: Disable exception specifiers
#   • Performance: Avoids checks for throw() specifications
#   • Example: void f() throw(int); → no runtime checks
#   • Safety: No guarantees, but avoids overhead
#   • C++11: Exception specifications deprecated
#   • Note: Use noexcept for modern C++ exception guarantees
#
# -fvisibility-inlines-hidden: Hide inline functions
#   • Performance: No PLT/GOT for inline calls
#   • Size: Smaller export tables
#   • Example: 
#     class X { inline void f() {} }; // Not exported
#   • ABI: Inlines are private to compilation unit
#
# -fdevirtualize: Convert virtual to direct calls
#   • Performance: 10-30% for OOP code
#   • Example:
#     Base *p = new Derived;
#     p->vfunc(); // Becomes Derived::vfunc() directly
#   • When: Type provable at compile time
#
# -fdevirtualize-speculatively: Profile-guided devirtualization
#   • Performance: Additional 5-10% improvement
#   • Example:
#     // Generates: if(likely Derived) Derived::f(); else p->f();
#   • Requires: PGO data or heuristics
#
# -fdevirtualize-at-ltrans: LTO devirtualization
#   • Performance: Cross-TU devirtualization
#   • Example: Interface in header, all implementations visible
#   • Requires: -flto enabled

# ╔════════════════════════════════════════════════════════════════════╗
# ║                    C++ SPECIFIC WARNINGS                           ║
# ╚════════════════════════════════════════════════════════════════════╝

# Remove C-only warnings first
string(REPLACE " -Wnested-externs"         ""  OPENAA_CXX_FLAGS  "${OPENAA_CXX_FLAGS}")
string(REPLACE " -Wjump-misses-init"       ""  OPENAA_CXX_FLAGS  "${OPENAA_CXX_FLAGS}")
string(REPLACE " -Wmissing-prototypes"     ""  OPENAA_CXX_FLAGS  "${OPENAA_CXX_FLAGS}")
string(REPLACE " -Wstrict-prototypes"      ""  OPENAA_CXX_FLAGS  "${OPENAA_CXX_FLAGS}")
string(REPLACE " -Wold-style-definition"   ""  OPENAA_CXX_FLAGS  "${OPENAA_CXX_FLAGS}")
string(REPLACE " -Wmissing-declarations"   ""  OPENAA_CXX_FLAGS  "${OPENAA_CXX_FLAGS}")
string(REPLACE " -Wbad-function-cast"      ""  OPENAA_CXX_FLAGS  "${OPENAA_CXX_FLAGS}")
string(REPLACE " -Wtraditional-conversion" ""  OPENAA_CXX_FLAGS  "${OPENAA_CXX_FLAGS}")

# Add C++ specific warnings
string(APPEND OPENAA_CXX_FLAGS " -Weffc++")
string(APPEND OPENAA_CXX_FLAGS " -Wzero-as-null-pointer-constant")
string(APPEND OPENAA_CXX_FLAGS " -Wsuggest-final-methods")
string(APPEND OPENAA_CXX_FLAGS " -Wstrict-null-sentinel")
string(APPEND OPENAA_CXX_FLAGS " -Wsuggest-final-types")
string(APPEND OPENAA_CXX_FLAGS " -Wdelete-non-virtual-dtor")
string(APPEND OPENAA_CXX_FLAGS " -Woverloaded-virtual")
string(APPEND OPENAA_CXX_FLAGS " -Wsuggest-override")
string(APPEND OPENAA_CXX_FLAGS " -Wnon-virtual-dtor")
string(APPEND OPENAA_CXX_FLAGS " -Wplacement-new=2")
string(APPEND OPENAA_CXX_FLAGS " -Wold-style-cast")
string(APPEND OPENAA_CXX_FLAGS " -Wuseless-cast")
string(APPEND OPENAA_CXX_FLAGS " -Wsign-promo")
string(APPEND OPENAA_CXX_FLAGS " -Wextra-semi")
string(APPEND OPENAA_CXX_FLAGS " -Wnoexcept")

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

# Set C++ flags
set(CMAKE_CXX_FLAGS_INIT "${OPENAA_CXX_FLAGS}" CACHE STRING "C++ compiler flags for maximum performance")

# C++ build type specific flags (same as C)
# Build flags in a variable first to avoid overwriting
set(OPENAA_CXX_FLAGS_RELEASE "")

# Release build - Maximum performance additional flags
string(APPEND OPENAA_CXX_FLAGS_RELEASE " -DNDEBUG")
string(APPEND OPENAA_CXX_FLAGS_RELEASE " -fwhole-program")
string(APPEND OPENAA_CXX_FLAGS_RELEASE " -fgcse-sm")
string(APPEND OPENAA_CXX_FLAGS_RELEASE " -fgcse-las")
string(APPEND OPENAA_CXX_FLAGS_RELEASE " -fira-loop-pressure")
string(APPEND OPENAA_CXX_FLAGS_RELEASE " -ftree-partial-pre")

set(CMAKE_CXX_FLAGS_RELEASE "${OPENAA_CXX_FLAGS_RELEASE}" CACHE STRING "C++ Release flags")

#=======================================================================
# Linker Flags - MAXIMUM PERFORMANCE & HARDENING
#=======================================================================

# ╔════════════════════════════════════════════════════════════════════╗
# ║                    EXECUTABLE LINKER FLAGS                         ║
# ╚════════════════════════════════════════════════════════════════════╝

set(OPENAA_EXEC_LINKER_FLAGS "")

string(APPEND OPENAA_EXEC_LINKER_FLAGS " -Wl,-O3")
string(APPEND OPENAA_EXEC_LINKER_FLAGS " -Wl,--gc-sections")
string(APPEND OPENAA_EXEC_LINKER_FLAGS " -Wl,--as-needed")
string(APPEND OPENAA_EXEC_LINKER_FLAGS " -Wl,--hash-style=gnu")
string(APPEND OPENAA_EXEC_LINKER_FLAGS " -Wl,--sort-common=descending")
string(APPEND OPENAA_EXEC_LINKER_FLAGS " -Wl,--build-id=sha1")
string(APPEND OPENAA_EXEC_LINKER_FLAGS " -pie")
string(APPEND OPENAA_EXEC_LINKER_FLAGS " -Wl,-z,relro")
string(APPEND OPENAA_EXEC_LINKER_FLAGS " -Wl,-z,now")
string(APPEND OPENAA_EXEC_LINKER_FLAGS " -Wl,-z,noexecstack")
string(APPEND OPENAA_EXEC_LINKER_FLAGS " -Wl,-z,separate-code")
string(APPEND OPENAA_EXEC_LINKER_FLAGS " -Wl,-z,defs")

set(CMAKE_EXE_LINKER_FLAGS_INIT "${OPENAA_EXEC_LINKER_FLAGS}" CACHE STRING "Executable linker flags")

# Linker optimization details:
# -Wl,-O3: Maximum linker optimization
#   • Optimizes: GOT/PLT layout, branch islands
#   • Performance: 2-5% improvement for call-heavy code
#   • Example: Groups hot functions together
#
# -Wl,--gc-sections: Garbage collect sections
#   • Size: Removes unreferenced functions/data
#   • Requires: -ffunction-sections -fdata-sections
#   • Typical: 10-30% size reduction
#
# -Wl,--as-needed: Only link used libraries
#   • Startup: Fewer relocations, faster launch
#   • Example: -lpthread dropped if not used
#
# -Wl,--hash-style=gnu: GNU hash table
#   • Performance: 30-50% faster symbol lookup
#   • Uses: Bloom filter for quick rejection
#
# -Wl,--sort-common=descending: Optimize BSS layout
#   • Size: Better packing of common symbols
#   • Example: Large structures first, less padding
#
# -Wl,--build-id=sha1: Unique binary identifier
#   • Debug: Maps crashes to exact binary
#   • Size: 20-byte .note.gnu.build-id section
#
# -pie: Position Independent Executable
#   • Security: Full ASLR randomization
#   • ARM64: Near-zero overhead (PC-relative)
#
# -Wl,-z,relro: Read-Only Relocations
#   • Security: GOT/PLT read-only after startup
#   • Prevents: GOT overwrite attacks
#
# -Wl,-z,now: Immediate binding
#   • Security: All symbols resolved at startup
#   • Trade-off: 1-4ms slower startup
#
# -Wl,-z,noexecstack: Non-executable stack
#   • Security: Prevents code injection
#   • Required: W^X (Write XOR Execute)
#
# -Wl,-z,separate-code: Separate code/data
#   • Security: Code pages never writable
#   • Performance: Better TLB usage
#
# -Wl,-z,defs: No undefined symbols
#   • Safety: Link fails if symbols missing
#   • Catches: Missing library dependencies


# Set initial release linker flags
set(OPENAA_EXEC_LINKER_FLAGS_RELEASE "")

# Release build - Maximum performance additional flags
string(APPEND OPENAA_EXEC_LINKER_FLAGS_RELEASE " -Wl,--strip-all")
string(APPEND OPENAA_EXEC_LINKER_FLAGS_RELEASE " -Wl,--discard-all")

# Release-only extras
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${OPENAA_EXEC_LINKER_FLAGS_RELEASE}" CACHE STRING "Release executable linker flags")

# -Wl,--strip-all: Remove all symbols
#   • Size: Minimum binary size
#   • Debug: Keep separate .debug file
#
# -Wl,--discard-all: Remove local symbols
#   • Size: Additional ~5% reduction

# ╔════════════════════════════════════════════════════════════════════╗
# ║                 SHARED LIBRARY LINKER FLAGS                        ║
# ╚════════════════════════════════════════════════════════════════════╝

# Set initial shared library linker flags
set(OPENAA_SHARED_LINKER_FLAGS "")

string(APPEND OPENAA_SHARED_LINKER_FLAGS " -Wl,-O3")
string(APPEND OPENAA_SHARED_LINKER_FLAGS " -Wl,--as-needed")
string(APPEND OPENAA_SHARED_LINKER_FLAGS " -Wl,--sort-common")
string(APPEND OPENAA_SHARED_LINKER_FLAGS " -Wl,--hash-style=gnu")
string(APPEND OPENAA_SHARED_LINKER_FLAGS " -Wl,-z,relro")
string(APPEND OPENAA_SHARED_LINKER_FLAGS " -Wl,-z,now")
string(APPEND OPENAA_SHARED_LINKER_FLAGS " -Wl,-z,noexecstack")

set(CMAKE_SHARED_LINKER_FLAGS_INIT "${OPENAA_SHARED_LINKER_FLAGS}" CACHE STRING "Shared library linker flags")

set(OPENAA_SHARED_LINKER_FLAGS_RELEASE "")

# Release build - Maximum performance additional flags
string(APPEND OPENAA_SHARED_LINKER_FLAGS_RELEASE " -Wl,--gc-sections")
string(APPEND OPENAA_SHARED_LINKER_FLAGS_RELEASE " -Wl,--version-script,${CMAKE_SOURCE_DIR}/exports.ver")


set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${OPENAA_SHARED_LINKER_FLAGS_RELEASE}" CACHE STRING "Release shared library linker flags")

# -Wl,--version-script: Symbol versioning
#   • ABI: Control exported symbols
#   • Size: Hide internal symbols
#   • Example: VERS_1.0 { global: api_*; local: *; };

set(CMAKE_STATIC_LINKER_FLAGS_INIT "" CACHE STRING "Static library flags")

#=======================================================================
# Profile-Guided Optimization (PGO) Support
#=======================================================================

option(ENABLE_PGO_GENERATE "Build instrumented binaries to collect profiles" OFF)
option(ENABLE_PGO_USE "Re-build using the collected profile data" OFF)

# Directory for profile data
set(PGO_DIR "${CMAKE_BINARY_DIR}/pgo-data" CACHE PATH "PGO data directory")

if(ENABLE_PGO_GENERATE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-generate=${PGO_DIR} -fprofile-dir=${PGO_DIR}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-generate=${PGO_DIR} -fprofile-dir=${PGO_DIR}")
    message(STATUS "PGO: Generation enabled → ${PGO_DIR}")
    message(STATUS "     Run typical workloads after building")
endif()

if(ENABLE_PGO_USE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-use=${PGO_DIR} -fprofile-correction")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-use=${PGO_DIR} -fprofile-correction")
    message(STATUS "PGO: Using profile data → 10-15% speedup")
endif()

# PGO workflow details:
# 1. Build with ENABLE_PGO_GENERATE=ON
# 2. Run representative workloads:
#    - Highway driving scenarios
#    - City traffic patterns  
#    - Sensor data processing
#    - Control loop execution
# 3. Profile data in ${PGO_DIR}/*.gcda
# 4. Rebuild with ENABLE_PGO_USE=ON
# 5. Result: 10-15% additional performance

#=======================================================================
# Build-Safe Configuration Validation
#=======================================================================

function(validate_safety_compliance)
    set(UNSAFE_MATH_FLAGS
        "-ffast-math"                 # Breaks IEEE 754
        "-Ofast"                      # Includes -ffast-math
        "-funsafe-math-optimizations" # Violates IEEE
        "-fassociative-math"          # Reorders operations
        "-ffinite-math-only"          # Assumes no NaN/Inf
        "-fno-honor-nans"             # Ignores NaN
        "-fno-honor-infinities"       # Ignores Inf
        "-freciprocal-math"           # Approximate division
        "-fcx-limited-range"          # Complex math shortcuts
        "-fno-signed-zeros"           # Loses -0.0 vs +0.0
    )
    
    foreach(flag ${UNSAFE_MATH_FLAGS})
        string(FIND "${CMAKE_C_FLAGS} ${CMAKE_CXX_FLAGS}" "${flag}" found_pos)
        if(found_pos GREATER -1)
            message(FATAL_ERROR 
                "Unsafe flag '${flag}' detected - build aborted.\n"
                "This flag can produce incorrect results.\n"
                "Remove it or document why it's safe for your use case.")
        endif()
    endforeach()
endfunction()

# Validate configuration
validate_safety_compliance()

#=======================================================================
# Advanced Build Options
#=======================================================================

# Unity builds for faster compilation
set(CMAKE_UNITY_BUILD ON CACHE BOOL "Enable unity builds")
set(CMAKE_UNITY_BUILD_BATCH_SIZE 8 CACHE STRING "Unity batch size")

# Unity build details:
# • Combines multiple .cpp files into one
# • Compilation: 50-70% faster
# • Better: Cross-file optimization
# • Risk: Name collisions, larger memory use

#=======================================================================
# Deprecated/Unused Flags Documentation
#=======================================================================

# -Wredundant-tags: Unnecessary class/struct keywords
#   • Example: class X; class X* p; → X* p;
#   • Cleaner: Reduces visual clutter
#   • Supported in GCC9
# -Wl,--icf=all: Identical Code Folding
#   • Size: Merges byte-identical functions
#   • Example: Template instantiations folded
#   • Typical: 5-15% code size reduction
#
#
# -msign-return-address=all: ARM64 PAC (Pointer Authentication)
#   • Status: Not available in GCC 8.3.0 for ARMv8.2-A
#   • Requires: ARMv8.3-A or later (Cortex-A75 is ARMv8.2-A)
#   • Alternative: Will trap with SIGILL on A75
#   • Future: Use -mbranch-protection in GCC 9+
#
# -mbranch-protection=standard: Unified PAC/BTI control
#   • Status: Introduced in GCC 9, not in QCC-8
#   • Future: Preferred way to enable pointer authentication
#
# -fcf-protection=full: Intel CET (Control-flow Enforcement)
#   • Platform: x86-only, no effect on ARM64
#   • QNX: Not applicable to aarch64 target
#
# -mindirect-branch=thunk: Spectre v2 mitigation
#   • Platform: x86-only retpoline mitigation
#   • ARM64: Different speculation barriers used
#
# -mspeculative-load-hardening: Spectre v1 mitigation
#   • Status: Clang-only, not in GCC 8.3.0
#
# -fstack-limit-*: Stack limit checking
#   • QNX: Thread stacks set explicitly
#   • Redundant: QNX uses guard pages
#
# DOCUMENTED BUT DISABLED FOR PERFORMANCE:
#
# -ftrapv: Trap on signed integer overflow
#   • Overhead: 5-10% for integer arithmetic
#   • Example: INT_MAX + 1 → SIGILL instead of wraparound
#   • Use case: Safety-critical calculations
#   • Alternative: __builtin_add_overflow() where needed
#
# -fstack-protector-strong: Stack buffer overflow detection
#   • Overhead: 3-5% average performance loss
#   • Example: Adds canary checks to functions with arrays
#   • Protection: Detects stack smashing attacks
#   • Alternative: Static analysis, careful coding
#
# -D_FORTIFY_SOURCE=2: Runtime bounds checking
#   • Overhead: 2-5% for string/memory operations
#   • Example: strcpy → __strcpy_chk with size validation
#   • Protection: Buffer overflow detection
#   • Alternative: strlcpy, snprintf, bounds checking
#
# -fsanitize=address: AddressSanitizer
#   • Overhead: 50-100% performance, 2-3x memory
#   • Protection: Comprehensive memory error detection
#   • Use case: Development and testing only
#
# -fsanitize=undefined: UndefinedBehaviorSanitizer
#   • Overhead: 20-50% performance impact
#   • Protection: Catches undefined behavior at runtime
#   • Use case: Testing and validation only

#=======================================================================
# Summary and Usage
#=======================================================================

message(STATUS "")
message(STATUS "Performance Configuration Summary:")
message(STATUS "- Optimization: -O3 with aggressive flags")
message(STATUS "- Architecture: ARMv8.2-A + Cortex-A75 specific")
message(STATUS "- LTO: Enabled with parallel compilation")
message(STATUS "- Safety: Minimal overhead features only")
message(STATUS "")