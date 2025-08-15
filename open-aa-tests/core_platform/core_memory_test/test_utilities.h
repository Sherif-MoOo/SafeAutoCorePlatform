/***********************************************************************************************************************
 *  TEST UTILITIES
 *  --------------------------------------------------------------------------------------------------------------------
 *  \file       test_utilities.h
 *  \brief      Test utilities, helpers and verification functions
 *  
 *  \details    Provides testing infrastructure including timing, verification,
 *              and pattern generation utilities
 ***********************************************************************************************************************/
#ifndef TEST_UTILITIES_H_
#define TEST_UTILITIES_H_

#include <cstddef>
#include <cstdint>
#include <chrono>
#include <algorithm>
#include <random>
#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <limits>
#include <cstdlib>
#include <type_traits>
#include "test_unsafe_wrapper.h"

namespace test {

namespace detail {

static inline const void* memchr_const_wrapper(const void* p, int v, std::size_t n) noexcept {
    return std::memchr(p, v, n);
}

static inline void* memchr_mut_wrapper(void* p, int v, std::size_t n) noexcept {
    return std::memchr(p, v, n);
}


} // namespace detail

// ====================================================================================================================
// CONSTANTS
// ====================================================================================================================

// Test buffer sizes covering all Memory class thresholds
constexpr std::size_t kTestSizes[] = {
    0,      // Empty
    1,      // Single byte
    3,      // Small odd
    4,      // Small power of 2
    7,      // Small prime
    8,      // 64-bit boundary
    15,     // Just under 16
    16,     // SSE/NEON boundary
    17,     // Just over 16
    31,     // Just under small threshold
    32,     // Small threshold boundary
    33,     // Just over small threshold
    63,     // Just under 64
    64,     // Cache line size
    127,    // Just under 128
    128,    // Medium size
    255,    // Just under medium threshold
    256,    // Medium threshold boundary
    257,    // Just over medium threshold
    512,    // Large power of 2
    1024,   // 1KB
    4096,   // 4KB page
    8192,   // 8KB
    16384,  // 16KB
    65536,  // 64KB
    262144  // 256KB
};

// Pattern values for testing
constexpr std::uint8_t kPatternValues[] = {
    0x00,   // Zero
    0xFF,   // All ones
    0xAA,   // Alternating 10101010
    0x55,   // Alternating 01010101
    0x0F,   // Low nibble
    0xF0,   // High nibble
    0x01,   // Minimum non-zero
    0xFE,   // Maximum non-full
    0x7F,   // Signed maximum
    0x80,   // Signed minimum
    0x42,   // Arbitrary value
    0xDE,   // Another arbitrary
};

// ====================================================================================================================
// TIMING UTILITIES
// ====================================================================================================================

/*!
 * \brief High-resolution timer for benchmarking
 */
class Timer {
public:
    using clock_type = std::chrono::high_resolution_clock;
    using time_point = clock_type::time_point;
    using duration = std::chrono::duration<double, std::nano>;
    
    Timer() noexcept : start_{clock_type::now()} {}
    
    [[nodiscard]] auto elapsed() const noexcept -> double {
        const auto end = clock_type::now();
        const duration diff = end - start_;
        return diff.count();
    }
    
    auto reset() noexcept -> void {
        start_ = clock_type::now();
    }
    
private:
    time_point start_;
};

// ====================================================================================================================
// BUFFER MANAGEMENT
// ====================================================================================================================

/*!
 * \brief Aligned buffer for testing
 * 
 * \details Ensures proper alignment for SIMD operations and cache line boundaries
 */
template<typename T = std::uint8_t>
class AlignedBuffer {
public:
    static constexpr std::size_t kAlignment = 64;  // Cache line alignment
    
    explicit AlignedBuffer(std::size_t count) noexcept 
        : size_{count}
        , buffer_{nullptr} {
        if (count > 0) {
            // Allocate with alignment
            void* ptr = nullptr;
            if (::posix_memalign(&ptr, kAlignment, count * sizeof(T)) == 0) {
                buffer_ = static_cast<T*>(ptr);
            }
        }
    }
    
    ~AlignedBuffer() noexcept {
        if (buffer_) {
            ::free(buffer_);
        }
    }
    
    // Delete copy operations
    AlignedBuffer(const AlignedBuffer&) = delete;
    auto operator=(const AlignedBuffer&) -> AlignedBuffer& = delete;
    
    // Move operations
    AlignedBuffer(AlignedBuffer&& other) noexcept 
        : size_{other.size_}
        , buffer_{other.buffer_} {
        other.size_ = 0;
        other.buffer_ = nullptr;
    }
    
    auto operator=(AlignedBuffer&& other) noexcept -> AlignedBuffer& {
        if (this != &other) {
            if (buffer_) {
                ::free(buffer_);
            }
            size_ = other.size_;
            buffer_ = other.buffer_;
            other.size_ = 0;
            other.buffer_ = nullptr;
        }
        return *this;
    }
    
    [[nodiscard]] auto data() noexcept -> T* { return buffer_; }
    [[nodiscard]] auto data() const noexcept -> const T* { return buffer_; }
    [[nodiscard]] auto size() const noexcept -> std::size_t { return size_; }
    [[nodiscard]] auto valid() const noexcept -> bool { return buffer_ != nullptr || size_ == 0; }
    
private:
    std::size_t size_;
    T* buffer_;
};

// ====================================================================================================================
// PATTERN GENERATION
// ====================================================================================================================

/*!
 * \brief Fills buffer with pattern for testing
 */
template<typename T>
inline auto fill_pattern(T* buffer, std::size_t count, std::uint8_t pattern) noexcept -> void {
    if (buffer && count > 0) {
        static_cast<void>(test::UnsafeOps::call(std::memset, buffer, pattern, count * sizeof(T)));
    }
}

/*!
 * \brief Fills buffer with sequential values
 */
template<typename T>
inline auto fill_sequential(T* buffer, std::size_t count, T start = 0) noexcept -> void {
    for (std::size_t i = 0; i < count; ++i) {
        buffer[i] = static_cast<T>(start + i);
    }
}

/*!
 * \brief Fills buffer with random values
 */
template<typename T>
inline auto fill_random(T* buffer, std::size_t count, unsigned seed = 42) noexcept -> void {
    std::mt19937 gen(seed);
    
    if constexpr (std::is_integral_v<T>) {
        std::uniform_int_distribution<std::conditional_t<sizeof(T) == 1, int, T>> dist(
            std::numeric_limits<T>::min(),
            std::numeric_limits<T>::max()
        );
        for (std::size_t i = 0; i < count; ++i) {
            buffer[i] = static_cast<T>(dist(gen));
        }
    } else {
        std::uniform_real_distribution<T> dist(T{0}, T{1});
        for (std::size_t i = 0; i < count; ++i) {
            buffer[i] = dist(gen);
        }
    }
}

// ====================================================================================================================
// VERIFICATION UTILITIES  
// ====================================================================================================================

/*!
 * \brief Verifies buffer contains expected pattern
 */
inline auto verify_pattern(const std::uint8_t* buffer, std::size_t count, std::uint8_t pattern) noexcept -> bool {
    for (std::size_t i = 0; i < count; ++i) {
        if (test::UnsafeOps::at(buffer, i) != pattern) {
            return false;
        }
    }
    return true;
}

/*!
 * \brief Verifies two buffers are equal
 */
inline auto verify_equal(const void* buf1, const void* buf2, std::size_t count) noexcept -> bool {
    return test::UnsafeOps::call(std::memcmp, buf1, buf2, count) == 0;
}

/*!
 * \brief Calculates checksum for verification
 */
inline auto calculate_checksum(const std::uint8_t* buffer, std::size_t count) noexcept -> std::uint64_t {
    std::uint64_t checksum = 0;
    for (std::size_t i = 0; i < count; ++i) {
        checksum = (checksum << 1) ^ test::UnsafeOps::at(buffer, i);
    }
    return checksum;
}

// ====================================================================================================================
// OUTPUT UTILITIES
// ====================================================================================================================

/*!
 * \brief Test result reporter
 */
class TestReporter {
public:
    TestReporter() noexcept
        : total_tests_{0}, passed_tests_{0}, failed_tests_{} {}

    auto report_test(const char* name, bool passed) noexcept -> void {
        ++total_tests_;
        if (passed) {
            ++passed_tests_;
            std::cout << "[PASS] " << name << "\n";
        } else {
            std::cout << "[FAIL] " << name << "\n";
            failed_tests_.emplace_back(name);
        }
    }

    auto report_benchmark(const char* name, double ns_per_op) noexcept -> void {
        std::cout << "[BENCH] " << name << ": "
                  << std::fixed << std::setprecision(2) << ns_per_op
                  << " ns/op\n";
    }

    auto report_comparison(const char* name1, double time1,
                           const char* name2, double time2) noexcept -> void {
        const double speedup = time2 / time1;
        std::cout << "[COMPARE] " << name1 << " vs " << name2 << ": "
                  << std::fixed << std::setprecision(2) << speedup << "x "
                  << (speedup > 1.0 ? "faster" : "slower") << "\n";
    }

    auto print_summary() const noexcept -> void {
        std::cout << "\n========================================\n";
        std::cout << "TEST SUMMARY\n";
        std::cout << "========================================\n";
        std::cout << "Total:  " << total_tests_ << "\n";
        std::cout << "Passed: " << passed_tests_ << "\n";
        std::cout << "Failed: " << (total_tests_ - passed_tests_) << "\n";
        if (!failed_tests_.empty()) {
            std::cout << "\nFailed tests:\n";
            for (const auto& test : failed_tests_) std::cout << "  - " << test << "\n";
        }
        std::cout << "========================================\n";
    }

    [[nodiscard]] auto all_passed() const noexcept -> bool {
        return total_tests_ > 0 && passed_tests_ == total_tests_;
    }

private:
    std::size_t total_tests_;
    std::size_t passed_tests_;
    std::vector<std::string> failed_tests_;
};

// ====================================================================================================================
// BENCHMARK RUNNER
// ====================================================================================================================

/*!
 * \brief Runs benchmark with warmup and multiple iterations
 */
template<typename Func>
inline auto benchmark(Func&& func, std::size_t iterations = 1000, std::size_t warmup = 100) noexcept -> double {
    // Warmup runs
    for (std::size_t i = 0; i < warmup; ++i) {
        func();
    }
    
    // Timed runs
    Timer timer;
    for (std::size_t i = 0; i < iterations; ++i) {
        func();
    }
    
    return timer.elapsed() / static_cast<double>(iterations);
}

} // namespace test

#endif // TEST_UTILITIES_H_