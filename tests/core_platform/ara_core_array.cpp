/**********************************************************************************************************************
 *  PROJECT
 *  -------------------------------------------------------------------------------------------------------------------
 *  \verbatim
 *  OpenAA: Open Source Adaptive AUTOSAR Project
 *  \endverbatim
 *  -------------------------------------------------------------------------------------------------------------------
 *  FILE DESCRIPTION
 *  -------------------------------------------------------------------------------------------------------------------
 *  \file       ara_core_array.cpp
 *  \brief      Comprehensive test application for the ara::core::Array template class.
 *
 *  \details    This file contains multiple test functions covering a wide range of scenarios:
 *              1.  Element access (checked vs. unchecked) and iteration
 *              2.  get<I>()
 *              3.  swap() and fill()
 *              4.  Comparison operators
 *              5.  Usage with user-defined class
 *              6.  Usage with user-defined struct
 *              7.  Copy and move semantics
 *              8.  Const correctness
 *              9.  Violation handling (out-of-range)
 *              10. Zero-sized arrays
 *              11. Reverse iterators
 *              12. Partial initialization
 *              13. Negative scenarios (compile-time & run-time) - commented out by default
 *              14. Two-dimensional (nested) arrays
 *
 *  \note       All variables are used, preventing compiler warnings about unused variables.
 *********************************************************************************************************************/

#if defined(__linux__)
#include <sys/prctl.h>
#endif
#include "ara/core/array.h" // The custom Array implementation header
#include <iostream>         // For std::cout (demonstrations)
#include <string>           // For std::string
#include <cassert>          // For runtime checks via assert

static constexpr std::string_view   kProcessNameView{"CoreArrayTest"};
static constexpr std::uint8_t       kMaxProcessName{15};

/**********************************************************************************************************************
 *  FORWARD DECLARATIONS
 *********************************************************************************************************************/
void TestElementAccessAndIterators();  // Test #1
void TestGetFunction();                // Test #2
void TestSwapAndFill();                // Test #3
void TestComparisonOperators();        // Test #4
void TestWithUserDefinedClass();       // Test #5
void TestWithUserDefinedStruct();      // Test #6
void TestCopyAndMoveSemantics();       // Test #7
void TestConstCorrectness();           // Test #8
void TestViolationHandling();          // Test #9
void TestZeroSizedArray();             // Test #10
void TestReverseIterators();           // Test #11
void TestPartialInitialization();      // Test #12
void TestNegativeScenarios();          // Test #13 (commented code)
void TestTwoDimensionalArrays();       // Test #14

/**********************************************************************************************************************
 *  DEMO TYPES FOR TESTING
 *********************************************************************************************************************/

/*!
 * \brief  A sample user-defined class to test copy, move, and comparison inside ara::core::Array
 *
 * \details
 * - All copy/move constructors and assignments are marked noexcept to satisfy Safe Mode's requirements.
 */
class SafeTestClass
{
public:
    // Default constructor
    SafeTestClass() noexcept : value_(0) {
        std::cout << "[SafeTestClass] Default Constructor\n";
    }

    // Parameterized constructor
    explicit SafeTestClass(int value) noexcept : value_(value) {
        std::cout << "[SafeTestClass] Param Constructor with value " << value << "\n";
    }

    // Copy constructor
    SafeTestClass(const SafeTestClass& other) noexcept : value_(other.value_) {
        std::cout << "[SafeTestClass] Copy Constructor\n";
    }

    // Move constructor
    SafeTestClass(SafeTestClass&& other) noexcept : value_(other.value_) {
        other.value_ = 0;
        std::cout << "[SafeTestClass] Move Constructor\n";
    }

    // Copy assignment
    SafeTestClass& operator=(const SafeTestClass& other) noexcept {
        if (this != &other) {
            value_ = other.value_;
            std::cout << "[SafeTestClass] Copy Assignment\n";
        }
        return *this;
    }

    // Move assignment
    SafeTestClass& operator=(SafeTestClass&& other) noexcept {
        if (this != &other) {
            value_ = other.value_;
            other.value_ = 0;
            std::cout << "[SafeTestClass] Move Assignment\n";
        }
        return *this;
    }

    // Comparisons
    bool operator==(const SafeTestClass& rhs) const { return (value_ == rhs.value_); }
    bool operator!=(const SafeTestClass& rhs) const { return !(*this == rhs); }
    bool operator<(const SafeTestClass& rhs) const { return value_ < rhs.value_; }
    bool operator>(const SafeTestClass& rhs) const { return rhs < *this; }
    bool operator<=(const SafeTestClass& rhs) const { return !(rhs < *this); }
    bool operator>=(const SafeTestClass& rhs) const { return !(*this < rhs); }

    // Accessor
    int GetValue() const { return value_; }

private:
    int value_;
};

/*!
 * \brief  A sample user-defined struct to test custom types in ara::core::Array
 *
 * \details
 * - All copy/move operations are implicitly noexcept as they involve only noexcept operations.
 * - Uses only types that have noexcept copy/move operations (e.g., int).
 */
struct SafeTestStruct
{
    int         id;
    int         score;

    // Default constructor
    SafeTestStruct() noexcept : id(0), score(0) {
        std::cout << "[SafeTestStruct] Default Constructor\n";
    }

    // Parameterized constructor
    SafeTestStruct(int id_, int score_) noexcept : id(id_), score(score_) {
        std::cout << "[SafeTestStruct] Param Constructor with id=" << id_ << ", score=" << score_ << "\n";
    }

    // Copy constructor
    SafeTestStruct(const SafeTestStruct& other) noexcept : id(other.id), score(other.score) {
        std::cout << "[SafeTestStruct] Copy Constructor\n";
    }

    // Move constructor
    SafeTestStruct(SafeTestStruct&& other) noexcept : id(other.id), score(other.score) {
        other.id = 0;
        other.score = 0;
        std::cout << "[SafeTestStruct] Move Constructor\n";
    }

    // Copy assignment
    SafeTestStruct& operator=(const SafeTestStruct& other) noexcept {
        if (this != &other) {
            id = other.id;
            score = other.score;
            std::cout << "[SafeTestStruct] Copy Assignment\n";
        }
        return *this;
    }

    // Move assignment
    SafeTestStruct& operator=(SafeTestStruct&& other) noexcept {
        if (this != &other) {
            id = other.id;
            score = other.score;
            other.id = 0;
            other.score = 0;
            std::cout << "[SafeTestStruct] Move Assignment\n";
        }
        return *this;
    }

    // Comparisons
    bool operator==(const SafeTestStruct& rhs) const {
        return (id == rhs.id) && (score == rhs.score);
    }
    bool operator!=(const SafeTestStruct& rhs) const {
        return !(*this == rhs);
    }
    bool operator<(const SafeTestStruct& rhs) const {
        if (id != rhs.id) {
            return id < rhs.id;
        }
        return score < rhs.score;
    }
    bool operator>(const SafeTestStruct& rhs) const {
        return rhs < *this;
    }
    bool operator<=(const SafeTestStruct& rhs) const {
        return !(rhs < *this);
    }
    bool operator>=(const SafeTestStruct& rhs) const {
        return !(*this < rhs);
    }
};

/**********************************************************************************************************************
 *  MAIN AND MENU
 *********************************************************************************************************************/
static void PrintUsage(const char* prog) {
    std::cout << "Usage: " << prog << " [test_number]\n"
              << "List of Available Tests:\n"
              << "  1  - Element Access and Iterators\n"
              << "  2  - get<I>() Functionality\n"
              << "  3  - Swap and Fill\n"
              << "  4  - Comparison Operators\n"
              << "  5  - Usage with User-Defined Class\n"
              << "  6  - Usage with User-Defined Struct\n"
              << "  7  - Copy and Move Semantics\n"
              << "  8  - Const Correctness\n"
              << "  9  - Violation Handling (Out-of-Range)\n"
              << " 10  - Zero-Sized Array\n"
              << " 11  - Reverse Iterators\n"
              << " 12  - Partial Initialization\n"
              << " 13  - Negative Scenarios (commented out)\n"
              << " 14  - Two-Dimensional Arrays\n";
}

int main(int argc, char* argv[])
{
    if (argc != 2) {
        PrintUsage(argv[0]);
        return 1;
    }
    static_assert(kProcessNameView.size() <= kMaxProcessName,
        "\n[ERROR] Process name is too long!!\n");

    #if defined(__linux__)
        prctl(PR_SET_NAME, kProcessNameView.data(), 0, 0, 0);
    #elif defined(__QNXNTO__)
        // On QNX, use pthread_setname_np() to set the name of the calling thread (main thread).
        // This name will be used as the process name.
        pthread_setname_np(pthread_self(), kProcessNameView.data());
    #endif

    std::string choice = argv[1];
    if      (choice == "1")  TestElementAccessAndIterators();
    else if (choice == "2")  TestGetFunction();
    else if (choice == "3")  TestSwapAndFill();
    else if (choice == "4")  TestComparisonOperators();
    else if (choice == "5")  TestWithUserDefinedClass();
    else if (choice == "6")  TestWithUserDefinedStruct();
    else if (choice == "7")  TestCopyAndMoveSemantics();
    else if (choice == "8")  TestConstCorrectness();
    else if (choice == "9")  TestViolationHandling();
    else if (choice == "10") TestZeroSizedArray();
    else if (choice == "11") TestReverseIterators();
    else if (choice == "12") TestPartialInitialization();
    else if (choice == "13") TestNegativeScenarios();
    else if (choice == "14") TestTwoDimensionalArrays();
    else {
        std::cout << "Invalid test number: " << choice << "\n";
        PrintUsage(argv[0]);
        return 1;
    }

    return 0;
}

/**********************************************************************************************************************
 *  TEST DEFINITIONS
 *********************************************************************************************************************/

/*!
 * \brief Test #1: Element Access and Iterators (both forward and range-based)
 */
void TestElementAccessAndIterators()
{
    std::cout << "\n=== Test 1: Element Access and Iterators ===\n";
    ara::core::Array<int,5> arr = {10, 20, 30, 40, 50};

    // at() => checked access
    std::cout << "arr.at(2) = " << arr.at(2) << " (expected 30)\n";
    assert(arr.at(2) == 30);

    // operator[] => unchecked
    std::cout << "arr[0] = " << arr[0] << " (expected 10)\n";
    assert(arr[0] == 10);

    // Forward iteration using iterators
    std::cout << "Forward iteration: ";
    int sum = 0;
    for (auto it = arr.begin(); it != arr.end(); ++it) {
        std::cout << *it << " ";
        sum += *it;
    }
    
    std::cout << "\nSum of elements = " << sum << " (expected 150)\n";
    assert(sum == 150);
}

/*!
 * \brief Test #2: get<I>() functionality
 */
void TestGetFunction()
{
    std::cout << "\n=== Test 2: get<I>() Functionality ===\n";
    
    #ifdef ARA_CORE_ARRAY_ENABLE_CONDITIONAL_EXCEPTIONS
        // Conditional Safe Mode: Use types that may throw
        ara::core::Array<std::string,3> strArr = {"Alpha", "Beta", "Gamma"};

        std::cout << "get<0>(strArr) => " << ara::core::get<0>(strArr) << "\n";
        assert(ara::core::get<0>(strArr) == "Alpha");

        std::cout << "get<2>(strArr) => " << ara::core::get<2>(strArr) << "\n";
        assert(ara::core::get<2>(strArr) == "Gamma");
    #else
        // Safe Mode: Types must not throw
        std::cout << "[Test Skipped] get<I>() with potentially-throwing types is not available in Safe Mode.\n";
    #endif
}

/*!
 * \brief Test #3: Swap and Fill
 */
void TestSwapAndFill()
{
    /* Compile-time test using a constexpr lambda and static_assert.
       This lambda creates two arrays, swaps them, fills one with 100,
       and then returns true if the operations produced the expected results.
    */
    static_assert([]() constexpr -> bool {
        ara::core::Array<int, 4> arr1 = {1, 2, 3, 4};
        ara::core::Array<int, 4> arr2 = {5, 6, 7, 8};
        swap(arr1, arr2);
        arr1.fill(100);
        return (arr1[0] == 100 && arr1[1] == 100 &&
                arr1[2] == 100 && arr1[3] == 100 &&
                arr2[0] == 1 && arr2[1] == 2 &&
                arr2[2] == 3 && arr2[3] == 4);
    }(), 
        "\n[ERROR]: constexpr swap and fill test failed.\n");

    /* Runtime test code: */
    std::cout << "\n=== Test 3: Swap and Fill ===\n";
    ara::core::Array<int, 4> arr1 = {1, 2, 3, 4};
    ara::core::Array<int, 4> arr2 = {5, 6, 7, 8};

    std::cout << "arr1 before swap: ";
    for (auto i : arr1)
        std::cout << i << " ";
    std::cout << "\narr2 before swap: ";
    for (auto i : arr2)
        std::cout << i << " ";

    // Perform swap
    swap(arr1, arr2);

    std::cout << "\narr1 after swap: ";
    for (auto i : arr1)
        std::cout << i << " ";
    std::cout << "\narr2 after swap: ";
    for (auto i : arr2)
        std::cout << i << " ";

    // Fill arr1 with 100
    arr1.fill(100);
    std::cout << "\narr1 after fill(100): ";
    for (auto i : arr1)
    {
        std::cout << i << " ";
        assert(i == 100);
    }
    std::cout << "\n";
}


/*!
 * \brief Test #4: Comparison Operators (==, !=, <, <=, >, >=)
 */
void TestComparisonOperators()
{
    std::cout << "\n=== Test 4: Comparison Operators ===\n";
    ara::core::Array<int,3> arrayA = {1,2,3};
    ara::core::Array<int,3> arrayB = {1,2,3};
    ara::core::Array<int,3> arrayC = {1,2,4};

    // Checking equality
    std::cout << "arrayA == arrayB => " << (arrayA == arrayB) << " (expected true)\n";
    assert(arrayA == arrayB);

    std::cout << "arrayA != arrayC => " << (arrayA != arrayC) << " (expected true)\n";
    assert(arrayA != arrayC);

    // Checking < and <=
    std::cout << "arrayA < arrayC  => " << (arrayA < arrayC) << " (expected true)\n";
    assert(arrayA < arrayC);

    std::cout << "arrayA <= arrayB => " << (arrayA <= arrayB) << " (expected true)\n";
    assert(arrayA <= arrayB);

    // Checking > and >=
    std::cout << "arrayC > arrayA  => " << (arrayC > arrayA) << " (expected true)\n";
    assert(arrayC > arrayA);

    std::cout << "arrayC >= arrayA => " << (arrayC >= arrayA) << " (expected true)\n";
    assert(arrayC >= arrayA);

    constexpr ara::core::Array<int,3> CompileTimearrayA = {1,2,3};
    constexpr ara::core::Array<int,3> CompileTimearrayB = {1,2,3};
    constexpr ara::core::Array<int,3> CompileTimearrayC = {1,2,4};

    // Checking equality
    std::cout << "CompileTimearrayA == CompileTimearrayB => " << (CompileTimearrayA == CompileTimearrayB) << " (expected true)\n";
    static_assert(CompileTimearrayA == CompileTimearrayB,
        "\n[ERROR] in ara::core::Array: Arrays are not CompileTimearrayA == CompileTimearrayB at compile time!\n"); 
    
    std::cout << "CompileTimearrayA != CompileTimearrayC => " << (CompileTimearrayA != CompileTimearrayC) << " (expected true)\n";
    static_assert(CompileTimearrayA == CompileTimearrayB,
        "\n[ERROR] in ara::core::Array:Arrays are not CompileTimearrayA != CompileTimearrayC at compile time!\n"); 
 
    // Checking < and <=
    std::cout << "CompileTimearrayA < CompileTimearrayB  => " << (CompileTimearrayA < CompileTimearrayC) << " (expected true)\n";
    static_assert(CompileTimearrayA < CompileTimearrayC,
        "\n[ERROR] in ara::core::Array:Arrays are not CompileTimearrayA < CompileTimearrayC at compile time!\n"); 

    std::cout << "CompileTimearrayA <= CompileTimearrayB => " << (CompileTimearrayA <= CompileTimearrayB) << " (expected true)\n";
    static_assert(CompileTimearrayA <= CompileTimearrayB,
        "\n[ERROR] in ara::core::Array:Arrays are not CompileTimearrayA <= CompileTimearrayB at compile time!\n"); 

    // Checking > and >=
    std::cout << "CompileTimearrayC > CompileTimearrayA  => " << (CompileTimearrayC > CompileTimearrayA) << " (expected true)\n";
    static_assert(CompileTimearrayC > CompileTimearrayA,
        "\n[ERROR] in ara::core::Array:Arrays are not CompileTimearrayC > CompileTimearrayA at compile time!\n"); 

    std::cout << "CompileTimearrayC >= CompileTimearrayA => " << (CompileTimearrayC >= CompileTimearrayA) << " (expected true)\n";
    static_assert(CompileTimearrayC >= CompileTimearrayA,
        "\n[ERROR] in ara::core::Array:Arrays are not CompileTimearrayC >= CompileTimearrayA at compile time!\n"); 
        
}

/*!
 * \brief Test #5: Usage with a user-defined class
 */
void TestWithUserDefinedClass()
{
    std::cout << "\n=== Test 5: Usage with User-Defined Class ===\n";
    ara::core::Array<SafeTestClass,3> classArr = { SafeTestClass(10), SafeTestClass(20), SafeTestClass(30) };

    // Check middle element
    auto middleVal = classArr.at(1).GetValue();
    std::cout << "Middle element's value => " << middleVal << " (expected 20)\n";
    assert(middleVal == 20);

    // Check first element
    assert(classArr[0].GetValue() == 10);

    // Summation
    int sum = 0;
    for (auto& obj : classArr) {
        sum += obj.GetValue();
    }
    std::cout << "Sum of all values => " << sum << " (expected 60)\n";
    assert(sum == 60);
}

/*!
 * \brief Test #6: Usage with a user-defined struct
 */
void TestWithUserDefinedStruct()
{
    std::cout << "\n=== Test 6: Usage with User-Defined Struct ===\n";
    ara::core::Array<SafeTestStruct,3> structArr = {
        SafeTestStruct{1, 95},
        SafeTestStruct{2, 88},
        SafeTestStruct{3, 76}
    };

    // Verify second element
    SafeTestStruct& secondRef = structArr.at(1);
    std::cout << "structArr[1] => ID=" << secondRef.id << ", Score=" << secondRef.score << "\n";
    assert(secondRef.id == 2 && secondRef.score == 88);

    // Print all
    for (size_t i = 0; i < structArr.size(); ++i) {
        std::cout << "structArr[" << i << "] => (ID=" 
                  << structArr[i].id << ", Score=" << structArr[i].score << ")\n";
    }

    #ifdef ARA_CORE_ARRAY_ENABLE_CONDITIONAL_EXCEPTIONS
        // Conditional Safe Mode: Additional tests with std::string
        std::cout << "\n=== Additional Test: Usage with std::string in Conditional Safe Mode ===\n";
        ara::core::Array<std::string,3> strArr = {"Alpha", "Beta", "Gamma"};

        // Check elements
        std::cout << "strArr[0] = " << strArr[0] << " (expected Alpha)\n";
        assert(strArr[0] == "Alpha");

        std::cout << "strArr[1] = " << strArr[1] << " (expected Beta)\n";
        assert(strArr[1] == "Beta");

        std::cout << "strArr[2] = " << strArr[2] << " (expected Gamma)\n";
        assert(strArr[2] == "Gamma");
    #endif
}

/*!
 * \brief Test #7: Copy and Move Semantics
 */
void TestCopyAndMoveSemantics()
{
    std::cout << "\n=== Test 7: Copy and Move Semantics ===\n";
    ara::core::Array<SafeTestClass, 2> original = { SafeTestClass(100), SafeTestClass(200) };

    // Copy constructor
    std::cout << "[Copy Constructor]\n";
    ara::core::Array<SafeTestClass, 2> copied = original;
    // *** Use 'copied' to avoid warnings ***
    std::cout << "copied[0].GetValue() => " << copied[0].GetValue()
              << " (expected 100)\n";
    std::cout << "copied[1].GetValue() => " << copied[1].GetValue()
              << " (expected 200)\n";
    assert(copied[0].GetValue() == 100);
    assert(copied[1].GetValue() == 200);

    // Move constructor
    std::cout << "[Move Constructor]\n";
    ara::core::Array<SafeTestClass, 2> moved = std::move(original);
    std::cout << "moved[0].GetValue() => " << moved[0].GetValue()
              << " (expected 100)\n";
    std::cout << "moved[1].GetValue() => " << moved[1].GetValue()
              << " (expected 200)\n";
    assert(moved[0].GetValue() == 100);
    assert(moved[1].GetValue() == 200);

    // original is now in a "moved-from" state; accessing its elements is safe but may have default values
    std::cout << "original[0].GetValue() after move => " << original[0].GetValue()
              << " (expected 0)\n";
    std::cout << "original[1].GetValue() after move => " << original[1].GetValue()
              << " (expected 0)\n";
    assert(original[0].GetValue() == 0);
    assert(original[1].GetValue() == 0);

    // Copy assignment
    std::cout << "[Copy Assignment]\n";
    ara::core::Array<SafeTestClass, 2> copyAssigned;
    copyAssigned = moved;
    std::cout << "copyAssigned[0].GetValue() => " << copyAssigned[0].GetValue()
              << " (expected 100)\n";
    std::cout << "copyAssigned[1].GetValue() => " << copyAssigned[1].GetValue()
              << " (expected 200)\n";
    assert(copyAssigned[0].GetValue() == 100);
    assert(copyAssigned[1].GetValue() == 200);

    // Move assignment
    std::cout << "[Move Assignment]\n";
    ara::core::Array<SafeTestClass, 2> moveAssigned;
    moveAssigned = std::move(copyAssigned);
    std::cout << "moveAssigned[0].GetValue() => " << moveAssigned[0].GetValue()
              << " (expected 100)\n";
    std::cout << "moveAssigned[1].GetValue() => " << moveAssigned[1].GetValue()
              << " (expected 200)\n";
    assert(moveAssigned[0].GetValue() == 100);
    assert(moveAssigned[1].GetValue() == 200);

    // copyAssigned is now in a "moved-from" state
    std::cout << "copyAssigned[0].GetValue() after move => " << copyAssigned[0].GetValue()
              << " (expected 0)\n";
    std::cout << "copyAssigned[1].GetValue() after move => " << copyAssigned[1].GetValue()
              << " (expected 0)\n";
    assert(copyAssigned[0].GetValue() == 0);
    assert(copyAssigned[1].GetValue() == 0);
}

/*!
 * \brief Test #8: Const Correctness
 */
void TestConstCorrectness()
{
    /* Compile-time test for the constexpr get() method.
     * A lambda is defined and immediately invoked in a static_assert.
     * If the get method is not constexpr-friendly, compilation will fail.
     */
    static_assert([]() constexpr -> bool {
        const ara::core::Array<int, 3> arr = {7, 8, 9};
        return ara::core::get<2>(arr) == 9;
    }(),
        "\n[ERROR]: Compile-time test for get<2>(const Array<int,3>) failed.\n");

    // Runtime tests:
    std::cout << "\n=== Test 8: Const Correctness ===\n";
    const ara::core::Array<int, 3> constArr = {7, 8, 9};

    std::cout << "constArr.at(1) => " << constArr.at(1) << " (expected 8)\n";
    assert(constArr.at(1) == 8);

    int val2 = ara::core::get<2>(constArr);
    std::cout << "get<2>(constArr) => " << val2 << " (expected 9)\n";
    assert(val2 == 9);

    // Iterate over constArr and sum the elements.
    int sum = 0;
    for (auto it = constArr.begin(); it != constArr.end(); ++it) {
        sum += *it;
    }
    std::cout << "sum of constArr => " << sum << " (expected 24)\n";
    assert(sum == 24);

    // Attempting to modify constArr would result in a compile error:
    // constArr[0] = 999;
}


/*!
 * \brief Test #9: Violation Handling (Out-of-Range)
 */
void TestViolationHandling()
{
    std::cout << "\n=== Test 9: Violation Handling ===\n";
    ara::core::Array<int,3> arr = {10, 20, 30};

    // valid
    std::cout << "arr.at(2) = " << arr.at(2) << " (expected 30)\n";
    assert(arr.at(2) == 30);

    // This next call should trigger a violation (and terminate the process)
    std::cout << "Attempting arr.at(3) => out-of-range => violation.\n";
    arr.at(3); 
}

/*!
 * \brief Test #10: Zero-Sized Array
 */
void TestZeroSizedArray()
{
    std::cout << "\n=== Test 10: Zero-Sized Array ===\n";
    ara::core::Array<int,0> emptyArr;

    std::cout << "emptyArr.size() => " << emptyArr.size() << " (expected 0)\n";
    assert(emptyArr.size() == 0);

    std::cout << "emptyArr.empty() => " << (emptyArr.empty() ? "true" : "false") << " (expected true)\n";
    assert(emptyArr.empty());

    assert(emptyArr.begin() == emptyArr.end());
    assert(emptyArr.data() == nullptr);

    // fill => no-op
    emptyArr.fill(42);
    assert(emptyArr.data() == nullptr);
    std::cout << "Called fill(42) on zero-sized => no effect.\n";
}

/*!
 * \brief Test #11: Reverse Iterators
 */
void TestReverseIterators()
{
    std::cout << "\n=== Test 11: Reverse Iterators ===\n";
    ara::core::Array<int,5> arr = {100, 200, 300, 400, 500};

    // forward iteration
    std::cout << "Forward: ";
    for (auto x : arr) {
        std::cout << x << " ";
    }
    std::cout << "\n";

    // reverse iteration
    std::cout << "Reverse: ";
    for (auto rit = arr.rbegin(); rit != arr.rend(); ++rit) {
        std::cout << *rit << " ";
    }
    std::cout << "\n";

    // const reverse iteration
    const auto& cRef = arr;
    std::cout << "Const Reverse: ";
    for (auto crit = cRef.crbegin(); crit != cRef.crend(); ++crit) {
        std::cout << *crit << " ";
    }
    std::cout << "\n";
}

/*!
 * \brief Test #12: Partial Initialization
 */
void TestPartialInitialization()
{
    std::cout << "\n=== Test 12: Partial Initialization ===\n";
    ara::core::Array<int,5> partialArr = {1, 2}; // rest default => 0,0,0

    for (size_t i = 0; i < partialArr.size(); ++i) {
        std::cout << "Index " << i << " => " << partialArr[i] << "\n";
    }
    // Checks
    assert(partialArr[0] == 1);
    assert(partialArr[1] == 2);
    assert(partialArr[2] == 0);
    assert(partialArr[3] == 0);
    assert(partialArr[4] == 0);
}

//--------------------------------------------------------------------------------------------------
// 13. Negative Scenarios (some compile-time, some run-time)
//--------------------------------------------------------------------------------------------------
void TestNegativeScenarios() {
    std::cout << "\n=== Test 13: Negative Scenarios ===\n";

    // ------------------------------------------------------------------------------
    // 1) Too many arguments to constructor => compile-time error:
    // ------------------------------------------------------------------------------
    /*
    // Description:
    // Attempting to initialize an Array with more elements than its size.
    // Expected Outcome:
    // Compile-time error due to exceeding the maximum number of allowed arguments.
    ara::core::Array<int,3> tooManyArgs(1, 2, 3, 4); // Error: Too many arguments
    */


    // ------------------------------------------------------------------------------
    // 2) get<I> with I >= N => compile-time error:
    // ------------------------------------------------------------------------------
    /*
    // Description:
    // Attempting to access an element with a compile-time index out of bounds.
    // Expected Outcome:
    // Compile-time error triggered by static_assert in get<I>().
    ara::core::Array<int,3> myArray = {10, 20, 30};
    int x = ara::core::get<3>(myArray); // Error: get<3>() out of range
    */
    // ------------------------------------------------------------------------------
    // 3) Attempting swap with different-sized arrays => compile-time error:
    // ------------------------------------------------------------------------------
    /*
    // Description:
    // Attempting to swap two Arrays of different sizes.
    // Expected Outcome:
    // Compile-time error triggered by static_assert in swap function.
    ara::core::Array<int,3> array3 = {1, 2, 3};
    ara::core::Array<int,4> array4 = {1, 2, 3, 4};
    swap(array3, array4); // Error: Cannot swap arrays of different type or size
    */

    // ------------------------------------------------------------------------------
    // 4) Out-of-range index on at() => run-time violation:
    // ------------------------------------------------------------------------------
    /*
    // Description:
    // Attempting to access an element with a run-time index out of bounds using at().
    // Expected Outcome:
    // Runtime termination with a violation message.
    ara::core::Array<int,2> arrSmall = {5, 6};
    std::cout << "arrSmall.at(2) => should trigger out-of-range violation...\n";
    arrSmall.at(2); // Runtime Error: Array access out of range
    */
    
    // ------------------------------------------------------------------------------
    // 5) Wrong data type in constructor => compile-time error:
    // ------------------------------------------------------------------------------
    /*
    // Description:
    // Attempting to initialize an Array with elements of an incompatible type.
    // Expected Outcome:
    // Compile-time error due to type mismatch.
    ara::core::Array<int,2> typeMismatch("Hello", "World"); // Error: Cannot convert std::string to int
    */

    // ------------------------------------------------------------------------------
    // 6) Copy-constructing an Array from a different T => compile-time error:
    // ------------------------------------------------------------------------------
    /*
    // Description:
    // Attempting to copy-construct an Array of one type from an Array of a different type.
    // Expected Outcome:
    // Compile-time error due to type mismatch.
    ara::core::Array<int,3> intArray3 = {1, 2, 3};
    ara::core::Array<double,3> copyOfInts(intArray3); // Error: No matching constructor
    */
    // ------------------------------------------------------------------------------
    // 7) Copy-assigning an Array from a different T => compile-time error:
    // ------------------------------------------------------------------------------
    /*
    // Description:
    // Attempting to copy-assign an Array of one type from an Array of a different type.
    // Expected Outcome:
    // Compile-time error due to type mismatch.
    ara::core::Array<int,3> intArrayA = {10, 20, 30};
    ara::core::Array<double,3> dblArrayB = {1.5, 2.5, 3.5};
    dblArrayB = intArrayA;  // Error: Cannot assign Array<int,3> to Array<double,3>
    */

    // ------------------------------------------------------------------------------
    // 8) Copy-constructing or assigning an Array from a different size => compile-time error:
    // ------------------------------------------------------------------------------
    /*
    // Description:
    // Attempting to copy-construct or copy-assign an Array from an Array of the same type but different size.
    // Expected Outcome:
    // Compile-time error due to size mismatch.
    ara::core::Array<int,3> arrSize3 = {1, 2, 3};
    ara::core::Array<int,4> arrSize4 = {5, 6, 7, 8};
    
    // Copy constructor scenario:
    ara::core::Array<int,4> copyFrom3(arrSize3);  // Error: No matching constructor
    
    // Copy assignment scenario:
    arrSize4 = arrSize3;  // Error: Cannot assign Array<int,3> to Array<int,4>
    */

    // ------------------------------------------------------------------------------
    // 9) Move-constructing an Array from a different T => compile-time error:
    // ------------------------------------------------------------------------------
    /*
    // Description:
    // Attempting to move-construct an Array of one type from an Array of a different type.
    // Expected Outcome:
    // Compile-time error due to type mismatch.
    ara::core::Array<int,3> intArray3 = {1, 2, 3};
    ara::core::Array<double,3> moveCopy(std::move(intArray3)); // Error: No matching constructor
    */

    // ------------------------------------------------------------------------------
    // 10) Move-assigning an Array from a different T => compile-time error:
    // ------------------------------------------------------------------------------
    /*
    // Description:
    // Attempting to move-assign an Array of one type from an Array of a different type.
    // Expected Outcome:
    // Compile-time error due to type mismatch.
    ara::core::Array<int,3> intArrayA = {10, 20, 30};
    ara::core::Array<double,3> dblArrayB = {1.5, 2.5, 3.5};
    dblArrayB = std::move(intArrayA); // Error: Cannot assign Array<int,3> to Array<double,3>
    */
    
    // ------------------------------------------------------------------------------
    // 11) Swapping Arrays of Different Types or Sizes => compile-time error:
    // ------------------------------------------------------------------------------
    /*
    // Description:
    // Attempting to swap two Arrays where either the element types or sizes differ.
    // Expected Outcome:
    // Compile-time error due to type or size mismatch.
    ara::core::Array<int,3> array3 = {1, 2, 3};
    ara::core::Array<double,4> array4 = {1.1, 2.2, 3.3, 4.4};
    
    // Different type:
    swap(array3, array4); // Error: Cannot swap arrays of different type or size
    
    // Different size with same type:
    ara::core::Array<int,4> array4SameType = {4, 5, 6, 7};
    swap(array3, array4SameType); // Error: Cannot swap arrays of different type or size
    */

    // ------------------------------------------------------------------------------
    // 12) Swapping zero-sized Array with non-zero-sized Array => compile-time error:
    // ------------------------------------------------------------------------------
    /*
    // Description:
    // Attempting to swap a zero-sized Array with a non-zero-sized Array.
    // Expected Outcome:
    // Compile-time error due to size mismatch.
    ara::core::Array<int,0> emptyArray;
    ara::core::Array<int,3> nonEmptyArray = {1, 2, 3};
    swap(emptyArray, nonEmptyArray); // Error: Cannot swap arrays of different type or size
    */

    // ------------------------------------------------------------------------------
    // 13) Attempting to access front() or back() on zero-sized Array => compile-time error:
    // ------------------------------------------------------------------------------
    /*
    // Description:
    // Attempting to access front() or back() on an Array with size 0.
    // Expected Outcome:
    // Compile-time error triggered by static_assert in front() and back().
    ara::core::Array<int,0> emptyArray;
    emptyArray.front(); // Error: front() called on zero-sized Array
    emptyArray.back();  // Error: back() called on zero-sized Array
    */

    // ------------------------------------------------------------------------------
    // 14) Attempting to initialize Array with incompatible types => compile-time error:
    // ------------------------------------------------------------------------------
    /*
    // Description:
    // Initializing Array<int,3> with std::string, which is not convertible to int.
    // Expected Outcome:
    // Compile-time error due to type mismatch.
    ara::core::Array<int,3> invalidInit = {"Hello", "World", "!"}; // Error: Cannot convert std::string to int
    */

    // ------------------------------------------------------------------------------
    // 15) Attempting to initialize Array with initializer list exceeding N => compile-time error:
    // ------------------------------------------------------------------------------
    /*
    // Description:
    // Initializing Array<int,3> with an initializer list that has more elements than N.
    // Expected Outcome:
    // Compile-time error due to exceeding the maximum number of elements.
    ara::core::Array<int,3> arrayExceed = {1, 2, 3, 4}; // Error: Too many arguments
    */

    // ------------------------------------------------------------------------------
    // 16) Attempting to initialize Array with initializer list causing narrowing conversions => compile-time error:
    // ------------------------------------------------------------------------------
    /*
    // Description:
    // Initializing Array<unsigned int,3> with negative values which cause narrowing.
    // Expected Outcome:
    // Compile-time error due to narrowing conversions.
    ara::core::Array<unsigned int,3> arrayNegative = {-1, -2, -3}; // Error: Narrowing conversion from int to unsigned int
    */

    // ------------------------------------------------------------------------------
    // 17) Attempting to initialize Array with mixed convertible and non-convertible types => compile-time error:
    // ------------------------------------------------------------------------------
    /*
    // Description:
    // Initializing Array<int,3> with a mix of convertible (double to int) and non-convertible (std::string to int) types.
    // Expected Outcome:
    // Compile-time error due to type mismatch.
    ara::core::Array<int,3> mixedInit = {1, 2.5, "Three"}; // Error: Cannot convert std::string to int
    */

    // ------------------------------------------------------------------------------
    // 18) Attempting to move-construct Array with incompatible sizes and types => compile-time error:
    // ------------------------------------------------------------------------------
    /*
    // Description:
    // Attempting to move-construct an Array from another Array with different size and type.
    // Expected Outcome:
    // Compile-time error due to type and size mismatch.
    ara::core::Array<int,3> intArray = {1, 2, 3};
    ara::core::Array<double,4> moveCopy(std::move(intArray)); // Error: No matching constructor
    */

    // ------------------------------------------------------------------------------
    // 19) Attempting to move-assign Array with different types and sizes => compile-time error:
    // ------------------------------------------------------------------------------
    /*
    // Description:
    // Attempting to move-assign an Array of one type and size to an Array of another type and size.
    // Expected Outcome:
    // Compile-time error due to type and size mismatch.
    ara::core::Array<int,3> intArrayA = {10, 20, 30};
    ara::core::Array<double,4> dblArrayB = {1.5, 2.5, 3.5, 4.5};
    dblArrayB = std::move(intArrayA); // Error: Cannot assign Array<int,3> to Array<double,4>
    */

    // ------------------------------------------------------------------------------
    // 20) Attempting to initialize Array with incompatible types using move semantics => compile-time error:
    // ------------------------------------------------------------------------------
    /*
    // Description:
    // Attempting to move-assign an Array of one type to an Array of another type.
    // Expected Outcome:
    // Compile-time error due to type mismatch.
    ara::core::Array<int,3> sourceArray = {1, 2, 3};
    ara::core::Array<std::string,3> targetArray;
    targetArray = std::move(sourceArray); // Error: Cannot assign Array<int,3> to Array<std::string,3>
    */

    // ------------------------------------------------------------------------------
    // 21) Negative SWAP scenarios
    // ------------------------------------------------------------------------------
    /*
    // (A) Attempting to swap Arrays of the same T but different sizes => compile-time error
    // Expected Outcome: static_assert or no matching call to swap(...).
    {
        std::cout << "[NEGATIVE] swap: Attempt to swap array of size 3 with array of size 4 => compile-time error.\n";
        
        ara::core::Array<int,3> arrSize3 = {1, 2, 3};
        ara::core::Array<int,4> arrSize4 = {4, 5, 6, 7};

        // The below line should fail to compile:
        swap(arrSize3, arrSize4); 
        // or arrSize3.swap(arrSize4);
    }

    // (B) Attempting to swap Arrays with different T => compile-time error
    // Expected Outcome: static_assert or no matching call to swap(...).
    {
        std::cout << "[NEGATIVE] swap: Attempt to swap array<int,3> with array<double,3> => compile-time error.\n";

        ara::core::Array<int,3>  arrInt  = {1,2,3};
        ara::core::Array<double,3> arrDbl = {1.1,2.2,3.3};

        // The below line should fail to compile:
        swap(arrInt, arrDbl);
    }
    */

    // ------------------------------------------------------------------------------
    // 22) Negative FILL scenarios
    // ------------------------------------------------------------------------------
    /*
    // (A) Attempting to fill an Array of const int => compile-time error
    // Because fill(...) requires copy assignment, but const int is not assignable.
    {
        std::cout << "[NEGATIVE] fill: Attempting to fill an array of const int => compile-time error.\n";

        ara::core::Array<const int,3> constArr = {1, 2, 3};
        // The below line should fail to compile, as fill requires T& to be assignable:
        constArr.fill(42);
    }

    // (B) Attempting to fill an Array whose T is not copy-assignable => compile-time error
    // For instance, we define a type with deleted operator=, then try to fill it.
    {
        std::cout << "[NEGATIVE] fill: Attempting to fill an array with a non-assignable type => compile-time error.\n";

        struct NoAssign {
            NoAssign() noexcept = default;
            NoAssign(const NoAssign&) noexcept = default;
            NoAssign& operator=(const NoAssign&) noexcept = delete; // deleted copy assignment
        };

        ara::core::Array<NoAssign,2> nonAssignableArr;
        // The below line should fail to compile, because fill(...) internally does operator=:
        nonAssignableArr.fill(NoAssign{});
    }
    */

    // ------------------------------------------------------------------------------
    // 23) Safe build
    // ------------------------------------------------------------------------------
    /*
    struct ThrowingComparable {
        int value;

        bool operator==(const ThrowingComparable& other) const { // Not noexcept
            return value == other.value;
        }

        bool operator<(const ThrowingComparable& other) const { // Not noexcept
            return value < other.value;
        }
    };

    // Instantiate ara::core::Array with ThrowingComparable
    ara::core::Array<ThrowingComparable, 2> arr = {ThrowingComparable{1}, ThrowingComparable{2}};

    // Use a comparison operator to trigger static_assert
    bool isEqual = (arr == arr); // This should trigger the static_assert

    std::cout << "Arrays are equal: " << isEqual << std::endl;
    */
    
    std::cout << "(All negative scenarios are currently commented out. "
                 "Uncomment each one individually to observe the intended compile-time or run-time failures.)\n"; 
}

/*!
 * \brief Test #14: Two-Dimensional Arrays
 */
void TestTwoDimensionalArrays()
{
    std::cout << "\n=== Test 14: Two-Dimensional Arrays ===\n";
    // We'll define a 2x3 matrix
    ara::core::Array<ara::core::Array<int,3>, 2> matrix = {
        ara::core::Array<int,3>{1,2,3},
        ara::core::Array<int,3>{4,5} // => partial => {4,5,0}
    };

    // Check row 0
    assert(matrix[0][0] == 1);
    assert(matrix[0][1] == 2);
    assert(matrix[0][2] == 3);

    // Check row 1
    assert(matrix[1][0] == 4);
    assert(matrix[1][1] == 5);
    assert(matrix[1][2] == 0);

    // fill first row with 99
    matrix[0].fill(99);
    assert(matrix[0][0] == 99 && matrix[0][1] == 99 && matrix[0][2] == 99);

    // swap row 0 and row 1
    swap(matrix[0], matrix[1]);
    // now row 0 => {4,5,0}, row 1 => {99,99,99}
    assert(matrix[0][0] == 4 && matrix[1][0] == 99);

    // Print final 2D array
    for (size_t r = 0; r < matrix.size(); ++r) {
        std::cout << "Row " << r << ": ";
        for (size_t c = 0; c < matrix[r].size(); ++c) {
            std::cout << matrix[r][c] << " ";
        }
        std::cout << "\n";
    }

    #ifdef ARA_CORE_ARRAY_ENABLE_CONDITIONAL_EXCEPTIONS
        // Conditional Safe Mode: Additional tests with two-dimensional arrays involving std::string
        std::cout << "\n=== Additional Test: Two-Dimensional Arrays with std::string ===\n";
        ara::core::Array<ara::core::Array<std::string,2>, 2> strMatrix = {
            ara::core::Array<std::string,2>{"Hello", "World"},
            ara::core::Array<std::string,2>{"Foo", "Bar"}
        };

        // Access elements
        std::cout << "strMatrix[0][0] = " << strMatrix[0][0] << " (expected Hello)\n";
        assert(strMatrix[0][0] == "Hello");
        std::cout << "strMatrix[1][1] = " << strMatrix[1][1] << " (expected Bar)\n";
        assert(strMatrix[1][1] == "Bar");

        // Modify elements
        strMatrix[0][1] = "Universe";
        std::cout << "strMatrix[0][1] after modification = " << strMatrix[0][1] << " (expected Universe)\n";
        assert(strMatrix[0][1] == "Universe");
    #endif
}
