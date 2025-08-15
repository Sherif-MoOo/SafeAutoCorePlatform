/***********************************************************************************************************************
 *  MEMORY TEST SUITE
 *  --------------------------------------------------------------------------------------------------------------------
 *  \file       ara_core_memory_test.cpp
 *  \brief      Comprehensive test suite and benchmarks for ara::core::Memory class
 *  
 *  \details    Tests all Memory class methods including corner cases, overlapping scenarios,
 *              alignment variations, and performance comparisons with standard library functions
 ***********************************************************************************************************************/

#include "ara/core/memory.h"
#include "test_utilities.h"
#include "test_unsafe_wrapper.h"

#include <iostream>
#include <iomanip>
#include <memory>
#include <algorithm>
#include <numeric>
#include <cwchar>
#if defined(__QNXNTO__)
    #include <pthread.h>
    #include <sched.h>
    #include <cerrno>
    #include <cstring>
#endif

// ====================================================================================================================
// TEST FUNCTIONS - COPY OPERATION
// ====================================================================================================================

/*!
 * \brief Tests Memory::copy with various sizes and patterns
 */
auto test_copy_basic() noexcept -> bool {
    test::TestReporter reporter;
    bool all_passed = true;
    
    std::cout << "\n=== Testing Memory::copy Basic Functionality ===\n";
    
    // Test each size threshold
    for (const auto size : test::kTestSizes) {
        test::AlignedBuffer<std::uint8_t> src(size);
        test::AlignedBuffer<std::uint8_t> dest(size);
        
        if (!src.valid() || !dest.valid()) {
            reporter.report_test(("Allocation for size " + std::to_string(size)).c_str(), false);
            all_passed = false;
            continue;
        }
        
        // Test with different patterns
        for (const auto pattern : test::kPatternValues) {
            test::fill_pattern(src.data(), size, pattern);
            test::fill_pattern(dest.data(), size, 0x00);  // Clear destination
            
            // Perform copy
            [[maybe_unused]] auto* result = ara::core::Memory::copy(dest.data(), src.data(), size);

            // Verify result pointer
            const bool ptr_correct = (result == dest.data());
            
            // Verify content
            const bool content_correct = test::verify_equal(dest.data(), src.data(), size);
            
            const bool test_passed = ptr_correct && content_correct;
            
            const std::string test_name = "Copy size " + std::to_string(size) + 
                                         " pattern 0x" + std::to_string(static_cast<int>(pattern));
            reporter.report_test(test_name.c_str(), test_passed);
            
            if (!test_passed) {
                all_passed = false;
            }
        }
    }
    
    return all_passed;
}

/*!
 * \brief Tests Memory::copy with null pointers and zero size
 */
auto test_copy_edge_cases() noexcept -> bool {
    test::TestReporter reporter;
    bool all_passed = true;
    
    std::cout << "\n=== Testing Memory::copy Edge Cases ===\n";
    
    // Test null pointer with zero size (should be safe)
    {
        [[maybe_unused]] auto* result = ara::core::Memory::copy(nullptr, nullptr, 0);
        const bool passed = (result == nullptr);
        reporter.report_test("Copy null pointers with size 0", passed);
        all_passed = all_passed && passed;
    }
    
    // Test valid pointers with zero size
    {
        std::uint8_t dummy_src = 0x42;
        std::uint8_t dummy_dest = 0x00;
        [[maybe_unused]] auto* result = ara::core::Memory::copy(&dummy_dest, &dummy_src, 0);
        const bool passed = (result == &dummy_dest) && (dummy_dest == 0x00);
        reporter.report_test("Copy valid pointers with size 0", passed);
        all_passed = all_passed && passed;
    }
    
    // Test unaligned pointers
    {
        test::AlignedBuffer<std::uint8_t> src(128);
        test::AlignedBuffer<std::uint8_t> dest(128);
        
        if (src.valid() && dest.valid()) {
            test::fill_pattern(src.data(), 128, 0xAB);
            
            // Test various misalignments
            for (std::size_t offset = 1; offset <= 7; ++offset) {
                test::fill_pattern(dest.data(), 128, 0x00);
                
                const std::size_t copy_size = 64;
                [[maybe_unused]] auto* result = ara::core::Memory::copy(
                    test::UnsafeOps::advance(dest.data(), offset),
                    test::UnsafeOps::advance(src.data(), offset),
                    copy_size
                );
                
                const bool passed = test::verify_pattern(
                    test::UnsafeOps::advance(dest.data(), offset), 
                    copy_size, 
                    0xAB
                );
                
                const std::string test_name = "Copy with offset " + std::to_string(offset);
                reporter.report_test(test_name.c_str(), passed);
                all_passed = all_passed && passed;
            }
        }
    }
    
    return all_passed;
}

// ====================================================================================================================
// TEST FUNCTIONS - MOVE OPERATION
// ====================================================================================================================

/*!
 * \brief Tests Memory::move with overlapping regions
 */
auto test_move_overlap() noexcept -> bool {
    test::TestReporter reporter;
    bool all_passed = true;
    
    std::cout << "\n=== Testing Memory::move Overlap Handling ===\n";
    
    // Test forward overlap (dest < src)
    {
        test::AlignedBuffer<std::uint8_t> buffer(256);
        if (buffer.valid()) {
            // Fill with sequential values
            test::fill_sequential(buffer.data(), 256);
            
            // Move with forward overlap
            const std::size_t src_offset = 50;
            const std::size_t dest_offset = 25;
            const std::size_t move_size = 100;
            
            [[maybe_unused]] auto* result = ara::core::Memory::move(
                test::UnsafeOps::advance(buffer.data(), dest_offset),
                test::UnsafeOps::advance(buffer.data(), src_offset),
                move_size
            );
            
            // Verify the moved data
            bool passed = true;
            for (std::size_t i = 0; i < move_size; ++i) {
                const auto expected = static_cast<std::uint8_t>(src_offset + i);
                const auto actual = test::UnsafeOps::at(buffer.data(), dest_offset + i);
                if (actual != expected) {
                    passed = false;
                    break;
                }
            }
            
            reporter.report_test("Move forward overlap", passed);
            all_passed = all_passed && passed;
        }
    }
    
    // Test backward overlap (dest > src)
    {
        test::AlignedBuffer<std::uint8_t> buffer(256);
        if (buffer.valid()) {
            test::fill_sequential(buffer.data(), 256);
            
            const std::size_t src_offset = 25;
            const std::size_t dest_offset = 50;
            const std::size_t move_size = 100;
            
            [[maybe_unused]] auto* result = ara::core::Memory::move(
                test::UnsafeOps::advance(buffer.data(), dest_offset),
                test::UnsafeOps::advance(buffer.data(), src_offset),
                move_size
            );
            
            bool passed = true;
            for (std::size_t i = 0; i < move_size; ++i) {
                const auto expected = static_cast<std::uint8_t>(src_offset + i);
                const auto actual = test::UnsafeOps::at(buffer.data(), dest_offset + i);
                if (actual != expected) {
                    passed = false;
                    break;
                }
            }
            
            reporter.report_test("Move backward overlap", passed);
            all_passed = all_passed && passed;
        }
    }
    
    // Test complete overlap (src == dest)
    {
        test::AlignedBuffer<std::uint8_t> buffer(128);
        if (buffer.valid()) {
            test::fill_pattern(buffer.data(), 128, 0xCD);
            
            [[maybe_unused]] auto* result = ara::core::Memory::move(buffer.data(), buffer.data(), 128);
            
            const bool passed = (result == buffer.data()) && 
                              test::verify_pattern(buffer.data(), 128, 0xCD);
            
            reporter.report_test("Move complete overlap (src == dest)", passed);
            all_passed = all_passed && passed;
        }
    }
    
    // Test various overlap scenarios with different sizes
    const std::size_t overlap_test_sizes[] = {1, 3, 7, 15, 31, 32, 33, 63, 128, 257};
    
    for (const auto size : overlap_test_sizes) {
        test::AlignedBuffer<std::uint8_t> buffer(size * 3);
        if (buffer.valid()) {
            test::fill_sequential(buffer.data(), size * 3);
            
            // Minimal overlap (1 byte)
            {
                [[maybe_unused]] auto* result = ara::core::Memory::move(
                    test::UnsafeOps::advance(buffer.data(), size - 1),
                    buffer.data(),
                    size
                );
                
                bool passed = true;
                for (std::size_t i = 0; i < size; ++i) {
                    const auto expected = static_cast<std::uint8_t>(i);
                    const auto actual = test::UnsafeOps::at(buffer.data(), size - 1 + i);
                    if (actual != expected) {
                        passed = false;
                        break;
                    }
                }
                
                const std::string test_name = "Move minimal overlap size " + std::to_string(size);
                reporter.report_test(test_name.c_str(), passed);
                all_passed = all_passed && passed;
            }
        }
    }
    
    return all_passed;
}

// ====================================================================================================================
// TEST FUNCTIONS - SET OPERATION
// ====================================================================================================================

/*!
 * \brief Tests Memory::set functionality
 */
auto test_set_operation() noexcept -> bool {
    test::TestReporter reporter;
    bool all_passed = true;
    
    std::cout << "\n=== Testing Memory::set Functionality ===\n";
    
    for (const auto size : test::kTestSizes) {
        test::AlignedBuffer<std::uint8_t> buffer(size);
        
        if (!buffer.valid()) {
            reporter.report_test(("Set allocation for size " + std::to_string(size)).c_str(), false);
            all_passed = false;
            continue;
        }
        
        for (const auto pattern : test::kPatternValues) {
            // Clear buffer first
            test::fill_pattern(buffer.data(), size, 0xFF);
            
            // Set with pattern
            [[maybe_unused]] auto* result = ara::core::Memory::set(buffer.data(), pattern, size);
            
            // Verify
            const bool ptr_correct = (result == buffer.data());
            const bool content_correct = test::verify_pattern(buffer.data(), size, pattern);
            const bool passed = ptr_correct && content_correct;
            
            const std::string test_name = "Set size " + std::to_string(size) + 
                                         " value 0x" + std::to_string(static_cast<int>(pattern));
            reporter.report_test(test_name.c_str(), passed);
            
            if (!passed) {
                all_passed = false;
            }
        }
    }
    
    return all_passed;
}

// ====================================================================================================================
// TEST FUNCTIONS - COMPARE OPERATION
// ====================================================================================================================

/*!
 * \brief Tests Memory::compare functionality
 */
auto test_compare_operation() noexcept -> bool {
    test::TestReporter reporter;
    bool all_passed = true;
    
    std::cout << "\n=== Testing Memory::compare Functionality ===\n";
    
    // Test equal buffers
    for (const auto size : test::kTestSizes) {
        test::AlignedBuffer<std::uint8_t> buf1(size);
        test::AlignedBuffer<std::uint8_t> buf2(size);
        
        if (!buf1.valid() || !buf2.valid()) {
            continue;
        }
        
        test::fill_pattern(buf1.data(), size, 0xAB);
        test::fill_pattern(buf2.data(), size, 0xAB);
        
        const int result = ara::core::Memory::compare(buf1.data(), buf2.data(), size);
        const bool passed = (result == 0);
        
        const std::string test_name = "Compare equal buffers size " + std::to_string(size);
        reporter.report_test(test_name.c_str(), passed);
        all_passed = all_passed && passed;
    }
    
    // Test different buffers
    {
        test::AlignedBuffer<std::uint8_t> buf1(128);
        test::AlignedBuffer<std::uint8_t> buf2(128);
        
        if (buf1.valid() && buf2.valid()) {
            // Test less than
            test::fill_pattern(buf1.data(), 128, 0x10);
            test::fill_pattern(buf2.data(), 128, 0x20);
            
            int result = ara::core::Memory::compare(buf1.data(), buf2.data(), 128);
            bool passed = (result < 0);
            reporter.report_test("Compare less than", passed);
            all_passed = all_passed && passed;
            
            // Test greater than
            result = ara::core::Memory::compare(buf2.data(), buf1.data(), 128);
            passed = (result > 0);
            reporter.report_test("Compare greater than", passed);
            all_passed = all_passed && passed;
            
            // Test difference in middle
            test::fill_pattern(buf1.data(), 128, 0x50);
            test::fill_pattern(buf2.data(), 128, 0x50);
            test::UnsafeOps::at(buf2.data(), 64) = 0x60;  // Make one byte different
            
            result = ara::core::Memory::compare(buf1.data(), buf2.data(), 128);
            passed = (result < 0);
            reporter.report_test("Compare difference in middle", passed);
            all_passed = all_passed && passed;
        }
    }
    
    return all_passed;
}

// ====================================================================================================================
// TEST FUNCTIONS - FIND OPERATION
// ====================================================================================================================

/*!
 * \brief Tests Memory::find functionality
 */
auto test_find_operation() noexcept -> bool {
    test::TestReporter reporter;
    bool all_passed = true;
    
    std::cout << "\n=== Testing Memory::find Functionality ===\n";
    
    // Test finding existing values
    for (const auto size : test::kTestSizes) {
        if (size == 0) continue;  // Skip zero size
        
        test::AlignedBuffer<std::uint8_t> buffer(size);
        if (!buffer.valid()) continue;
        
        // Fill with pattern and place target at various positions
        test::fill_pattern(buffer.data(), size, 0x00);
        
        // Test finding at beginning
        {
            test::UnsafeOps::at(buffer.data(), 0) = 0x42;
            const void* result = ara::core::Memory::find(buffer.data(), 0x42, size);
            const bool passed = (result == buffer.data());
            
            const std::string test_name = "Find at beginning size " + std::to_string(size);
            reporter.report_test(test_name.c_str(), passed);
            all_passed = all_passed && passed;
        }
        
        // Test finding at end
        {
            test::fill_pattern(buffer.data(), size, 0x00);
            test::UnsafeOps::at(buffer.data(), size - 1) = 0x42;
            const void* result = ara::core::Memory::find(buffer.data(), 0x42, size);
            const bool passed = (result == test::UnsafeOps::advance(buffer.data(), size - 1));
            
            const std::string test_name = "Find at end size " + std::to_string(size);
            reporter.report_test(test_name.c_str(), passed);
            all_passed = all_passed && passed;
        }
        
        // Test finding in middle
        if (size > 2) {
            test::fill_pattern(buffer.data(), size, 0x00);
            const std::size_t mid = size / 2;
            test::UnsafeOps::at(buffer.data(), mid) = 0x42;
            const void* result = ara::core::Memory::find(buffer.data(), 0x42, size);
            const bool passed = (result == test::UnsafeOps::advance(buffer.data(), mid));
            
            const std::string test_name = "Find in middle size " + std::to_string(size);
            reporter.report_test(test_name.c_str(), passed);
            all_passed = all_passed && passed;
        }
    }
    
    // Test not finding value
    {
        test::AlignedBuffer<std::uint8_t> buffer(256);
        if (buffer.valid()) {
            test::fill_pattern(buffer.data(), 256, 0xAA);
            const void* result = ara::core::Memory::find(buffer.data(), 0xBB, 256);
            const bool passed = (result == nullptr);
            reporter.report_test("Find non-existent value", passed);
            all_passed = all_passed && passed;
        }
    }
    
    // Test non-const version
    {
        test::AlignedBuffer<std::uint8_t> buffer(128);
        if (buffer.valid()) {
            test::fill_pattern(buffer.data(), 128, 0x00);
            test::UnsafeOps::at(buffer.data(), 50) = 0xFF;
            
            void* result = ara::core::Memory::find(buffer.data(), 0xFF, 128);
            const bool passed = (result == test::UnsafeOps::advance(buffer.data(), 50));
            reporter.report_test("Find non-const version", passed);
            all_passed = all_passed && passed;
        }
    }
    
    return all_passed;
}

// ====================================================================================================================
// TEST FUNCTIONS - WIDE CHARACTER OPERATIONS
// ====================================================================================================================

/*!
 * \brief Tests wide character operations
 */
auto test_wide_char_operations() noexcept -> bool {
    test::TestReporter reporter;
    bool all_passed = true;
    
    std::cout << "\n=== Testing Wide Character Operations ===\n";
    
    // Test wfind
    {
        const wchar_t test_string[] = L"Hello, World! This is a test string.";
        const std::size_t len = std::wcslen(test_string);
        
        // Find existing character
        const wchar_t* result = ara::core::Memory::wfind(test_string, L'W', len);
        bool passed = (result != nullptr && *result == L'W');
        reporter.report_test("wfind existing character", passed);
        all_passed = all_passed && passed;
        
        // Find non-existing character
        result = ara::core::Memory::wfind(test_string, L'Z', len);
        passed = (result == nullptr);
        reporter.report_test("wfind non-existing character", passed);
        all_passed = all_passed && passed;
        
        // Find with zero length
        result = ara::core::Memory::wfind(test_string, L'H', 0);
        passed = (result == nullptr);
        reporter.report_test("wfind with zero length", passed);
        all_passed = all_passed && passed;
    }
    
    // Test wcompare
    {
        const wchar_t str1[] = L"Hello";
        const wchar_t str2[] = L"Hello";
        const wchar_t str3[] = L"World";
        
        // Equal strings
        int result = ara::core::Memory::wcompare(str1, str2, 5);
        bool passed = (result == 0);
        reporter.report_test("wcompare equal strings", passed);
        all_passed = all_passed && passed;
        
        // Different strings
        result = ara::core::Memory::wcompare(str1, str3, 5);
        passed = (result < 0);  // 'H' < 'W'
        reporter.report_test("wcompare different strings", passed);
        all_passed = all_passed && passed;
        
        // Zero length comparison
        result = ara::core::Memory::wcompare(str1, str3, 0);
        passed = (result == 0);
        reporter.report_test("wcompare zero length", passed);
        all_passed = all_passed && passed;
    }
    
    // Test with various sizes
    {
        test::AlignedBuffer<wchar_t> buf1(256);
        test::AlignedBuffer<wchar_t> buf2(256);
        
        if (buf1.valid() && buf2.valid()) {
            // Fill with pattern
            for (std::size_t i = 0; i < 256; ++i) {
                test::UnsafeOps::at(buf1.data(), i) = static_cast<wchar_t>(i);
                test::UnsafeOps::at(buf2.data(), i) = static_cast<wchar_t>(i);
            }
            
            // Place target character
            test::UnsafeOps::at(buf1.data(), 128) = L'X';
            
            const wchar_t* result = ara::core::Memory::wfind(buf1.data(), L'X', 256);
            const bool passed = (result == test::UnsafeOps::advance(buf1.data(), 128));
            reporter.report_test("wfind in large buffer", passed);
            all_passed = all_passed && passed;
        }
    }
    
    return all_passed;
}

// ====================================================================================================================
// BENCHMARK FUNCTIONS
// ====================================================================================================================

/*!
 * \brief Benchmarks copy operations
 */
auto benchmark_copy_operations() noexcept -> void {
    std::cout << "\n=== Benchmarking Copy Operations ===\n";
    test::TestReporter reporter;
    
    const std::size_t benchmark_sizes[] = {8, 16, 32, 64, 128, 256, 512, 1024, 4096, 16384, 65536};
    
    for (const auto size : benchmark_sizes) {
        test::AlignedBuffer<std::uint8_t> src(size);
        test::AlignedBuffer<std::uint8_t> dest(size);
        
        if (!src.valid() || !dest.valid()) continue;
        
        test::fill_random(src.data(), size);
        
        // Benchmark ara::core::Memory::copy
        const double ara_time = test::benchmark([&]() noexcept {
            static_cast<void>(ara::core::Memory::copy(dest.data(), src.data(), size));
        });
        
        // Benchmark std::memcpy
        const double std_time = test::benchmark([&]() noexcept {
            static_cast<void>(test::UnsafeOps::call(std::memcpy, dest.data(), src.data(), size));
        });
        
        std::cout << "Size " << std::setw(6) << size << " bytes: ";
        std::cout << "ara::Memory " << std::fixed << std::setprecision(2) << ara_time << " ns, ";
        std::cout << "std::memcpy " << std::fixed << std::setprecision(2) << std_time << " ns, ";
        std::cout << "speedup " << std::fixed << std::setprecision(2) << (std_time / ara_time) << "x\n";
    }
}

/*!
 * \brief Benchmarks move operations with overlap
 */
auto benchmark_move_operations() noexcept -> void {
    std::cout << "\n=== Benchmarking Move Operations (Overlapping) ===\n";
    
    const std::size_t benchmark_sizes[] = {8, 16, 32, 64, 128, 256, 512, 1024, 4096};
    
    for (const auto size : benchmark_sizes) {
        test::AlignedBuffer<std::uint8_t> buffer(size * 2);
        
        if (!buffer.valid()) continue;
        
        test::fill_random(buffer.data(), size * 2);
        
        // Create overlapping scenario
        const std::size_t overlap = size / 2;
        
        // Benchmark ara::core::Memory::move
        const double ara_time = test::benchmark([&]() noexcept {
            static_cast<void>(ara::core::Memory::move(
                test::UnsafeOps::advance(buffer.data(), overlap),
                buffer.data(),
                size
            ));
        });
        
        // Benchmark std::memmove
        const double std_time = test::benchmark([&]() noexcept {
            static_cast<void>(test::UnsafeOps::call(std::memmove,
                test::UnsafeOps::advance(buffer.data(), overlap),
                buffer.data(),
                size
            ));
        });
        
        std::cout << "Size " << std::setw(6) << size << " bytes: ";
        std::cout << "ara::Memory " << std::fixed << std::setprecision(2) << ara_time << " ns, ";
        std::cout << "std::memmove " << std::fixed << std::setprecision(2) << std_time << " ns, ";
        std::cout << "speedup " << std::fixed << std::setprecision(2) << (std_time / ara_time) << "x\n";
    }
}

/*!
 * \brief Benchmarks set operations
 */
auto benchmark_set_operations() noexcept -> void {
    std::cout << "\n=== Benchmarking Set Operations ===\n";
    
    const std::size_t benchmark_sizes[] = {8, 16, 32, 64, 128, 256, 512, 1024, 4096, 16384, 65536};
    
    for (const auto size : benchmark_sizes) {
        test::AlignedBuffer<std::uint8_t> buffer(size);
        
        if (!buffer.valid()) continue;
        
        // Benchmark ara::core::Memory::set
        const double ara_time = test::benchmark([&]() noexcept {
            static_cast<void>(ara::core::Memory::set(buffer.data(), 0x42, size));
        });
        
        // Benchmark std::memset
        const double std_time = test::benchmark([&]() noexcept {
            static_cast<void>(test::UnsafeOps::call(std::memset, buffer.data(), 0x42, size));
        });
        
        std::cout << "Size " << std::setw(6) << size << " bytes: ";
        std::cout << "ara::Memory " << std::fixed << std::setprecision(2) << ara_time << " ns, ";
        std::cout << "std::memset " << std::fixed << std::setprecision(2) << std_time << " ns, ";
        std::cout << "speedup " << std::fixed << std::setprecision(2) << (std_time / ara_time) << "x\n";
    }
}

/*!
 * \brief Benchmarks compare operations
 */
auto benchmark_compare_operations() noexcept -> void {
    std::cout << "\n=== Benchmarking Compare Operations ===\n";
    
    const std::size_t benchmark_sizes[] = {8, 16, 32, 64, 128, 256, 512, 1024, 4096};
    
    for (const auto size : benchmark_sizes) {
        test::AlignedBuffer<std::uint8_t> buf1(size);
        test::AlignedBuffer<std::uint8_t> buf2(size);
        
        if (!buf1.valid() || !buf2.valid()) continue;
        
        test::fill_random(buf1.data(), size);
        static_cast<void>(test::UnsafeOps::call(std::memcpy, buf2.data(), buf1.data(), size));

        // Benchmark ara::core::Memory::compare
        const double ara_time = test::benchmark([&]() noexcept {
            static_cast<void>(ara::core::Memory::compare(buf1.data(), buf2.data(), size));
        });
        
        // Benchmark std::memcmp
        const double std_time = test::benchmark([&]() noexcept {
            static_cast<void>(test::UnsafeOps::call(std::memcmp, buf1.data(), buf2.data(), size));
        });
        
        std::cout << "Size " << std::setw(6) << size << " bytes: ";
        std::cout << "ara::Memory " << std::fixed << std::setprecision(2) << ara_time << " ns, ";
        std::cout << "std::memcmp " << std::fixed << std::setprecision(2) << std_time << " ns, ";
        std::cout << "speedup " << std::fixed << std::setprecision(2) << (std_time / ara_time) << "x\n";
    }
}

/*!
 * \brief Benchmarks find operations
 */
auto benchmark_find_operations() noexcept -> void {
    std::cout << "\n=== Benchmarking Find Operations ===\n";
    
    const std::size_t benchmark_sizes[] = {8, 16, 32, 64, 128, 256, 512, 1024, 4096};
    
    for (const auto size : benchmark_sizes) {
        test::AlignedBuffer<std::uint8_t> buffer(size);
        
        if (!buffer.valid()) continue;
        
        test::fill_pattern(buffer.data(), size, 0x00);
        // Place target at 75% position
        const std::size_t target_pos = (size * 3) / 4;
        test::UnsafeOps::at(buffer.data(), target_pos) = 0xFF;
        
        // Benchmark ara::core::Memory::find
        const double ara_time = test::benchmark([&]() noexcept {
            static_cast<void>(ara::core::Memory::find(buffer.data(), 0xFF, size));
        });
        
        // Benchmark std::memchr
        const double std_time = test::benchmark([&]() noexcept {
            static_cast<void>(test::UnsafeOps::call(
                test::detail::memchr_const_wrapper,
                buffer.data(), 0xFF, size
            ));
        });
        
        std::cout << "Size " << std::setw(6) << size << " bytes: ";
        std::cout << "ara::Memory " << std::fixed << std::setprecision(2) << ara_time << " ns, ";
        std::cout << "std::memchr " << std::fixed << std::setprecision(2) << std_time << " ns, ";
        std::cout << "speedup " << std::fixed << std::setprecision(2) << (std_time / ara_time) << "x\n";
    }
}

// ====================================================================================================================
// STRESS TESTS
// ====================================================================================================================

/*!
 * \brief Stress test with random operations
 */
auto stress_test_random_operations() noexcept -> bool {
    std::cout << "\n=== Running Stress Tests ===\n";
    test::TestReporter reporter;
    bool all_passed = true;
    
    std::mt19937 gen(12345);  // Fixed seed for reproducibility
    std::uniform_int_distribution<std::size_t> size_dist(1, 1024);
    std::uniform_int_distribution<int> op_dist(0, 4);
    std::uniform_int_distribution<int> byte_dist(0, 255);
    
    const std::size_t num_iterations = 10000;
    
    for (std::size_t iter = 0; iter < num_iterations; ++iter) {
        const std::size_t size = size_dist(gen);
        const int operation = op_dist(gen);
        
        test::AlignedBuffer<std::uint8_t> src(size);
        test::AlignedBuffer<std::uint8_t> dest(size);
        test::AlignedBuffer<std::uint8_t> reference(size);
        
        if (!src.valid() || !dest.valid() || !reference.valid()) {
            continue;
        }
        
        // Initialize with random data
        test::fill_random(src.data(), size, static_cast<unsigned>(iter));
        
        bool test_passed = true;
        
        switch (operation) {
            case 0: {  // Copy
                static_cast<void>(ara::core::Memory::copy(dest.data(), src.data(), size));
                static_cast<void>(test::UnsafeOps::call(std::memcpy, reference.data(), src.data(), size));
                test_passed = test::verify_equal(dest.data(), reference.data(), size);
                break;
            }
            case 1: {  // Move (non-overlapping for simplicity in stress test)
                static_cast<void>(ara::core::Memory::move(dest.data(), src.data(), size));
                static_cast<void>(test::UnsafeOps::call(std::memmove, reference.data(), src.data(), size));
                test_passed = test::verify_equal(dest.data(), reference.data(), size);
                break;
            }
            case 2: {  // Set
                const int value = byte_dist(gen);
                static_cast<void>(ara::core::Memory::set(dest.data(), value, size));
                static_cast<void>(test::UnsafeOps::call(std::memset, reference.data(), value, size));
                test_passed = test::verify_equal(dest.data(), reference.data(), size);
                break;
            }
            case 3: {  // Compare
                static_cast<void>(test::UnsafeOps::call(std::memcpy, dest.data(), src.data(), size));
                const int ara_result = ara::core::Memory::compare(dest.data(), src.data(), size);
                const int std_result = test::UnsafeOps::call(std::memcmp, dest.data(), src.data(), size);
                // Compare signs (both zero, both negative, or both positive)
                test_passed = ((ara_result == 0 && std_result == 0) ||
                              (ara_result < 0 && std_result < 0) ||
                              (ara_result > 0 && std_result > 0));
                break;
            }
            case 4: {  // Find
                const int value = byte_dist(gen);
                const std::size_t pos = size_dist(gen) % size;
                test::fill_pattern(dest.data(), size, 0);
                test::UnsafeOps::at(dest.data(), pos) = static_cast<std::uint8_t>(value);
                
                const void* ara_result = ara::core::Memory::find(dest.data(), value, size);
                const void* std_result = test::UnsafeOps::call(
                    test::detail::memchr_const_wrapper,
                    dest.data(), value, size
                );
                test_passed = (ara_result == std_result);
                break;
            }
            default:
                std::cerr << "Unknown operation " << operation << " in stress test\n";
                test_passed = false;
                break;
        }
        
        if (!test_passed) {
            std::cout << "Stress test failed at iteration " << iter 
                     << " operation " << operation << " size " << size << "\n";
            all_passed = false;
            break;
        }
    }
    
    reporter.report_test("Stress test random operations", all_passed);
    return all_passed;
}

#if defined(__QNXNTO__)
/* ------------------------------------------------------------------------------------------------------------------ */
/*  Convert scheduler policy to readable text                                                                          */
/* ------------------------------------------------------------------------------------------------------------------ */
static inline const char* qnx_policy_name(const int policy) noexcept {
    switch (policy) {
        case SCHED_FIFO: return "SCHED_FIFO";
        case SCHED_RR:   return "SCHED_RR";
#ifdef SCHED_SPORADIC
        case SCHED_SPORADIC: return "SCHED_SPORADIC";
#endif
        case SCHED_OTHER: return "SCHED_OTHER";
        default:          return "UNKNOWN";
    }
}

/* ------------------------------------------------------------------------------------------------------------------ */
/*  Log current thread scheduling policy & priority                                                                    */
/* ------------------------------------------------------------------------------------------------------------------ */
static inline void qnx_log_sched_info(const char* tag) noexcept {
    int policy = 0;
    struct sched_param param;
    std::memset(&param, 0, sizeof(param));

    if (pthread_getschedparam(pthread_self(), &policy, &param) == 0) {
        const int pmin = sched_get_priority_min(policy);
        const int pmax = sched_get_priority_max(policy);
        std::cout << "[QNX] " << tag << " — policy=" << qnx_policy_name(policy)
                  << ", base_priority=" << param.sched_priority
                  << ", range=[" << pmin << "," << pmax << "]\n";
    } else {
        std::cout << "[QNX] " << tag << " — failed to read sched params (errno=" << errno << ")\n";
    }
}

/* ------------------------------------------------------------------------------------------------------------------ */
/*  Elevate the calling thread to the highest realtime priority possible                                               */
/* ------------------------------------------------------------------------------------------------------------------ */
static inline void qnx_set_highest_realtime_priority() noexcept {
    /* Log before changing */
    qnx_log_sched_info("BEFORE");

    /* Aim for top of SCHED_FIFO */
    const int policy = SCHED_FIFO;
    struct sched_param sp;
    std::memset(&sp, 0, sizeof(sp));
    sp.sched_priority = sched_get_priority_max(policy);

    int rc = pthread_setschedparam(pthread_self(), policy, &sp);
    if (rc != 0) {
        /* If we lack ability to set very high priorities, fall back once to a commonly-allowed high level (e.g., 63). */
        /* NOTE: Exact allowed ceilings depend on system abilities; adjust if your platform uses a different ceiling.   */
        const int fallback = 63; /* Typical non-privileged ceiling on many QNX setups */
        sp.sched_priority = (fallback < sched_get_priority_max(policy)) ? fallback
                                                                        : sched_get_priority_max(policy);
        rc = pthread_setschedparam(pthread_self(), policy, &sp);

        if (rc != 0) {
            std::cout << "[QNX] FAILED to set realtime priority. errno=" << rc
                      << " (" << std::strerror(rc) << ")\n";
        } else {
            std::cout << "[QNX] Realtime priority set with FALLBACK priority=" << sp.sched_priority << "\n";
        }
    } else {
        std::cout << "[QNX] Realtime priority set to MAX priority=" << sp.sched_priority << "\n";
    }

    /* Log after changing */
    qnx_log_sched_info("AFTER");
}
#endif

// ====================================================================================================================
// MAIN FUNCTION
// ====================================================================================================================

/*!
 * \brief Main test entry point
 */
auto main() noexcept -> int {
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                       ARA::CORE::MEMORY TEST SUITE                           ║\n";
    std::cout << "║                                                                              ║\n";
    std::cout << "║  High-Performance Memory Operations Testing & Benchmarking                   ║\n";
    std::cout << "║  C++ Standard: C++17                                                         ║\n";
    std::cout << "║  Compiler: " << 
#if defined(__clang__)
        "Clang " << __clang_major__ << "." << __clang_minor__ << "." << __clang_patchlevel__
        << std::string(55, ' ') << "║\n";
#elif defined(__GNUC__)
        "GCC " << __GNUC__ << "." << __GNUC_MINOR__ << "." << __GNUC_PATCHLEVEL__
        << std::string(56, ' ') << "║\n";
#else
        "Unknown"
#endif
    std::cout << "║  Architecture: " <<
#if defined(__x86_64__) || defined(_M_X64)
        "x86_64"  << std::string(57, ' ') << "║\n";
#elif defined(__aarch64__) || defined(_M_ARM64)
        "AArch64" << std::string(55, ' ') << "║\n";
#elif defined(__arm__) || defined(_M_ARM)
        "ARM"     << std::string(59, ' ') << "║\n";
#else
        "Unknown"
#endif
    std::cout << "║  SIMD Support: " <<
#if defined(__AVX512F__)
        "AVX-512" << std::string(56, ' ') << "║\n";
#elif defined(__AVX2__)
        "AVX2" << std::string(58, ' ') << "║\n";
#elif defined(__SSE2__)
        "SSE2" << std::string(58, ' ') << "║\n";
#elif defined(__ARM_NEON)
        "NEON" << std::string(58, ' ') << "║\n";
#else
        "None" << std::string(58, ' ') << "║\n";
#endif
    std::cout << "╚══════════════════════════════════════════════════════════════════════════════╝\n\n";
    
#if defined(__QNXNTO__)
    qnx_set_highest_realtime_priority();
#endif

    test::TestReporter global_reporter;
    bool all_tests_passed = true;
    
    // Run functional tests
    // std::cout << "\n### FUNCTIONAL TESTS ###\n";
    
    // all_tests_passed = test_copy_basic() && all_tests_passed;
    // all_tests_passed = test_copy_edge_cases() && all_tests_passed;
    // all_tests_passed = test_move_overlap() && all_tests_passed;
    // all_tests_passed = test_set_operation() && all_tests_passed;
    // all_tests_passed = test_compare_operation() && all_tests_passed;
    // all_tests_passed = test_find_operation() && all_tests_passed;
    // all_tests_passed = test_wide_char_operations() && all_tests_passed;
    
    // // Run stress tests
    // all_tests_passed = stress_test_random_operations() && all_tests_passed;
    
    // Run benchmarks
    std::cout << "\n### PERFORMANCE BENCHMARKS ###\n";
    
    benchmark_copy_operations();
    benchmark_move_operations();
    benchmark_set_operations();
    benchmark_compare_operations();
    benchmark_find_operations();
    
    // Final summary
    std::cout << "\n========================================\n";
    std::cout << "FINAL RESULT: " << (all_tests_passed ? "ALL TESTS PASSED" : "SOME TESTS FAILED") << "\n";
    std::cout << "========================================\n";
    
    return all_tests_passed ? 0 : 1;
}