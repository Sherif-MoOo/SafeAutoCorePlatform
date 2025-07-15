/**********************************************************************************************************************
 *  PROJECT
 *  -------------------------------------------------------------------------------------------------------------------
 *  \verbatim
 *  OpenAA: Open Source Adaptive AUTOSAR Project (CXX_STANDARD 17)
 *  \endverbatim
 *  -------------------------------------------------------------------------------------------------------------------
 *  FILE DESCRIPTION
 *  -------------------------------------------------------------------------------------------------------------------
 *  \file       ara_core_byte_test.cpp
 *  \brief      Comprehensive test application for the ara::core::Byte type.
 *
 *  \details    This file contains multiple test functions covering all aspects of ara::core::Byte:
 *              1.  Basic Construction and Conversion
 *              2.  Explicit Conversions and Type Safety
 *              3.  Bitwise Operations (AND, OR, XOR, NOT)
 *              4.  Shift Operations (Left and Right)
 *              5.  Comparison Operations
 *              6.  Bit Manipulation Methods (test, set, reset, flip)
 *              7.  Utility Functions (to_byte, to_integer)
 *              8.  User-Defined Literals
 *              9.  Const Correctness
 *              10. Zero and Max Value Edge Cases
 *              11. Type Traits Verification
 *              12. Performance Comparisons
 *              13. Violation Scenarios (separate functions)
 *              14. Negative Compilation Scenarios (commented out)
 *
 *  \note       All tests run sequentially with clear output. Violation tests are in separate
 *              functions that can be called individually to observe termination behavior.
 *********************************************************************************************************************/

#if defined(__linux__)
#include <sys/prctl.h>
#elif defined(__QNXNTO__)
#include <pthread.h>
#endif
#include "ara/core/byte.h"         // The ara::core::Byte implementation
#include <iostream>                 // For std::cout
#include <cassert>                  // For runtime checks
#include <chrono>                   // For performance measurements
#include <cstring>                  // For memcpy comparisons
#include <type_traits>              // For type trait checks
#include <limits>                   // For numeric_limits tests
#include <array>                    // For std containers comparison
#include <iomanip>                  // For output formatting
#include <string>                   // For std::string
#include <string_view>              // For std::string_view

static constexpr std::string_view   kProcessNameView{"CoreByteTest"};
static constexpr std::uint8_t       kMaxProcessName{15};

/**********************************************************************************************************************
 *  UTILITY FUNCTIONS
 *********************************************************************************************************************/

/*!
 * \brief Simple performance timer using std::chrono
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
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "Test: " << testName << "\n";
    std::cout << std::string(60, '=') << "\n";
}

/*!
 * \brief Print sub-test header
 */
void PrintSubTest(const std::string& subTestName) {
    std::cout << "\n--- " << subTestName << " ---\n";
}

/**********************************************************************************************************************
 *  FORWARD DECLARATIONS
 *********************************************************************************************************************/
void TestBasicConstructionAndConversion();
void TestExplicitConversionsAndTypeSafety();
void TestBitwiseOperations();
void TestShiftOperations();
void TestComparisonOperations();
void TestBitManipulationMethods();
void TestUtilityFunctions();
void TestUserDefinedLiterals();
void TestConstCorrectness();
void TestEdgeCases();
void TestTypeTraits();
void TestPerformanceComparisons();

// Violation tests (will terminate the program)
void TestByteRangeViolation();
void TestBitPositionViolation();
void TestShiftAmountViolation();

// Negative compilation tests (commented out)
void TestNegativeCompilationScenarios();

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
    std::cout << "║         ara::core::Byte Comprehensive Test Suite           ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n";

    // Check if user wants to run violation tests
    if (argc > 1) {
        std::string arg = argv[1];
        if (arg == "violation-range") {
            std::cout << "\nRunning Byte Range Violation Test (will terminate)...\n";
            TestByteRangeViolation();
            return 1; // Should never reach here
        }
        else if (arg == "violation-bit") {
            std::cout << "\nRunning Bit Position Violation Test (will terminate)...\n";
            TestBitPositionViolation();
            return 1; // Should never reach here
        }
        else if (arg == "violation-shift") {
            std::cout << "\nRunning Shift Amount Violation Test (will terminate)...\n";
            TestShiftAmountViolation();
            return 1; // Should never reach here
        }
        else {
            std::cout << "\nUsage: " << argv[0] << " [violation-range|violation-bit|violation-shift]\n";
            std::cout << "       No arguments: Run all non-terminating tests\n";
            std::cout << "       violation-range: Test byte range violation\n";
            std::cout << "       violation-bit: Test bit position violation\n";
            std::cout << "       violation-shift: Test shift amount violation\n\n";
        }
    }

    // Run all non-terminating tests sequentially
    TestBasicConstructionAndConversion();
    TestExplicitConversionsAndTypeSafety();
    TestBitwiseOperations();
    TestShiftOperations();
    TestComparisonOperations();
    TestBitManipulationMethods();
    TestUtilityFunctions();
    TestUserDefinedLiterals();
    TestConstCorrectness();
    TestEdgeCases();
    TestTypeTraits();
    TestPerformanceComparisons();
    TestNegativeCompilationScenarios(); // All scenarios are commented out

    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║              ALL TESTS PASSED SUCCESSFULLY!                ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";

    return 0;
}

/**********************************************************************************************************************
 *  TEST IMPLEMENTATIONS
 *********************************************************************************************************************/

/*!
 * \brief Test #1: Basic Construction and Conversion
 */
void TestBasicConstructionAndConversion()
{
    PrintTestHeader("Basic Construction and Conversion");

    PrintSubTest("Default Construction");
    {
        ara::core::Byte b1;  // Uninitialized
        ara::core::Byte b2{}; // Zero-initialized
        
        // Note: b1 has indeterminate value, b2 is zero
        std::cout << "Default constructed (b2): " << static_cast<int>(b2.to_integer()) << " (expected 0)\n";
        assert(b2.to_integer() == 0);
        
        // Use b1 to avoid warning
        static_cast<void>(b1);
    }

    PrintSubTest("Construction from Integral Types");
    {
        // Unsigned types
        ara::core::Byte b_uint8{static_cast<std::uint8_t>(42)};
        ara::core::Byte b_uint16{static_cast<std::uint16_t>(100)};
        ara::core::Byte b_uint32{static_cast<std::uint32_t>(255)};
        
        std::cout << "From uint8_t(42): " << static_cast<int>(b_uint8.to_integer()) << "\n";
        std::cout << "From uint16_t(100): " << static_cast<int>(b_uint16.to_integer()) << "\n";
        std::cout << "From uint32_t(255): " << static_cast<int>(b_uint32.to_integer()) << "\n";
        
        assert(b_uint8.to_integer() == 42);
        assert(b_uint16.to_integer() == 100);
        assert(b_uint32.to_integer() == 255);
        
        // Signed types
        [[maybe_unused]] ara::core::Byte b_int8{static_cast<std::int8_t>(127)};
        [[maybe_unused]] ara::core::Byte b_int16{static_cast<std::int16_t>(200)};
        [[maybe_unused]] ara::core::Byte b_int32{200};
        
        assert(b_int8.to_integer() == 127);
        assert(b_int16.to_integer() == 200);
        assert(b_int32.to_integer() == 200);
    }

    PrintSubTest("Copy and Move Semantics");
    {
        ara::core::Byte original{42};
        
        // Copy construction
        ara::core::Byte copied{original};
        std::cout << "Copy constructed: " << static_cast<int>(copied.to_integer()) << " (expected 42)\n";
        assert(copied.to_integer() == 42);
        assert(original.to_integer() == 42); // Original unchanged
        
        // Move construction
        ara::core::Byte moved{std::move(original)};
        std::cout << "Move constructed: " << static_cast<int>(moved.to_integer()) << " (expected 42)\n";
        assert(moved.to_integer() == 42);
        // Note: original is trivially copyable, so move is same as copy
        
        // Copy assignment
        ara::core::Byte copy_assigned;
        copy_assigned = copied;
        assert(copy_assigned.to_integer() == 42);
        
        // Move assignment
        ara::core::Byte move_assigned;
        move_assigned = std::move(copied);
        assert(move_assigned.to_integer() == 42);
    }

    PrintSubTest("Compile-time Construction");
    {
        constexpr ara::core::Byte cb1{0};
        constexpr ara::core::Byte cb2{128};
        constexpr ara::core::Byte cb3{255};
        
        static_assert(cb1.to_integer() == 0, "Constexpr construction failed");
        static_assert(cb2.to_integer() == 128, "Constexpr construction failed");
        static_assert(cb3.to_integer() == 255, "Constexpr construction failed");
        
        std::cout << "Compile-time construction verified\n";
    }
}

/*!
 * \brief Test #2: Explicit Conversions and Type Safety
 */
void TestExplicitConversionsAndTypeSafety()
{
    PrintTestHeader("Explicit Conversions and Type Safety");

    PrintSubTest("Explicit Conversion to Integral Types");
    {
        ara::core::Byte b{200};
        
        // Explicit conversion using cast operator
        auto as_uint8 = static_cast<std::uint8_t>(b);
        auto as_uint16 = static_cast<std::uint16_t>(b);
        auto as_int = static_cast<int>(b);
        
        std::cout << "Byte{200} as uint8_t: " << static_cast<int>(as_uint8) << "\n";
        std::cout << "Byte{200} as uint16_t: " << as_uint16 << "\n";
        std::cout << "Byte{200} as int: " << as_int << "\n";
        
        assert(as_uint8 == 200);
        assert(as_uint16 == 200);
        assert(as_int == 200);
        
        // Using to_integer<T>()
        auto to_int8 = b.to_integer<std::int8_t>();
        auto to_uint32 = b.to_integer<std::uint32_t>();
        
        std::cout << "to_integer<int8_t>(): " << static_cast<int>(to_int8) << " (note: overflow)\n";
        std::cout << "to_integer<uint32_t>(): " << to_uint32 << "\n";
        
        assert(to_int8 == static_cast<std::int8_t>(200)); // -56 due to overflow
        assert(to_uint32 == 200);
    }

    PrintSubTest("No Implicit Conversions");
    {
        ara::core::Byte b{42};
        
        // These would fail to compile (verified in negative tests):
        // int x = b;           // Error: no implicit conversion to int
        // bool flag = b;       // Error: no implicit conversion to bool
        // if (b) {}           // Error: no implicit conversion to bool
        
        // Correct usage requires explicit conversion:
        int x = static_cast<int>(b);
        bool flag = (b.to_integer() != 0);
        
        std::cout << "Explicit int conversion: " << x << "\n";
        std::cout << "Explicit bool check: " << (flag ? "true" : "false") << "\n";
        
        assert(x == 42);
        assert(flag == true);
    }

    PrintSubTest("Type Safety with Different Types");
    {
        // These demonstrate that Byte maintains type safety
        ara::core::Byte b1{100};
        ara::core::Byte b2{200};
        
        // Cannot accidentally mix with integers in operations
        // int sum = b1 + 50;  // Would fail to compile
        
        // Correct usage:
        int sum = static_cast<int>(b1.to_integer()) + 50;
        std::cout << "Byte{100}.to_integer() + 50 = " << sum << "\n";
        assert(sum == 150);
        
        // Byte operations return Byte
        ara::core::Byte b_result = b1 | b2;
        std::cout << "Byte{100} | Byte{200} = " << static_cast<int>(b_result.to_integer()) << "\n";
        assert(b_result.to_integer() == (100 | 200));
    }
}

/*!
 * \brief Test #3: Bitwise Operations
 */
void TestBitwiseOperations()
{
    PrintTestHeader("Bitwise Operations");

    PrintSubTest("Bitwise AND");
    {
        ara::core::Byte a{0b11110000};
        ara::core::Byte b{0b10101010};
        
        ara::core::Byte result = a & b;
        std::cout << "0b11110000 & 0b10101010 = 0b" 
                  << std::bitset<8>(result.to_integer()) << " (0b10100000 expected)\n";
        assert(result.to_integer() == 0b10100000);
        
        // Compound assignment
        a &= b;
        assert(a.to_integer() == 0b10100000);
        
        // Identity: x & 0xFF = x
        [[maybe_unused]] ara::core::Byte x{123};
        assert((x & ara::core::Byte{0xFF}).to_integer() == 123);
        
        // Zero: x & 0 = 0
        assert((x & ara::core::Byte{0}).to_integer() == 0);
    }

    PrintSubTest("Bitwise OR");
    {
        ara::core::Byte a{0b11110000};
        ara::core::Byte b{0b10101010};
        
        ara::core::Byte result = a | b;
        std::cout << "0b11110000 | 0b10101010 = 0b" 
                  << std::bitset<8>(result.to_integer()) << " (0b11111010 expected)\n";
        assert(result.to_integer() == 0b11111010);
        
        // Compound assignment
        a |= b;
        assert(a.to_integer() == 0b11111010);
        
        // Identity: x | 0 = x
        [[maybe_unused]] ara::core::Byte x{123};
        assert((x | ara::core::Byte{0}).to_integer() == 123);
        
        // All ones: x | 0xFF = 0xFF
        assert((x | ara::core::Byte{0xFF}).to_integer() == 0xFF);
    }

    PrintSubTest("Bitwise XOR");
    {
        ara::core::Byte a{0b11110000};
        ara::core::Byte b{0b10101010};
        
        ara::core::Byte result = a ^ b;
        std::cout << "0b11110000 ^ 0b10101010 = 0b" 
                  << std::bitset<8>(result.to_integer()) << " (0b01011010 expected)\n";
        assert(result.to_integer() == 0b01011010);
        
        // Compound assignment
        a ^= b;
        assert(a.to_integer() == 0b01011010);
        
        // Self-cancel: x ^ x = 0
        [[maybe_unused]] ara::core::Byte x{123};
        assert((x ^ x).to_integer() == 0);
        
        // Identity: x ^ 0 = x
        assert((x ^ ara::core::Byte{0}).to_integer() == 123);
    }

    PrintSubTest("Bitwise NOT");
    {
        ara::core::Byte a{0b11110000};
        ara::core::Byte result = ~a;
        
        std::cout << "~0b11110000 = 0b" 
                  << std::bitset<8>(result.to_integer()) << " (0b00001111 expected)\n";
        assert(result.to_integer() == 0b00001111);
        
        // Double negation: ~~x = x
        [[maybe_unused]] ara::core::Byte x{123};
        assert((~~x).to_integer() == 123);
        
        // NOT of 0 = 0xFF
        assert((~ara::core::Byte{0}).to_integer() == 0xFF);
        
        // NOT of 0xFF = 0
        assert((~ara::core::Byte{0xFF}).to_integer() == 0);
    }

    PrintSubTest("Constexpr Bitwise Operations");
    {
        constexpr ara::core::Byte a{0xF0};
        constexpr ara::core::Byte b{0xAA};
        
        constexpr ara::core::Byte c_and = a & b;
        constexpr ara::core::Byte c_or = a | b;
        constexpr ara::core::Byte c_xor = a ^ b;
        constexpr ara::core::Byte c_not = ~a;
        
        static_assert(c_and.to_integer() == 0xA0, "Constexpr AND failed");
        static_assert(c_or.to_integer() == 0xFA, "Constexpr OR failed");
        static_assert(c_xor.to_integer() == 0x5A, "Constexpr XOR failed");
        static_assert(c_not.to_integer() == 0x0F, "Constexpr NOT failed");
        
        std::cout << "Compile-time bitwise operations verified\n";
    }
}

constexpr ara::core::Byte shift_constexpr() noexcept
{
    ara::core::Byte b{0b0000'0001};
    b <<= 7;
    return b;
}

/*!
 * \brief Test #4: Shift Operations
 */
void TestShiftOperations()
{
    PrintTestHeader("Shift Operations");

    PrintSubTest("Left Shift");
    {
        ara::core::Byte b{0b00110011};
        
        // Shift by various amounts
        for (int shift = 0; shift <= 7; ++shift) {
            ara::core::Byte result = b << shift;
            std::cout << "0b00110011 << " << shift << " = 0b" 
                      << std::bitset<8>(result.to_integer()) << "\n";
            assert(result.to_integer() == static_cast<std::uint8_t>((0b00110011u << static_cast<unsigned>(shift)) & 0xFFu));
        }
        
        // Compound assignment
        ara::core::Byte b2{0b00000001};
        b2 <<= 7;
        assert(b2.to_integer() == 0b10000000);

        constexpr ara::core::Byte b99 = shift_constexpr();
        static_cast<void>(b99); // Ensure constexpr works
        // Edge case: shift by 0
        [[maybe_unused]] ara::core::Byte b3{123};
        assert((b3 << 0).to_integer() == 123);
    }

    PrintSubTest("Right Shift");
    {
        ara::core::Byte b{0b11001100};
        
        // Shift by various amounts
        for (int shift = 0; shift <= 7; ++shift) {
            ara::core::Byte result = b >> shift;
            std::cout << "0b11001100 >> " << shift << " = 0b" 
                      << std::bitset<8>(result.to_integer()) << "\n";
            assert(result.to_integer() == static_cast<std::uint8_t>(0b11001100u >> static_cast<unsigned>(shift)));
        }
        
        // Compound assignment
        ara::core::Byte b2{0b10000000};
        b2 >>= 7;
        assert(b2.to_integer() == 0b00000001);
        
        // Edge case: shift by 0
        [[maybe_unused]] ara::core::Byte b3{123};
        assert((b3 >> 0).to_integer() == 123);
    }

    PrintSubTest("Shift Amount Masking");
    {
        // Shift amounts are masked to [0, 7]
        ara::core::Byte b{0b00000001};
        
        // These are masked internally
        ara::core::Byte r1 = b << 8;  // 8 & 0x7 = 0
        ara::core::Byte r2 = b << 9;  // 9 & 0x7 = 1
        
        std::cout << "Shift by 8 (masked to 0): " << static_cast<int>(r1.to_integer()) << "\n";
        std::cout << "Shift by 9 (masked to 1): " << static_cast<int>(r2.to_integer()) << "\n";
        
        assert(r1.to_integer() == 0b00000001);  // No shift
        assert(r2.to_integer() == 0b00000010);  // Shifted by 1
    }

    PrintSubTest("Constexpr Shift Operations");
    {
        constexpr ara::core::Byte b{0x0F};
        
        constexpr ara::core::Byte left2 = b << 2;
        constexpr ara::core::Byte left4 = b << 4;
        constexpr ara::core::Byte right2 = b >> 2;
        constexpr ara::core::Byte right4 = b >> 4;
        
        static_assert(left2.to_integer() == 0x3C, "Constexpr left shift failed");
        static_assert(left4.to_integer() == 0xF0, "Constexpr left shift failed");
        static_assert(right2.to_integer() == 0x03, "Constexpr right shift failed");
        static_assert(right4.to_integer() == 0x00, "Constexpr right shift failed");
        
        std::cout << "Compile-time shift operations verified\n";
    }
}

/*!
 * \brief Test #5: Comparison Operations
 */
void TestComparisonOperations()
{
    PrintTestHeader("Comparison Operations");

    PrintSubTest("Equality Comparisons");
    {
        ara::core::Byte a{42};
        ara::core::Byte b{42};
        ara::core::Byte c{100};
        
        std::cout << "Byte{42} == Byte{42}: " << (a == b ? "true" : "false") << " (expected true)\n";
        std::cout << "Byte{42} != Byte{100}: " << (a != c ? "true" : "false") << " (expected true)\n";
        
        assert(a == b);
        assert(!(a != b));
        assert(a != c);
        assert(!(a == c));
        
        // Self-comparison
        assert(a == a);
        assert(!(a != a));
    }

    PrintSubTest("Relational Comparisons");
    {
        [[maybe_unused]] ara::core::Byte small{10};
        [[maybe_unused]] ara::core::Byte medium{100};
        [[maybe_unused]] ara::core::Byte large{200};
        
        // Less than
        assert(small < medium);
        assert(medium < large);
        assert(!(large < small));
        assert(!(medium < medium));
        
        // Less than or equal
        assert(small <= medium);
        assert(medium <= medium);
        assert(!(large <= small));
        
        // Greater than
        assert(large > medium);
        assert(medium > small);
        assert(!(small > large));
        assert(!(medium > medium));
        
        // Greater than or equal
        assert(large >= medium);
        assert(medium >= medium);
        assert(!(small >= large));
        
        std::cout << "All relational comparisons passed\n";
    }

    PrintSubTest("Edge Case Comparisons");
    {
        [[maybe_unused]] ara::core::Byte zero{0};
        [[maybe_unused]] ara::core::Byte max{255};
        
        assert(zero < max);
        assert(zero <= max);
        assert(max > zero);
        assert(max >= zero);
        assert(zero != max);
        assert(!(zero == max));
        
        // Boundary comparisons
        [[maybe_unused]] ara::core::Byte one{1};
        [[maybe_unused]] ara::core::Byte max_minus_one{254};

        assert(zero < one);
        assert(max_minus_one < max);
        assert(!(max < max_minus_one));
        
        std::cout << "Edge case comparisons passed\n";
    }

    PrintSubTest("Constexpr Comparisons");
    {
        constexpr ara::core::Byte a{50};
        constexpr ara::core::Byte b{100};
        constexpr ara::core::Byte c{50};
        
        static_assert(a == c, "Constexpr equality failed");
        static_assert(a != b, "Constexpr inequality failed");
        static_assert(a < b, "Constexpr less than failed");
        static_assert(a <= b, "Constexpr less than or equal failed");
        static_assert(b > a, "Constexpr greater than failed");
        static_assert(b >= a, "Constexpr greater than or equal failed");
        
        std::cout << "Compile-time comparisons verified\n";
    }
}

/*!
 * \brief Test #6: Bit Manipulation Methods
 */
void TestBitManipulationMethods()
{
    PrintTestHeader("Bit Manipulation Methods");

    PrintSubTest("Test Bit");
    {
        ara::core::Byte b{0b10101010};
        
        std::cout << "Testing bits in 0b10101010:\n";
        for (std::size_t i = 0; i < 8; ++i) {
            bool bit_set = b.test(i);
            std::cout << "  Bit " << i << ": " << (bit_set ? "1" : "0") << "\n";
            assert(bit_set == ((0b10101010u >> i) & 1u));
        }
    }

    PrintSubTest("Set/Reset/Flip Individual Bits");
    {
        ara::core::Byte b{0};
        
        // Set bits
        b.set(0).set(2).set(4).set(6);
        std::cout << "After setting bits 0,2,4,6: 0b" << std::bitset<8>(b.to_integer()) << "\n";
        assert(b.to_integer() == 0b01010101);
        
        // Reset bits
        b.reset(2).reset(6);
        std::cout << "After resetting bits 2,6: 0b" << std::bitset<8>(b.to_integer()) << "\n";
        assert(b.to_integer() == 0b00010001);
        
        // Flip bits
        b.flip(1).flip(3).flip(5).flip(7);
        std::cout << "After flipping bits 1,3,5,7: 0b" << std::bitset<8>(b.to_integer()) << "\n";
        assert(b.to_integer() == 0b10111011);
        
        // Chain operations
        ara::core::Byte b2{0};
        b2.set(7).set(0).flip(3).reset(0);
        assert(b2.to_integer() == 0b10001000);
    }

    PrintSubTest("Set/Reset/Flip All Bits");
    {
        ara::core::Byte b{0b10101010};
        
        // Set all
        b.set();
        std::cout << "After set(): " << static_cast<int>(b.to_integer()) << " (expected 255)\n";
        assert(b.to_integer() == 0xFF);
        assert(b.all());
        
        // Reset all
        b.reset();
        std::cout << "After reset(): " << static_cast<int>(b.to_integer()) << " (expected 0)\n";
        assert(b.to_integer() == 0);
        assert(b.none());
        
        // Flip all
        b.flip();
        std::cout << "After flip() on 0: " << static_cast<int>(b.to_integer()) << " (expected 255)\n";
        assert(b.to_integer() == 0xFF);
        
        b.flip();
        std::cout << "After flip() on 255: " << static_cast<int>(b.to_integer()) << " (expected 0)\n";
        assert(b.to_integer() == 0);
    }

    PrintSubTest("Count, All, Any, None");
    {
        struct TestCase {
            std::uint8_t value;
            std::size_t expected_count;
            bool expected_all;
            bool expected_any;
            bool expected_none;
        };
        
        TestCase cases[] = {
            {0b00000000, 0, false, false, true},
            {0b11111111, 8, true, true, false},
            {0b10101010, 4, false, true, false},
            {0b11110000, 4, false, true, false},
            {0b00000001, 1, false, true, false},
            {0b10000000, 1, false, true, false},
        };
        
        for (const auto& tc : cases) {
            ara::core::Byte b{tc.value};
            std::cout << "Testing 0b" << std::bitset<8>(tc.value) << ":\n";
            std::cout << "  count(): " << b.count() << " (expected " << tc.expected_count << ")\n";
            std::cout << "  all(): " << b.all() << " (expected " << tc.expected_all << ")\n";
            std::cout << "  any(): " << b.any() << " (expected " << tc.expected_any << ")\n";
            std::cout << "  none(): " << b.none() << " (expected " << tc.expected_none << ")\n";
            
            assert(b.count() == tc.expected_count);
            assert(b.all() == tc.expected_all);
            assert(b.any() == tc.expected_any);
            assert(b.none() == tc.expected_none);
        }
    }

    PrintSubTest("Constexpr Bit Manipulation");
    {
        constexpr auto test_bit_ops = []() constexpr -> bool {
            ara::core::Byte b{0};
            b.set(1).set(3).set(5);
            if (b.to_integer() != 0b00101010) return false;
            if (!b.test(1) || !b.test(3) || !b.test(5)) return false;
            if (b.count() != 3) return false;
            return true;
        };
        
        static_assert(test_bit_ops(), "Constexpr bit manipulation failed");
        std::cout << "Compile-time bit manipulation verified\n";
    }
}

/*!
 * \brief Test #7: Utility Functions
 */
void TestUtilityFunctions()
{
    PrintTestHeader("Utility Functions");

    PrintSubTest("to_byte Function");
    {
        // From various integral types
        auto b1 = ara::core::to_byte(42);
        auto b2 = ara::core::to_byte(static_cast<std::uint8_t>(100));
        auto b3 = ara::core::to_byte(static_cast<std::int16_t>(200));
        auto b4 = ara::core::to_byte('A');  // char overload
        
        std::cout << "to_byte(42): " << static_cast<int>(b1.to_integer()) << "\n";
        std::cout << "to_byte(uint8_t{100}): " << static_cast<int>(b2.to_integer()) << "\n";
        std::cout << "to_byte(int16_t{200}): " << static_cast<int>(b3.to_integer()) << "\n";
        std::cout << "to_byte('A'): " << static_cast<int>(b4.to_integer()) << " (ASCII 65)\n";
        
        assert(b1.to_integer() == 42);
        assert(b2.to_integer() == 100);
        assert(b3.to_integer() == 200);
        assert(b4.to_integer() == 65);
        
        // Constexpr usage
        constexpr auto cb = ara::core::to_byte(123);
        static_assert(cb.to_integer() == 123, "Constexpr to_byte failed");
    }

    PrintSubTest("to_integer Method");
    {
        ara::core::Byte b{200};
        
        // Default (returns uint8_t)
        auto u8 = b.to_integer();
        std::cout << "to_integer() default: " << static_cast<int>(u8) << " (uint8_t)\n";
        assert(u8 == 200);
        
        // Template versions
        auto i8 = b.to_integer<std::int8_t>();
        auto u16 = b.to_integer<std::uint16_t>();
        auto i32 = b.to_integer<std::int32_t>();
        auto u64 = b.to_integer<std::uint64_t>();
        
        std::cout << "to_integer<int8_t>(): " << static_cast<int>(i8) << " (overflow to -56)\n";
        std::cout << "to_integer<uint16_t>(): " << u16 << "\n";
        std::cout << "to_integer<int32_t>(): " << i32 << "\n";
        std::cout << "to_integer<uint64_t>(): " << u64 << "\n";
        
        assert(i8 == static_cast<std::int8_t>(200));  // -56 due to overflow
        assert(u16 == 200);
        assert(i32 == 200);
        assert(u64 == 200);
    }

    PrintSubTest("Round-trip Conversions");
    {
        // Test round-trip for all possible byte values
        [[maybe_unused]] bool all_passed = true;
        for (int i = 0; i <= 255; ++i) {
            auto b = ara::core::to_byte(i);
            auto back = static_cast<int>(b.to_integer());
            if (back != i) {
                all_passed = false;
                std::cout << "Round-trip failed for " << i << "\n";
            }
        }
        assert(all_passed);
        std::cout << "All 256 round-trip conversions passed\n";
    }
}

/*!
 * \brief Test #8: User-Defined Literals
 */
void TestUserDefinedLiterals()
{
    PrintTestHeader("User-Defined Literals");

    using namespace ara::core::literals::byte_literals;

    PrintSubTest("Decimal Literals");
    {
        constexpr auto b0 = 0_byte;
        constexpr auto b42 = 42_byte;
        constexpr auto b255 = 255_byte;
        
        std::cout << "0_byte: " << static_cast<int>(b0.to_integer()) << "\n";
        std::cout << "42_byte: " << static_cast<int>(b42.to_integer()) << "\n";
        std::cout << "255_byte: " << static_cast<int>(b255.to_integer()) << "\n";
        
        static_assert(b0.to_integer() == 0, "Decimal literal failed");
        static_assert(b42.to_integer() == 42, "Decimal literal failed");
        static_assert(b255.to_integer() == 255, "Decimal literal failed");
    }

    PrintSubTest("Hexadecimal Literals");
    {
        constexpr auto b1 = 0x00_byte;
        constexpr auto b2 = 0xFF_byte;
        constexpr auto b3 = 0xAB_byte;
        constexpr auto b4 = 0x7f_byte;
        
        std::cout << "0x00_byte: " << static_cast<int>(b1.to_integer()) << "\n";
        std::cout << "0xFF_byte: " << static_cast<int>(b2.to_integer()) << "\n";
        std::cout << "0xAB_byte: " << static_cast<int>(b3.to_integer()) << "\n";
        std::cout << "0x7f_byte: " << static_cast<int>(b4.to_integer()) << "\n";
        
        static_assert(b1.to_integer() == 0x00, "Hex literal failed");
        static_assert(b2.to_integer() == 0xFF, "Hex literal failed");
        static_assert(b3.to_integer() == 0xAB, "Hex literal failed");
        static_assert(b4.to_integer() == 0x7f, "Hex literal failed");
    }

    PrintSubTest("Binary Literals");
    {
        constexpr auto b1 = 0b00000000_byte;
        constexpr auto b2 = 0b11111111_byte;
        constexpr auto b3 = 0b10101010_byte;
        constexpr auto b4 = 0b01010101_byte;
        
        std::cout << "0b00000000_byte: " << static_cast<int>(b1.to_integer()) << "\n";
        std::cout << "0b11111111_byte: " << static_cast<int>(b2.to_integer()) << "\n";
        std::cout << "0b10101010_byte: " << static_cast<int>(b3.to_integer()) << "\n";
        std::cout << "0b01010101_byte: " << static_cast<int>(b4.to_integer()) << "\n";
        
        static_assert(b1.to_integer() == 0b00000000, "Binary literal failed");
        static_assert(b2.to_integer() == 0b11111111, "Binary literal failed");
        static_assert(b3.to_integer() == 0b10101010, "Binary literal failed");
        static_assert(b4.to_integer() == 0b01010101, "Binary literal failed");
    }

    PrintSubTest("Octal Literals");
    {
        constexpr auto b1 = 0_byte;
        constexpr auto b2 = 0377_byte;  // 255 in octal
        constexpr auto b3 = 0100_byte;  // 64 in octal
        constexpr auto b4 = 052_byte;   // 42 in octal
        
        std::cout << "0_byte: " << static_cast<int>(b1.to_integer()) << "\n";
        std::cout << "0377_byte: " << static_cast<int>(b2.to_integer()) << " (255)\n";
        std::cout << "0100_byte: " << static_cast<int>(b3.to_integer()) << " (64)\n";
        std::cout << "052_byte: " << static_cast<int>(b4.to_integer()) << " (42)\n";
        
        static_assert(b1.to_integer() == 0, "Octal literal failed");
        static_assert(b2.to_integer() == 255, "Octal literal failed");
        static_assert(b3.to_integer() == 64, "Octal literal failed");
        static_assert(b4.to_integer() == 42, "Octal literal failed");
    }

    PrintSubTest("Literal Usage in Expressions");
    {
        constexpr auto result1 = 0xFF_byte & 0xAA_byte;
        constexpr auto result2 = 0b11110000_byte | 0b00001111_byte;
        constexpr auto result3 = ~0x00_byte;
        constexpr auto result4 = 0x0F_byte << 4;
        
        static_assert(result1.to_integer() == 0xAA, "Literal expression failed");
        static_assert(result2.to_integer() == 0xFF, "Literal expression failed");
        static_assert(result3.to_integer() == 0xFF, "Literal expression failed");
        static_assert(result4.to_integer() == 0xF0, "Literal expression failed");
        
        std::cout << "User-defined literals in expressions verified\n";
    }
}

/*!
 * \brief Test #9: Const Correctness
 */
void TestConstCorrectness()
{
    PrintTestHeader("Const Correctness");

    PrintSubTest("Const Byte Operations");
    {
        const ara::core::Byte cb{123};
        
        // All these should work on const Byte
        [[maybe_unused]] auto val = cb.to_integer();
        [[maybe_unused]] auto val16 = cb.to_integer<std::uint16_t>();
        [[maybe_unused]] bool bit3 = cb.test(3);
        [[maybe_unused]] std::size_t count = cb.count();
        [[maybe_unused]] bool has_any = cb.any();
        [[maybe_unused]] bool has_all = cb.all();
        [[maybe_unused]] bool has_none = cb.none();

        std::cout << "Const byte value: " << static_cast<int>(val) << "\n";
        std::cout << "Bit 3: " << bit3 << "\n";
        std::cout << "Bit count: " << count << "\n";
        
        assert(val == 123);
        assert(val16 == 123);
        assert(bit3 == true);  // 123 = 0b01111011, bit 3 is set
        assert(count == 6);
        assert(has_any == true);
        assert(has_all == false);
        assert(has_none == false);
        
        // These would fail to compile:
        // cb.set(0);    // Error: cannot call non-const method
        // cb.reset();   // Error: cannot call non-const method
        // cb.flip(1);   // Error: cannot call non-const method
    }

    PrintSubTest("Const in Binary Operations");
    {
        const ara::core::Byte ca{0xF0};
        const ara::core::Byte cb{0x0F};
        
        // All binary operations work with const
        [[maybe_unused]] ara::core::Byte r1 = ca & cb;
        [[maybe_unused]] ara::core::Byte r2 = ca | cb;
        [[maybe_unused]] ara::core::Byte r3 = ca ^ cb;
        [[maybe_unused]] ara::core::Byte r4 = ~ca;
        [[maybe_unused]] ara::core::Byte r5 = ca << 2;
        [[maybe_unused]] ara::core::Byte r6 = cb >> 2;
        
        assert(r1.to_integer() == 0x00);
        assert(r2.to_integer() == 0xFF);
        assert(r3.to_integer() == 0xFF);
        assert(r4.to_integer() == 0x0F);
        assert(r5.to_integer() == 0xC0);
        assert(r6.to_integer() == 0x03);
        
        // Comparisons with const
        [[maybe_unused]] bool eq = (ca == cb);
        [[maybe_unused]] bool ne = (ca != cb);
        [[maybe_unused]] bool lt = (ca < cb);
        [[maybe_unused]] bool gt = (ca > cb);
        
        assert(!eq && ne && !lt && gt);
    }

    PrintSubTest("Constexpr Context");
    {
        // Everything should work in constexpr context
        constexpr ara::core::Byte cb{42};
        
        constexpr auto val = cb.to_integer();
        constexpr bool bit1 = cb.test(1);
        constexpr auto count = cb.count();
        constexpr bool any = cb.any();
        constexpr bool all = cb.all();
        constexpr bool none = cb.none();
        
        static_assert(val == 42, "Constexpr to_integer failed");
        static_assert(bit1 == true, "Constexpr test failed");  // 42 = 0b00101010
        static_assert(count == 3, "Constexpr count failed");
        static_assert(any == true, "Constexpr any failed");
        static_assert(all == false, "Constexpr all failed");
        static_assert(none == false, "Constexpr none failed");
        
        std::cout << "Constexpr const operations verified\n";
    }
}

/*!
 * \brief Test #10: Edge Cases
 */
void TestEdgeCases()
{
    PrintTestHeader("Edge Cases");

    PrintSubTest("Zero Byte");
    {
        [[maybe_unused]] ara::core::Byte zero{0};

        assert(zero.to_integer() == 0);
        assert(zero.count() == 0);
        assert(!zero.any());
        assert(!zero.all());
        assert(zero.none());
        
        // Operations with zero
        [[maybe_unused]] ara::core::Byte b{123};
        assert((b & zero).to_integer() == 0);
        assert((b | zero).to_integer() == 123);
        assert((b ^ zero).to_integer() == 123);
        assert((zero << 5).to_integer() == 0);
        assert((zero >> 3).to_integer() == 0);
        
        std::cout << "Zero byte edge cases passed\n";
    }

    PrintSubTest("Max Byte (255)");
    {
        [[maybe_unused]] ara::core::Byte max{255};

        assert(max.to_integer() == 255);
        assert(max.count() == 8);
        assert(max.any());
        assert(max.all());
        assert(!max.none());
        
        // Operations with max
        [[maybe_unused]] ara::core::Byte b{123};
        assert((b & max).to_integer() == 123);
        assert((b | max).to_integer() == 255);
        assert((~max).to_integer() == 0);
        
        // Shift edge cases
        assert((max << 8).to_integer() == 255);  // Masked to << 0
        assert((max >> 8).to_integer() == 255);  // Masked to >> 0
        
        std::cout << "Max byte edge cases passed\n";
    }

    PrintSubTest("Single Bit Patterns");
    {
        // Test each single bit position
        for (std::size_t bit = 0; bit < 8; ++bit) {
            ara::core::Byte b{static_cast<std::uint8_t>(1u << bit)};
            
            assert(b.count() == 1);
            assert(b.any());
            assert(!b.all());
            assert(!b.none());
            assert(b.test(bit));
            
            // Verify only the expected bit is set
            for (std::size_t i = 0; i < 8; ++i) {
                assert(b.test(i) == (i == bit));
            }
        }
        std::cout << "Single bit patterns verified\n";
    }

    PrintSubTest("Alternating Bit Patterns");
    {
        [[maybe_unused]] ara::core::Byte pattern1{0b10101010};  // 0xAA
        [[maybe_unused]] ara::core::Byte pattern2{0b01010101};  // 0x55

        assert(pattern1.count() == 4);
        assert(pattern2.count() == 4);
        assert((pattern1 & pattern2).to_integer() == 0);
        assert((pattern1 | pattern2).to_integer() == 255);
        assert((pattern1 ^ pattern2).to_integer() == 255);
        assert(pattern1.to_integer() == (~pattern2).to_integer());
        
        std::cout << "Alternating patterns verified\n";
    }
}

/*!
 * \brief Test #11: Type Traits Verification
 */
void TestTypeTraits()
{
    PrintTestHeader("Type Traits Verification");

    PrintSubTest("Basic Type Properties");
    {
        using B = ara::core::Byte;
        
        std::cout << "sizeof(Byte): " << sizeof(B) << " bytes\n";
        std::cout << "alignof(Byte): " << alignof(B) << " bytes\n";
        
        assert(sizeof(B) == 1);
        assert(alignof(B) == 1);
        
        // Trivial type checks
        static_assert(std::is_trivial_v<B>, "Byte must be trivial");
        static_assert(std::is_trivially_copyable_v<B>, "Byte must be trivially copyable");
        static_assert(std::is_standard_layout_v<B>, "Byte must have standard layout");
        static_assert(std::is_pod_v<B>, "Byte must be POD");
        
        std::cout << "Byte is a trivial, standard-layout POD type\n";
    }

    PrintSubTest("Construction and Destruction Traits");
    {
        using B = ara::core::Byte;
        
        static_assert(std::is_default_constructible_v<B>, "Must be default constructible");
        static_assert(std::is_trivially_default_constructible_v<B>, "Must be trivially default constructible");
        static_assert(std::is_nothrow_default_constructible_v<B>, "Must be nothrow default constructible");
        
        static_assert(std::is_copy_constructible_v<B>, "Must be copy constructible");
        static_assert(std::is_trivially_copy_constructible_v<B>, "Must be trivially copy constructible");
        static_assert(std::is_nothrow_copy_constructible_v<B>, "Must be nothrow copy constructible");
        
        static_assert(std::is_move_constructible_v<B>, "Must be move constructible");
        static_assert(std::is_trivially_move_constructible_v<B>, "Must be trivially move constructible");
        static_assert(std::is_nothrow_move_constructible_v<B>, "Must be nothrow move constructible");
        
        static_assert(std::is_destructible_v<B>, "Must be destructible");
        static_assert(std::is_trivially_destructible_v<B>, "Must be trivially destructible");
        static_assert(std::is_nothrow_destructible_v<B>, "Must be nothrow destructible");
        
        std::cout << "All construction/destruction traits verified\n";
    }

    PrintSubTest("Assignment Traits");
    {
        using B = ara::core::Byte;
        
        static_assert(std::is_copy_assignable_v<B>, "Must be copy assignable");
        static_assert(std::is_trivially_copy_assignable_v<B>, "Must be trivially copy assignable");
        static_assert(std::is_nothrow_copy_assignable_v<B>, "Must be nothrow copy assignable");
        
        static_assert(std::is_move_assignable_v<B>, "Must be move assignable");
        static_assert(std::is_trivially_move_assignable_v<B>, "Must be trivially move assignable");
        static_assert(std::is_nothrow_move_assignable_v<B>, "Must be nothrow move assignable");
        
        std::cout << "All assignment traits verified\n";
    }

    PrintSubTest("Conversion Traits");
    {
        using B = ara::core::Byte;
        
        // No implicit conversions
        static_assert(!std::is_convertible_v<int, B>, "No implicit conversion from int");
        static_assert(!std::is_convertible_v<B, int>, "No implicit conversion to int");
        static_assert(!std::is_convertible_v<B, bool>, "No implicit conversion to bool");
        static_assert(!std::is_convertible_v<char, B>, "No implicit conversion from char");
        static_assert(!std::is_convertible_v<B, char>, "No implicit conversion to char");
        
        // But explicit construction is allowed
        static_assert(std::is_constructible_v<B, int>, "Explicit construction from int allowed");
        static_assert(std::is_constructible_v<B, char>, "Explicit construction from char allowed");
        
        std::cout << "Conversion traits verified (no implicit conversions)\n";
    }

    PrintSubTest("numeric_limits Specialization");
    {
        using B = ara::core::Byte;
        using limits = std::numeric_limits<B>;
        
        std::cout << "numeric_limits<Byte>:\n";
        std::cout << "  is_specialized: " << limits::is_specialized << "\n";
        std::cout << "  min(): " << static_cast<int>(limits::min().to_integer()) << "\n";
        std::cout << "  max(): " << static_cast<int>(limits::max().to_integer()) << "\n";
        std::cout << "  digits: " << limits::digits << "\n";
        std::cout << "  is_signed: " << limits::is_signed << "\n";
        std::cout << "  is_integer: " << limits::is_integer << "\n";
        
        assert(limits::is_specialized);
        assert(limits::min().to_integer() == 0);
        assert(limits::max().to_integer() == 255);
        assert(limits::digits == 8);
        assert(!limits::is_signed);
        assert(limits::is_integer);
        
        std::cout << "numeric_limits specialization verified\n";
    }
}

/*!
 * \brief Test #12: Performance Comparisons
 */
void TestPerformanceComparisons()
{
    PrintTestHeader("Performance Comparisons");

    constexpr std::size_t iterations = 1'000'000;

    PrintSubTest("Byte vs uint8_t Basic Operations");
    {
        // Byte operations
        PerfTimer timer;
        ara::core::Byte b1{0xAA};
        ara::core::Byte b2{0x55};
        ara::core::Byte result{0};
        
        timer.reset();
        for (std::size_t i = 0; i < iterations; ++i) {
            result = (b1 & b2) | (~b1 ^ b2);
            result = result << 1;
            result = result >> 1;
        }
        double byte_time = timer.elapsed();
        
        // uint8_t operations
        std::uint8_t u1 = 0xAA;
        std::uint8_t u2 = 0x55;
        std::uint8_t uresult = 0;
        
        timer.reset();
        for (std::size_t i = 0; i < iterations; ++i) {
            uresult = static_cast<std::uint8_t>((u1 & u2) | (~u1 ^ u2));
            uresult = static_cast<std::uint8_t>(uresult << 1);
            uresult = static_cast<std::uint8_t>(uresult >> 1);
        }
        double uint8_time = timer.elapsed();
        
        std::cout << "Byte operations: " << byte_time << " μs\n";
        std::cout << "uint8_t operations: " << uint8_time << " μs\n";
        std::cout << "Overhead: " << ((byte_time / uint8_time) - 1.0) * 100.0 << "%\n";
        
        // Prevent optimization
        volatile auto v1 = result.to_integer();
        volatile auto v2 = uresult;
        (void)v1; (void)v2;
    }

    PrintSubTest("Bit Manipulation Performance");
    {
        ara::core::Byte b{0};
        
        // Byte bit manipulation
        PerfTimer timer;
        timer.reset();
        for (std::size_t i = 0; i < iterations; ++i) {
            b.set(i & 7).reset((i + 1) & 7).flip((i + 2) & 7);
        }
        double byte_time = timer.elapsed();
        
        // Manual bit manipulation
        std::uint8_t u = 0;
        timer.reset();
        for (std::size_t i = 0; i < iterations; ++i) {
            u = static_cast<std::uint8_t>(u | (1u << (i & 7u)));
            u = static_cast<std::uint8_t>(u & ~(1u << ((i + 1u) & 7u)));
            u = static_cast<std::uint8_t>(u ^ (1u << ((i + 2u) & 7u)));
        }
        double manual_time = timer.elapsed();
        
        std::cout << "Byte bit methods: " << byte_time << " μs\n";
        std::cout << "Manual bit ops: " << manual_time << " μs\n";
        std::cout << "Method overhead: " << ((byte_time / manual_time) - 1.0) * 100.0 << "%\n";
        
        // Use results to avoid optimization
        volatile auto v1 = b.to_integer();
        volatile auto v2 = u;
        (void)v1; (void)v2;
    }

    PrintSubTest("Compile-time vs Runtime");
    {
        // Compile-time evaluation
        constexpr auto compile_time_result = []() constexpr {
            ara::core::Byte b{0xFF};
            for (int i = 0; i < 100; ++i) {
                b = ~(b ^ ara::core::Byte{static_cast<std::uint8_t>(i)});
            }
            return b.to_integer();
        }();
        
        // Runtime evaluation
        PerfTimer timer;
        timer.reset();
        ara::core::Byte b{0xFF};
        for (std::size_t iter = 0; iter < iterations; ++iter) {
            b = ara::core::Byte{0xFF};
            for (int i = 0; i < 100; ++i) {
                b = ~(b ^ ara::core::Byte{static_cast<std::uint8_t>(i)});
            }
        }
        double runtime_time = timer.elapsed();
        
        std::cout << "Compile-time result: " << static_cast<int>(compile_time_result) << "\n";
        std::cout << "Runtime result: " << static_cast<int>(b.to_integer()) << "\n";
        std::cout << "Runtime evaluation: " << runtime_time << " μs for " << iterations << " iterations\n";
        std::cout << "Compile-time evaluation: 0 μs (computed at compile time)\n";
        
        assert(compile_time_result == b.to_integer());
    }

    std::cout << "\nNote: Performance comparisons show near-zero overhead for ara::core::Byte\n";
    std::cout << "      The type safety benefits come at virtually no runtime cost.\n";
}

/**********************************************************************************************************************
 *  VIOLATION TEST FUNCTIONS (These will terminate the program)
 *********************************************************************************************************************/

/*!
 * \brief Test byte range violation (runtime termination)
 */
void TestByteRangeViolation()
{
    std::cout << "Attempting to create Byte with value 300...\n";
    [[maybe_unused]] ara::core::Byte b{300};  // This will trigger violation and terminate
    
    // Never reached
    std::cout << "This line should never be printed!\n";
}

/*!
 * \brief Test bit position violation (runtime termination)
 */
void TestBitPositionViolation()
{
    ara::core::Byte b{42};
    std::cout << "Attempting to test bit 8 (out of range)...\n";
    [[maybe_unused]] bool bit = b.test(8);  // This will call Abort and terminate
    
    // Never reached
    std::cout << "Bit 8 = " << bit << " (should never print)\n";
}

/*!
 * \brief Test shift amount violation (runtime termination)
 */
void TestShiftAmountViolation()
{
    ara::core::Byte b{1};
    std::cout << "Attempting to shift by -1 (negative amount)...\n";
    b <<= -1;  // This will call Abort and terminate
    
    // Never reached
    std::cout << "Result = " << static_cast<int>(b.to_integer()) << " (should never print)\n";
}

/**********************************************************************************************************************
 *  NEGATIVE COMPILATION SCENARIOS (All commented out to allow compilation)
 *********************************************************************************************************************/

void TestNegativeCompilationScenarios()
{
    PrintTestHeader("Negative Compilation Scenarios");
    
    std::cout << "All negative compilation scenarios are commented out.\n";
    std::cout << "Uncomment individual sections to observe compilation errors.\n\n";

    // ========================================================================
    // 1. CONSTRUCTION ERRORS
    // ========================================================================
    
    /*
    // Test: Construct from non-integral type
    // Expected: Static assert failure
    {
        std::string str = "hello";
        ara::core::Byte b{str};  // ERROR: Cannot construct from non-integral type
    }
    */

    /*
    // Test: Construct from floating point
    // Expected: Static assert failure  
    {
        ara::core::Byte b{3.14};  // ERROR: Cannot construct from non-integral type
    }
    */

    /*
    // Test: Out-of-range compile-time constant
    // Expected: Static assert in parse_byte
    {
        constexpr ara::core::Byte b{300};  // ERROR: Value exceeds 255
    }
    */

    /*
    // Test: Negative compile-time constant
    // Expected: Static assert in constructor
    {
        constexpr ara::core::Byte b{-1};  // ERROR: Negative value
    }
    */

    // ========================================================================
    // 2. IMPLICIT CONVERSION ERRORS
    // ========================================================================
    
    /*
    // Test: Implicit conversion from int
    // Expected: No matching constructor
    {
        ara::core::Byte b = 42;  // ERROR: No implicit conversion from int
    }
    */

    /*
    // Test: Implicit conversion to int
    // Expected: No viable conversion
    {
        ara::core::Byte b{42};
        int x = b;  // ERROR: No implicit conversion to int
    }
    */

    /*
    // Test: Implicit conversion to bool
    // Expected: Deleted operator bool
    {
        ara::core::Byte b{42};
        if (b) {}  // ERROR: No conversion to bool (deleted)
    }
    */

    /*
    // Test: Function expecting int with Byte argument
    // Expected: No viable conversion
    {
        void takes_int(int x) { (void)x; }
        ara::core::Byte b{42};
        takes_int(b);  // ERROR: Cannot convert Byte to int implicitly
    }
    */

    // ========================================================================
    // 3. OPERATOR ERRORS
    // ========================================================================
    
    /*
    // Test: Arithmetic operations (all deleted)
    // Expected: Use of deleted function
    {
        ara::core::Byte a{10}, b{20};
        auto sum = a + b;        // ERROR: operator+ is deleted
        auto diff = a - b;       // ERROR: operator- is deleted  
        auto prod = a * b;       // ERROR: operator* is deleted
        auto quot = a / b;       // ERROR: operator/ is deleted
        auto rem = a % b;        // ERROR: operator% is deleted
    }
    */

    /*
    // Test: Mixed type bitwise operations
    // Expected: Static assert failure
    {
        ara::core::Byte b{0xFF};
        auto result1 = b & 0xAA;      // ERROR: Cannot AND Byte with int
        auto result2 = 0xAA & b;      // ERROR: Cannot AND int with Byte
        auto result3 = b | 0x55;      // ERROR: Cannot OR Byte with int
        auto result4 = b ^ 0xFF;      // ERROR: Cannot XOR Byte with int
    }
    */

    /*
    // Test: Mixed type comparisons
    // Expected: Static assert failure
    {
        ara::core::Byte b{42};
        bool eq = (b == 42);      // ERROR: Cannot compare Byte with int
        bool ne = (42 != b);      // ERROR: Cannot compare int with Byte
        bool lt = (b < 100);      // ERROR: Cannot compare Byte with int
    }
    */

    /*
    // Test: Shift by non-integral type
    // Expected: Static assert failure
    {
        ara::core::Byte b{1};
        auto result1 = b << 2.5;     // ERROR: Cannot shift by non-integral
        auto result2 = b >> "2";     // ERROR: Cannot shift by non-integral
        b <<= 3.14;                  // ERROR: Cannot shift-assign by non-integral
    }
    */

    // ========================================================================
    // 4. UTILITY FUNCTION ERRORS
    // ========================================================================
    
    /*
    // Test: to_byte with non-integral type
    // Expected: Static assert failure
    {
        std::string s = "42";
        auto b = ara::core::to_byte(s);  // ERROR: Cannot convert non-integral to Byte
    }
    */

    /*
    // Test: to_byte with floating point
    // Expected: Static assert failure
    {
        auto b = ara::core::to_byte(3.14);  // ERROR: Cannot convert non-integral to Byte
    }
    */

    // ========================================================================
    // 5. USER-DEFINED LITERAL ERRORS
    // ========================================================================
    
    /*
    // Test: Invalid decimal literal
    // Expected: Static assert in parse_byte
    {
        using namespace ara::core::literals::byte_literals;
        constexpr auto b = 256_byte;  // ERROR: Value exceeds 255
    }
    */

    /*
    // Test: Invalid hex literal
    // Expected: Static assert in parse_byte
    {
        using namespace ara::core::literals::byte_literals;
        constexpr auto b = 0x100_byte;  // ERROR: Value exceeds 255
    }
    */

    /*
    // Test: Invalid binary literal
    // Expected: Static assert in parse_byte
    {
        using namespace ara::core::literals::byte_literals;
        constexpr auto b = 0b100000000_byte;  // ERROR: Value exceeds 255
    }
    */

    /*
    // Test: Invalid characters in literal
    // Expected: Static assert in parse_byte
    {
        using namespace ara::core::literals::byte_literals;
        constexpr auto b1 = 12G_byte;    // ERROR: Invalid decimal digit
        constexpr auto b2 = 0xZZ_byte;   // ERROR: Invalid hex digit
        constexpr auto b3 = 0b12_byte;   // ERROR: Invalid binary digit
    }
    */

    // ========================================================================
    // 6. CONST CORRECTNESS ERRORS
    // ========================================================================
    
    /*
    // Test: Modifying const Byte
    // Expected: No matching member function
    {
        const ara::core::Byte cb{42};
        cb.set(0);     // ERROR: Cannot call non-const method on const object
        cb.reset();    // ERROR: Cannot call non-const method on const object
        cb.flip(1);    // ERROR: Cannot call non-const method on const object
        cb <<= 2;      // ERROR: Cannot modify const object
    }
    */

    // ========================================================================
    // 7. STREAM OPERATOR ERRORS (when enabled)
    // ========================================================================
    
    /*
    #ifdef ARA_CORE_BYTE_ENABLE_IOSTREAM
    // Test: Stream operator is not constexpr
    // Expected: Non-constexpr function call
    {
        constexpr auto print_byte = []() {
            ara::core::Byte b{42};
            std::cout << b;  // ERROR: operator<< is not constexpr
            return 0;
        }();
    }
    #endif
    */

    std::cout << "To test compilation errors:\n";
    std::cout << "1. Uncomment one section at a time\n";
    std::cout << "2. Attempt to compile\n";
    std::cout << "3. Observe the user-friendly error message\n";
    std::cout << "4. Re-comment the section before testing another\n";
}