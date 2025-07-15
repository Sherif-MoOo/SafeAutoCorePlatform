/**********************************************************************************************************************
 *  PROJECT
 *  -------------------------------------------------------------------------------------------------------------------
 *  \verbatim
 *  OpenAA: Open Source Adaptive AUTOSAR Project (CXX_STANDARD 17/20)
 *  \endverbatim
 *  -------------------------------------------------------------------------------------------------------------------
 *  FILE DESCRIPTION
 *  -------------------------------------------------------------------------------------------------------------------
 *  \file       ara_core_span_test.cpp
 *  \brief      Comprehensive test application for the ara::core::Span type.
 *
 *  \details    This file contains extensive tests for ara::core::Span covering:
 *              1.  Construction and Type Deduction
 *              2.  Element Access and Bounds Checking
 *              3.  Iterators and Range-based For Loops
 *              4.  Subspan Operations
 *              5.  Comparison Operations
 *              6.  Size and Capacity Queries
 *              7.  C++26 Features (contains, starts_with, ends_with, split)
 *              8.  Initializer List Support (C++17)
 *              9.  Ranges Support (C++17 custom, C++20 std::ranges)
 *              10. Factory Functions (MakeSpan)
 *              11. Byte Representation (as_bytes, as_writable_bytes)
 *              12. Type Traits and Concepts
 *              13. Performance Comparisons
 *              14. Violation Scenarios (separate functions)
 *              15. Negative Compilation Scenarios (commented out)
 *
 *  \note       Compile with -std=c++17 or -std=c++20 to test different features
 *********************************************************************************************************************/

#if defined(__linux__)
#include <sys/prctl.h>
#elif defined(__QNXNTO__)
#include <pthread.h>
#endif

#include "ara/core/span.h"          // The ara::core::Span implementation
#include "ara/core/array.h"         // For ara::core::Array tests
#include <iostream>                 // For output
#include <vector>                   // For container tests
#include <string>                   // For string tests
#include <deque>                    // For non-contiguous container tests
#include <list>                     // For non-contiguous container tests
#include <cassert>                  // For runtime assertions
#include <chrono>                   // For performance measurements
#include <iomanip>                  // For output formatting
#include <string_view>              // For string_view tests
#include <type_traits>              // For type trait checks
#include <memory>                   // For unique_ptr tests
#include <numeric>                  // For std::iota
#include <cmath>

#if __cplusplus >= 202002L
#include <ranges>                   // For C++20 ranges tests
#include <concepts>                 // For C++20 concepts tests
#endif

#include "ara/core/string_view.h"  // For string view tests

static constexpr std::string_view   kProcessNameView{"CoreSpanTest"};
static constexpr std::uint8_t       kMaxProcessName{15};

/**********************************************************************************************************************
 *  UTILITY FUNCTIONS
 *********************************************************************************************************************/

/*!
 * \brief Simple performance timer
 */
class PerfTimer {
    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = Clock::time_point;
    using Duration = std::chrono::duration<double, std::micro>;

    TimePoint start_;
public:
    PerfTimer() : start_(Clock::now()) {}
    
    [[nodiscard]] auto elapsed() const -> double {
        using namespace std::chrono;
        // 1. cast the difference to integral microseconds
        auto us = duration_cast<microseconds>(Clock::now() - start_).count();
        // 2. convert to double for the public API
        return static_cast<double>(us);
    }
    
    void reset() { start_ = Clock::now(); }
};

/*!
 * \brief Print test section header
 */
void PrintTestHeader(const std::string& testName) {
    std::cout << "\n" << std::string(70, '=') << "\n";
    std::cout << "Test: " << testName << "\n";
    std::cout << std::string(70, '=') << "\n";
}

/*!
 * \brief Print sub-test header
 */
void PrintSubTest(const std::string& subTestName) {
    std::cout << "\n--- " << subTestName << " ---\n";
}

/*!
 * \brief Print C++ standard version
 */
void PrintCppStandard() {
    std::cout << "Compiled with C++";
    #if __cplusplus >= 202002L
        std::cout << "20";
    #elif __cplusplus >= 201703L
        std::cout << "17";
    #else
        std::cout << "14 or earlier";
    #endif
    std::cout << " (Standard value: " << __cplusplus << ")\n";
}

/**********************************************************************************************************************
 *  FORWARD DECLARATIONS
 *********************************************************************************************************************/
void TestConstruction();
void TestElementAccess();
void TestIterators();
void TestSubspanOperations();
void TestComparisons();
void TestSizeAndCapacity();
void TestCpp26Features();
void TestInitializerListSupport();
void TestRangesSupport();
void TestFactoryFunctions();
void TestByteRepresentation();
void TestTypeTraits();
void TestPerformance();

// Violation tests (will terminate)
void TestSizeViolation();
void TestBoundsViolation();
void TestNullPointerViolation();
void TestRangeViolation();
void TestEmptyAccessViolation();
void TestSubspanOffsetViolation();
void TestSubspanCountViolation();

// Negative compilation tests (commented out)
void TestNegativeCompilation();

/**********************************************************************************************************************
 *  MAIN FUNCTION
 *********************************************************************************************************************/
int main(int argc, char* argv[])
{
    static_assert(kProcessNameView.size() <= kMaxProcessName,
        "\n[ERROR] Process name is too long!!\n");

    #if defined(__linux__)
        prctl(PR_SET_NAME, kProcessNameView.data(), 0, 0, 0);
    #elif defined(__QNXNTO__)
        pthread_setname_np(pthread_self(), kProcessNameView.data());
    #endif

    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║         ara::core::Span Comprehensive Test Suite           ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";
    
    PrintCppStandard();

    // Check if user wants to run violation tests
    if (argc > 1) {
        std::string arg = argv[1];
        if (arg == "violation-size") {
            std::cout << "\nRunning Size Violation Test (will terminate)...\n";
            TestSizeViolation();
            return 1; // Should never reach here
        } else if (arg == "violation-bounds") {
            std::cout << "\nRunning Bounds Violation Test (will terminate)...\n";
            TestBoundsViolation();
            return 1; // Should never reach here
        } else if (arg == "null-pointer") {
            std::cout << "\nRunning Null Pointer Violation Test (will terminate)...\n";
            TestNullPointerViolation();
            return 1; // Should never reach here
        } else if (arg == "range-violation") {
            std::cout << "\nRunning Range Violation Test (will terminate)...\n";
            TestRangeViolation();
            return 1; // Should never reach here
        } else if (arg == "empty-access") {
            std::cout << "\nRunning Empty Access Violation Test (will terminate)...\n";
            TestEmptyAccessViolation();
            return 1; // Should never reach here
        } else if (arg == "subspan-offset") {
            std::cout << "\nRunning Subspan Offset Violation Test (will terminate)...\n";
            TestSubspanOffsetViolation();
            return 1; // Should never reach here
        } else if (arg == "subspan-count") {
            std::cout << "\nRunning Subspan Count Violation Test (will terminate)...\n";
            TestSubspanCountViolation();
            return 1; // Should never reach here
        } else {
            std::cout << "\nUsage: " << argv[0] << " [violation-size|violation-bounds|null-pointer]\n";
            std::cout << "       No arguments: Run all non-terminating tests\n";
            std::cout << "       violation-size: Test size mismatch violation\n";
            std::cout << "       violation-bounds: Test bounds violation\n";
            std::cout << "       null-pointer: Test null pointer violation\n";
            std::cout << "       range-violation: Test range violation\n";
            std::cout << "       empty-access: Test empty access violation\n";
            std::cout << "       subspan-offset: Test subspan offset violation\n";
            std::cout << "       subspan-count: Test subspan count violation\n";

            return 1; // Invalid argument
        }
    }

    // Run all non-terminating tests
    TestConstruction();
    TestElementAccess();
    TestIterators();
    TestSubspanOperations();
    TestComparisons();
    TestSizeAndCapacity();
    TestCpp26Features();
    TestInitializerListSupport();
    TestRangesSupport();
    TestFactoryFunctions();
    TestByteRepresentation();
    TestTypeTraits();
    TestPerformance();
    TestNegativeCompilation(); // All scenarios commented out

    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║              ALL TESTS PASSED SUCCESSFULLY!                ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";

    return 0;
}

/**********************************************************************************************************************
 *  TEST IMPLEMENTATIONS
 *********************************************************************************************************************/

/*!
 * \brief Test #1: Construction and Type Deduction
 */
void TestConstruction()
{
    PrintTestHeader("Construction and Type Deduction");

    PrintSubTest("Default Construction");
    {
        ara::core::Span<int> s1;  // Dynamic extent, empty
        ara::core::Span<int, 0> s2;  // Static extent 0
        
        std::cout << "Dynamic empty span size: " << s1.size() << " (expected 0)\n";
        std::cout << "Static empty span size: " << s2.size() << " (expected 0)\n";
        
        assert(s1.size() == 0);
        assert(s1.data() == nullptr);
        assert(s1.empty());
        
        assert(s2.size() == 0);
        assert(s2.data() == nullptr);
        assert(s2.empty());
    }

    PrintSubTest("Construction from Pointer and Size");
    {
        int arr[] = {1, 2, 3, 4, 5};
        
        ara::core::Span<int> s1(arr, 5);  // Dynamic extent
        ara::core::Span<int, 5> s2(arr, 5);  // Static extent
        
        std::cout << "Dynamic span from ptr+size: size=" << s1.size() << "\n";
        std::cout << "Static span from ptr+size: size=" << s2.size() << "\n";
        
        assert(s1.size() == 5);
        assert(s1.data() == arr);
        assert(s1[0] == 1 && s1[4] == 5);
        
        assert(s2.size() == 5);
        assert(s2.data() == arr);
        assert(s2[0] == 1 && s2[4] == 5);
    }

    PrintSubTest("Construction from Iterator Pair");
    {
        int arr[] = {10, 20, 30, 40, 50};
        
        ara::core::Span<int> s1(arr, arr + 5);
        ara::core::Span<int, 5> s2(arr, arr + 5);
        
        assert(s1.size() == 5);
        assert(s1[2] == 30);
        
        assert(s2.size() == 5);
        assert(s2[2] == 30);
        
        std::cout << "Iterator pair construction successful\n";
    }

    PrintSubTest("Construction from C-style Array");
    {
        int arr[6] = {1, 2, 3, 4, 5, 6};
        
        ara::core::Span<int> s1(arr);  // Deduces size 6
        ara::core::Span<int, 6> s2(arr);  // Static size must match
        
        std::cout << "Deduced size from array: " << s1.size() << "\n";
        
        assert(s1.size() == 6);
        assert(s2.size() == 6);
        
        // Test const array
        const int carr[3] = {7, 8, 9};
        ara::core::Span<const int> s3(carr);
        assert(s3.size() == 3);
        assert(s3[1] == 8);
    }

    PrintSubTest("Construction from std::array");
    {
        std::array<int, 4> arr = {10, 20, 30, 40};
        
        ara::core::Span<int> s1(arr);
        ara::core::Span<int, 4> s2(arr);
        ara::core::Span<const int> s3(arr);  // Non-const to const
        
        assert(s1.size() == 4);
        assert(s2.size() == 4);
        assert(s3.size() == 4);
        
        // Const array
        const std::array<int, 2> carr = {50, 60};
        ara::core::Span<const int> s4(carr);
        assert(s4.size() == 2);
        
        std::cout << "std::array construction successful\n";
    }

    PrintSubTest("Construction from move_iterator pair + Location");
    {
        int arr[] = {11, 22, 33, 44, 55};
    
        using move_it = std::move_iterator<int*>;
    
        move_it first(arr);        // begin
        move_it last (arr + 5);    // end
    
        /* Wrap 'last' explicitly – now the parameter type matches exactly    */
    
        ara::core::Span<int>   s1(first, last);   // generic constructor
        ara::core::Span<int,5> s2(first, last);   // idem
    
        std::cout << "dyn span size: "  << s1.size() << " (expected 5)\n";
        std::cout << "stat span size: " << s2.size() << " (expected 5)\n";
    
        assert(s1.size() == 5);
        assert(s1.front() == 11 && s1.back() == 55);
    
        assert(s2.size() == 5);
        assert(s2.front() == 11 && s2.back() == 55);
    }

    PrintSubTest("Construction from Containers");
    {
        std::vector<int> vec = {1, 2, 3, 4, 5};
        ara::core::Span<int> s1(vec);
        
        std::cout << "Span from vector: size=" << s1.size() << "\n";
        assert(s1.size() == 5);
        assert(s1[2] == 3);
        
        // String
        std::string str = "Hello";
        ara::core::Span<char> s2(str);
        assert(s2.size() == 5);
        assert(s2[0] == 'H');
        
        // String view
        std::string_view sv = "World";
        ara::core::Span<const char> s3(sv);
        assert(s3.size() == 5);
        assert(s3[0] == 'W');
        
        std::cout << "Container construction successful\n";
    }


#if __cplusplus >= 202002L
    PrintSubTest("Construction from std::ranges::subrange");
    {
        int arr[] = { 5, 10, 15, 20, 25, 30 };

        std::ranges::subrange sub(arr + 1, arr + 5);

        ara::core::Span<int>   s1(sub);          
        ara::core::Span<int,4> s2(sub);          

        std::cout << "dyn span size: "  << s1.size() << " (expected 4)\n";
        std::cout << "stat span size: " << s2.size() << " (expected 4)\n";

        assert(s1.size() == 4);
        assert(s1.front() == 10 && s1.back() == 25);

        assert(s2.size() == 4);
        assert(s2.front() == 10 && s2.back() == 25);

        std::cout << "subrange construction successful\n";
    }
#endif

    PrintSubTest("Converting Constructor");
    {
        int arr[] = {1, 2, 3};
        
        // Non-const to const
        ara::core::Span<int> s1(arr);
        ara::core::Span<const int> s2(s1);  // OK: int* to const int*
        
        assert(s2.size() == 3);
        assert(s2[1] == 2);
        
        // Static to dynamic
        ara::core::Span<int, 3> s3(arr);
        ara::core::Span<int> s4(s3);  // OK: static to dynamic
        
        assert(s4.size() == 3);
        
        // Dynamic to static (runtime check)
        ara::core::Span<int> s5(arr, 3);
        ara::core::Span<int, 3> s6(s5);  // OK if sizes match
        
        assert(s6.size() == 3);
        
        std::cout << "Converting constructors successful\n";
    }

    PrintSubTest("Copy and Move Semantics");
    {
        int arr[] = {10, 20, 30};
        ara::core::Span<int> original(arr);
        
        // Copy construction
        [[maybe_unused]] ara::core::Span<int> copied(original);
        assert(copied.data() == original.data());
        assert(copied.size() == original.size());
        
        // Move construction (trivial for span)
        ara::core::Span<int> moved(std::move(original));
        assert(moved.data() == arr);
        assert(moved.size() == 3);
        
        // Assignment
        ara::core::Span<int> assigned;
        assigned = moved;
        assert(assigned.data() == arr);
        
        std::cout << "Copy/move semantics verified\n";
    }

    PrintSubTest("Compile-time Construction");
    {
        static constexpr int carr[] = {1, 2, 3, 4, 5};
        constexpr ara::core::Span<const int> s1(carr);
        constexpr ara::core::Span<const int, 5> s2(carr);
        
        static_assert(s1.size() == 5, "Constexpr size failed");
        static_assert(s1[2] == 3, "Constexpr element access failed");
        static_assert(s2.size() == 5, "Constexpr static size failed");
        
        std::cout << "Compile-time construction verified\n";
    }

}

/*!
 * \brief Test #2: Element Access and Bounds Checking
 */
void TestElementAccess()
{
    PrintTestHeader("Element Access and Bounds Checking");

    PrintSubTest("Subscript Operator");
    {
        int arr[] = {10, 20, 30, 40, 50};
        ara::core::Span<int> s(arr);
        
        // Read access
        std::cout << "Elements via operator[]: ";
        for (std::size_t i = 0; i < s.size(); ++i) {
            std::cout << s[i] << " ";
        }
        std::cout << "\n";
        
        // Write access
        s[2] = 35;
        assert(arr[2] == 35);
        
        // Const span
        ara::core::Span<const int> cs(arr);
        assert(cs[2] == 35);
        // cs[2] = 40;  // Would fail: cannot modify const
    }

    PrintSubTest("at() Method (Bounds Checked)");
    {
        int arr[] = {1, 2, 3, 4, 5};
        ara::core::Span<int> s(arr);
        
        // Valid access
        assert(s.at(0) == 1);
        assert(s.at(4) == 5);
        
        s.at(2) = 30;
        assert(arr[2] == 30);
        
        std::cout << "at() method provides bounds checking\n";
        
        // Invalid access would terminate (tested separately)
    }

    PrintSubTest("Front and Back");
    {
        int arr[] = {100, 200, 300};
        ara::core::Span<int> s(arr);
        
        assert(s.front() == 100);
        assert(s.back() == 300);
        
        s.front() = 150;
        s.back() = 350;
        
        assert(arr[0] == 150);
        assert(arr[2] == 350);
        
        std::cout << "front() and back() working correctly\n";
    }

    PrintSubTest("Data Pointer Access");
    {
        int arr[] = {5, 10, 15};
        ara::core::Span<int> s(arr);
        
        [[maybe_unused]] int* ptr = s.data();
        assert(ptr == arr);
        assert(ptr[1] == 10);
        
        // Empty span
        [[maybe_unused]] ara::core::Span<int> empty;
        assert(empty.data() == nullptr);
        
        std::cout << "data() pointer access verified\n";
    }

    PrintSubTest("Const Correctness");
    {
        int arr[] = {1, 2, 3};
        const ara::core::Span<int> s(arr);  // const span of non-const elements
        
        // Can still modify elements through const span
        s[0] = 10;
        assert(arr[0] == 10);
        
        // But span of const elements prevents modification
        ara::core::Span<const int> cs(arr);
        [[maybe_unused]] const int& ref = cs[0];  // OK
        // cs[0] = 20;  // ERROR: cannot modify const element
        
        std::cout << "Const correctness verified\n";
    }

    PrintSubTest("Compile-time Element Access");
    {
        static constexpr int arr[] = {10, 20, 30, 40};
        constexpr ara::core::Span<const int, 4> s(arr);
        
        static_assert(s[0] == 10, "Constexpr element access failed");
        static_assert(s[3] == 40, "Constexpr element access failed");
        static_assert(s.front() == 10, "Constexpr front() failed");
        static_assert(s.back() == 40, "Constexpr back() failed");
        
        std::cout << "Compile-time element access verified\n";
    }
}

/*!
 * \brief Test #3: Iterators and Range-based For Loops
 */
void TestIterators()
{
    PrintTestHeader("Iterators and Range-based For Loops");

    PrintSubTest("Basic Iterator Operations");
    {
        int arr[] = {1, 2, 3, 4, 5};
        ara::core::Span<int> s(arr);
        
        // Forward iteration
        std::cout << "Forward iteration: ";
        for (auto it = s.begin(); it != s.end(); ++it) {
            std::cout << *it << " ";
        }
        std::cout << "\n";
        
        // Iterator arithmetic
        auto it = s.begin();
        assert(*it == 1);
        assert(*(it + 2) == 3);
        assert(*(s.end() - 1) == 5);
        assert(s.end() - s.begin() == 5);
        
        // Random access
        it += 3;
        assert(*it == 4);
        it -= 2;
        assert(*it == 2);
    }

    PrintSubTest("Const Iterators");
    {
        int arr[] = {10, 20, 30};
        ara::core::Span<int> s(arr);
        
        [[maybe_unused]] auto cit = s.cbegin();
        assert(*cit == 10);
        
        // Can't modify through const iterator
        // *cit = 15;  // ERROR
        
        // Const iterator from non-const span
        for (auto it = s.cbegin(); it != s.cend(); ++it) {
            [[maybe_unused]] const int& val = *it;  // OK
        }
        
        std::cout << "Const iterators verified\n";
    }

    PrintSubTest("Reverse Iterators");
    {
        int arr[] = {1, 2, 3, 4, 5};
        ara::core::Span<int> s(arr);
        
        std::cout << "Reverse iteration: ";
        for (auto it = s.rbegin(); it != s.rend(); ++it) {
            std::cout << *it << " ";
        }
        std::cout << "\n";
        
        assert(*s.rbegin() == 5);
        assert(*(s.rend() - 1) == 1);
        
        // Const reverse
        [[maybe_unused]] auto crit = s.crbegin();
        assert(*crit == 5);
    }

    PrintSubTest("Range-based For Loop");
    {
        int arr[] = {10, 20, 30, 40, 50};
        ara::core::Span<int> s(arr);
        
        std::cout << "Range-based for: ";
        for (int val : s) {
            std::cout << val << " ";
        }
        std::cout << "\n";
        
        // Modify through range-based for
        for (int& val : s) {
            val += 5;
        }
        
        assert(arr[0] == 15);
        assert(arr[4] == 55);
        
        // Const range
        ara::core::Span<const int> cs(arr);
        for ([[maybe_unused]] const int& val : cs) {
            // Can only read
        }
    }

    PrintSubTest("Iterator Traits");
    {
        using Span = ara::core::Span<int>;
        using It = Span::iterator;
        
        static_assert(std::is_same_v<
            std::iterator_traits<It>::iterator_category,
            std::random_access_iterator_tag
        >, "Must be random access iterator");
        
        static_assert(std::is_same_v<
            std::iterator_traits<It>::value_type,
            int
        >, "Value type must be int");
        
        std::cout << "Iterator traits verified\n";
    }

    PrintSubTest("Algorithm Compatibility");
    {
        int arr[] = {5, 2, 8, 1, 9, 3};
        ara::core::Span<int> s(arr);
        
        // Sort
        std::sort(s.begin(), s.end());
        
        std::cout << "After sort: ";
        for (int val : s) {
            std::cout << val << " ";
        }
        std::cout << "\n";
        
        assert(std::is_sorted(s.begin(), s.end()));
        
        // Find
        [[maybe_unused]] auto it = ara::core::find(s.begin(), s.end(), 5);
        assert(it != s.end());
        assert(*it == 5);
        
        // Accumulate
        [[maybe_unused]] int sum = std::accumulate(s.begin(), s.end(), 0);
        assert(sum == 1 + 2 + 3 + 5 + 8 + 9);
    }
}

/*!
 * \brief Test #4: Subspan Operations
 */
void TestSubspanOperations()
{
    PrintTestHeader("Subspan Operations");

    PrintSubTest("first() - Static Count");
    {
        int arr[] = {1, 2, 3, 4, 5, 6, 7, 8};
        ara::core::Span<int> s(arr);
        
        [[maybe_unused]] auto first3 = s.first<3>();
        static_assert(first3.size() == 3, "Static size mismatch");
        
        std::cout << "First 3 elements: ";
        for (int val : first3) {
            std::cout << val << " ";
        }
        std::cout << "\n";
        
        assert(first3[0] == 1);
        assert(first3[2] == 3);
        
        // Static span
        ara::core::Span<int, 8> s2(arr);
        [[maybe_unused]] auto first5 = s2.first<5>();
        static_assert(first5.size() == 5, "Static size mismatch");
    }

    PrintSubTest("first() - Dynamic Count");
    {
        int arr[] = {10, 20, 30, 40, 50};
        ara::core::Span<int> s(arr);
        
        [[maybe_unused]] auto first2 = s.first(2);
        assert(first2.size() == 2);
        assert(first2[0] == 10);
        assert(first2[1] == 20);
        
        // Edge cases
        [[maybe_unused]] auto first0 = s.first(0);
        assert(first0.empty());

        [[maybe_unused]] auto firstAll = s.first(5);
        assert(firstAll.size() == 5);
    }

    PrintSubTest("last() - Static Count");
    {
        int arr[] = {1, 2, 3, 4, 5, 6};
        ara::core::Span<int> s(arr);

        [[maybe_unused]] auto last3 = s.last<3>();
        static_assert(last3.size() == 3, "Static size mismatch");
        
        std::cout << "Last 3 elements: ";
        for (int val : last3) {
            std::cout << val << " ";
        }
        std::cout << "\n";
        
        assert(last3[0] == 4);
        assert(last3[2] == 6);
    }

    PrintSubTest("last() - Dynamic Count");
    {
        int arr[] = {100, 200, 300, 400};
        ara::core::Span<int> s(arr);

        [[maybe_unused]] auto last2 = s.last(2);
        assert(last2.size() == 2);
        assert(last2[0] == 300);
        assert(last2[1] == 400);
    }

    PrintSubTest("subspan() - Static Offset and Count");
    {
        int arr[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        ara::core::Span<int> s(arr);
        
        // Middle 4 elements starting at index 3
        auto sub = s.subspan<3, 4>();
        static_assert(sub.size() == 4, "Static size mismatch");
        
        std::cout << "Subspan [3, 4): ";
        for (int val : sub) {
            std::cout << val << " ";
        }
        std::cout << "\n";
        
        assert(sub[0] == 4);
        assert(sub[3] == 7);
        
        // To end
        [[maybe_unused]] auto toEnd = s.subspan<7>();  // Default count = dynamic_extent
        assert(toEnd.size() == 3);
        assert(toEnd[0] == 8);
    }

    PrintSubTest("subspan() - Dynamic Offset and Count");
    {
        int arr[] = {10, 20, 30, 40, 50, 60, 70};
        ara::core::Span<int> s(arr);

        [[maybe_unused]] auto sub1 = s.subspan(2, 3);
        assert(sub1.size() == 3);
        assert(sub1[0] == 30);
        assert(sub1[2] == 50);
        
        // To end
        [[maybe_unused]] auto sub2 = s.subspan(4);  // Default count
        assert(sub2.size() == 3);
        assert(sub2[0] == 50);
        
        // Empty subspan
        [[maybe_unused]] auto empty = s.subspan(7, 0);
        assert(empty.empty());
    }

    PrintSubTest("Chained Subspan Operations");
    {
        int arr[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        ara::core::Span<int> s(arr);
        
        // Take first 8, then last 5 of those, then middle 3
        auto result = s.first<8>().last<5>().subspan<1, 3>();
        
        std::cout << "Chained result: ";
        for (int val : result) {
            std::cout << val << " ";
        }
        std::cout << "\n";
        
        assert(result.size() == 3);
        assert(result[0] == 5);
        assert(result[2] == 7);
    }

    PrintSubTest("Subspan Type Deduction");
    {
        int arr[] = {1, 2, 3, 4, 5};
        
        // Static extent preserved when possible
        ara::core::Span<int, 5> s(arr);
        
        auto f3 = s.first<3>();
        static_assert(std::is_same_v<decltype(f3), ara::core::Span<int, 3>>,
                      "Should deduce static extent");
        
        auto l2 = s.last<2>();
        static_assert(std::is_same_v<decltype(l2), ara::core::Span<int, 2>>,
                      "Should deduce static extent");
        
        auto sub = s.subspan<1, 3>();
        static_assert(std::is_same_v<decltype(sub), ara::core::Span<int, 3>>,
                      "Should deduce static extent");
        
        // Dynamic extent when count is runtime
        auto dyn = s.first(3);
        static_assert(std::is_same_v<decltype(dyn), ara::core::Span<int, ara::core::dynamic_extent>>,
                      "Should deduce dynamic extent");
    }
}

/*!
 * \brief Test #5: Comparison Operations
 */
void TestComparisons()
{
    PrintTestHeader("Comparison Operations");

    PrintSubTest("Equality Comparisons");
    {
        int arr1[] = {1, 2, 3, 4, 5};
        int arr2[] = {1, 2, 3, 4, 5};
        int arr3[] = {1, 2, 3, 4, 6};
        
        ara::core::Span<int> s1(arr1);
        ara::core::Span<int> s2(arr2);
        ara::core::Span<int> s3(arr3);
        
        // Equal content
        assert(s1 == s2);
        assert(!(s1 != s2));
        
        // Different content
        assert(s1 != s3);
        assert(!(s1 == s3));
        
        // Different sizes
        ara::core::Span<int> s4(arr1, 3);
        assert(s1 != s4);
        
        // Empty spans
        [[maybe_unused]] ara::core::Span<int> empty1;
        [[maybe_unused]] ara::core::Span<int> empty2;
        assert(empty1 == empty2);
        
        std::cout << "Equality comparisons working correctly\n";
    }

    PrintSubTest("Lexicographical Comparisons");
    {
        int arr1[] = {1, 2, 3};
        int arr2[] = {1, 2, 4};
        int arr3[] = {1, 2, 3, 4};
        
        ara::core::Span<int> s1(arr1);
        ara::core::Span<int> s2(arr2);
        ara::core::Span<int> s3(arr3);
        
        // Less than
        assert(s1 < s2);   // {1,2,3} < {1,2,4}
        assert(s1 < s3);   // {1,2,3} < {1,2,3,4}
        assert(!(s2 < s1));
        
        // Less than or equal
        assert(s1 <= s2);
        assert(s1 <= s1);  // Equal
        
        // Greater than
        assert(s2 > s1);
        assert(s3 > s1);
        
        // Greater than or equal
        assert(s2 >= s1);
        assert(s1 >= s1);  // Equal
        
        std::cout << "Lexicographical comparisons verified\n";
    }

    PrintSubTest("Mixed Type Comparisons");
    {
        int arr1[] = {1, 2, 3};
        const int arr2[] = {1, 2, 3};
        
        ara::core::Span<int> s1(arr1);
        ara::core::Span<const int> s2(arr2);
        
        // Can compare Span<T> with Span<const T>
        assert(s1 == s2);
        assert(!(s1 != s2));
        assert(!(s1 < s2));
        assert(s1 <= s2);
        
        // Different extents
        ara::core::Span<int, 3> s3(arr1);
        ara::core::Span<int> s4(arr1);
        
        assert(s3 == s4);  // Same content
        
        std::cout << "Mixed type comparisons working\n";
    }

    PrintSubTest("Compile-time Comparisons");
    {
        static constexpr int arr1[] = {1, 2, 3};
        static constexpr int arr2[] = {1, 2, 4};
        
        constexpr ara::core::Span<const int, 3> s1(arr1);
        constexpr ara::core::Span<const int, 3> s2(arr2);
        
        static_assert(s1 != s2, "Constexpr inequality failed");
        static_assert(s1 < s2, "Constexpr less than failed");
        static_assert(s1 <= s2, "Constexpr less equal failed");
        static_assert(s2 > s1, "Constexpr greater than failed");
        static_assert(s2 >= s1, "Constexpr greater equal failed");
        
        std::cout << "Compile-time comparisons verified\n";
    }
}

/*!
 * \brief Test #6: Size and Capacity Queries
 */
void TestSizeAndCapacity()
{
    PrintTestHeader("Size and Capacity Queries");

    PrintSubTest("size() and size_bytes()");
    {
        int arr[] = {1, 2, 3, 4, 5};
        ara::core::Span<int> s1(arr);
        ara::core::Span<int, 5> s2(arr);
        
        assert(s1.size() == 5);
        assert(s2.size() == 5);
        
        assert(s1.size_bytes() == 5 * sizeof(int));
        assert(s2.size_bytes() == 5 * sizeof(int));
        
        // Different element sizes
        char carr[] = "Hello";
        ara::core::Span<char> s3(carr, 5);
        assert(s3.size_bytes() == 5 * sizeof(char));
        
        struct Large { int data[10]; };
        Large larr[3];
        ara::core::Span<Large> s4(larr);
        assert(s4.size_bytes() == 3 * sizeof(Large));
        
        std::cout << "size() and size_bytes() verified\n";
    }

    PrintSubTest("empty()");
    {
        [[maybe_unused]] ara::core::Span<int> empty1;
        [[maybe_unused]] ara::core::Span<int, 0> empty2;
        
        assert(empty1.empty());
        assert(empty2.empty());
        
        int arr[] = {1};
        ara::core::Span<int> nonempty(arr);
        assert(!nonempty.empty());
        
        // After operations
        [[maybe_unused]] auto sub = nonempty.subspan(1, 0);
        assert(sub.empty());
        
        std::cout << "empty() checks verified\n";
    }

    PrintSubTest("Static extent Property");
    {
        ara::core::Span<int> dynamic;
        ara::core::Span<int, 0> static0;
        
        static_assert(dynamic.extent == ara::core::dynamic_extent,
                      "Dynamic extent mismatch");
        static_assert(static0.extent == 0, "Static extent mismatch");
        
        std::cout << "Static extent property verified\n";
        std::cout << "Dynamic extent value: " << ara::core::dynamic_extent << "\n";
    }

    PrintSubTest("Compile-time Size Queries");
    {
        static constexpr int arr[] = {1, 2, 3, 4};
        constexpr ara::core::Span<const int, 4> s(arr);
        
        static_assert(s.size() == 4, "Constexpr size failed");
        static_assert(s.size_bytes() == 4 * sizeof(int), "Constexpr size_bytes failed");
        static_assert(!s.empty(), "Constexpr empty failed");
        
        constexpr ara::core::Span<const int, 0> empty;
        static_assert(empty.empty(), "Constexpr empty span failed");
        
        std::cout << "Compile-time size queries verified\n";
    }
}

namespace {
    constexpr int kSplitArray[] = {10, 20, 30, 40, 50};
    constexpr int kInitListArr[] = {1,2,3,4,5};
}

/*!
 * \brief Test #7: C++26 Features
 */
void TestCpp26Features()
{
    PrintTestHeader("C++26 Features (Backported to C++17)");

    PrintSubTest("contains() Method");
    {
        int arr[] = {1, 2, 3, 4, 5};
        ara::core::Span<int> s(arr);
        
        assert(s.contains(3));
        assert(!s.contains(6));
        assert(s.contains(1));
        assert(s.contains(5));
        
        // Empty span
        [[maybe_unused]] ara::core::Span<int> empty;
        assert(!empty.contains(1));
        
        // Constexpr
        static constexpr int carr[] = {10, 20, 30};
        constexpr ara::core::Span<const int> cs(carr);
        static_assert(cs.contains(20), "Constexpr contains failed");
        static_assert(!cs.contains(25), "Constexpr contains failed");
        
        std::cout << "contains() method verified\n";
    }

    PrintSubTest("starts_with() Method");
    {
        int arr[] = {1, 2, 3, 4, 5, 6};
        ara::core::Span<int> s(arr);
        
        [[maybe_unused]] int prefix1[] = {1, 2, 3};
        [[maybe_unused]] int prefix2[] = {1, 2, 4};
        [[maybe_unused]] int prefix3[] = {1, 2, 3, 4, 5, 6};
        
        assert(s.starts_with(ara::core::Span<int>(prefix1)));
        assert(!s.starts_with(ara::core::Span<int>(prefix2)));
        assert(s.starts_with(ara::core::Span<int>(prefix3)));
        
        // Empty prefix
        assert(s.starts_with(ara::core::Span<int>()));
        
        // Longer prefix
        [[maybe_unused]] int longer[] = {1, 2, 3, 4, 5, 6, 7};
        assert(!s.starts_with(ara::core::Span<int>(longer)));
        
        std::cout << "starts_with() method verified\n";
    }

    PrintSubTest("ends_with() Method");
    {
        int arr[] = {1, 2, 3, 4, 5, 6};
        ara::core::Span<int> s(arr);
        
        [[maybe_unused]] int suffix1[] = {4, 5, 6};
        [[maybe_unused]] int suffix2[] = {4, 5, 7};
        [[maybe_unused]] int suffix3[] = {1, 2, 3, 4, 5, 6};
        
        assert(s.ends_with(ara::core::Span<int>(suffix1)));
        assert(!s.ends_with(ara::core::Span<int>(suffix2)));
        assert(s.ends_with(ara::core::Span<int>(suffix3)));
        
        // Empty suffix
        assert(s.ends_with(ara::core::Span<int>()));
        
        std::cout << "ends_with() method verified\n";
    }

    PrintSubTest("split() Method");
    {
        int arr[] = {1, 2, 3, 0, 4, 5, 6};
        ara::core::Span<int> s(arr);
        
        [[maybe_unused]] auto [before, after] = s.split(0);
        
        assert(before.size() == 3);
        assert(before[0] == 1);
        assert(before[2] == 3);
        
        assert(after.size() == 3);
        assert(after[0] == 4);
        assert(after[2] == 6);
        
        // Split on non-existent
        [[maybe_unused]] auto [all, none] = s.split(99);
        assert(all.size() == 7);
        assert(none.empty());
        
        // Split at beginning
        int arr2[] = {0, 1, 2, 3};
        [[maybe_unused]] auto [empty, rest] = ara::core::Span<int>(arr2).split(0);
        assert(empty.empty());
        assert(rest.size() == 3);
        
        std::cout << "split() method verified\n";
    }

    PrintSubTest("Compile-time C++26 Features");
    {
        constexpr ara::core::Span<const int> s(kSplitArray);  
        
        // contains
        static_assert(s.contains(30), "Constexpr contains failed");
        static_assert(!s.contains(35), "Constexpr contains failed");
        
        // starts_with
        static constexpr int prefix[] = {10, 20};
        static_assert(s.starts_with(ara::core::Span<const int>(prefix)),
                      "Constexpr starts_with failed");
        
        // ends_with
        static constexpr int suffix[] = {40, 50};
        static_assert(s.ends_with(ara::core::Span<const int>(suffix)),
                      "Constexpr ends_with failed");
        
        // split
        constexpr auto result = s.split(30);
        static_assert(result.first.size() == 2, "Constexpr split failed");
        static_assert(result.second.size() == 2, "Constexpr split failed");
        
        std::cout << "Compile-time C++26 features verified\n";
    }
}

/*!
 * \brief Test #8: Initializer List Support
 */
void TestInitializerListSupport()
{
    PrintTestHeader("Initializer List Support (C++17)");

    PrintSubTest("Basic Initializer List Construction");
    {
        // Only works for const element types
        ara::core::Span<const int> s1 = {1, 2, 3, 4, 5}; // dangling initializer list
        // ara::core::Span<int> s2 = {1, 2, 3}; // Would fail: non-const initializer list
        
        std::cout << "Initializer list elements: ";
        for (int val : s1) {
            std::cout << val << " ";
        }
        std::cout << "\n";
    }

    PrintSubTest("Static Extent with Initializer List");
    {
        ara::core::Span<const int, 3> s1 = {10, 20, 30};
        
        assert(s1.size() == 3);
        assert(s1[1] == 20);
        
        for (int val : s1) {
            std::cout << val << " ";
        }
        std::cout << "\n";
        // Size must match for static extent
        // ara::core::Span<const int, 3> s2 = {1, 2};  // Would trigger violation
        
        std::cout << "Static extent with initializer list verified\n";
    }

    PrintSubTest("Initializer List in Expressions");
    {
        auto process = [](ara::core::Span<const int> s) {
            int sum = 0;
            for (int val : s) {
                sum += val;
            }
            return sum;
        };
        
        int result = process({1, 2, 3, 4, 5});
        assert(result == 15);
        
        std::cout << "Initializer list in function call: sum = " << result << "\n";
    }

    PrintSubTest("Compile-time Initializer List");
    {
        // Works in C++17 with some limitations
        constexpr ara::core::Span<const int> s(kInitListArr, 5);

        static_assert(s.size() == 5, "Constexpr size failed");
        static_assert(s[2] == 3, "Constexpr access failed");
        
        std::cout << "Compile-time initializer support verified\n";
    }
}

/*!
 * \brief Test #9: Ranges Support
 */
void TestRangesSupport()
{
    PrintTestHeader("Ranges Support");

#if __cplusplus >= 202002L
    PrintSubTest("C++20 std::ranges Integration");
    {
        int arr[] = {5, 2, 8, 1, 9, 3};
        ara::core::Span<int> s(arr);
        
        // Span works with ranges algorithms
        std::ranges::sort(s);
        
        std::cout << "After ranges::sort: ";
        for (int val : s) {
            std::cout << val << " ";
        }
        std::cout << "\n";
        
        assert(std::ranges::is_sorted(s));
        
        // Ranges views
        auto even_view = s | std::views::filter([](int x) { return x % 2 == 0; });
        
        std::cout << "Even numbers: ";
        for (int val : even_view) {
            std::cout << val << " ";
        }
        std::cout << "\n";
        
        // Transform view
        auto squared = s | std::views::transform([](int x) { return x * x; });
        
        std::cout << "Squared values: ";
        for (int val : squared) {
            std::cout << val << " ";
        }
        std::cout << "\n";
    }
    
    PrintSubTest("Span as a Borrowed Range");
    {
        // Span satisfies borrowed_range concept
        static_assert(std::ranges::borrowed_range<ara::core::Span<int>>,
                      "Span should be a borrowed range");
        
        [[maybe_unused]] int arr[] = {1, 2, 3, 4, 5};
        
        auto get_span = []() -> ara::core::Span<int> {
            static int local[] = {1, 2, 3, 4, 5};
            return ara::core::Span<int>(local);
        };
        
        // Safe to use dangling check
        [[maybe_unused]] auto result = std::ranges::find(get_span(), 3);
        // result is std::ranges::dangling if not borrowed
        
        std::cout << "Borrowed range concept verified\n";
    }
#endif

    PrintSubTest("C++17 Custom Ranges Support");
    {
        int arr[] = {1, 2, 3, 4, 5, 6};
        ara::core::Span<int> s(arr);
        
        // Transform
        auto doubled = ara::core::ranges::transform(s, [](int x) noexcept -> int { return x * 2; });

        std::cout << "Custom transform (doubled): ";
        for (auto val : doubled) {
            std::cout << val << " ";
        }
        std::cout << "\n";
        
        // Filter
        auto evens = ara::core::ranges::filter(s, [](int x) noexcept -> bool { return x % 2 == 0; });

        std::cout << "Custom filter (evens): ";
        for (int val : evens) {
            std::cout << val << " ";
        }
        std::cout << "\n";
        
        // Take
        [[maybe_unused]] auto first3 = ara::core::ranges::take(s, 3);
        assert(first3.size() == 3);
        
        // Drop
        [[maybe_unused]] auto skip2 = ara::core::ranges::drop(s, 2);
        assert(skip2.size() == 4);
        assert(skip2[0] == 3);
        
        // Take while
        [[maybe_unused]] auto small = ara::core::ranges::take_while(s, [](int x) { return x < 4; });
        assert(small.size() == 3);
        
        // Drop while
        [[maybe_unused]] auto large = ara::core::ranges::drop_while(s, [](int x) { return x < 4; });
        assert(large.size() == 3);
        assert(large[0] == 4);
    }

    PrintSubTest("Ranges Utilities");
    {
        // Check if span is detected as span
        using S = ara::core::Span<int>;
        static_assert(ara::core::ranges::is_span_v<S>, "Should detect span");
        static_assert(!ara::core::ranges::is_span_v<std::vector<int>>, "Should not detect vector");
        
        // Enable borrowed range
        static_assert(ara::core::ranges::enable_borrowed_range<S>::value,
                      "Span should enable borrowed range");
        
        // View creation
        int arr[] = {1, 2, 3};
        [[maybe_unused]] auto view = ara::core::ranges::view(ara::core::Span<int>(arr));
        assert(view.size() == 3);
        
        // From range
        std::vector<int> vec = {10, 20, 30};
        [[maybe_unused]] auto span = ara::core::ranges::from_range(vec);
        assert(span.size() == 3);
        assert(span[1] == 20);
        
        std::cout << "Custom ranges utilities verified\n";
    }
}

/*!
 * \brief Test #10: Factory Functions
 */
void TestFactoryFunctions()
{
    PrintTestHeader("Factory Functions (MakeSpan)");

    PrintSubTest("MakeSpan from Pointer and Size");
    {
        int arr[] = {1, 2, 3, 4, 5};
        
        [[maybe_unused]] auto s = ara::core::MakeSpan(arr, 5);
        static_assert(std::is_same_v<decltype(s), ara::core::Span<int>>,
                      "Should deduce dynamic extent");
        
        assert(s.size() == 5);
        assert(s[2] == 3);
        
        // Const array
        const int carr[] = {10, 20, 30};
        auto cs = ara::core::MakeSpan(carr, 3);
        static_assert(std::is_same_v<decltype(cs), ara::core::Span<const int>>,
                      "Should deduce const element type");
        
        std::cout << "MakeSpan from pointer+size verified\n";
    }

    PrintSubTest("MakeSpan from Iterator Pair");
    {
        int arr[] = {10, 20, 30, 40, 50};

        [[maybe_unused]] auto s = ara::core::MakeSpan(arr, arr + 5);
        assert(s.size() == 5);
        assert(s[0] == 10);
        
        // Partial range
        [[maybe_unused]] auto partial = ara::core::MakeSpan(arr + 1, arr + 4);
        assert(partial.size() == 3);
        assert(partial[0] == 20);
        
        std::cout << "MakeSpan from iterators verified\n";
    }

    PrintSubTest("MakeSpan from C-style Array");
    {
        int arr[6] = {1, 2, 3, 4, 5, 6};
        
        [[maybe_unused]] auto s = ara::core::MakeSpan(arr);
        static_assert(std::is_same_v<decltype(s), ara::core::Span<int, 6>>,
                      "Should deduce static extent from array");
        
        assert(s.size() == 6);
        
        // 2D array (decays to pointer)
        int arr2d[3][4] = {{1, 2, 3, 4}, {5, 6, 7, 8}, {9, 10, 11, 12}};
        [[maybe_unused]] auto s2d = ara::core::MakeSpan(arr2d);
        static_assert(std::is_same_v<decltype(s2d), ara::core::Span<int[4], 3>>,
                      "Should handle 2D array");
        
        std::cout << "MakeSpan from array verified\n";
    }

    PrintSubTest("MakeSpan from Containers");
    {
        // Vector
        std::vector<int> vec = {1, 2, 3, 4, 5};
        [[maybe_unused]] auto sv = ara::core::MakeSpan(vec);
        assert(sv.size() == 5);
        
        // Const vector
        const std::vector<int> cvec = {10, 20, 30};
        [[maybe_unused]] auto csv = ara::core::MakeSpan(cvec);
        static_assert(std::is_same_v<decltype(csv), ara::core::Span<const int>>,
                      "Should deduce const from const container");
        
        // String
        std::string str = "Hello";
        [[maybe_unused]] auto ss = ara::core::MakeSpan(str);
        assert(ss.size() == 5);
        assert(ss[0] == 'H');
        
        // std::array
        std::array<double, 3> arr = {1.1, 2.2, 3.3};
        [[maybe_unused]] auto sa = ara::core::MakeSpan(arr);
        assert(sa.size() == 3);
        
        std::cout << "MakeSpan from containers verified\n";
    }
}

template <typename T>
constexpr T constexpr_abs(T x) {
    return x < T(0) ? -x : x;
}

template <typename T>
constexpr bool almost_equal(T a, T b, T abs_epsilon = std::numeric_limits<T>::epsilon(),
                            T rel_epsilon = std::numeric_limits<T>::epsilon()) {
    static_assert(std::numeric_limits<T>::is_iec559, "Requires IEEE-754 float/double");
    T diff = constexpr_abs(a - b);
    if (diff <= abs_epsilon) {
        return true; // small absolute difference
    }
    // Otherwise use relative error
    T largest = (constexpr_abs(a) > constexpr_abs(b)) ? constexpr_abs(a) : constexpr_abs(b);
    return diff <= largest * rel_epsilon;
}

/*!
 * \brief Test #11: Byte Representation
 */
void TestByteRepresentation()
{
    PrintTestHeader("Byte Representation (as_bytes/as_writable_bytes)");

    PrintSubTest("as_bytes() for Reading");
    {
        std::uint32_t arr[] = { 0x12345678u, 0x9ABCDEF0u };
        ara::core::Span<std::uint32_t> s(arr);

        auto bytes = ara::core::as_bytes(s);
        assert(bytes.size() == 2 * sizeof(std::uint32_t));

        std::cout << "Bytes of std::uint32_t array: ";

        const unsigned char* raw =
            reinterpret_cast<const unsigned char*>(arr);

        for (std::size_t i = 0; i < bytes.size(); ++i) {
            std::cout << std::hex << static_cast<int>(raw[i]) << " ";
        }
        std::cout << std::dec << "\n";
        
        // Const span
        ara::core::Span<const std::uint32_t, 2> cs(arr);
        auto cbytes = ara::core::as_bytes(cs);
        static_assert(std::is_same_v<decltype(cbytes), 
                      ara::core::Span<const ara::core::Byte, 2 * sizeof(std::uint32_t)>>,
                      "Should deduce const byte span with static extent");
    }

    PrintSubTest("as_writable_bytes() for Modification");
    {
        std::uint32_t arr[] = { 0x12345678u };
        ara::core::Span<std::uint32_t> s(arr);
        
        auto bytes = ara::core::as_writable_bytes(s);
        
        // Modify first byte
        bytes[0] = ara::core::Byte{0xFF};
        
        std::cout << "After modifying first byte: 0x" 
                  << std::hex << arr[0] << std::dec << "\n";
        
        // Should have modified the original
        assert((arr[0] & 0xFF) == 0xFF);
        
        // Not available for const spans
        // ara::core::Span<const int> cs(arr);
        // auto wbytes = ara::core::as_writable_bytes(cs);  // ERROR
    }

    PrintSubTest("Static Extent Preservation");
    {
        int arr[3] = {1, 2, 3};
        ara::core::Span<int, 3> s(arr);
        
        auto bytes = ara::core::as_bytes(s);
        static_assert(bytes.extent == 3 * sizeof(int),
                      "Should calculate static byte extent");
        
        auto wbytes = ara::core::as_writable_bytes(s);
        static_assert(wbytes.extent == 3 * sizeof(int),
                      "Should calculate static byte extent");
        
        std::cout << "Static extent preservation verified\n";
    }

    PrintSubTest("Working with Structures - Safe Read");
    {
        struct Point {
            float x, y, z;
        };

        Point points[] = {{1.0f, 2.0f, 3.0f}, {4.0f, 5.0f, 6.0f}};
        ara::core::Span<Point> s(points);

        auto bytes = ara::core::as_bytes(s);
        assert(bytes.size() == 2 * sizeof(Point));

        // Read floats safely using memcpy
        std::vector<float> float_values;
        float_values.reserve(bytes.size() / sizeof(float));

        for (std::size_t i = 0; i < bytes.size(); i += sizeof(float)) {
            float value;
            std::memcpy(&value, bytes.data() + i, sizeof(float));
            float_values.push_back(value);
        }

        assert(float_values.size() == 6);
        assert(almost_equal(float_values[0], 1.0f));
        assert(almost_equal(float_values[1], 2.0f));

        std::cout << "Structure byte representation verified\n";
    }
}

/*!
 * \brief Test #12: Type Traits and Concepts
 */
void TestTypeTraits()
{
    PrintTestHeader("Type Traits and Concepts");

    PrintSubTest("Basic Type Properties");
    {
        using S1 = ara::core::Span<int>;
        using S2 = ara::core::Span<int, 5>;
        
        // Size
        std::cout << "sizeof(Span<int>): " << sizeof(S1) << " bytes\n";
        std::cout << "sizeof(Span<int, 5>): " << sizeof(S2) << " bytes\n";
        
        assert(sizeof(S1) == 2 * sizeof(void*));  // pointer + size
        assert(sizeof(S2) == sizeof(void*));       // pointer only
        
        // Trivial type
        static_assert(std::is_trivially_copyable_v<S1>, "Span must be trivially copyable");
        static_assert(std::is_standard_layout_v<S1>,     "Span must have standard layout");
        static_assert(std::is_trivially_copyable_v<S2>, "Static Span must be trivially copyable");
        static_assert(std::is_standard_layout_v<S2>,     "Static Span must have standard layout");
        
        std::cout << "Span is a trivial, standard-layout type\n";
    }

    PrintSubTest("Member Type Aliases");
    {
        using S = ara::core::Span<const int, 10>;
        
        static_assert(std::is_same_v<S::element_type, const int>,
                      "element_type mismatch");
        static_assert(std::is_same_v<S::value_type, int>,
                      "value_type should remove cv");
        static_assert(std::is_same_v<S::size_type, std::size_t>,
                      "size_type mismatch");
        static_assert(std::is_same_v<S::difference_type, std::ptrdiff_t>,
                      "difference_type mismatch");
        static_assert(std::is_same_v<S::pointer, const int*>,
                      "pointer mismatch");
        static_assert(std::is_same_v<S::reference, const int&>,
                      "reference mismatch");
        
        std::cout << "Member type aliases verified\n";
    }

    PrintSubTest("Iterator Properties");
    {
        using S = ara::core::Span<int>;
        using It = S::iterator;
        using CIt = S::const_iterator;
        
        // Iterator category
        static_assert(std::is_same_v<
            std::iterator_traits<It>::iterator_category,
            std::random_access_iterator_tag
        >, "Must be random access iterator");
        
        // Convertibility
        static_assert(std::is_convertible_v<It, CIt>,
                      "iterator must convert to const_iterator");
        static_assert(!std::is_convertible_v<CIt, It>,
                      "const_iterator must not convert to iterator");
        
        std::cout << "Iterator properties verified\n";
    }

#if __cplusplus >= 202002L
    PrintSubTest("C++20 Concepts");
    {
        using S = ara::core::Span<int>;
        
        // Range concepts
        static_assert(std::ranges::range<S>, "Span must be a range");
        static_assert(std::ranges::contiguous_range<S>, "Span must be contiguous");
        static_assert(std::ranges::sized_range<S>, "Span must be sized");
        static_assert(std::ranges::borrowed_range<S>, "Span must be borrowed");
        
        // Iterator concepts
        static_assert(std::contiguous_iterator<S::iterator>,
                      "Span iterator must be contiguous");
        
        std::cout << "C++20 concepts verified\n";
    }
#endif

    PrintSubTest("SFINAE Helpers");
    {
        // Test internal type traits
        using namespace ara::core::detail;
        
        // is_span
        static_assert(is_span_v<ara::core::Span<int>>, "Should detect span");
        static_assert(!is_span_v<std::vector<int>>, "Should not detect vector");
        
        // is_std_array
        static_assert(is_std_array_v<std::array<int, 5>>, "Should detect std::array");
        static_assert(!is_std_array_v<int[5]>, "Should not detect C array");
        
        // has_data_and_size
        static_assert(has_data_and_size_v<std::vector<int>>, "Vector has data/size");
        static_assert(has_data_and_size_v<std::string>, "String has data/size");
        static_assert(!has_data_and_size_v<std::list<int>>, "List lacks data()");
        
        std::cout << "SFINAE helpers verified\n";
    }
}

/*!
 * \brief Test #13: Performance Comparisons
 */
void TestPerformance()
{
    PrintTestHeader("Performance Comparisons");

    constexpr std::size_t size = 1'000'000;
    std::vector<int> vec(size);
    std::iota(vec.begin(), vec.end(), 0);

    PrintSubTest("Span vs Raw Pointer Performance");
    {
        PerfTimer timer;
        
        // Raw pointer access
        timer.reset();
        [[maybe_unused]] long long sum1 = 0;
        int* ptr = vec.data();
        for (std::size_t i = 0; i < size; ++i) {
            sum1 += ptr[i];
        }
        double raw_time = timer.elapsed();
        
        // Dynamic span access
        timer.reset();
        [[maybe_unused]] long long sum2 = 0;
        ara::core::Span<int> s(vec);
        for (std::size_t i = 0; i < size; ++i) {
            sum2 += s[i];
        }
        double span_time = timer.elapsed();
        
        // Static span access (if size were known at compile time)
        timer.reset();
        [[maybe_unused]] long long sum3 = 0;
        // Simulate with first 1000 elements
        ara::core::Span<int, 1000> s_static(vec.data(), 1000);
        for (std::size_t i = 0; i < 1000; ++i) {
            sum3 += s_static[i];
        }
        double static_span_time = timer.elapsed() * (size / 1000.0);  // Scale up
        
        std::cout << "Raw pointer: " << raw_time << " μs\n";
        std::cout << "Dynamic span: " << span_time << " μs (overhead: " 
                  << ((span_time / raw_time) - 1.0) * 100.0 << "%)\n";
        std::cout << "Static span (scaled): " << static_span_time << " μs\n";
        
        assert(sum1 == sum2);  // Ensure same result
    }

    PrintSubTest("Iterator Performance");
    {
        PerfTimer timer;
        
        // STL iterator
        timer.reset();
        [[maybe_unused]] long long sum1 = 0;
        for (auto it = vec.begin(); it != vec.end(); ++it) {
            sum1 += *it;
        }
        double vec_iter_time = timer.elapsed();
        
        // Span iterator
        ara::core::Span<int> s(vec);
        timer.reset();
        [[maybe_unused]] long long sum2 = 0;
        for (auto it = s.begin(); it != s.end(); ++it) {
            sum2 += *it;
        }
        double span_iter_time = timer.elapsed();
        
        // Range-based for
        timer.reset();
        [[maybe_unused]] long long sum3 = 0;
        for (int val : s) {
            sum3 += val;
        }
        double range_time = timer.elapsed();
        
        std::cout << "Vector iterator: " << vec_iter_time << " μs\n";
        std::cout << "Span iterator: " << span_iter_time << " μs\n";
        std::cout << "Range-based for: " << range_time << " μs\n";
        
        assert(sum1 == sum2 && sum2 == sum3);
    }

    PrintSubTest("Subspan Performance");
    {
        ara::core::Span<int> s(vec);
        PerfTimer timer;
        
        // Dynamic subspan
        timer.reset();
        for (std::size_t i = 0; i < 10000; ++i) {
            auto sub = s.subspan(i % 100, 100);
            volatile auto x = sub[0];  // Prevent optimization
            (void)x;
        }
        double dynamic_time = timer.elapsed();
        
        // Static subspan (when possible)
        timer.reset();
        for (std::size_t i = 0; i < 10000; ++i) {
            auto sub = s.subspan<0, 100>();  // Static offset and count
            volatile auto x = sub[0];
            (void)x;
        }
        double static_time = timer.elapsed();
        
        std::cout << "Dynamic subspan: " << dynamic_time << " μs\n";
        std::cout << "Static subspan: " << static_time << " μs\n";
        std::cout << "Static optimization: " 
                  << (1.0 - static_time / dynamic_time) * 100.0 << "% faster\n";
    }

    std::cout << "\nNote: ara::core::Span provides near-zero overhead compared to raw pointers\n";
    std::cout << "      Static extent spans can provide additional optimizations\n";
}

/**********************************************************************************************************************
 *  VIOLATION TEST FUNCTIONS (Will terminate)
 *********************************************************************************************************************/

/*!
 * \brief Test size mismatch violation
 */
void TestSizeViolation()
{
    std::cout << "Creating static span with size mismatch...\n";
    
    int arr[] = {1, 2, 3, 4, 5};
    ara::core::Span<int, 3> s(arr, 5);  // Size 5 doesn't match static extent 3
    
    // Never reached
    std::cout << "This should never print!\n";
}

/*!
 * \brief Test bounds violation
 */
void TestBoundsViolation()
{
    std::cout << "Accessing out-of-bounds element with at()...\n";
    
    int arr[] = {1, 2, 3};
    ara::core::Span<int> s(arr);
    
    [[maybe_unused]] int val = s.at(5);  // Index 5 >= size 3
    
    // Never reached
    std::cout << "Value: " << val << " (should never print)\n";
}

/*!
 * \brief Test null‐pointer violation: constructing a non‐empty span from nullptr.
 */
void TestNullPointerViolation()
{
    std::cout << "Creating span from nullptr with nonzero size (will terminate)...\n";
    int* p = nullptr;
    // Dynamic‐extent constructor: nullptr + count > 0 → triggers null‐pointer violation.
    ara::core::Span<int> s(p, 1);
    // Never reached
    std::cout << "ERROR: this should never print!\n";
}

/*!
 * \brief Test range‐order violation: constructing from iterators where last < first.
 */
void TestRangeViolation()
{
    std::cout << "Constructing span with reversed iterator pair (will terminate)...\n";
    int arr[] = {1,2,3,4};
    
    // Attempting to create span with reversed iterators
    [[maybe_unused]] auto s = ara::core::Span<int>(arr + 3, arr + 1);

    // This should never be reached
    std::cout << "ERROR: this should never print!\n";
}

/*!
 * \brief Test empty‐access violation: calling front() or back() on an empty span.
 */
void TestEmptyAccessViolation()
{
    std::cout << "Accessing front()/back() on empty span (will terminate)...\n";
    ara::core::Span<int> empty;
    [[maybe_unused]] int a = empty.front();
    // Or: empty.back();
    std::cout << "ERROR: this should never print!\n";
}

/*!
 * \brief Test subspan‐offset violation: taking subspan with offset > size().
 */
void TestSubspanOffsetViolation()
{
    std::cout << "Creating subspan with offset > size (will terminate)...\n";
    int arr[] = {1,2,3};
    ara::core::Span<int> s(arr);
    // subspan(offset, count) where offset 5 > size 3
    [[maybe_unused]] auto sub = s.subspan(5, 1);
    std::cout << "ERROR: this should never print!\n";
}

/*!
 * \brief Test subspan‐count violation: taking subspan with count > remaining size.
 */
void TestSubspanCountViolation()
{
    std::cout << "Creating subspan with count > remaining elements (will terminate)...\n";
    int arr[] = {1,2,3,4};
    ara::core::Span<int> s(arr);
    // subspan<Offset,Count>() static: e.g. Offset=2,Count=5 → 2+5 > 4
    [[maybe_unused]] auto sub = s.subspan<2, 5>();
    std::cout << "ERROR: this should never print!\n";
}

/**********************************************************************************************************************
 *  NEGATIVE COMPILATION SCENARIOS (All commented out)
 *********************************************************************************************************************/

void TestNegativeCompilation()
{
    PrintTestHeader("Negative Compilation Scenarios");
    
    std::cout << "All negative compilation scenarios are commented out.\n";
    std::cout << "Uncomment individual sections to observe compilation errors.\n\n";

    // ========================================================================
    // 1. CONSTRUCTION ERRORS
    // ========================================================================
    
    /*
    // Test: Size mismatch for static extent
    {
        int arr[5] = {1, 2, 3, 4, 5};
        ara::core::Span<int, 3> s(arr);  // ERROR: Array size 5 != span extent 3
    }
    */

    /*
    // Test: Non-contiguous container
    {
        std::list<int> lst = {1, 2, 3};
        ara::core::Span<int> s(lst);  // ERROR: std::list is not contiguous
    }
    */

    /*
    // Test: Const to non-const conversion
    {
        const int arr[] = {1, 2, 3};
        ara::core::Span<int> s(arr);  // ERROR: Cannot drop const
    }
    */

    /*
    // Test: Invalid converting construction
    {
        ara::core::Span<const int> cs;
        ara::core::Span<int> s(cs);  // ERROR: Cannot convert const T* to T*
    }
    */

    // ========================================================================
    // 2. ELEMENT ACCESS ERRORS
    // ========================================================================
    
    /*
    // Test: Modify through const element span
    {
        int arr[] = {1, 2, 3};
        ara::core::Span<const int> s(arr);
        s[0] = 10;  // ERROR: Cannot modify const element
    }
    */

    /*
    // Test: front() on empty span (runtime UB, not compile error)
    // This would be undefined behavior at runtime, not a compile error
    */

    // ========================================================================
    // 3. ITERATOR ERRORS
    // ========================================================================
    
    /*
    // Test: Modify through const_iterator
    {
        int arr[] = {1, 2, 3};
        ara::core::Span<int> s(arr);
        auto it = s.cbegin();
        *it = 10;  // ERROR: Cannot modify through const_iterator
    }
    */

    // ========================================================================
    // 4. SUBSPAN ERRORS
    // ========================================================================
    
    /*
    // Test: Static subspan exceeds extent
    {
        int arr[5] = {1, 2, 3, 4, 5};
        ara::core::Span<int, 5> s(arr);
        auto sub = s.subspan<3, 4>();  // ERROR: 3 + 4 > 5
    }
    */

    /*
    // Test: Static offset exceeds extent
    {
        int arr[3] = {1, 2, 3};
        ara::core::Span<int, 3> s(arr);
        auto sub = s.subspan<5>();  // ERROR: Offset 5 > extent 3
    }
    */

    /*
    // Test: first() with count exceeding static extent
    {
        int arr[3] = {1, 2, 3};
        ara::core::Span<int, 3> s(arr);
        auto f = s.first<5>();  // ERROR: Count 5 > extent 3
    }
    */

    // ========================================================================
    // 5. BYTE REPRESENTATION ERRORS
    // ========================================================================
    
    /*
    // Test: as_writable_bytes on const span
    {
        int arr[] = {1, 2, 3};
        ara::core::Span<const int> s(arr);
        auto bytes = ara::core::as_writable_bytes(s);  // ERROR: Cannot get writable bytes of const
    }
    */

    // ========================================================================
    // 6. INITIALIZER LIST ERRORS
    // ========================================================================
    
    /*
    // Test: Initializer list for non-const elements
    {
        ara::core::Span<int> s = {1, 2, 3};  // ERROR: Only const element types support init list
    }
    */

    /*
    // Test: Size mismatch with static extent
    {
        ara::core::Span<const int, 3> s = {1, 2, 3, 4, 5};  // ERROR: Size 5 != extent 3
    }
    */

    // ========================================================================
    // 7. TYPE DEDUCTION ERRORS
    // ========================================================================
    
    /*
    // Test: Ambiguous deduction
    {
        auto s = ara::core::MakeSpan({1, 2, 3});  // ERROR: Cannot deduce from initializer_list
    }
    */

    std::cout << "To test compilation errors:\n";
    std::cout << "1. Uncomment one section at a time\n";
    std::cout << "2. Attempt to compile\n";
    std::cout << "3. Observe the error message\n";
    std::cout << "4. Re-comment the section before testing another\n";
}