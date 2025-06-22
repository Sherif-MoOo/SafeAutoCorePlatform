/**********************************************************************************************************************
 *  PROJECT
 *  -------------------------------------------------------------------------------------------------------------------
 *  \verbatim
 *  OpenAA: Open Source Adaptive AUTOSAR Project (CXX_STANDARD 17/20)
 *  \endverbatim
 *  -------------------------------------------------------------------------------------------------------------------
 *  FILE DESCRIPTION
 *  -------------------------------------------------------------------------------------------------------------------
 *  \file       ara_core_string_view_test.cpp
 *  \brief      Comprehensive test application for the ara::core::StringView type.
 *
 *  \details    This file contains extensive tests for ara::core::StringView covering:
 *              1.  Construction and Type Deduction
 *              2.  Element Access and Bounds Checking
 *              3.  Iterators and Range-based For Loops
 *              4.  Capacity and Size Queries
 *              5.  String Operations (substr, copy, compare)
 *              6.  Searching Operations (find, rfind, find_first_of, etc.)
 *              7.  Modifiers (remove_prefix, remove_suffix, swap)
 *              8.  C++26 Features (starts_with, ends_with, contains)
 *              9.  Comparison Operations
 *              10. Stream Operations
 *              11. User-defined Literals
 *              12. Factory Functions (MakeStringView)
 *              13. Ranges Support
 *              14. Hash Support
 *              15. Performance Comparisons
 *              16. Violation Scenarios (separate functions)
 *              17. Negative Compilation Scenarios (commented out)
 *
 *  \note       Compile with -std=c++17 or -std=c++20 to test different features
 *              Designed to compile cleanly with -Wall -Wextra -Wpedantic -Werror
 *********************************************************************************************************************/

#if defined(__linux__)
#include <sys/prctl.h>
#elif defined(__QNXNTO__)
#include <pthread.h>
#endif

#include "ara/core/string_view.h"   // The ara::core::StringView implementation
#include "ara/core/array.h"         // For ara::core::Array tests
#include <iostream>                 // For output
#include <vector>                   // For container tests
#include <string>                   // For string tests
#include <cstring>                  // For strlen, memcpy
#include <cwchar>                   // For wcslen
#include <cassert>                  // For runtime assertions
#include <chrono>                   // For performance measurements
#include <iomanip>                  // For output formatting
#include <type_traits>              // For type trait checks
#include <algorithm>                // For algorithms
#include <sstream>                  // For stringstream tests
#include <unordered_map>            // For hash tests
#include <limits>                   // For numeric_limits
#include <unordered_set>          // For hash set tests

#if __cplusplus >= 202002L
#include <ranges>                   // For C++20 ranges tests
#include <concepts>                 // For C++20 concepts tests
#endif

static constexpr std::string_view   kProcessNameView{"CoreSvTest"};
static constexpr std::uint8_t       kMaxProcessName{15};

using namespace ara::core::literals::string_view_literals;

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
        return Duration(Clock::now() - start_).count();
    }
    
    void reset() { start_ = Clock::now(); }
};

/*!
 * \brief Print test section header
 */
inline void PrintTestHeader(const std::string& testName) {
    std::cout << "\n" << std::string(70U, '=') << "\n";
    std::cout << "Test: " << testName << "\n";
    std::cout << std::string(70U, '=') << "\n";
}

/*!
 * \brief Print sub-test header
 */
inline void PrintSubTest(const std::string& subTestName) {
    std::cout << "\n--- " << subTestName << " ---\n";
}

/*!
 * \brief Print C++ standard version
 */
inline void PrintCppStandard() {
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
void TestCapacityAndSize();
void TestStringOperations();
void TestSearchingOperations();
void TestModifiers();
void TestCpp26Features();
void TestComparisons();
void TestStreamOperations();
void TestUserDefinedLiterals();
void TestFactoryFunctions();
void TestRangesSupport();
void TestHashSupport();
void TestTypeTraits();
void TestPerformance();

// Violation tests (will terminate)
[[noreturn]] void TestNullptrViolation();
[[noreturn]] void TestBoundsViolation();
[[noreturn]] void TestEmptyAccessViolation();
[[noreturn]] void TestPosViolation();
[[noreturn]] void TestRemoveViolation();

// Negative compilation tests (commented out)
void TestNegativeCompilation();

/**********************************************************************************************************************
 *  MAIN FUNCTION
 *********************************************************************************************************************/
int main(int argc, char* argv[])
{
    static_assert(kProcessNameView.size() <= kMaxProcessName,
        "Process name is too long!");

#if defined(__linux__)
    [[maybe_unused]] auto prctl_result = prctl(PR_SET_NAME, kProcessNameView.data(), 0, 0, 0);
#elif defined(__QNXNTO__)
    [[maybe_unused]] auto pthread_result = pthread_setname_np(pthread_self(), kProcessNameView.data());
#endif

    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║       ara::core::StringView Comprehensive Test Suite       ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";
    
    PrintCppStandard();

    // Check if user wants to run violation tests
    if (argc > 1) {
        std::string arg = argv[1];
        if (arg == "violation-nullptr") {
            std::cout << "\nRunning Nullptr Violation Test (will terminate)...\n";
            TestNullptrViolation();
        }
        else if (arg == "violation-bounds") {
            std::cout << "\nRunning Bounds Violation Test (will terminate)...\n";
            TestBoundsViolation();
        }
        else if (arg == "violation-empty") {
            std::cout << "\nRunning Empty Access Violation Test (will terminate)...\n";
            TestEmptyAccessViolation();
        }
        else if (arg == "violation-pos") {
            std::cout << "\nRunning Position Violation Test (will terminate)...\n";
            TestPosViolation();
        }
        else if (arg == "violation-remove") {
            std::cout << "\nRunning Remove Violation Test (will terminate)...\n";
            TestRemoveViolation();
        }
        else {
            std::cout << "\nUsage: " << argv[0] << " [violation-type]\n";
            std::cout << "       No arguments: Run all non-terminating tests\n";
            std::cout << "       violation-nullptr: Test nullptr construction violation\n";
            std::cout << "       violation-bounds: Test bounds violation\n";
            std::cout << "       violation-empty: Test empty access violation\n";
            std::cout << "       violation-pos: Test position violation\n";
            std::cout << "       violation-remove: Test remove operation violation\n\n";
        }
    }

    // Run all non-terminating tests
    TestConstruction();
    TestElementAccess();
    TestIterators();
    TestCapacityAndSize();
    TestStringOperations();
    TestSearchingOperations();
    TestModifiers();
    TestCpp26Features();
    TestComparisons();
    TestStreamOperations();
    TestUserDefinedLiterals();
    TestFactoryFunctions();
    TestRangesSupport();
    TestHashSupport();
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
        [[maybe_unused]] ara::core::StringView sv1;
        [[maybe_unused]] ara::core::WStringView wsv1;

        std::cout << "Default StringView size: " << sv1.size() << " (expected 0)\n";
        std::cout << "Default StringView data: " << (sv1.data() == nullptr ? "nullptr" : "not null") << "\n";
        
        assert(sv1.size() == 0);
        assert(sv1.data() == nullptr);
        assert(sv1.empty());
        
        assert(wsv1.size() == 0);
        assert(wsv1.data() == nullptr);
        assert(wsv1.empty());
    }

    PrintSubTest("Construction from Pointer and Count");
    {
        const char* str = "Hello World";
        [[maybe_unused]] ara::core::StringView sv1(str, 5);  // Just "Hello"
        [[maybe_unused]] ara::core::StringView sv2(str, 11); // Full string

        std::cout << "StringView from ptr+count: \"";
        std::cout.write(sv1.data(), static_cast<std::streamsize>(sv1.size()));
        std::cout << "\"\n";
        std::cout << "Size: " << sv1.size() << "\n";
        
        assert(sv1.size() == 5);
        assert(sv1[0] == 'H');
        assert(sv1[4] == 'o');
        
        assert(sv2.size() == 11);
        assert(sv2[10] == 'd');
    }

    PrintSubTest("Construction from C-string");
    {
        const char* cstr = "AUTOSAR Adaptive";
        [[maybe_unused]] ara::core::StringView sv(cstr);

        std::cout << "StringView from C-string: \"";
        std::cout.write(sv.data(), static_cast<std::streamsize>(sv.size()));
        std::cout << "\"\n";
        std::cout << "Size: " << sv.size() << " (should be " << std::strlen(cstr) << ")\n";
        
        assert(sv.size() == std::strlen(cstr));
        assert(sv.data() == cstr);
        
        // Wide string
        const wchar_t* wcstr = L"Wide String";
        ara::core::WStringView wsv(wcstr);
        assert(wsv.size() == std::wcslen(wcstr));
    }

    PrintSubTest("Construction from std::string");
    {
        std::string str = "C++ StringView";
        ara::core::StringView sv(str);
        
        assert(sv.size() == str.size());
        assert(sv.data() == str.data());
        
        // Modify string doesn't affect view's size
        std::string str2 = "Mutable";
        ara::core::StringView sv2(str2);
        [[maybe_unused]] auto original_size = sv2.size();
        str2.append(" String");
        assert(sv2.size() == original_size); // View size unchanged
        
        std::cout << "StringView from std::string successful\n";
    }

    PrintSubTest("Construction from Containers");
    {
        // Vector of chars
        std::vector<char> vec = {'V', 'e', 'c', 't', 'o', 'r'};
        ara::core::StringView sv1(vec);
        
        assert(sv1.size() == 6);
        assert(sv1[0] == 'V');
        assert(sv1[5] == 'r');
        
        // Array
        ara::core::Array<char, 5> arr = {'A', 'r', 'r', 'a', 'y'};
        ara::core::StringView sv2(arr);
        
        assert(sv2.size() == 5);
        assert(sv2[0] == 'A');
        
        std::cout << "Container construction successful\n";
    }

    PrintSubTest("Copy and Move Semantics");
    {
        const char* str = "Original";
        ara::core::StringView original(str);
        
        // Copy construction
        [[maybe_unused]] ara::core::StringView copied(original);
        assert(copied.data() == original.data());
        assert(copied.size() == original.size());
        
        // Move construction (trivial for string_view)
        ara::core::StringView moved(std::move(original));
        assert(moved.data() == str);
        assert(moved.size() == 8);
        
        // Assignment
        ara::core::StringView assigned;
        assigned = moved;
        assert(assigned.data() == str);
        
        std::cout << "Copy/move semantics verified\n";
    }

    PrintSubTest("Compile-time Construction");
    {
        static constexpr const char cstr[] = "Compile Time";
        constexpr ara::core::StringView sv1(cstr);
        constexpr ara::core::StringView sv2(cstr, 7); // "Compile"
        
        static_assert(sv1.size() == 12, "Constexpr size failed");
        static_assert(sv1[0] == 'C', "Constexpr element access failed");
        static_assert(sv2.size() == 7, "Constexpr size with count failed");
        
        std::cout << "Compile-time construction verified\n";
    }

#if __cplusplus > 201703L
    PrintSubTest("UTF-8/16/32 String Views");
    {
        const char8_t* u8str = u8"UTF-8 String";
        ara::core::U8StringView u8sv(u8str);
        assert(u8sv.size() == 12);
        
        const char16_t* u16str = u"UTF-16 String";
        ara::core::U16StringView u16sv(u16str);
        assert(u16sv.size() == 13);
        
        const char32_t* u32str = U"UTF-32 String";
        ara::core::U32StringView u32sv(u32str);
        assert(u32sv.size() == 13);
        
        std::cout << "Unicode string views verified\n";
    }
#endif
}

/*!
 * \brief Test #2: Element Access and Bounds Checking
 */
void TestElementAccess()
{
    PrintTestHeader("Element Access and Bounds Checking");

    PrintSubTest("Subscript Operator");
    {
        const char* str = "AUTOSAR";
        ara::core::StringView sv(str);
        
        std::cout << "Elements via operator[]: ";
        for (std::size_t i = 0; i < sv.size(); ++i) {
            std::cout << sv[i];
        }
        std::cout << "\n";
        
        assert(sv[0] == 'A');
        assert(sv[3] == 'O');
        assert(sv[6] == 'R');
        
        // Const correctness
        const ara::core::StringView csv(str);
        assert(csv[0] == 'A');
    }

    PrintSubTest("at() Method (Bounds Checked)");
    {
        const char* str = "Safety";
        ara::core::StringView sv(str);
        
        // Valid access
        assert(sv.at(0) == 'S');
        assert(sv.at(5) == 'y');
        
        std::cout << "at() method provides bounds checking\n";
        
        // Invalid access would terminate (tested separately)
    }

    PrintSubTest("Front and Back");
    {
        const char* str = "Boundaries";
        ara::core::StringView sv(str);
        
        assert(sv.front() == 'B');
        assert(sv.back() == 's');
        
        // Single character
        [[maybe_unused]] ara::core::StringView single("X", 1);
        assert(single.front() == 'X');
        assert(single.back() == 'X');
        
        std::cout << "front() = '" << sv.front() << "', back() = '" << sv.back() << "'\n";
    }

    PrintSubTest("Data Pointer Access");
    {
        const char* str = "Direct Access";
        ara::core::StringView sv(str);
        
        [[maybe_unused]] const char* ptr = sv.data();
        assert(ptr == str);
        assert(ptr[0] == 'D');
        assert(ptr[6] == ' ');
        
        // Empty view
        [[maybe_unused]] ara::core::StringView empty;
        assert(empty.data() == nullptr);
        
        std::cout << "data() pointer access verified\n";
    }

    PrintSubTest("Compile-time Element Access");
    {
        static constexpr const char str[] = "Constexpr";
        constexpr ara::core::StringView sv(str);
        
        static_assert(sv[0] == 'C', "Constexpr element access failed");
        static_assert(sv[8] == 'r', "Constexpr element access failed");
        static_assert(sv.front() == 'C', "Constexpr front() failed");
        static_assert(sv.back() == 'r', "Constexpr back() failed");
        static_assert(sv.at(4) == 't', "Constexpr at() failed");
        
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
        const char* str = "Iterator";
        [[maybe_unused]] ara::core::StringView sv(str);

        std::cout << "Forward iteration: ";
        for (auto it = sv.begin(); it != sv.end(); ++it) {
            std::cout << *it;
        }
        std::cout << "\n";
        
        // Iterator arithmetic
        [[maybe_unused]] auto it = sv.begin();
        assert(*it == 'I');
        assert(*(it + 2) == 'e');
        assert(*(sv.end() - 1) == 'r');
        assert(sv.end() - sv.begin() == static_cast<std::ptrdiff_t>(sv.size()));
        
        // Random access
        it += 3;
        assert(*it == 'r');
        it -= 1;
        assert(*it == 'e');
    }

    PrintSubTest("Const Iterators");
    {
        const char* str = "Const";
        ara::core::StringView sv(str);
        
        [[maybe_unused]] auto cit = sv.cbegin();
        assert(*cit == 'C');
        
        // Iterate with const iterators
        std::cout << "Const iteration: ";
        for (auto it = sv.cbegin(); it != sv.cend(); ++it) {
            std::cout << *it;
        }
        std::cout << "\n";
    }

    PrintSubTest("Reverse Iterators");
    {
        const char* str = "Reverse";
        ara::core::StringView sv(str);
        
        std::cout << "Reverse iteration: ";
        for (auto it = sv.rbegin(); it != sv.rend(); ++it) {
            std::cout << *it;
        }
        std::cout << "\n";
        
        assert(*sv.rbegin() == 'e');
        assert(*(sv.rend() - 1) == 'R');
        
        // Const reverse
        [[maybe_unused]] auto crit = sv.crbegin();
        assert(*crit == 'e');
    }

    PrintSubTest("Range-based For Loop");
    {
        const char* str = "Range Loop";
        ara::core::StringView sv(str);
        
        std::cout << "Range-based for: ";
        for (char ch : sv) {
            std::cout << ch;
        }
        std::cout << "\n";
        
        // Count specific character
        [[maybe_unused]] std::size_t count = 0;
        for (char ch : sv) {
            if (ch == 'o') ++count;
        }
        assert(count == 2); // Two 'o's in "Range Loop"
    }

    PrintSubTest("Iterator Traits");
    {
        using SV = ara::core::StringView;
        using It = SV::iterator;
        
        static_assert(std::is_same_v<
            std::iterator_traits<It>::iterator_category,
            std::random_access_iterator_tag
        >, "Must be random access iterator");
        
        static_assert(std::is_same_v<
            std::iterator_traits<It>::value_type,
            char
        >, "Value type must be char");
        
        std::cout << "Iterator traits verified\n";
    }

    PrintSubTest("Algorithm Compatibility");
    {
        const char* str = "Algorithm Test";
        ara::core::StringView sv(str);
        
        // Find
        [[maybe_unused]] auto it = std::find(sv.begin(), sv.end(), 'T');
        assert(it != sv.end());
        assert(*it == 'T');
        
        // Count
        [[maybe_unused]] auto count = std::count(sv.begin(), sv.end(), 't');
        assert(count == 2); // 't' appears twice
        
        // Any of
        const char* vowels = "aeiouAEIOU";
        [[maybe_unused]] auto vowel_it = std::find_first_of(sv.begin(), sv.end(), vowels, vowels + 10);
        assert(vowel_it != sv.end());
        assert(*vowel_it == 'A');
        
        std::cout << "STL algorithm compatibility verified\n";
    }
}

/*!
 * \brief Test #4: Capacity and Size Queries
 */
void TestCapacityAndSize()
{
    PrintTestHeader("Capacity and Size Queries");

    PrintSubTest("size() and length()");
    {
        const char* str = "Size Test";
        ara::core::StringView sv(str);
        
        assert(sv.size() == 9);
        assert(sv.length() == 9);
        assert(sv.size() == sv.length());
        
        // Empty
        [[maybe_unused]] ara::core::StringView empty;
        assert(empty.size() == 0);
        assert(empty.length() == 0);
        
        std::cout << "Size: " << sv.size() << ", Length: " << sv.length() << "\n";
    }

    PrintSubTest("empty()");
    {
        [[maybe_unused]] ara::core::StringView empty1;
        [[maybe_unused]] ara::core::StringView empty2("");
        [[maybe_unused]] ara::core::StringView empty3("", 0);

        assert(empty1.empty());
        assert(empty2.empty());
        assert(empty3.empty());

        [[maybe_unused]] ara::core::StringView nonempty("x");
        assert(!nonempty.empty());
        
        // After operations
        ara::core::StringView sv("test");
        sv.remove_prefix(4);
        assert(sv.empty());
        
        std::cout << "empty() checks verified\n";
    }

    PrintSubTest("max_size()");
    {
        ara::core::StringView sv;
        [[maybe_unused]] auto max = sv.max_size();
        
        std::cout << "max_size(): " << max << "\n";
        assert(max == std::numeric_limits<std::size_t>::max());
        
        // Same for all string views
        [[maybe_unused]] ara::core::WStringView wsv;
        assert(wsv.max_size() == std::numeric_limits<std::size_t>::max() / sizeof(wchar_t));
    }

    PrintSubTest("Compile-time Size Queries");
    {
        static constexpr const char str[] = "Compile";
        constexpr ara::core::StringView sv(str);
        
        static_assert(sv.size() == 7, "Constexpr size failed");
        static_assert(sv.length() == 7, "Constexpr length failed");
        static_assert(!sv.empty(), "Constexpr empty failed");
        
        constexpr ara::core::StringView empty;
        static_assert(empty.empty(), "Constexpr empty string failed");
        static_assert(empty.size() == 0, "Constexpr empty size failed");
        
        std::cout << "Compile-time size queries verified\n";
    }
}

/*!
 * \brief Test #5: String Operations
 */
void TestStringOperations()
{
    PrintTestHeader("String Operations");

    PrintSubTest("copy() Method");
    {
        const char* str = "Copy this string";
        ara::core::StringView sv(str);
        
        char buffer[20] = {0};
        [[maybe_unused]] auto copied = sv.copy(buffer, 4); // Copy "Copy"
        assert(copied == 4);
        assert(std::strncmp(buffer, "Copy", 4) == 0);
        
        // Copy with offset
        std::memset(buffer, 0, sizeof(buffer));
        copied = sv.copy(buffer, 6, 5); // Copy "this s"
        assert(copied == 6);
        assert(std::strncmp(buffer, "this s", 6) == 0);
        
        // Copy more than available
        copied = sv.copy(buffer, 100, 10);
        assert(copied == 6); // Only "string" available
        
        std::cout << "copy() method verified\n";
    }

    PrintSubTest("substr() Method");
    {
        const char* str = "Substring operations";
        ara::core::StringView sv(str);
        
        // Basic substring
        [[maybe_unused]] auto sub1 = sv.substr(0, 9);
        assert(sub1.size() == 9);
        assert(sub1 == "Substring");
        
        // From middle
        [[maybe_unused]] auto sub2 = sv.substr(10);
        assert(sub2 == "operations");
        
        // Single character
        [[maybe_unused]] auto sub3 = sv.substr(5, 1);
        assert(sub3.size() == 1);
        assert(sub3[0] == 'r');
        
        // Clamp to end
        [[maybe_unused]] auto sub4 = sv.substr(15, 100);
        assert(sub4.size() == 5);
        assert(sub4 == "tions");
        
        std::cout << "substr() operations verified\n";
    }

    PrintSubTest("compare() Method");
    {
        [[maybe_unused]] ara::core::StringView sv1("ABC");
        [[maybe_unused]] ara::core::StringView sv2("ABC");
        [[maybe_unused]] ara::core::StringView sv3("ABD");
        [[maybe_unused]] ara::core::StringView sv4("AB");
        
        // Equal strings
        assert(sv1.compare(sv2) == 0);
        
        // Less than
        assert(sv1.compare(sv3) < 0);
        
        // Greater than
        assert(sv3.compare(sv1) > 0);
        
        // Different lengths
        assert(sv1.compare(sv4) > 0);
        assert(sv4.compare(sv1) < 0);
        
        // With positions and counts
        [[maybe_unused]] ara::core::StringView sv5("Hello World");
        assert(sv5.compare(6, 5, "World") == 0);
        assert(sv5.compare(0, 5, "Hello") == 0);
        
        // Compare substrings
        [[maybe_unused]] ara::core::StringView sv6("Testing");
        [[maybe_unused]] ara::core::StringView sv7("Testing123");
        assert(sv6.compare(0, 4, sv7, 0, 4) == 0); // "Test" == "Test"
        
        std::cout << "compare() operations verified\n";
    }

    PrintSubTest("Compile-time String Operations");
    {
        static constexpr const char str[] = "Constexpr String";
        constexpr ara::core::StringView sv(str);
        
        // substr
        constexpr auto sub = sv.substr(10, 6);
        static_assert(sub.size() == 6, "Constexpr substr size failed");
        static_assert(sub == "String", "Constexpr substr failed");
        
        // compare
        static_assert(sv.compare("Constexpr String") == 0, "Constexpr compare failed");
        static_assert(sv.compare(0, 9, "Constexpr") == 0, "Constexpr compare partial failed");
        
        std::cout << "Compile-time string operations verified\n";
    }
}

/*!
 * \brief Test #6: Searching Operations
 */
void TestSearchingOperations()
{
    PrintTestHeader("Searching Operations");

    PrintSubTest("find() Method");
    {
        const char* str = "Find in this string to find again";
        ara::core::StringView sv(str);

        // Find character
        [[maybe_unused]] auto pos1 = sv.find('i');
        assert(pos1 == 1); // First 'i' at position 1

        // Find from position
        [[maybe_unused]] auto pos2 = sv.find('i', pos1 + 1);
        assert(pos2 == 5); // Next 'i' at position 5

        // Find string
        [[maybe_unused]] auto pos3 = sv.find("find");
        assert(pos3 == 23); // "find" at position 23

        // Find with count
        [[maybe_unused]] auto pos4 = sv.find("string", 0, 3); // Find "str"
        assert(pos4 == 13);

        // Not found
        [[maybe_unused]] auto pos5 = sv.find('z');
        assert(pos5 == ara::core::StringView::npos);

        [[maybe_unused]] auto pos6 = sv.find("xyz");
        assert(pos6 == ara::core::StringView::npos);

        std::cout << "find() operations verified\n";
    }

    PrintSubTest("rfind() Method");
    {
        const char* str = "Last find should find last";
        ara::core::StringView sv(str);

        // Find from end
        [[maybe_unused]] auto pos1 = sv.rfind("find");
        assert(pos1 == 17); // Last "find" (index 17, zero-based)

        // Find character from end
        [[maybe_unused]] auto pos2 = sv.rfind('a');
        assert(pos2 == 23); // Last 'a'

        // Limited search
        [[maybe_unused]] auto pos3 = sv.rfind('f', 10);
        assert(pos3 == 5); // 'f' at or before position 10

        // Not found
        [[maybe_unused]] auto pos4 = sv.rfind('z');
        assert(pos4 == ara::core::StringView::npos);

        std::cout << "rfind() operations verified\n";
    }

    PrintSubTest("find_first_of() Method");
    {
        const char* str = "Find any of these vowels";
        ara::core::StringView sv(str);
        
        // Find first vowel
        [[maybe_unused]] auto pos1 = sv.find_first_of("aeiou");
        assert(pos1 == 1); // 'i' in "Find"
        
        // From position
        [[maybe_unused]] auto pos2 = sv.find_first_of("aeiou", pos1 + 1);
        assert(pos2 == 5); // 'a' in "any"
        
        // Single character
        [[maybe_unused]] auto pos3 = sv.find_first_of('e');
        assert(pos3 == 14); // 'e' in "these"
        
        // Not found
        [[maybe_unused]] auto pos4 = sv.find_first_of("xyz");
        assert(pos4 == 7);
        
        std::cout << "find_first_of() operations verified\n";
    }

    PrintSubTest("find_last_of() Method");
    {
        const char* str = "Find last occurrence of any";
        ara::core::StringView sv(str);
        
        // Find last vowel
        [[maybe_unused]] auto pos1 = sv.find_last_of("aeiou");
        assert(pos1 == 24); // 'a' in "any"
        
        // Limited search
        [[maybe_unused]] auto pos2 = sv.find_last_of("aeiou", 20);
        assert(pos2 == 19); // 'o' in "of"
        
        std::cout << "find_last_of() operations verified\n";
    }

    PrintSubTest("find_first_not_of() Method");
    {
        const char* str = "   Find first non-space";
        ara::core::StringView sv(str);
        
        // Skip spaces
        [[maybe_unused]] auto pos1 = sv.find_first_not_of(' ');
        assert(pos1 == 3); // 'F'
        
        // Skip multiple characters
        [[maybe_unused]] auto pos2 = sv.find_first_not_of(" Fin");
        assert(pos2 == 6); // 'd' in "Find"
        
        // All match
        ara::core::StringView sv2("aaaa");
        [[maybe_unused]] auto pos3 = sv2.find_first_not_of('a');
        assert(pos3 == ara::core::StringView::npos);
        
        std::cout << "find_first_not_of() operations verified\n";
    }

    PrintSubTest("find_last_not_of() Method");
    {
        const char* str = "Trim spaces   ";
        ara::core::StringView sv(str);
        
        // Find last non-space
        [[maybe_unused]] auto pos1 = sv.find_last_not_of(' ');
        assert(pos1 == 10); // 's' in "spaces"
        
        // Multiple characters
        [[maybe_unused]] auto pos2 = sv.find_last_not_of(" s");
        assert(pos2 == 9); // 'e' in "spaces"
        
        std::cout << "find_last_not_of() operations verified\n";
    }

    PrintSubTest("Compile-time Searching");
    {
        static constexpr const char str[] = "Compile time search";
        constexpr ara::core::StringView sv(str);
        
        // find
        static_assert(sv.find('t') == 8, "Constexpr find char failed");
        static_assert(sv.find("time") == 8, "Constexpr find string failed");
        
        // rfind
        static_assert(sv.rfind('e') == 14, "Constexpr rfind failed");
        
        // find_first_of
        static_assert(sv.find_first_of("aeiou") == 1, "Constexpr find_first_of failed");
        
        // find_last_of
        static_assert(sv.find_last_of("aeiou") == 15, "Constexpr find_last_of failed");
        
        std::cout << "Compile-time searching verified\n";
    }
}

/*!
 * \brief Test #7: Modifiers
 */
void TestModifiers()
{
    PrintTestHeader("Modifiers");

    PrintSubTest("remove_prefix()");
    {
        const char* str = "Remove prefix test";
        ara::core::StringView sv(str);
        
        sv.remove_prefix(7); // Remove "Remove "
        assert(sv == "prefix test");
        assert(sv.size() == 11);
        
        sv.remove_prefix(7); // Remove "prefix "
        assert(sv == "test");
        assert(sv.size() == 4);
        
        // Remove all
        sv.remove_prefix(4);
        assert(sv.empty());
        
        std::cout << "remove_prefix() verified\n";
    }

    PrintSubTest("remove_suffix()");
    {
        const char* str = "Remove suffix test";
        ara::core::StringView sv(str);
        
        sv.remove_suffix(5); // Remove " test"
        assert(sv == "Remove suffix");
        assert(sv.size() == 13);
        
        sv.remove_suffix(7); // Remove " suffix"
        assert(sv == "Remove");
        assert(sv.size() == 6);
        
        // Remove all
        sv.remove_suffix(6);
        assert(sv.empty());
        
        std::cout << "remove_suffix() verified\n";
    }

    PrintSubTest("swap()");
    {
        const char* str1 = "First";
        const char* str2 = "Second String";
        
        ara::core::StringView sv1(str1);
        ara::core::StringView sv2(str2);
        
        [[maybe_unused]] auto ptr1 = sv1.data();
        [[maybe_unused]] auto size1 = sv1.size();
        [[maybe_unused]] auto ptr2 = sv2.data();
        [[maybe_unused]] auto size2 = sv2.size();
        
        sv1.swap(sv2);
        
        assert(sv1.data() == ptr2);
        assert(sv1.size() == size2);
        assert(sv2.data() == ptr1);
        assert(sv2.size() == size1);
        
        // Non-member swap
        ara::core::swap(sv1, sv2);
        assert(sv1.data() == ptr1);
        assert(sv2.data() == ptr2);
        
        std::cout << "swap() operations verified\n";
    }

    PrintSubTest("Chained Modifiers");
    {
        const char* str = "   Trim whitespace   ";
        ara::core::StringView sv(str);
        
        // Find first non-space
        [[maybe_unused]] auto start = sv.find_first_not_of(' ');
        if (start != ara::core::StringView::npos) {
            sv.remove_prefix(start);
        }
        
        // Find last non-space
        [[maybe_unused]] auto end = sv.find_last_not_of(' ');
        if (end != ara::core::StringView::npos) {
            sv.remove_suffix(sv.size() - end - 1);
        }
        
        assert(sv == "Trim whitespace");
        
        std::cout << "Chained modifiers for trimming verified\n";
    }
}

/*!
 * \brief Test #8: C++26 Features
 */
void TestCpp26Features()
{
    PrintTestHeader("C++26 Features (Backported)");

    PrintSubTest("starts_with() Method");
    {
        const char* str = "AUTOSAR Adaptive Platform";
        ara::core::StringView sv(str);
        
        // String view
        assert(sv.starts_with(ara::core::StringView("AUTO")));
        assert(sv.starts_with(ara::core::StringView("AUTOSAR")));
        assert(!sv.starts_with(ara::core::StringView("AUTO SAR")));
        assert(!sv.starts_with(ara::core::StringView("Adaptive")));
        
        // Character
        assert(sv.starts_with('A'));
        assert(!sv.starts_with('B'));
        
        // C-string
        assert(sv.starts_with("AUTOSAR"));
        assert(!sv.starts_with("Platform"));
        
        // Empty prefix
        assert(sv.starts_with(ara::core::StringView()));
        assert(sv.starts_with(""));
        
        // Longer prefix
        assert(!sv.starts_with("AUTOSAR Adaptive Platform Extended"));
        
        std::cout << "starts_with() verified\n";
    }

    PrintSubTest("ends_with() Method");
    {
        const char* str = "AUTOSAR Adaptive Platform";
        ara::core::StringView sv(str);
        
        // String view
        assert(sv.ends_with(ara::core::StringView("Platform")));
        assert(sv.ends_with(ara::core::StringView("tform")));
        assert(!sv.ends_with(ara::core::StringView("platform"))); // Case sensitive
        
        // Character
        assert(sv.ends_with('m'));
        assert(!sv.ends_with('M'));
        
        // C-string
        assert(sv.ends_with("Platform"));
        assert(!sv.ends_with("AUTOSAR"));
        
        // Empty suffix
        assert(sv.ends_with(ara::core::StringView()));
        assert(sv.ends_with(""));
        
        std::cout << "ends_with() verified\n";
    }

    PrintSubTest("contains() Method");
    {
        const char* str = "Find substring in this text";
        ara::core::StringView sv(str);
        
        // String view
        assert(sv.contains(ara::core::StringView("substring")));
        assert(sv.contains(ara::core::StringView("this")));
        assert(!sv.contains(ara::core::StringView("missing")));
        
        // Character
        assert(sv.contains('s'));
        assert(!sv.contains('z'));
        
        // C-string
        assert(sv.contains("text"));
        assert(!sv.contains("TEXT")); // Case sensitive
        
        // Empty
        assert(sv.contains(ara::core::StringView()));
        assert(sv.contains(""));
        
        // At boundaries
        assert(sv.contains("Find"));
        assert(sv.contains("text"));
        
        std::cout << "contains() verified\n";
    }

    PrintSubTest("Compile-time C++26 Features");
    {
        static constexpr const char str[] = "Compile time C++26";
        constexpr ara::core::StringView sv(str);
        
        // starts_with
        static_assert(sv.starts_with("Compile"), "Constexpr starts_with failed");
        static_assert(sv.starts_with('C'), "Constexpr starts_with char failed");
        static_assert(!sv.starts_with("compile"), "Constexpr starts_with case failed");
        
        // ends_with
        static_assert(sv.ends_with("++26"), "Constexpr ends_with failed");
        static_assert(sv.ends_with('6'), "Constexpr ends_with char failed");
        static_assert(!sv.ends_with("++20"), "Constexpr ends_with failed");
        
        // contains
        static_assert(sv.contains("time"), "Constexpr contains failed");
        static_assert(sv.contains(' '), "Constexpr contains char failed");
        static_assert(!sv.contains("C++20"), "Constexpr contains failed");
        
        std::cout << "Compile-time C++26 features verified\n";
    }
}

/*!
 * \brief Test #9: Comparison Operations
 */
void TestComparisons()
{
    PrintTestHeader("Comparison Operations");

    PrintSubTest("Equality Comparisons");
    {
        [[maybe_unused]] ara::core::StringView sv1("Hello");
        [[maybe_unused]] ara::core::StringView sv2("Hello");
        [[maybe_unused]] ara::core::StringView sv3("World");
        [[maybe_unused]] ara::core::StringView sv4("Hell");

        // Equal
        assert(sv1 == sv2);
        assert(!(sv1 != sv2));
        
        // Not equal
        assert(sv1 != sv3);
        assert(!(sv1 == sv3));
        assert(sv1 != sv4);
        
        // With C-strings
        assert(sv1 == "Hello");
        assert("Hello" == sv1);
        assert(sv1 != "World");
        assert("World" != sv1);
        
        // Empty
        [[maybe_unused]] ara::core::StringView empty1;
        [[maybe_unused]] ara::core::StringView empty2;
        assert(empty1 == empty2);
        assert(empty1 == "");
        assert("" == empty1);
        
        std::cout << "Equality comparisons verified\n";
    }

    PrintSubTest("Ordering Comparisons");
    {
        [[maybe_unused]] ara::core::StringView sv1("ABC");
        [[maybe_unused]] ara::core::StringView sv2("ABD");
        [[maybe_unused]] ara::core::StringView sv3("AB");
        [[maybe_unused]] ara::core::StringView sv4("ABC");

        // Less than
        assert(sv1 < sv2);   // "ABC" < "ABD"
        assert(sv3 < sv1);   // "AB" < "ABC"
        assert(!(sv1 < sv4)); // Equal
        assert(!(sv2 < sv1));
        
        // Less than or equal
        assert(sv1 <= sv2);
        assert(sv1 <= sv4);  // Equal
        assert(sv3 <= sv1);
        
        // Greater than
        assert(sv2 > sv1);   // "ABD" > "ABC"
        assert(sv1 > sv3);   // "ABC" > "AB"
        assert(!(sv1 > sv4)); // Equal
        
        // Greater than or equal
        assert(sv2 >= sv1);
        assert(sv1 >= sv4);  // Equal
        assert(sv1 >= sv3);
        
        // With C-strings
        assert(sv1 < "ABD");
        assert("AB" < sv1);
        assert(sv1 <= "ABC");
        assert("ABC" <= sv1);
        assert(sv1 > "AB");
        assert("ABD" > sv1);
        assert(sv1 >= "ABC");
        assert("ABC" >= sv1);
        
        std::cout << "Ordering comparisons verified\n";
    }

    PrintSubTest("Case Sensitivity");
    {
        [[maybe_unused]] ara::core::StringView sv1("Hello");
        [[maybe_unused]] ara::core::StringView sv2("hello");
        
        // Case sensitive
        assert(sv1 != sv2);
        assert(sv1 < sv2); // 'H' < 'h' in ASCII
        
        std::cout << "Case sensitive comparisons verified\n";
    }

    PrintSubTest("Compile-time Comparisons");
    {
        static constexpr ara::core::StringView sv1("ABC");
        static constexpr ara::core::StringView sv2("ABD");
        static constexpr ara::core::StringView sv3("ABC");
        
        static_assert(sv1 == sv3, "Constexpr equality failed");
        static_assert(sv1 != sv2, "Constexpr inequality failed");
        static_assert(sv1 < sv2, "Constexpr less than failed");
        static_assert(sv1 <= sv3, "Constexpr less equal failed");
        static_assert(sv2 > sv1, "Constexpr greater than failed");
        static_assert(sv1 >= sv3, "Constexpr greater equal failed");
        
        // With string literals
        static_assert(sv1 == "ABC", "Constexpr literal equality failed");
        static_assert(sv1 < "ABD", "Constexpr literal comparison failed");
        
        std::cout << "Compile-time comparisons verified\n";
    }
}

/*!
 * \brief Test #10: Stream Operations
 */
void TestStreamOperations()
{
    PrintTestHeader("Stream Operations");

    PrintSubTest("Output Stream Operator");
    {
        const char* str = "Stream Output Test";
        ara::core::StringView sv(str);
        
        std::ostringstream oss;
        oss << sv;
        assert(oss.str() == str);
        
        // Multiple outputs
        ara::core::StringView sv1("Hello");
        ara::core::StringView sv2(" ");
        ara::core::StringView sv3("World");
        
        std::ostringstream oss2;
        oss2 << sv1 << sv2 << sv3;
        assert(oss2.str() == "Hello World");
        
        // Empty string view
        ara::core::StringView empty;
        std::ostringstream oss3;
        oss3 << "Before" << empty << "After";
        assert(oss3.str() == "BeforeAfter");
        
        std::cout << "Stream output: \"" << sv << "\"\n";
    }

    PrintSubTest("Wide Stream Support");
    {   

        #if defined(__QNXNTO__) && (__cplusplus >= 202002L)
        #  define HAS_QNX_WIDE_STREAM_ISSUE 1
        #else
        #  define HAS_QNX_WIDE_STREAM_ISSUE 0
        #endif

        constexpr bool has_qnx_wide_stream_issue = HAS_QNX_WIDE_STREAM_ISSUE;

        if constexpr (has_qnx_wide_stream_issue) {
            std::cout
                << "(wide-stream tests skipped due to unknown QNX libc++ issue: "
                   "-Walloc-size-larger-than is triggered)\n";
        } else {
            constexpr const wchar_t* kExpected = L"Wide Stream";
            ara::core::WStringView     wsv{kExpected};

            std::wostringstream woss;
            woss << wsv;

            assert((woss.str() == kExpected) &&
                   "WStringView did not round-trip through std::wostringstream");

            std::cout << "Wide stream support verified\n";
        }

    }
    
    PrintSubTest("Stream Manipulators");
    {
        const char* str = "Format";
        ara::core::StringView sv(str);
        
        std::ostringstream oss;
        oss << std::setw(10) << std::left << sv;
        assert(oss.str() == "Format    ");
        
        std::ostringstream oss2;
        oss2 << std::setw(10) << std::right << sv;
        assert(oss2.str() == "    Format");
        
        std::cout << "Stream manipulators work correctly\n";
    }
}

/*!
 * \brief Test #11: User-defined Literals
 */
void TestUserDefinedLiterals()
{
    PrintTestHeader("User-defined Literals");

    using namespace ara::core::literals::string_view_literals;

    PrintSubTest("Basic String View Literals");
    {
        [[maybe_unused]] auto sv1 = "Hello World"_sv;
        static_assert(std::is_same_v<decltype(sv1), ara::core::StringView>,
                      "Should deduce StringView type");
        
        assert(sv1.size() == 11);
        assert(sv1 == "Hello World");
        
        // Empty literal
        [[maybe_unused]] auto sv2 = ""_sv;
        assert(sv2.empty());
        
        // Use in expressions
        assert("ABC"_sv < "ABD"_sv);
        assert("Test"_sv.starts_with('T'));
        
        std::cout << "Basic string view literal: \"" << sv1 << "\"\n";
    }

    PrintSubTest("Wide String View Literals");
    {
        [[maybe_unused]] auto wsv = L"Wide String"_sv;
        static_assert(std::is_same_v<decltype(wsv), ara::core::WStringView>,
                      "Should deduce WStringView type");
        
        assert(wsv.size() == 11);
        assert(wsv[0] == L'W');
        
        std::cout << "Wide string view literals verified\n";
    }

#if __cplusplus > 201703L
    PrintSubTest("UTF String View Literals");
    {
        [[maybe_unused]] auto u8sv = u8"UTF-8"_sv;
        static_assert(std::is_same_v<decltype(u8sv), ara::core::U8StringView>,
                      "Should deduce U8StringView type");
        assert(u8sv.size() == 5);
        
        [[maybe_unused]] auto u16sv = u"UTF-16"_sv;
        static_assert(std::is_same_v<decltype(u16sv), ara::core::U16StringView>,
                      "Should deduce U16StringView type");
        assert(u16sv.size() == 6);
        
        [[maybe_unused]] auto u32sv = U"UTF-32"_sv;
        static_assert(std::is_same_v<decltype(u32sv), ara::core::U32StringView>,
                      "Should deduce U32StringView type");
        assert(u32sv.size() == 6);
        
        std::cout << "UTF string view literals verified\n";
    }
#endif

    PrintSubTest("Compile-time Literals");
    {
        constexpr auto sv = "Compile Time"_sv;
        static_assert(sv.size() == 12, "Constexpr literal size failed");
        static_assert(sv[0] == 'C', "Constexpr literal access failed");
        static_assert(sv.contains("Time"), "Constexpr literal operation failed");
        
        std::cout << "Compile-time literals verified\n";
    }

    PrintSubTest("Literal in Functions");
    {
        [[maybe_unused]] auto process = [](ara::core::StringView sv) {
            return sv.size();
        };
        
        assert(process("Direct literal"_sv) == 14);
        
        // Temporary binding
        [[maybe_unused]] const auto& ref = "Temporary"_sv;
        assert(ref.size() == 9);
        
        std::cout << "Literals in function calls verified\n";
    }
}

/*!
 * \brief Test #12: Factory Functions
 */
namespace {
    template<typename T>
    using IsCharView = std::bool_constant<
        std::is_same_v<T, ara::core::BasicStringView<char>> ||
        std::is_same_v<T, ara::core::BasicStringView<const char>>
    >;
}

void TestFactoryFunctions()
{
    PrintTestHeader("Factory Functions (MakeStringView)");

    // 1) pointer + size --------------------------------------------------------
    PrintSubTest("MakeStringView from Pointer and Size");
    {
        const char* str = "Factory Function";
        [[maybe_unused]] auto sv  = ara::core::MakeStringView(str, 7);   // "Factory"
        const wchar_t* wstr = L"Wide Factory";
        [[maybe_unused]] auto wsv = ara::core::MakeStringView(wstr, 4);  // L"Wide"

        static_assert(IsCharView<decltype(sv)>::value,      "Must be char-based view");
        static_assert(std::is_same_v<decltype(wsv), ara::core::WStringView>,
                      "Must be wide-char view");

        assert(sv.size() == 7 && sv == "Factory");
        assert(wsv.size() == 4);
    }

    // 2) plain C-string --------------------------------------------------------
    PrintSubTest("MakeStringView from C-string");
    {
        const char* cstr = "C String Factory";
        [[maybe_unused]] auto sv = ara::core::MakeStringView(cstr);
        static_assert(IsCharView<decltype(sv)>::value, "char-based view expected");
        assert(sv.size() == std::strlen(cstr));

        char mutable_str[] = "Mutable";
        [[maybe_unused]] auto msv = ara::core::MakeStringView(mutable_str);
        static_assert(IsCharView<decltype(msv)>::value, "char-based view expected");
    }

    // 3) STL / custom containers ---------------------------------------------
    PrintSubTest("MakeStringView from Containers");
    {
        std::string str = "String Container";
        [[maybe_unused]] auto sv1 = ara::core::MakeStringView(str);
        const std::string cstr2 = "Const String";
        [[maybe_unused]] auto sv2 = ara::core::MakeStringView(cstr2);

        std::vector<char> vec = {'V','e','c'};
        [[maybe_unused]] auto sv3 = ara::core::MakeStringView(vec);

        ara::core::Array<char,5> arr = {'A','r','r','a','y'};
        [[maybe_unused]] auto sv4 = ara::core::MakeStringView(arr);

        static_assert(IsCharView<decltype(sv1)>::value, "std::string → char view");
        static_assert(IsCharView<decltype(sv2)>::value, "const std::string → char view");
        static_assert(IsCharView<decltype(sv3)>::value, "std::vector<char> → char view");
        static_assert(IsCharView<decltype(sv4)>::value, "ara::core::Array<char> → char view");

        assert(sv3.size() == 3);
        assert(sv4.size() == 5);
    }

    // 4) generic deduction sanity --------------------------------------------
    PrintSubTest("Type Deduction");
    {
        char  arr[]  = "Mutable";
        const char carr[] = "Const";

        [[maybe_unused]] auto sv1 = ara::core::MakeStringView(arr);
        [[maybe_unused]] auto sv2 = ara::core::MakeStringView(carr);
        [[maybe_unused]] auto sv3 = ara::core::MakeStringView("Literal");

        static_assert(IsCharView<decltype(sv1)>::value, "mutable char[] → char view");
        static_assert(IsCharView<decltype(sv2)>::value, "const char[]  → char view");
        static_assert(IsCharView<decltype(sv3)>::value, "string-literal → char view");
    }

    std::cout << "All MakeStringView factory-function tests passed\n";
}
/*!
 * \brief Test #13: Ranges Support
 */
void TestRangesSupport()
{
    PrintTestHeader("Ranges Support");

#if __cplusplus >= 202002L
    PrintSubTest("C++20 std::ranges Integration");
    {
        const char* str = "Range Operations";
        ara::core::StringView sv(str);

        /* ------------------------------------------------------------------
         *   Vowel counter
         * ------------------------------------------------------------------*/
        auto is_vowel /* predicate */ = [](char c) /*-> bool*/ {
            return c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u'
                || c == 'A' || c == 'E' || c == 'I' || c == 'O' || c == 'U';
        };

        [[maybe_unused]] auto vowel_count = std::ranges::count_if(sv, is_vowel);
        assert(vowel_count == 7);                /*  ←  was 6, real count is 7 */

        /* ------------------------------------------------------------------
         *   Upper-case transformation (avoid UB: cast to unsigned char)
         * ------------------------------------------------------------------*/
        std::string upper;
        std::ranges::transform(
            sv,
            std::back_inserter(upper),
            [](char c) /*-> char*/ { return static_cast<char>(std::toupper(static_cast<unsigned char>(c))); }
        );
        assert(upper == "RANGE OPERATIONS");     /* unchanged */
        
        std::cout << "C++20 ranges integration verified\n";
    }
    
    PrintSubTest("StringView as Borrowed Range");
    {
        static_assert(std::ranges::borrowed_range<ara::core::StringView>,
                      "StringView should be a borrowed range");
        
        // This is safe because StringView is borrowed
        auto get_sv = []() -> ara::core::StringView {
            std::string local = "Temporary";
            return ara::core::StringView(local); // Intentionally dangling
        };
        
        // Would be std::ranges::dangling if not borrowed
        [[maybe_unused]] auto result = std::ranges::find(get_sv(), 'T');
        
        std::cout << "Borrowed range concept verified\n";
    }
#endif

    PrintSubTest("Custom Ranges Support");
    {
        using namespace ara::core::literals::string_view_literals;
        
        const char* str = "Custom Range Support";
        ara::core::StringView sv(str);
        
        // Split by space
        auto parts = ara::core::ranges::split(sv, ' ');
        std::vector<ara::core::StringView> words;
        for (auto part : parts) { words.push_back(part); }
        assert(words.size() == 3);

        assert(words[0] == "Custom");
        assert(words[1] == "Range");
        assert(words[2] == "Support");
        
        // Transform to uppercase
        auto to_upper = [](char c) -> char { 
            return static_cast<char>(std::toupper(static_cast<unsigned char>(c))); 
        };
        auto upper_view = ara::core::ranges::transform(sv, to_upper);
        std::string upper_str;
        for (char c : upper_view) {
            upper_str += c;
        }
        assert(upper_str == "CUSTOM RANGE SUPPORT");
        
        // Filter vowels
        auto is_vowel = [](char c) {
            return c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u' ||
                   c == 'A' || c == 'E' || c == 'I' || c == 'O' || c == 'U';
        };
        auto vowels = ara::core::ranges::filter(sv, is_vowel);
        std::string vowel_str;
        for (char c : vowels) {
            vowel_str += c;
        }
        assert(vowel_str == "uoaeuo");
        
        std::cout << "Custom ranges support verified\n";
    }

    PrintSubTest("Ranges Utilities");
    {
        // Trim whitespace
        [[maybe_unused]] auto sv = ara::core::ranges::trim("  Trimmed  "_sv);
        assert(sv == "Trimmed");
        
        // Take/Drop
        ara::core::StringView sv2("Take and Drop");
        [[maybe_unused]] auto taken = ara::core::ranges::take(sv2, 4);
        assert(taken == "Take");
        
        [[maybe_unused]] auto dropped = ara::core::ranges::drop(sv2, 9);
        assert(dropped == "Drop");
        
        // Take/Drop while
        [[maybe_unused]] auto digits = ara::core::ranges::take_while("123abc"_sv, 
            [](char c) { return std::isdigit(static_cast<unsigned char>(c)); });
        assert(digits == "123");
        
        [[maybe_unused]] auto no_digits = ara::core::ranges::drop_while("123abc"_sv,
            [](char c) { return std::isdigit(static_cast<unsigned char>(c)); });
        assert(no_digits == "abc");
        
        std::cout << "Ranges utilities verified\n";
    }
}

/*!
 * \brief Test #14: Hash Support
 */
void TestHashSupport()
{
    PrintTestHeader("Hash Support");

    PrintSubTest("Basic Hash Operations");
    {
        ara::core::StringView sv1("Hello");
        ara::core::StringView sv2("Hello");
        ara::core::StringView sv3("World");
        
        std::hash<ara::core::StringView> hasher;
        
        [[maybe_unused]] auto h1 = hasher(sv1);
        [[maybe_unused]] auto h2 = hasher(sv2);
        [[maybe_unused]] auto h3 = hasher(sv3);
        
        // Equal strings should have equal hashes
        assert(h1 == h2);
        
        // Different strings should (likely) have different hashes
        assert(h1 != h3);
        
        std::cout << "Hash of \"Hello\": " << h1 << "\n";
        std::cout << "Hash of \"World\": " << h3 << "\n";
    }

    PrintSubTest("Hash Map Usage");
    {
        std::unordered_map<ara::core::StringView, int> map;
        
        map["one"] = 1;
        map["two"] = 2;
        map["three"] = 3;
        
        assert(map["one"] == 1);
        assert(map["two"] == 2);
        assert(map["three"] == 3);
        
        // Lookup with different but equal string view
        const char* str = "two";
        ara::core::StringView sv(str);
        assert(map[sv] == 2);
        
        // Count occurrences
        assert(map.count("one") == 1);
        assert(map.count("four") == 0);
        
        std::cout << "StringView as hash map key verified\n";
    }

    PrintSubTest("Hash Quality");
    {
        // Test that similar strings have different hashes
        std::vector<ara::core::StringView> similar = {
            "test", "Test", "tEst", "teSt", "tesT",
            "tests", "testing", "tested"
        };
        
        std::hash<ara::core::StringView> hasher;
        std::unordered_set<std::size_t> hashes;
        
        for (const auto& sv : similar) {
            hashes.insert(hasher(sv));
        }
        
        // All should have different hashes (high probability)
        assert(hashes.size() == similar.size());
        
        std::cout << "Hash quality verified (no collisions in similar strings)\n";
    }

    PrintSubTest("Wide String Hash");
    {
        [[maybe_unused]] ara::core::WStringView wsv1(L"Wide Hash");
        [[maybe_unused]] ara::core::WStringView wsv2(L"Wide Hash");
        [[maybe_unused]] ara::core::WStringView wsv3(L"Different");
        
        [[maybe_unused]] std::hash<ara::core::WStringView> whasher;
        
        assert(whasher(wsv1) == whasher(wsv2));
        assert(whasher(wsv1) != whasher(wsv3));
        
        std::cout << "Wide string view hash verified\n";
    }
}

/*!
 * \brief Test #15: Type Traits
 */
void TestTypeTraits()
{
    PrintTestHeader("Type Traits and Properties");

    PrintSubTest("Basic Type Properties");
    {
        using SV = ara::core::StringView;
        using WSV = ara::core::WStringView;
        
        // Size
        std::cout << "sizeof(StringView): " << sizeof(SV) << " bytes\n";
        std::cout << "sizeof(WStringView): " << sizeof(WSV) << " bytes\n";
        
        assert(sizeof(SV) == sizeof(const char*) + sizeof(std::size_t));
        assert(sizeof(WSV) == sizeof(const wchar_t*) + sizeof(std::size_t));
        
        // Trivial type
        static_assert(std::is_trivially_copyable_v<SV>, 
                      "StringView must be trivially copyable");
        static_assert(std::is_trivially_destructible_v<SV>,
                      "StringView must be trivially destructible");
        static_assert(std::is_standard_layout_v<SV>,
                      "StringView must have standard layout");
        
        std::cout << "StringView is a trivial, standard-layout type\n";
    }

    PrintSubTest("Member Type Aliases");
    {
        using SV = ara::core::StringView;
        
        static_assert(std::is_same_v<SV::value_type, char>,
                      "value_type mismatch");
        static_assert(std::is_same_v<SV::pointer, char*>,
                      "pointer mismatch");
        static_assert(std::is_same_v<SV::const_pointer, const char*>,
                      "const_pointer mismatch");
        static_assert(std::is_same_v<SV::reference, char&>,
                      "reference mismatch");
        static_assert(std::is_same_v<SV::const_reference, const char&>,
                      "const_reference mismatch");
        static_assert(std::is_same_v<SV::size_type, std::size_t>,
                      "size_type mismatch");
        static_assert(std::is_same_v<SV::difference_type, std::ptrdiff_t>,
                      "difference_type mismatch");
        
        std::cout << "Member type aliases verified\n";
    }

    PrintSubTest("Iterator Properties");
    {
        using SV = ara::core::StringView;
        using It = SV::iterator;
        using CIt = SV::const_iterator;
        
        // Iterator types should be pointers
        static_assert(std::is_same_v<It, const char*>,
                      "iterator must be const char*");
        static_assert(std::is_same_v<CIt, const char*>,
                      "const_iterator must be const char*");
        
        // Iterator category
        static_assert(std::is_same_v<
            std::iterator_traits<It>::iterator_category,
            std::random_access_iterator_tag
        >, "Must be random access iterator");
        
        std::cout << "Iterator properties verified\n";
    }

    PrintSubTest("Deduction Guides");
    {
        // From array
        const char arr[] = "Array";
        ara::core::StringView sv1(arr);
        static_assert(std::is_same_v<
                          decltype(sv1),
                          ara::core::StringView>,
                      "Should deduce StringView from array");
        
        // From pointer
        const char* ptr = "Pointer";
        ara::core::BasicStringView sv2(ptr);
        static_assert(std::is_same_v<decltype(sv2), ara::core::StringView>,
                      "Should deduce StringView from pointer");
        
        // From string
        std::string str = "String";
        ara::core::BasicStringView sv3(str);
        static_assert(std::is_same_v<decltype(sv3), ara::core::StringView>,
                      "Should deduce StringView from string");
        
        std::cout << "Deduction guides verified\n";
    }

    PrintSubTest("Nullptr Construction Deleted");
    {
        // This verifies that nullptr constructor is deleted
        static_assert(!std::is_constructible_v<ara::core::StringView, std::nullptr_t>,
                      "StringView must not be constructible from nullptr");
        
        std::cout << "Nullptr construction properly deleted\n";
    }
}

/*!
 * \brief Test #16: Performance Comparisons
 */
void TestPerformance()
{
    PrintTestHeader("Performance Comparisons");

    const std::size_t iterations = 1'000'000;
    const char* test_string = "This is a test string for performance measurement";
    [[maybe_unused]] const std::size_t test_len = std::strlen(test_string);

    PrintSubTest("Construction Performance");
    {
        PerfTimer timer;
        
        // StringView construction
        timer.reset();
        for (std::size_t i = 0; i < iterations; ++i) {
            [[maybe_unused]] ara::core::StringView sv(test_string);
        }
        double sv_time = timer.elapsed();
        
        // std::string construction
        timer.reset();
        for (std::size_t i = 0; i < iterations; ++i) {
            [[maybe_unused]] std::string str(test_string);
        }
        double string_time = timer.elapsed();
        
        std::cout << "StringView construction: " << sv_time << " μs\n";
        std::cout << "std::string construction: " << string_time << " μs\n";
        std::cout << "StringView is " << (string_time / sv_time) 
                  << "x faster\n";
    }

    PrintSubTest("Substring Performance");
    {
        ara::core::StringView sv(test_string);
        std::string str(test_string);
        PerfTimer timer;
        
        // StringView substr
        timer.reset();
        for (std::size_t i = 0; i < iterations; ++i) {
            [[maybe_unused]] auto sub = sv.substr(10, 20);
        }
        double sv_time = timer.elapsed();
        
        // std::string substr
        timer.reset();
        for (std::size_t i = 0; i < iterations; ++i) {
            [[maybe_unused]] auto sub = str.substr(10, 20);
        }
        double string_time = timer.elapsed();
        
        std::cout << "StringView substr: " << sv_time << " μs\n";
        std::cout << "std::string substr: " << string_time << " μs\n";
        std::cout << "StringView is " << (string_time / sv_time) 
                  << "x faster\n";
    }

    PrintSubTest("Find Performance");
    {
        ara::core::StringView sv(test_string);
        std::string str(test_string);
        PerfTimer timer;
        
        // StringView find
        timer.reset();
        [[maybe_unused]] std::size_t count1 = 0;
        for (std::size_t i = 0; i < iterations / 10; ++i) {
            auto pos = sv.find("performance");
            if (pos != ara::core::StringView::npos) ++count1;
        }
        double sv_time = timer.elapsed();
        
        // std::string find
        timer.reset();
        [[maybe_unused]] std::size_t count2 = 0;
        for (std::size_t i = 0; i < iterations / 10; ++i) {
            auto pos = str.find("performance");
            if (pos != std::string::npos) ++count2;
        }
        double string_time = timer.elapsed();
        
        assert(count1 == count2);
        
        std::cout << "StringView find: " << sv_time << " μs\n";
        std::cout << "std::string find: " << string_time << " μs\n";
        std::cout << "Performance ratio: " << (string_time / sv_time) << "x\n";
    }

    PrintSubTest("Iteration Performance");
    {
        ara::core::StringView sv(test_string);
        PerfTimer timer;
        
        // Range-based for
        timer.reset();
        [[maybe_unused]] std::size_t sum1 = 0;
        for (std::size_t i = 0; i < iterations / 100; ++i) {
            for (char c : sv) {
                sum1 += static_cast<std::size_t>(c);
            }
        }
        double range_time = timer.elapsed();
        
        // Index-based
        timer.reset();
        [[maybe_unused]] std::size_t sum2 = 0;
        for (std::size_t i = 0; i < iterations / 100; ++i) {
            for (std::size_t j = 0; j < sv.size(); ++j) {
                sum2 += static_cast<std::size_t>(sv[j]);
            }
        }
        double index_time = timer.elapsed();
        
        // Iterator-based
        timer.reset();
        [[maybe_unused]] std::size_t sum3 = 0;
        for (std::size_t i = 0; i < iterations / 100; ++i) {
            for (auto it = sv.begin(); it != sv.end(); ++it) {
                sum3 += static_cast<std::size_t>(*it);
            }
        }
        double iter_time = timer.elapsed();
        
        assert(sum1 == sum2 && sum2 == sum3);
        
        std::cout << "Range-based for: " << range_time << " μs\n";
        std::cout << "Index-based: " << index_time << " μs\n";
        std::cout << "Iterator-based: " << iter_time << " μs\n";
    }

    std::cout << "\nNote: ara::core::StringView provides significant performance benefits\n";
    std::cout << "      over std::string for non-owning string operations\n";
}

/**********************************************************************************************************************
 *  VIOLATION TEST FUNCTIONS (Will terminate)
 *********************************************************************************************************************/

/*!
 * \brief Test nullptr construction violation
 */
[[noreturn]] void TestNullptrViolation()
{
    std::cout << "Constructing StringView from nullptr...\n";
    
    const char* null_ptr = nullptr;
    [[maybe_unused]] ara::core::StringView sv(null_ptr);  // Will trigger violation
    
    // Never reached
    std::cout << "This should never print!\n";
    std::abort();
}

/*!
 * \brief Test bounds violation with at()
 */
[[noreturn]] void TestBoundsViolation()
{
    std::cout << "Accessing out-of-bounds element with at()...\n";
    
    ara::core::StringView sv("Test");
    [[maybe_unused]] char c = sv.at(10);  // Index 10 >= size 4
    
    // Never reached
    std::cout << "Character: " << c << " (should never print)\n";
    std::abort();
}

/*!
 * \brief Test empty access violation
 */
[[noreturn]] void TestEmptyAccessViolation()
{
    std::cout << "Accessing front() on empty StringView...\n";
    
    ara::core::StringView sv;
    [[maybe_unused]] char c = sv.front();  // Empty access
    
    // Never reached
    std::cout << "Character: " << c << " (should never print)\n";
    std::abort();
}

/*!
 * \brief Test position violation in substr
 */
[[noreturn]] void TestPosViolation()
{
    std::cout << "Creating substr with invalid position...\n";
    
    ara::core::StringView sv("Short");
    [[maybe_unused]] auto sub = sv.substr(10);  // pos 10 > size 5
    
    // Never reached
    std::cout << "Substr size: " << sub.size() << " (should never print)\n";
    std::abort();
}

/*!
 * \brief Test remove operation violation
 */
[[noreturn]] void TestRemoveViolation()
{
    std::cout << "Removing more characters than available...\n";
    
    ara::core::StringView sv("Test");
    sv.remove_prefix(10);  // Removing 10 chars from 4-char string
    
    // Never reached
    std::cout << "Remaining size: " << sv.size() << " (should never print)\n";
    std::abort();
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
    // Test: Direct nullptr construction (deleted)
    {
        ara::core::StringView sv(nullptr);  // ERROR: Deleted constructor
    }
    */

    /*
    // Test: Temporary string binding (would create dangling reference)
    {
        ara::core::StringView sv = std::string("temporary");  // ERROR: Deleted rvalue constructor
    }
    */

    /*
    // Test: Non-contiguous container
    {
        std::list<char> lst = {'a', 'b', 'c'};
        ara::core::StringView sv(lst);  // ERROR: std::list has no data() member
    }
    */

    // ========================================================================
    // 2. CONST CORRECTNESS ERRORS
    // ========================================================================
    
    /*
    // Test: Modifying through const StringView elements
    {
        const char str[] = "const";
        ara::core::StringView sv(str);  // This is fine
        sv[0] = 'C';  // This would work! StringView doesn't enforce const on elements
        
        // But this correctly prevents modification:
        ara::core::BasicStringView<const char> csv(str);
        csv[0] = 'C';  // ERROR: Cannot modify const char
    }
    */

    /*
    // Test: Converting const to non-const
    {
        const char str[] = "const";
        ara::core::BasicStringView<const char> csv(str);
        ara::core::BasicStringView<char> sv(csv);  // ERROR: Cannot convert const char* to char*
    }
    */

    // ========================================================================
    // 3. ITERATOR ERRORS
    // ========================================================================
    
    /*
    // Test: Modify through const_iterator
    {
        ara::core::StringView sv("test");
        auto it = sv.cbegin();
        *it = 'T';  // ERROR: Cannot modify through const_iterator
    }
    */

    // ========================================================================
    // 4. TYPE MISMATCH ERRORS
    // ========================================================================
    
    /*
    // Test: Mixing character types
    {
        const char* str = "narrow";
        ara::core::WStringView wsv(str);  // ERROR: Cannot convert char* to wchar_t*
    }
    */

    /*
    // Test: Incompatible traits
    {
        struct CustomTraits : std::char_traits<char> {};
        ara::core::BasicStringView<char, CustomTraits> sv1("test");
        ara::core::StringView sv2 = sv1;  // ERROR: Different traits types
    }
    */

    // ========================================================================
    // 5. INITIALIZER LIST ERRORS
    // ========================================================================
    
    /*
    // Test: Attempting brace initialization (no initializer_list constructor)
    {
        ara::core::StringView sv{'H', 'e', 'l', 'l', 'o'};  // ERROR: No matching constructor
    }
    */

    // ========================================================================
    // 6. STATIC MEMBER ACCESS ERRORS
    // ========================================================================
    
    /*
    // Test: npos is not a function
    {
        ara::core::StringView sv("test");
        auto pos = sv.npos();  // ERROR: npos is a static member, not a function
    }
    */

    // ========================================================================
    // 7. RANGE CONSTRUCTION ERRORS
    // ========================================================================
    
    /*
    // Test: Iterator type mismatch
    {
        std::vector<int> vec = {65, 66, 67};  // ASCII values
        ara::core::StringView sv(vec.begin(), vec.end());  // ERROR: int* != char*
    }
    */

    // ========================================================================
    // 8. METHOD MISUSE ERRORS
    // ========================================================================
    
    /*
    // Test: copy() requires output buffer
    {
        ara::core::StringView sv("test");
        sv.copy(nullptr, 4);  // Would compile but UB at runtime
    }
    */

    std::cout << "To test compilation errors:\n";
    std::cout << "1. Uncomment one section at a time\n";
    std::cout << "2. Attempt to compile\n";
    std::cout << "3. Observe the error message\n";
    std::cout << "4. Re-comment the section before testing another\n";
}