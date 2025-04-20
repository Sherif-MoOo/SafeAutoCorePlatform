/**********************************************************************************************************************
 *  PROJECT
 *  -------------------------------------------------------------------------------------------------------------------
 *  \verbatim
 *  OpenAA: Open Source Adaptive AUTOSAR Project (CXX_STANDARD 17)
 *  \endverbatim
 *  -------------------------------------------------------------------------------------------------------------------
 *  FILE DESCRIPTION
 *  -------------------------------------------------------------------------------------------------------------------
 *  \file       ara/core/array.h
 *  \brief      Definition and implementation of the ara::core::Array template class.
 *
 *  \details    This file defines and implements the ara::core::Array template class, a fixed-size array container
 *              designed for the OpenAA project. It provides functionalities similar to std::array with additional
 *              customizations to meet Adaptive AUTOSAR requirements (e.g., [SWS_CORE_00040], [SWS_CORE_13017],
 *              [SWS_CORE_11200], [SWS_CORE_01201], etc.), including violation handling and optimized memory allocation.
 *
 *  \note       Based on the Adaptive AUTOSAR SWS (e.g., R24-11) requirements for the "Array" type, especially:
 *              - [SWS_CORE_01201] (Definition of ara::core::Array)
 *              - [SWS_CORE_01265], [SWS_CORE_01266] (operator[])
 *              - [SWS_CORE_01273], [SWS_CORE_01274] (at())
 *              - [SWS_CORE_01241] (fill())
 *              - [SWS_CORE_00040] (No exceptions used – custom violation handling)
 *              - [SWS_CORE_13017] (Out-of-range message format)
 *              - [SWS_CORE_01290..01295] (comparison operators)
 *********************************************************************************************************************/
 
#ifndef OPEN_AA_ADAPTIVE_AUTOSAR_LIBS_INCLUDE_ARA_CORE_ARRAY_H_
#define OPEN_AA_ADAPTIVE_AUTOSAR_LIBS_INCLUDE_ARA_CORE_ARRAY_H_

/**********************************************************************************************************************
 *  INCLUDES: files that required by the template class
 *********************************************************************************************************************/
/*!
 * \brief  Includes necessary standard headers for array operations, plus any additional
 *         headers for violation handling or logging (per AUTOSAR guidelines).
 *
 * [SWS_CORE_01214], [SWS_CORE_01215]: cstddef for std::size_t, std::ptrdiff_t
 * [SWS_CORE_01212], [SWS_CORE_01213]: iterator for std::reverse_iterator
 * [SWS_CORE_01241]: fill uses type traits for noexcept checks
 * [SWS_CORE_00040]: we do not throw exceptions – we do custom violation handling
 */
#include <tuple>          // For std::tuple_size / tuple_element declarations
#include <cstring>        // For std::memcpy, std::memset
#include <cstddef>        // For std::size_t, std::ptrdiff_t
#include <iostream>       // For std::cout (demonstrations)
#include <iterator>       // For std::reverse_iterator
#include <type_traits>    // For std::is_nothrow_move_constructible, std::is_nothrow_move_assignable, etc.
#include <utility>        // For std::declval, std::move, std::forward

#include "ara/core/internal/location_utils.h"       // For capturing file/line details
#include "ara/core/internal/violation_handler.h"    // To Trigger the violation

/**********************************************************************************************************************
 *  SECTION: Forward Declaration
 *********************************************************************************************************************/
/*!
 * \brief  Forward declaration of the Array class template.
 */
template <typename T, std::size_t N>
class Array;

/**********************************************************************************************************************
 *  TUPLE: INTERFACE SPECIALISATIONS
 *  ---------------------------------------------------------------------------------------------------------------
 *  ⌂  std::tuple_size   [SWS_CORE_01280]
 *  ⌂  std::tuple_element[ SWS_CORE_01281 / 01285 ]
 *
 *  Rationale:
 *  ──────────
 *  Supplying these partial specialisations makes ara::core::Array<T,N> a fully
 *  “tuple‑like” type in the sense of [tuple.helper] in the C++ Standard.  This
 *  unlocks:
 *      • structured bindings        →  auto [x,y] = myArray;
 *      • std::get<I>()              →  value = std::get<2>(myArray);
 *      • std::apply / tuple_cat …   →  standard generic utilities
 *  and it is explicitly demanded by AUTOSAR SWS ( 8.8.3 / 8.8.4).
 *
 *  The implementation is ZERO‑overhead: it only creates compile‑time metadata.
 *********************************************************************************************************************/
namespace std
{
   /*---------------------------------------------------------------------------------------------------------------
    *  (1) tuple_size – “How many elements?”
    *-------------------------------------------------------------------------------------------------------------*/
   /*!
    * \brief   Primary trait giving the fixed size N of ara::core::Array<T,N>.
    *
    * \tparam  T  Element type stored in the Array.
    * \tparam  N  Number of elements (array extent).
    *
    * \note    [SWS_CORE_01280] – must model a C++14 UnaryTypeTrait whose BaseCharacteristic
    *        is std::integral_constant<std::size_t,N>.
    */
    template<class T, std::size_t N>
    struct tuple_size< ara::core::Array<T,N> >
        : std::integral_constant<std::size_t, N>  // UnaryTypeTrait
    {};

   /*---------------------------------------------------------------------------------------------------------------
    *  (2) tuple_element – “What is the type of element I?”
    *-------------------------------------------------------------------------------------------------------------*/
   /*!
    * \brief   Yields the element **type** of ara::core::Array<T,N> at compile‑time index \c I.
    *
    * \details
    *   • If I ≥ N the implementation triggers a compile‑time error as mandated by the spec
    *     (“shall flag the condition I >= N as a compile error” – SWS_CORE_01281).\n
    *   • Because every element of ara::core::Array<T,N> has the same type \c T, the alias
    *     simply forwards `using type = T;` (SWS_CORE_01285).
    *
    * \tparam  I  Zero‑based element index requested at compile time.
    * \tparam  T  Element type stored in the Array.
    * \tparam  N  Number of elements in the Array.
    */
    template<std::size_t I, class T, std::size_t N>
    struct tuple_element<I, ara::core::Array<T,N>>
    {
        static_assert(I < N,
            "\n[ERROR] std::tuple_element<I, ara::core::Array<T,N>> : "
            "index I is out of range (I >= N).\n");

        using type = T;                         // every element is T
    };
} // namespace std
 

/**********************************************************************************************************************
 *  NAMESPACE: ara::core
 *********************************************************************************************************************/
/*!
 * \brief  The ara::core namespace, within which our AUTOSAR Adaptive Platform
 *         data types and utilities reside.
 */
namespace ara {
namespace core {

/**********************************************************************************************************************
 *  SECTION: Internal Utilities
 *********************************************************************************************************************/
/*!
 * \brief Contains internal details for handling internal utilities.
 *
 * This namespace encapsulates helper traits and functions that are used internally by the \c ara::core::Array 
 * implementation. These details are subject to change and are not part of the public API.
 *
 * \note This not proposed by the Specification of Adaptive Platform Core
 */
namespace detail {

constexpr auto is_constant_evaluated() noexcept -> bool {
    #if defined(__cpp_lib_is_constant_evaluated) \
        && (__cpp_lib_is_constant_evaluated >= 202002L)
        return std::is_constant_evaluated();              // C++20 standard API
    #elif defined(__has_builtin) && __has_builtin(__builtin_is_constant_evaluated)
        return __builtin_is_constant_evaluated();         // GCC/Clang builtin in C++17
    #elif defined(_MSC_VER)
        return __is_constant_evaluated();                 // MSVC intrinsic
    #else
        return true;                                     // fallback: always compile-time
    #endif
}

/*!
 * \brief Trait to detect whether an array of type T[N] can be list‑initialized
 *        with arguments of types Args... without narrowing conversions.
 *
 * This trait uses SFINAE to check the validity of the list‑initialization expression:
 *   T[N]{ std::declval<Args>()... }
 * If well‑formed (and no narrowing occurs), it inherits from std::true_type;
 * otherwise, from std::false_type.
 *
 * \tparam T    The element type of the array.
 * \tparam N    The number of elements in the array.
 * \tparam Args The types of the initializer arguments.
 *
 * \note Conforms to AUTOSAR C++ Guidelines (SWS_CORE_11200).
 * \since C++17
 * \see std::is_brace_constructible (C++20)
 */
template <typename T, std::size_t N, typename... Args>
struct is_brace_initializable_array
{
private:
    /* Selected if U[N]{ Args... } is well‑formed */
    template <typename U, typename = decltype(U{ std::declval<Args>()... })>
    static auto test(int) -> std::true_type;

    /* Fallback if substitution in the above fails */
    template <typename...>
    static auto test(...) -> std::false_type;

public:

    /* Integral constant type: std::true_type or std::false_type */
    using type = decltype(test<T[N]>(0));

    /* Shorthand for the boolean result (for trait compatibility) */
    using value_type = bool;

    /* The raw boolean result of the check */
    static constexpr value_type value = type::value;
};

/*!
 * \brief Variable template for is_brace_initializable_array.
 *
 * Simplifies usage:
 *   if constexpr (is_brace_initializable_array_v<MyType, 3, int, double, char>) { … }
 *
 * \tparam T    The element type of the array.
 * \tparam N    The number of elements in the array.
 * \tparam Args The types of the initializer arguments.
 */
template <typename T, std::size_t N, typename... Args>
inline constexpr bool is_brace_initializable_array_v =
    is_brace_initializable_array<T, N, Args...>::value;


/*!
 * \brief Primary helper trait to detect if a type is an \c ara::core::Array.
 *
 * By default, any type is not considered an \c ara::core::Array.
 *
 * \tparam T The type to check.
 */
template <typename...>
struct is_array : std::false_type {};

/*!
 * \brief Specialization for \c ara::core::Array.
 *
 * If a type matches \c ara::core::Array<T, N> for any \c T and \c N,
 * this trait yields \c std::true_type.
 *
 * \tparam T The element type.
 * \tparam N The size of the array.
 */
template <typename T, std::size_t N>
struct is_array<ara::core::Array<T, N>> : std::true_type {};

/*!
 * \brief Trait to detect if the parameter pack \c Args contains exactly one argument
 *        and that (after decay) is an \c ara::core::Array.
 *
 * This trait evaluates to \c true if:
 * - The number of arguments is exactly one, and
 * - The decayed type of that argument is recognized as an \c ara::core::Array.
 *
 * The fold expression (\c is_array<std::decay_t<Args>>::value && ...) applies the check to the
 * argument (in this case just one) and returns \c true only if the condition holds.
 *
 * \tparam Args The types of the arguments.
 */
template <typename... Args>
struct is_single_same_array 
    : std::bool_constant<
          (sizeof...(Args) == 1) && (is_array<std::decay_t<Args>>::value && ...)
      > 
{};

/*!
 * \brief Convenience variable template for \c is_single_same_array.
 *
 * This variable template allows for a simplified syntax:
 *
 * \code
 * if constexpr (is_single_same_array_v<ArgType>)
 * {
 *     // ...
 * }
 * \endcode
 *
 * \tparam Args The types of the arguments.
 */
template <typename... Args>
inline constexpr bool is_single_same_array_v = is_single_same_array<Args...>::value;


/*!
 * \brief Performs a lexicographical comparison between two ranges.
 *
 * This function compares the elements in the ranges \c [first1,last1) and \c [first2,last2)
 * one by one:
 * - For each pair of corresponding elements, if the element from the first range is less than the element
 *   from the second range, the function returns \c true.
 * - If the element from the second range is less than the element from the first range, the function returns \c false.
 * - If the elements are equal, the comparison continues.
 * - When the end of one of the ranges is reached:
 *     - If the first range is exhausted but the second still has elements, the first range is considered
 *       lexicographically less and the function returns \c true.
 *     - Otherwise, it returns \c false.
 *
 * \tparam InputIt1 The type of the input iterator for the first range.
 * \tparam InputIt2 The type of the input iterator for the second range.
 * \param first1 An iterator pointing to the first element of the first range.
 * \param last1  An iterator pointing past the last element of the first range.
 * \param first2 An iterator pointing to the first element of the second range.
 * \param last2  An iterator pointing past the last element of the second range.
 * \return \c true if the first range is lexicographically less than the second range; \c false otherwise.
 *
 * \note The function is declared as \c constexpr so that if both the iterators and the element comparison
 *       are \c constexpr, the entire operation can be evaluated at compile time.
 */
template <typename InputIt1, typename InputIt2>
constexpr auto lex_compare(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2) noexcept -> bool {
    for (; first1 != last1 && first2 != last2; ++first1, ++first2) {
        if (*first1 < *first2)
            return true;
        if (*first2 < *first1)
            return false;
    }
    return (first1 == last1) && (first2 != last2);
}


/**********************************************************************************************************************
 *  SECTION: Array Storage Base Classes
 *********************************************************************************************************************/
/*!
* \brief Base class for array storage.
*
* Splits out storage to handle partial specialization for \c N > 0 versus \c N == 0.
*
* \tparam T The type of elements.
* \tparam N The number of elements in the array.
* \tparam B A boolean indicating whether \c N > 0.
*
* \note [SWS_CORE_01201]
*/
template <typename T, std::size_t N, bool B = (N > 0)>
struct ArrayStorage;

/*!
 * \brief Primary template for Array storage when \c N > 0.
 *
 * Provides the actual storage for \c N elements of type \c T and supports direct brace‑initialization.
 *
 * \tparam T The element type.
 * \tparam N The number of elements in the array.
 *
 * \note [SWS_CORE_01201]
 */
template <typename T, std::size_t N>
struct ArrayStorage<T, N, true> {
protected:
    /*! \brief Actual storage for \c N elements of type \c T. */
    T data_[N]{};

    /*!
     * \brief Variadic constructor to initialize \c data_ with up to \c N arguments using brace‑initialization.
     *
     * The constructor is constrained to accept at most \c N arguments, all of which must be convertible to \c T,
     * and such that brace‑initialization does not cause narrowing conversions.
     *
     * \tparam Args The types of the constructor arguments.
     * \param args The arguments to initialize the array.
     *
     * \note [SWS_CORE_01201], [SWS_CORE_01214], [SWS_CORE_01215], [SWS_CORE_01241]
     */
    template <typename... Args,
              typename = std::enable_if_t<
                  // Condition #1: Must not exceed N arguments
                  (sizeof...(Args) <= N) &&
                  // Condition #2: Each argument must be convertible to T
                  (std::conjunction_v<std::is_convertible<Args, T>...>) &&
                  // Condition #3: Brace-initialization does not cause narrowing
                  (detail::is_brace_initializable_array_v<T, N, Args...>) &&
                  // Condition #4: Prevent constructor from being selected when Args... is exactly Array<T, N>
                  (!detail::is_single_same_array_v<Args...>)
              >>
    constexpr ArrayStorage(Args&&... args)
#ifdef ARA_CORE_ARRAY_ENABLE_CONDITIONAL_EXCEPTIONS
        noexcept(std::conjunction_v<std::is_nothrow_constructible<T, Args&&>...>)
#else
        noexcept
#endif
        : data_{std::forward<Args>(args)...} 
    {
#ifndef ARA_CORE_ARRAY_ENABLE_CONDITIONAL_EXCEPTIONS
        static_assert(std::conjunction_v<std::is_nothrow_constructible<T, Args&&>...>,
            "\n[ERROR] in ara::core::Array: The type T and args must be noexcept.\n");
#endif  
    }

    /*!
     * \brief Default constructor for the storage.
     *
     * Zero‑initializes \c data_.
     */
    constexpr ArrayStorage() noexcept = default;

    /*!
     * \brief Defaulted copy constructor.
     */
    constexpr ArrayStorage(const ArrayStorage&) noexcept = default;

    /*!
     * \brief Defaulted move constructor.
     */
    constexpr ArrayStorage(ArrayStorage&&) noexcept = default;

    /*!
     * \brief Defaulted copy assignment operator.
     */
    constexpr ArrayStorage& operator=(const ArrayStorage&) noexcept = default;

    /*!
     * \brief Defaulted move assignment operator.
     */
    constexpr ArrayStorage& operator=(ArrayStorage&&) noexcept = default;
};

/*!
 * \brief Partial specialization for Array storage when \c N == 0.
 *
 * No actual storage is allocated for zero‑sized arrays.
 *
 * \tparam T The element type.
 * \tparam N The (zero) number of elements in the array.
 *
 * \note [SWS_CORE_01201]
 */
template <typename T, std::size_t N>
struct ArrayStorage<T, N, false> {
protected:
    /*!
     * \brief Variadic constructor for \c N == 0.
     *
     * This constructor is enabled only when no arguments are provided.
     *
     * \tparam Args The types of constructor arguments (must be empty).
     */
    template <typename... Args,
              typename = std::enable_if_t<(sizeof...(Args) == 0)>>
    constexpr ArrayStorage(Args&&...) noexcept { /* Do Nothing */ }

    /*!
     * \brief Default constructor for zero‑sized storage.
     */
    constexpr ArrayStorage() noexcept = default;

    /*!
     * \brief Defaulted copy constructor.
     */
    constexpr ArrayStorage(const ArrayStorage&) noexcept = default;

    /*!
     * \brief Defaulted move constructor.
     */
    constexpr ArrayStorage(ArrayStorage&&) noexcept = default;

    /*!
     * \brief Defaulted copy assignment operator.
     */
    constexpr ArrayStorage& operator=(const ArrayStorage&) noexcept = default;

    /*!
     * \brief Defaulted move assignment operator.
     */
    constexpr ArrayStorage& operator=(ArrayStorage&&) noexcept = default;
};
 

} // namespace detail
    

/**********************************************************************************************************************
 *  CLASS: ara::core::Array
 *********************************************************************************************************************/
/*!
 * \brief  A fixed-size array template for the Adaptive AUTOSAR platform.
 *
 * \tparam T  Type of elements stored in the array.
 * \tparam N  Number of elements in the array.
 *
 * \details
 * - [SWS_CORE_11200]: Should behave like std::array, except at() uses Violations instead of exceptions.
 * - Logging out-of-range (fulfills [SWS_CORE_13017]).
 * - No exceptions for out-of-range, consistent with [SWS_CORE_00040].
 * - Complies with [SWS_CORE_01201], which defines the API class \a ara::core::Array.
 * - Provides fill(), swap(), and comparison operators as required by [SWS_CORE_01241], [SWS_CORE_01242], etc.
 *
 * \pre  Typically, we expect N > 0, though zero-sized arrays are handled appropriately.
 * \note Additionally, we enforce T's no-throw move/copy constructible and assignable properties
 *       conditionally based on the macro, aligning with [SWS_CORE_00040].
 *
 * \note  [SWS_CORE_01201], [SWS_CORE_11200], [SWS_CORE_00040], [SWS_CORE_13017], [SWS_CORE_01241], [SWS_CORE_01242]
 */
template <typename T, std::size_t N>
class Array final : private detail::ArrayStorage<T, N>
{
public:
#ifdef ARA_CORE_ARRAY_ENABLE_CONDITIONAL_EXCEPTIONS
    // In Conditional Safe Mode, allow potentially-throwing types
#else
    /*!
     * \brief Enforce that T cannot throw exceptions during move or copy operations.
     *
     * \note  [SWS_CORE_00040]
     */
    static_assert(std::is_nothrow_constructible_v<T> &&
                  std::is_nothrow_move_constructible_v<T> &&
                  std::is_nothrow_move_assignable_v<T> &&
                  std::is_nothrow_copy_constructible_v<T> &&
                  std::is_nothrow_copy_assignable_v<T>,
                "\n[ERROR] in ara::core::Array: The type T must be noexcept and move and copy constructible\n"
                "        and assignable without throwing exceptions. Please ensure that T's constructors and\n"
                "        assignment operators are marked 'noexcept'.\n");
#endif

    // -----------------------------------------------------------------------------------
    // TYPE ALIASES (public) [SWS_CORE_01210..01220]
    // -----------------------------------------------------------------------------------
    using value_type             = T;                                                   /*!< [SWS_CORE_01216]: Type of the elements  */
    using size_type              = std::size_t;                                         /*!< [SWS_CORE_01214]: Used for indexing      */
    using difference_type        = std::ptrdiff_t;                                      /*!< [SWS_CORE_01215]: Used for pointer diffs */
    using reference              = T&;                                                  /*!< [SWS_CORE_01210]: Type of a reference    */
    using const_reference        = const T&;                                            /*!< [SWS_CORE_01211]: Type of a const-ref    */
    using pointer                = T*;                                                  /*!< [SWS_CORE_01217]: Pointer to element     */
    using const_pointer          = const T*;                                            /*!< [SWS_CORE_01218]: Const pointer          */
    using iterator               = T*;                                                  /*!< [SWS_CORE_01212]: Iterator type          */
    using const_iterator         = const T*;                                            /*!< [SWS_CORE_01213]: Const iterator type    */
    using reverse_iterator       = std::reverse_iterator<iterator>;                     /*!< [SWS_CORE_01219]: Reverse iterator      */
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;               /*!< [SWS_CORE_01220]: Const reverse iterator*/

    // -----------------------------------------------------------------------------------
    // 1) VARIADIC CONSTRUCTOR (CONSTRAINED) [SWS_CORE_01241], [SWS_CORE_01201]
    // -----------------------------------------------------------------------------------
    /*!
     * \brief Variadic constructor that can take up to N elements of type \c T.
     *
     * \tparam Args  Parameter pack of arguments to forward to \c T.
     * \pre (sizeof...(Args) <= N) AND each \c Arg is convertible to \c T 
     *      AND brace-initialization with Args... does not cause narrowing conversions.
     *      AND Args... is not exactly one Array<T, N> (prevents unintended copy/move).
     * \details
     * - If user passes more than N arguments => compile-time error.
     * - Ensures each argument is convertible to T, preventing spurious usage.
     * - `noexcept` is conditionally specified based on whether initializing T with Args... is noexcept.
     *
     * \note  [SWS_CORE_01241], [SWS_CORE_01201]
     */
    template <typename... Args,
              typename = std::enable_if_t<
                  // Condition #1: Must not exceed N arguments
                  (sizeof...(Args) <= N) &&
                  // Condition #2: Each argument must be convertible to T
                  (std::conjunction_v<std::is_convertible<Args, T>...>) &&
                  // Condition #3: Brace-initialization does not cause narrowing
                  (detail::is_brace_initializable_array_v<T, N, Args...>) &&
                  // Condition #4: Prevent constructor from being selected when Args... is exactly Array<T, N>
                  (!detail::is_single_same_array_v<Args...>)
              >>
    constexpr Array(Args&&... args) 
#ifdef ARA_CORE_ARRAY_ENABLE_CONDITIONAL_EXCEPTIONS
        noexcept(std::conjunction_v<std::is_nothrow_constructible<T, Args&&>...>)
#else
        noexcept
#endif
        : detail::ArrayStorage<T, N>(std::forward<Args>(args)...)
    {
        // Base class handles data_ initialization.
#ifndef ARA_CORE_ARRAY_ENABLE_CONDITIONAL_EXCEPTIONS
        static_assert(std::conjunction_v<std::is_nothrow_constructible<T, Args&&>...>,
            "\n[ERROR] in ara::core::Array: The type T and args must be noexcept.\n");
#endif
        // No additional initialization needed.
    }

    // -----------------------------------------------------------------------------------
    // 2) REJECTING CONSTRUCTOR (TOO MANY OR WRONG TYPES) [SWS_CORE_01241]
    // -----------------------------------------------------------------------------------
    /*!
     * \brief Overload constructor that catches calls violating the above constraints.
     *
     * \tparam Args  Parameter pack that either exceeds N or has arguments not convertible to T.
     * \note         This never actually constructs anything; it only fires `static_assert` errors.
     *
     * \note  [SWS_CORE_01241]
     */
    template <
        typename... Args,
        // Condition: either too many arguments OR not all convertible OR narrowing
        typename = std::enable_if_t<
            (sizeof...(Args) > N) ||
            (!detail::is_single_same_array_v<Args...>) && 
            ((!std::conjunction_v<std::is_convertible<Args, T>...>) ||
             (!detail::is_brace_initializable_array_v<T, N, Args...>))
        >,
        int = 0
    >
    constexpr Array(Args&&...) noexcept
    {
        static_assert(sizeof...(Args) <= N,
            "\n[ERROR] Too many arguments passed to Array<T,N> constructor!\n"
            "        Up to N elements are allowed.\n");

        static_assert(std::conjunction_v<std::is_convertible<Args, T>...>,
            "\n[ERROR] One or more arguments cannot be converted to T.\n");

        static_assert(detail::is_brace_initializable_array_v<T, N, Args...>,
            "\n[ERROR] Brace-initialization would cause narrowing conversions.\n");
    }

    // -----------------------------------------------------------------------------------
    // 3) DEFAULT AND COPY/MOVE OPERATIONS [SWS_CORE_01201]
    // -----------------------------------------------------------------------------------
    constexpr Array() noexcept = default;                                       /*!< [SWS_CORE_01201]: Default constructor */
    constexpr Array(const Array&) noexcept = default;                           /*!< [SWS_CORE_01201]: Copy constructor */
    constexpr Array(Array&&) noexcept = default;                                /*!< [SWS_CORE_01201]: Move constructor */
    constexpr auto operator=(const Array&) noexcept -> Array& = default;        /*!< [SWS_CORE_01201]: Copy assignment */ 
    constexpr auto operator=(Array&&) noexcept -> Array& = default;             /*!< [SWS_CORE_01201]: Move assignment */ 

    // -----------------------------------------------------------------------------------
    // 4) OPERATOR[] [SWS_CORE_01265], [SWS_CORE_01266]
    // -----------------------------------------------------------------------------------
    /*!
     * \brief  [old req before the update] Unchecked subscript (mutable). Out-of-range => undefined behavior ([SWS_CORE_01266]).
     *
     * \param  idx  The index to access.
     * \return     Reference to the element at index \c idx.
     *
     * \note  [SWS_CORE_01265], [SWS_CORE_01266] Accessing a non-existing element through this operation now will trigger a violation
     */
    constexpr auto operator[](size_type idx) noexcept -> reference
    {
        // Per [SWS_CORE_01266], operator[] does NOT do bound checks. 
        // Accessing out-of-range is undefined behavior.
        return this->at(idx);
    }

    /*!
     * \brief  [old req before the update] Unchecked subscript (const). Out-of-range => undefined behavior.
     *
     * \param  idx  The index to access.
     * \return     Const reference to the element at index \c idx.
     *
     * \note  [SWS_CORE_01265], [SWS_CORE_01266] Accessing a non-existing element through this operation now will trigger a violation
     */
    constexpr auto operator[](size_type idx) const noexcept -> const_reference
    {
        return this->at(idx);
    }

    // -----------------------------------------------------------------------------------
    // 5) at() [SWS_CORE_01273], [SWS_CORE_01274]
    // -----------------------------------------------------------------------------------
    /*!
     * \brief  Checked element access => triggers Violation if out-of-range.
     *
     * \param  idx  The index to access.
     * \return     Reference to the element at index \c idx.
     *
     * \note   [SWS_CORE_01273], [SWS_CORE_01274]
     * \note   If idx >= N => logs & terminates. No exceptions.
     */
    constexpr auto at(size_type idx) noexcept -> reference
    {
        if (idx >= N) {
            TriggerOutOfRangeViolation(
                ARA_CORE_INTERNAL_FILELINE,
                idx,
                N
            );
        }
        return this->data_[idx];
    }

    /*!
     * \brief  Checked element access (const) => triggers Violation if out-of-range.
     *
     * \param  idx  The index to access.
     * \return     Const reference to the element at index \c idx.
     *
     * \note   [SWS_CORE_01273], [SWS_CORE_01274]
     * \note   If idx >= N => logs & terminates. No exceptions.
     */
    constexpr auto at(size_type idx) const noexcept -> const_reference
    {
        if (idx >= N) {
            TriggerOutOfRangeViolation(
                ARA_CORE_INTERNAL_FILELINE,
                idx,
                N
            );
        }

        return this->data_[idx];
    }

    // -----------------------------------------------------------------------------------
    // 6) front(), back() [SWS_CORE_01267..01270]
    // -----------------------------------------------------------------------------------
    /*!
     * \brief  Returns a mutable reference to the first element. 
     *
     * \return Reference to the first element.
     *
     * \note   [SWS_CORE_01267]
     * \note   Compile-time error if N==0 => no front.
     */
    constexpr auto front() noexcept -> reference
    {
        static_assert(N > 0,
            "\n[ERROR] front() called on zero-sized Array!\n");
        return this->data_[0];
    }

    /*!
     * \brief  Returns a const reference to the first element. 
     *
     * \return Const reference to the first element.
     *
     * \note   [SWS_CORE_01270]
     * \note   Compile-time error if N==0 => no front.
     */
    constexpr auto front() const noexcept -> const_reference
    {
        static_assert(N > 0,
            "\n[ERROR] front() called on zero-sized Array!\n");
        return this->data_[0];
    }

    /*!
     * \brief  Returns a mutable reference to the last element. 
     *
     * \return Reference to the last element.
     *
     * \note   [SWS_CORE_01269]
     * \note   Compile-time error if N==0 => no back.
     */
    constexpr auto back() noexcept -> reference
    {
        static_assert(N > 0,
            "\n[ERROR] back() called on zero-sized Array!\n");
        return this->data_[N - 1];
    }

    /*!
     * \brief  Returns a const reference to the last element. 
     *
     * \return Const reference to the last element.
     *
     * \note   [SWS_CORE_01270]
     * \note   Compile-time error if N==0 => no back.
     */
    constexpr auto back() const noexcept -> const_reference
    {
        static_assert(N > 0,
            "\n[ERROR] back() called on zero-sized Array!\n");
        return this->data_[N - 1];
    }

    // -----------------------------------------------------------------------------------
    // 7) data() [SWS_CORE_01271..01272]
    // -----------------------------------------------------------------------------------
    /*!
     * \brief  Returns pointer to the first element (mutable). If N==0 => nullptr.
     *
     * \return Pointer to the first element or nullptr if N==0.
     *
     * \note   [SWS_CORE_01271], [SWS_CORE_01272]
     */
    constexpr auto data() noexcept -> pointer
    {
        if constexpr (N > 0) {
            return this->data_;
        } else {
            return nullptr;
        }
    }

    /*!
     * \brief  Returns pointer to the first element (const). If N==0 => nullptr.
     *
     * \return Const pointer to the first element or nullptr if N==0.
     *
     * \note   [SWS_CORE_01271], [SWS_CORE_01272]
     */
    constexpr auto data() const noexcept -> const_pointer
    {
        if constexpr (N > 0) {
            return this->data_;
        } else {
            return nullptr;
        }
    }

    // -----------------------------------------------------------------------------------
    // 8) size(), max_size(), empty() [SWS_CORE_01262..01264]
    // -----------------------------------------------------------------------------------
    /*!
     * \brief  Returns the number of elements, which is N.
     *
     * \return Number of elements in the array.
     *
     * \note   [SWS_CORE_01262]
     */
    constexpr auto size() const noexcept -> size_type
    {
        return N;
    }

    /*!
     * \brief  Returns max size => same as N, for a fixed-size array.
     *
     * \return Maximum number of elements supported by the array.
     *
     * \note   [SWS_CORE_01263]
     */
    constexpr auto max_size() const noexcept -> size_type
    {
        return N;
    }

    /*!
     * \brief  Returns whether this Array is empty => (N==0).
     *
     * \return \c true if the array is empty; \c false otherwise.
     *
     * \note   [SWS_CORE_01264]
     */
    constexpr auto empty() const noexcept -> bool
    {
        return (N == 0);
    }

    // -----------------------------------------------------------------------------------
    // 9) ITERATORS [SWS_CORE_01250..01261]
    // -----------------------------------------------------------------------------------
    /*!
     * \brief  Returns iterator to first element (mutable).
     *
     * \return Iterator pointing to the first element.
     *
     * \note   [SWS_CORE_01250]
     */
    constexpr auto begin() noexcept -> iterator
    {
        return data();
    }

    /*!
     * \brief  Returns const_iterator to first element.
     *
     * \return Const iterator pointing to the first element.
     *
     * \note   [SWS_CORE_01251]
     */
    constexpr auto begin() const noexcept -> const_iterator
    {
        return data();
    }

    /*!
     * \brief  Returns iterator to one-past-last element (mutable).
     *
     * \return Iterator pointing past the last element.
     *
     * \note   [SWS_CORE_01252]
     */
    constexpr auto end() noexcept -> iterator
    {
        if constexpr (N > 0) {
            return data() + N;
        } else {
            return nullptr;
        }
    }

    /*!
     * \brief  Returns const_iterator to one-past-last element.
     *
     * \return Const iterator pointing past the last element.
     *
     * \note   [SWS_CORE_01253]
     */
    constexpr auto end() const noexcept -> const_iterator
    {
        if constexpr (N > 0) {
            return data() + N;
        } else {
            return nullptr;
        }
    }

    /*!
     * \brief  Returns const_iterator to first element (cbegin).
     *
     * \return Const iterator pointing to the first element.
     *
     * \note   [SWS_CORE_01258]
     */
    constexpr auto cbegin() const noexcept -> const_iterator
    {
        return begin();
    }

    /*!
     * \brief  Returns const_iterator to one-past-last element (cend).
     *
     * \return Const iterator pointing past the last element.
     *
     * \note   [SWS_CORE_01259]
     */
    constexpr auto cend() const noexcept -> const_iterator
    {
        return end();
    }

    /*!
     * \brief  Returns const_reverse_iterator to last element (crbegin).
     *
     * \return Const reverse iterator pointing to the last element.
     *
     * \note   [SWS_CORE_01260]
     */
    constexpr auto crbegin() const noexcept -> const_reverse_iterator
    {
        return const_reverse_iterator(end());
    }

    /*!
     * \brief  Returns const_reverse_iterator to one-before-first element (crend).
     *
     * \return Const reverse iterator pointing past the first element.
     *
     * \note   [SWS_CORE_01261]
     */
    constexpr auto crend() const noexcept -> const_reverse_iterator
    {
        return const_reverse_iterator(begin());
    }

    /*!
     * \brief  Returns reverse_iterator to last element (rbegin).
     *
     * \return Reverse iterator pointing to the last element.
     *
     * \note   [SWS_CORE_01254]
     */
    constexpr auto rbegin() noexcept -> reverse_iterator
    {
        return reverse_iterator(end());
    }

    /*!
     * \brief  Returns const_reverse_iterator to last element (rbegin).
     *
     * \return Const reverse iterator pointing to the last element.
     *
     * \note   [SWS_CORE_01255]
     */
    constexpr auto rbegin() const noexcept -> const_reverse_iterator
    {
        return const_reverse_iterator(end());
    }

    /*!
     * \brief  Returns reverse_iterator to one-before-first (rend).
     *
     * \return Reverse iterator pointing past the first element.
     *
     * \note   [SWS_CORE_01256]
     */
    constexpr auto rend() noexcept -> reverse_iterator
    {
        return reverse_iterator(begin());
    }

    /*!
     * \brief  Returns const_reverse_iterator to one-before-first (rend).
     *
     * \return Const reverse iterator pointing past the first element.
     *
     * \note   [SWS_CORE_01257]
     */
    constexpr auto rend() const noexcept -> const_reverse_iterator
    {
        return const_reverse_iterator(begin());
    }

    // -----------------------------------------------------------------------------------
    // 10) fill() [SWS_CORE_01241]
    // -----------------------------------------------------------------------------------
    /*!
     * \brief  Assign the given value to all elements of this Array.
     *
     * \param  val  The value to assign to all elements.
     *
     * \note   [SWS_CORE_01241]
     * \note   The noexcept specification is conditionally applied based on whether T's copy assignment is noexcept.
     *         Instead of using std::fill_n (which is not constexpr in C++17), we use a loop to assign the value,
     *         enabling compile-time evaluation when possible.
     */
    template <typename U = T, size_type M = N>
    constexpr auto fill(const T& val)
#ifdef ARA_CORE_ARRAY_ENABLE_CONDITIONAL_EXCEPTIONS
    noexcept(std::is_nothrow_copy_assignable_v<T>)
#else
    noexcept
#endif
    -> std::enable_if_t<std::is_trivially_copyable_v<U> && 
                        std::is_trivially_constructible_v<U> && (M > 0), void>
    {

#ifndef ARA_CORE_ARRAY_ENABLE_CONDITIONAL_EXCEPTIONS
        // Ensure T's fill is noexcept if exceptions are disabled
        static_assert(std::is_nothrow_copy_assignable_v<T>,
            "\n[ERROR] ara::core::Array: The type T's fill operation must be noexcept when exceptions are disabled.\n");
#endif
        
        if (!detail::is_constant_evaluated()) {
            // If not evaluated at compile time, use memset for performance
            if (val == T{}) {
                std::memset(this->data_, 0, N * sizeof(T));
            } else if constexpr (sizeof(T) == 1) {
                
                if constexpr (std::is_same_v<T, bool>) {
                    std::memset(this->data_, val ? 0x01 : 0x00, N); 
                } else {
                    std::memset(this->data_, val, N * sizeof(T));
                }

            } else {
                for (size_type i = 0; i < N; ++i) {
                    this->data_[i] = val;
                }
            }
        } else {
            // If evaluated at compile time, use a loop for constexpr evaluation
            for (size_type i = 0; i < N; ++i) {
                this->data_[i] = val;
            }
        }
    }
    /*!
     * \brief  Assign the given value to all elements of this Array.
     *
     * \param  val  The value to assign to all elements.
     *
     * \note   [SWS_CORE_01241]
     * \note   The noexcept specification is conditionally applied based on whether T's copy assignment is noexcept.
     *         Instead of using std::fill_n (which is not constexpr in C++17), we use a loop to assign the value,
     *         enabling compile-time evaluation when possible.
     */
    template <typename U = T, size_type M = N>
    constexpr auto fill(const T& val)
#ifdef ARA_CORE_ARRAY_ENABLE_CONDITIONAL_EXCEPTIONS
    noexcept(std::is_nothrow_copy_assignable_v<T>)
#else
    noexcept
#endif
    -> std::enable_if_t<std::is_trivially_copyable_v<U> && 
                        !std::is_trivially_constructible_v<U> && (M > 0), void>
    {

#ifndef ARA_CORE_ARRAY_ENABLE_CONDITIONAL_EXCEPTIONS
        // Ensure T's fill is noexcept if exceptions are disabled
        static_assert(std::is_nothrow_copy_assignable_v<T>,
            "\n[ERROR] ara::core::Array: The type T's fill operation must be noexcept when exceptions are disabled.\n");
#endif
        
        if (!detail::is_constant_evaluated()) {
            if constexpr (sizeof(T) == 1) {
                
                if constexpr (std::is_same_v<T, bool>) {
                    std::memset(this->data_, val ? 0x01 : 0x00, N); 
                } else {
                    std::memset(this->data_, val, N * sizeof(T));
                }

            } else {

                for (size_type i = 0; i < N; ++i) {
                    this->data_[i] = val;
                }

            }
        } else {
            // If evaluated at compile time, use a loop for constexpr evaluation
            for (size_type i = 0; i < N; ++i) {
                this->data_[i] = val;
            }
        }
    }

    /*!
     * \brief  Assign the given value to all elements of this Array.
     *
     * \param  val  The value to assign to all elements.
     *
     * \note   [SWS_CORE_01241]
     * \note   The noexcept specification is conditionally applied based on whether T's copy assignment is noexcept.
     *         Instead of using std::fill_n (which is not constexpr in C++17), we use a loop to assign the value,
     *         enabling compile-time evaluation when possible.
     */
    template <typename U = T, size_type M = N>
    constexpr auto fill(const T& val)
#ifdef ARA_CORE_ARRAY_ENABLE_CONDITIONAL_EXCEPTIONS
    noexcept(std::is_nothrow_copy_assignable_v<T>)
#else
    noexcept
#endif
    -> std::enable_if_t<!std::is_trivially_copyable_v<U> || (M == 0), void>
    {

#ifndef ARA_CORE_ARRAY_ENABLE_CONDITIONAL_EXCEPTIONS
        // Ensure T's fill is noexcept if exceptions are disabled
        static_assert(std::is_nothrow_copy_assignable_v<T>,
            "\n[ERROR] ara::core::Array: The type T's fill operation must be noexcept when exceptions are disabled.\n");
#endif

        /* If N > 0, loop over the array and assign val to each element.
           This loop is constexpr-friendly so that it can be evaluated at compile time if T's operations are constexpr. */
        if constexpr (N > 0) {
            for (size_type i = 0; i < N; ++i) {
                this->data_[i] = val;
            }
        }
    }

    // -----------------------------------------------------------------------------------
    // 11) swap() [SWS_CORE_01242]
    // -----------------------------------------------------------------------------------
    /*!
     * \brief  Exchange the contents of *this with those of another array of the same size N.
     *
     * \param  other  The other Array to swap with.
     *
     * \note   [SWS_CORE_01242]
     * \note   This function is marked constexpr so that, if T's move construction and move assignment are constexpr,
     *         the swap can be performed at compile time.
     */
    template <typename U = T, size_type M = N>
    constexpr auto swap(Array& other) 
#ifdef ARA_CORE_ARRAY_ENABLE_CONDITIONAL_EXCEPTIONS
        noexcept(noexcept(std::swap(std::declval<T&>(), std::declval<T&>())))
#else
        noexcept
#endif
    -> std::enable_if_t<std::is_trivially_copyable_v<U> && (M > 0), void>
    {   
#ifndef ARA_CORE_ARRAY_ENABLE_CONDITIONAL_EXCEPTIONS
        // Ensure T's swap is noexcept if exceptions are disabled
        static_assert(noexcept(std::swap(std::declval<T&>(), std::declval<T&>())),
            "\n[ERROR] ara::core::Array: The type T's swap operation must be noexcept when exceptions are disabled.\n");
#endif

        // This allows the compiler to optimize the swap at compile time if possible.
        if (!detail::is_constant_evaluated()) {
            alignas(T) std::byte buffer[N * sizeof(T)]{};
            std::memcpy(buffer, this->data(), N * sizeof(T));
            std::memcpy(this->data(), other.data(), N * sizeof(T));
            std::memcpy(other.data(), buffer, N * sizeof(T));
        } else {
            for (size_type i = 0; i < N; ++i)
            {
                T temp = std::move(this->data_[i]);
                this->data_[i] = std::move(other.data_[i]);
                other.data_[i] = std::move(temp);
            }
        }

    }

    /*!
     * \brief  Exchange the contents of *this with those of another array of the same size N.
     *
     * \param  other  The other Array to swap with.
     *
     * \note   [SWS_CORE_01242]
     * \note   This function is marked constexpr so that, if T's move construction and move assignment are constexpr,
     *         the swap can be performed at compile time.
     */
    template <typename U = T, size_type M = N>
    constexpr auto swap(Array& other) 
#ifdef ARA_CORE_ARRAY_ENABLE_CONDITIONAL_EXCEPTIONS
        noexcept(noexcept(std::swap(std::declval<T&>(), std::declval<T&>())))
#else
        noexcept
#endif
    -> std::enable_if_t<!std::is_trivially_copyable_v<U> || (M == 0), void>
    {   
#ifndef ARA_CORE_ARRAY_ENABLE_CONDITIONAL_EXCEPTIONS
        // Ensure T's swap is noexcept if exceptions are disabled
        static_assert(noexcept(std::swap(std::declval<T&>(), std::declval<T&>())),
            "\n[ERROR] ara::core::Array: The type T's swap operation must be noexcept when exceptions are disabled.\n");
#endif

        if constexpr (M > 0)
        {
            for (size_type i = 0; i < N; ++i)
            {
                T temp = std::move(this->data_[i]);
                this->data_[i] = std::move(other.data_[i]);
                other.data_[i] = std::move(temp);
            }
        }
    }

private:

    /*!
     * \brief Logs + terminates upon array-access-out-of-range for ara::core::Array.
     * \param location     The stripped file/line location (e.g., "file.cpp:123").
     * \param invalidIndex The invalid index that was requested.
     * \param arraySize    The total number of elements in the array.
     *
     * \details
     * - Logs the violation and terminates the process.
     * - [[noreturn]] ensures that the compiler knows this function will not return.
     *
     * \note  [SWS_CORE_13017], [SWS_CORE_00090], [SWS_CORE_00091]
     */
    [[noreturn]] inline auto TriggerOutOfRangeViolation(std::string_view location,
                                                        size_type invalidIndex,
                                                        size_type arraySize) const noexcept -> void
    {   
        auto& violation_trigger = ara::core::internal::ViolationHandler::Instance();
        violation_trigger.TriggerArrayAccessOutOfRangeViolation(location, invalidIndex, arraySize);
    }

};

/**********************************************************************************************************************
 *  NON-MEMBER FUNCTIONS
 *********************************************************************************************************************/

/********************************************************************************************
 *  get<I> (lvalue, rvalue, const) [SWS_CORE_01282], [SWS_CORE_01283], [SWS_CORE_01284]
 ********************************************************************************************/
/*!
 * \brief   Retrieves the I-th element (mutable reference) from \c arr.
 *
 * \tparam  I   The compile-time index.
 * \tparam  T   The element type.
 * \tparam  N   The array size.
 *
 * \param   arr The array from which to retrieve the element.
 * \return      Mutable reference to the I-th element.
 *
 * \note  [SWS_CORE_01282]
 * \note  If \c I >= N, compile-time static_assert fails ("out of range").
 */
template <std::size_t I, typename T, std::size_t N>
constexpr auto get(Array<T, N>& arr) noexcept -> T&
{
    static_assert(I < N,
        "\n[ERROR] get<I>() out of range!\n"
        "        I must be less than N in ara::core::Array.\n");
    return arr.data()[I];
}

/*!
 * \brief   Retrieves the I-th element (rvalue reference) from an rvalue \c arr.
 *
 * \tparam  I   The compile-time index.
 * \tparam  T   The element type.
 * \tparam  N   The array size.
 *
 * \param   arr The rvalue array from which to retrieve the element.
 * \return      Rvalue reference to the I-th element.
 *
 * \note  [SWS_CORE_01283]
 * \note  If \c I >= N, compile-time static_assert fails ("out of range").
 */
template <std::size_t I, typename T, std::size_t N>
constexpr auto get(Array<T, N>&& arr) noexcept -> T&&
{
    static_assert(I < N,
        "\n[ERROR] get<I>() out of range!\n"
        "        I must be less than N in ara::core::Array.\n");
    return std::move(arr.data()[I]);
}

/*!
 * \brief   Retrieves the I-th element (const reference) from a \c const \c arr.
 *
 * \tparam  I   The compile-time index.
 * \tparam  T   The element type.
 * \tparam  N   The array size.
 *
 * \param   arr The const array from which to retrieve the element.
 * \return      Const reference to the I-th element.
 *
 * \note  [SWS_CORE_01284]
 * \note  If \c I >= N, compile-time static_assert fails ("out of range").
 */
template <std::size_t I, typename T, std::size_t N>
constexpr auto get(const Array<T, N>& arr) noexcept -> const T&
{
    static_assert(I < N,
        "\n[ERROR] get<I>() out of range!\n"
        "        I must be less than N in ara::core::Array.\n");
    return arr.data()[I];
}

/********************************************************************************************
 *  Comparison operators [SWS_CORE_01290..01295]
 ********************************************************************************************/
/*!
 * \brief  Checks if two Arrays have \e equal content (elementwise).
 *
 * \tparam T  The type of elements stored in the arrays.
 * \tparam N  The number of elements in the arrays.
 * \param  lhs The first array to compare.
 * \param  rhs The second array to compare.
 * \return \c true if all elements are equal; \c false otherwise.
 *
 * \note   [SWS_CORE_01290]
 */
template <typename T, std::size_t N>
constexpr auto operator==(const Array<T, N>& lhs, const Array<T, N>& rhs)
#ifdef ARA_CORE_ARRAY_ENABLE_CONDITIONAL_EXCEPTIONS
    noexcept(noexcept(std::declval<T&>() == std::declval<T&>()))
#else
    noexcept
#endif
-> bool
{
#ifndef ARA_CORE_ARRAY_ENABLE_CONDITIONAL_EXCEPTIONS
    static_assert(noexcept(std::declval<T&>() == std::declval<T&>()),
        "\n[ERROR] in ara::core::Array: The type T's operator== must be marked 'noexcept' when exceptions are disabled.\n");
#endif

    for (std::size_t i = 0; i < N; ++i) {
        if (!(lhs[i] == rhs[i])) {
            return false;
        }
    }
    return true;
}

/*!
 * \brief  Checks if two Arrays differ in content.
 *
 * \tparam T  The type of elements stored in the arrays.
 * \tparam N  The number of elements in the arrays.
 * \param  lhs The first array to compare.
 * \param  rhs The second array to compare.
 * \return \c true if any element differs; \c false otherwise.
 *
 * \note   [SWS_CORE_01291]
 */
template <typename T, std::size_t N>
constexpr auto operator!=(const Array<T, N>& lhs, const Array<T, N>& rhs)
#ifdef ARA_CORE_ARRAY_ENABLE_CONDITIONAL_EXCEPTIONS
    noexcept(noexcept(!(lhs == rhs)))
#else
    noexcept
#endif
-> bool
{
#ifndef ARA_CORE_ARRAY_ENABLE_CONDITIONAL_EXCEPTIONS
    static_assert(noexcept(!(lhs == rhs)),
        "\n[ERROR] in ara::core::Array: The operator!= must be marked 'noexcept' when exceptions are disabled.\n");
#endif

    return !(lhs == rhs);
}

/*!
 * \brief  Lexicographical compare: returns true if \c lhs < \c rhs.
 *
 * \tparam T  The type of elements stored in the arrays.
 * \tparam N  The number of elements in the arrays.
 * \param  lhs The first array to compare.
 * \param  rhs The second array to compare.
 * \return \c true if \c lhs is lexicographically less than \c rhs; \c false otherwise.
 *
 * \note   [SWS_CORE_01292]
 */
template <typename T, std::size_t N>
constexpr auto operator<(const Array<T, N>& lhs, const Array<T, N>& rhs)
#ifdef ARA_CORE_ARRAY_ENABLE_CONDITIONAL_EXCEPTIONS
    noexcept(noexcept(std::declval<T&>() < std::declval<T&>()))
#else
    noexcept
#endif
-> bool
{
#ifndef ARA_CORE_ARRAY_ENABLE_CONDITIONAL_EXCEPTIONS
    static_assert(noexcept(std::declval<T&>() < std::declval<T&>()),
        "\n[ERROR] in ara::core::Array: The type T's operator< must be marked 'noexcept' when exceptions are disabled.\n");
#endif

    return detail::lex_compare(lhs.begin(), lhs.end(),
                               rhs.begin(), rhs.end());
}

/*!
 * \brief  Lexicographical compare: returns true if \c lhs <= \c rhs.
 *
 * \tparam T  The type of elements stored in the arrays.
 * \tparam N  The number of elements in the arrays.
 * \param  lhs The first array to compare.
 * \param  rhs The second array to compare.
 * \return \c true if \c lhs is lexicographically less than or equal to \c rhs; \c false otherwise.
 *
 * \note   [SWS_CORE_01294]
 */
template <typename T, std::size_t N>
constexpr auto operator<=(const Array<T, N>& lhs, const Array<T, N>& rhs)
#ifdef ARA_CORE_ARRAY_ENABLE_CONDITIONAL_EXCEPTIONS
    noexcept(noexcept(!(rhs < lhs)))
#else
    noexcept
#endif
-> bool
{
#ifndef ARA_CORE_ARRAY_ENABLE_CONDITIONAL_EXCEPTIONS
    static_assert(noexcept(!(rhs < lhs)),
        "\n[ERROR] in ara::core::Array: The operator<= must be marked 'noexcept' when exceptions are disabled.\n");
#endif

    return !(rhs < lhs);
}

/*!
 * \brief  Lexicographical compare: returns true if \c lhs > \c rhs.
 *
 * \tparam T  The type of element in the Array.
 * \tparam N  The number of elements in the Array.
 * \param  lhs The left-hand side of the comparison.
 * \param  rhs The right-hand side of the comparison.
 * \return \c true if \c lhs is lexicographically greater than \c rhs; \c false otherwise.
 *
 * \note   [SWS_CORE_01293]
 */
template <typename T, std::size_t N>
constexpr auto operator>(const Array<T, N>& lhs, const Array<T, N>& rhs)
#ifdef ARA_CORE_ARRAY_ENABLE_CONDITIONAL_EXCEPTIONS
    noexcept(noexcept(rhs < lhs))
#else
    noexcept
#endif
-> bool
{
#ifndef ARA_CORE_ARRAY_ENABLE_CONDITIONAL_EXCEPTIONS
    static_assert(noexcept(rhs < lhs),
        "\n[ERROR] in ara::core::Array: The operator> must be marked 'noexcept' when exceptions are disabled.\n");
#endif

    return (rhs < lhs);
}

/*!
 * \brief  Lexicographical compare: returns true if \c lhs >= \c rhs.
 *
 * \tparam T  The type of element in the Array.
 * \tparam N  The number of elements in the Array.
 * \param  lhs The left-hand side of the comparison.
 * \param  rhs The right-hand side of the comparison.
 * \return \c true if \c lhs is lexicographically greater than or equal to \c rhs; \c false otherwise.
 *
 * \note   [SWS_CORE_01295]
 */
template <typename T, std::size_t N>
constexpr auto operator>=(const Array<T, N>& lhs, const Array<T, N>& rhs)
#ifdef ARA_CORE_ARRAY_ENABLE_CONDITIONAL_EXCEPTIONS
    noexcept(noexcept(!(lhs < rhs)))
#else
    noexcept
#endif
-> bool
{
#ifndef ARA_CORE_ARRAY_ENABLE_CONDITIONAL_EXCEPTIONS
    static_assert(noexcept(!(lhs < rhs)),
        "\n[ERROR] in ara::core::Array: The operator>= must be marked 'noexcept' when exceptions are disabled.\n");
#endif

    return !(lhs < rhs);
}


/********************************************************************************************
 *  swap (Non-Member Function) [SWS_CORE_01296]
 *********************************************************************************************/
/*!
 * \brief  Overload of std::swap for ara::core::Array, calls Array::swap internally.
 *
 * \tparam T  The type of elements stored in the arrays.
 * \tparam N  The number of elements in the arrays.
 * \param  lhs The first array to swap.
 * \param  rhs The second array to swap.
 *
 * \note   [SWS_CORE_01296]
 * \note   This function enables the use of \c std::swap with ara::core::Array.
 *         It delegates the swapping to the member \c swap() function of Array.
 */
template <typename T, std::size_t N>
constexpr auto swap(Array<T, N>& lhs, Array<T, N>& rhs)
#ifdef ARA_CORE_ARRAY_ENABLE_CONDITIONAL_EXCEPTIONS
    noexcept(noexcept(lhs.swap(rhs)))
#else
    noexcept
#endif
-> void
{
#ifndef ARA_CORE_ARRAY_ENABLE_CONDITIONAL_EXCEPTIONS
    // Ensure T's swap is noexcept if exceptions are disabled
    static_assert(noexcept(lhs.swap(rhs)),
        "\n[ERROR] ara::core::Array: The type T's swap operation must be noexcept when exceptions are disabled.\n");
#endif

    lhs.swap(rhs);
}

/********************************************************************************************
 *  SECTION: SFINAE Protection
 ********************************************************************************************/

/*!
 * \brief Provides user-friendly compile-time error messages for invalid equality comparisons
 *        between arrays of different types or sizes.
 *
 * \tparam T  Type of elements in the first array.
 * \tparam N  Number of elements in the first array.
 * \tparam U  Type of elements in the second array.
 * \tparam M  Number of elements in the second array.
 *
 * \param lhs The first array (unused parameter, for type deduction only).
 * \param rhs The second array (unused parameter, for type deduction only).
 *
 * \details
 *  - Prevents accidental equality comparisons between arrays of different types or sizes.
 *  - Triggers clear and descriptive static assertions at compile time.
 *
 * \note [SWS_CORE_01290]
 */
template <typename T, std::size_t N, typename U, std::size_t M>
constexpr auto operator==(const Array<T, N>& /*lhs*/, const Array<U, M>& /*rhs*/) noexcept
    -> std::enable_if_t<!(std::is_same_v<T, U> && (N == M)), bool>
{
    static_assert(std::is_same_v<T, U>,
        "\n[ERROR] Cannot compare arrays of different types!\n"
        "        (operator==(Array<T,N>&, Array<U,M>&)) in ara::core::Array.\n");

    static_assert(N == M,
        "\n[ERROR] Cannot compare arrays of different sizes!\n"
        "        (operator==(Array<T,N>&, Array<U,M>&)) in ara::core::Array.\n");

    return false;
}

/*!
 * \brief Provides user-friendly compile-time error messages for invalid inequality comparisons
 *        between arrays of different types or sizes.
 *
 * \tparam T  Type of elements in the first array.
 * \tparam N  Number of elements in the first array.
 * \tparam U  Type of elements in the second array.
 * \tparam M  Number of elements in the second array.
 *
 * \param lhs The first array (unused parameter, for type deduction only).
 * \param rhs The second array (unused parameter, for type deduction only).
 *
 * \details
 *  - Prevents accidental inequality comparisons between arrays of different types or sizes.
 *  - Triggers clear and descriptive static assertions at compile time.
 *
 * \note [SWS_CORE_01291]
 */
template <typename T, std::size_t N, typename U, std::size_t M>
constexpr auto operator!=(const Array<T, N>& /*lhs*/, const Array<U, M>& /*rhs*/) noexcept
    -> std::enable_if_t<!(std::is_same_v<T, U> && (N == M)), bool>
{
    static_assert(std::is_same_v<T, U>,
        "\n[ERROR] Cannot compare arrays of different types!\n"
        "        (operator!=(Array<T,N>&, Array<U,M>&)) in ara::core::Array.\n");

    static_assert(N == M,
        "\n[ERROR] Cannot compare arrays of different sizes!\n"
        "        (operator!=(Array<T,N>&, Array<U,M>&)) in ara::core::Array.\n");

    return true;
}
/*!
 * \brief Provides user-friendly compile-time error messages for invalid less-than comparisons
 *        between arrays of different types or sizes.
 *
 * \tparam T  Type of elements in the first array.
 * \tparam N  Number of elements in the first array.
 * \tparam U  Type of elements in the second array.
 * \tparam M  Number of elements in the second array.
 *
 * \param lhs The first array (unused parameter, for type deduction only).
 * \param rhs The second array (unused parameter, for type deduction only).
 *
 * \details
 *  - Prevents accidental less-than comparisons between arrays of different types or sizes.
 *  - Triggers clear and descriptive static assertions at compile time.
 *
 * \note [SWS_CORE_01292]
 */
template <typename T, std::size_t N, typename U, std::size_t M>
constexpr auto operator<(const Array<T, N>& /*lhs*/, const Array<U, M>& /*rhs*/) noexcept
    -> std::enable_if_t<!(std::is_same_v<T, U> && (N == M)), bool>
{
    static_assert(std::is_same_v<T, U>,
        "\n[ERROR] Cannot compare arrays of different types!\n"
        "        (operator<(Array<T,N>&, Array<U,M>&)) in ara::core::Array.\n");

    static_assert(N == M,
        "\n[ERROR] Cannot compare arrays of different sizes!\n"
        "        (operator<(Array<T,N>&, Array<U,M>&)) in ara::core::Array.\n");

    return false;
}

/*!
 * \brief Provides user-friendly compile-time error messages for invalid less-than-or-equal comparisons
 *        between arrays of different types or sizes.
 *
 * \tparam T  Type of elements in the first array.
 * \tparam N  Number of elements in the first array.
 * \tparam U  Type of elements in the second array.
 * \tparam M  Number of elements in the second array.
 *
 * \param lhs The first array (unused parameter, for type deduction only).
 * \param rhs The second array (unused parameter, for type deduction only).
 *
 * \details
 *  - Prevents accidental less-than-or-equal comparisons between arrays of different types or sizes.
 *  - Triggers clear and descriptive static assertions at compile time.
 *
 * \note [SWS_CORE_01294]
 */
template <typename T, std::size_t N, typename U, std::size_t M>
constexpr auto operator<=(const Array<T, N>& /*lhs*/, const Array<U, M>& /*rhs*/) noexcept
    -> std::enable_if_t<!(std::is_same_v<T, U> && (N == M)), bool>
{
    static_assert(std::is_same_v<T, U>,
        "\n[ERROR] Cannot compare arrays of different types!\n"
        "        (operator<=(Array<T,N>&, Array<U,M>&)) in ara::core::Array.\n");

    static_assert(N == M,
        "\n[ERROR] Cannot compare arrays of different sizes!\n"
        "        (operator<=(Array<T,N>&, Array<U,M>&)) in ara::core::Array.\n");

    return true;
}

/*!
 * \brief Provides user-friendly compile-time error messages for invalid greater-than comparisons
 *        between arrays of different types or sizes.
 *
 * \tparam T  Type of elements in the first array.
 * \tparam N  Number of elements in the first array.
 * \tparam U  Type of elements in the second array.
 * \tparam M  Number of elements in the second array.
 *
 * \param lhs The first array (unused parameter, for type deduction only).
 * \param rhs The second array (unused parameter, for type deduction only).
 *
 * \details
 *  - Prevents accidental greater-than comparisons between arrays of different types or sizes.
 *  - Triggers clear and descriptive static assertions at compile time.
 *
 * \note [SWS_CORE_01293]
 */
template <typename T, std::size_t N, typename U, std::size_t M>
constexpr auto operator>(const Array<T, N>& /*lhs*/, const Array<U, M>& /*rhs*/) noexcept
    -> std::enable_if_t<!(std::is_same_v<T, U> && (N == M)), bool>
{
    static_assert(std::is_same_v<T, U>,
        "\n[ERROR] Cannot compare arrays of different types!\n"
        "        (operator>(Array<T,N>&, Array<U,M>&)) in ara::core::Array.\n");

    static_assert(N == M,
        "\n[ERROR] Cannot compare arrays of different sizes!\n"
        "        (operator>(Array<T,N>&, Array<U,M>&)) in ara::core::Array.\n");

    return false;
}

/*!
 * \brief Provides user-friendly compile-time error messages for invalid greater-than-or-equal comparisons
 *        between arrays of different types or sizes.
 *
 * \tparam T  Type of elements in the first array.
 * \tparam N  Number of elements in the first array.
 * \tparam U  Type of elements in the second array.
 * \tparam M  Number of elements in the second array.
 *
 * \param lhs The first array (unused parameter, for type deduction only).
 * \param rhs The second array (unused parameter, for type deduction only).
 *
 * \details
 *  - Prevents accidental greater-than-or-equal comparisons between arrays of different types or sizes.
 *  - Triggers clear and descriptive static assertions at compile time.
 *
 * \note [SWS_CORE_01295]
 */
template <typename T, std::size_t N, typename U, std::size_t M>
constexpr auto operator>=(const Array<T, N>& /*lhs*/, const Array<U, M>& /*rhs*/) noexcept
    -> std::enable_if_t<!(std::is_same_v<T, U> && (N == M)), bool>
{
    static_assert(std::is_same_v<T, U>,
        "\n[ERROR] Cannot compare arrays of different types!\n"
        "        (operator>=(Array<T,N>&, Array<U,M>&)) in ara::core::Array.\n");

    static_assert(N == M,
        "\n[ERROR] Cannot compare arrays of different sizes!\n"
        "        (operator>=(Array<T,N>&, Array<U,M>&)) in ara::core::Array.\n");

    return true;
}

/*!
 * \brief Provides user-friendly compile-time error messages for invalid swap operations
 *        between arrays of different types or sizes.
 *
 * \tparam T  Type of elements in the first array.
 * \tparam N  Number of elements in the first array.
 * \tparam U  Type of elements in the second array.
 * \tparam M  Number of elements in the second array.
 *
 * \param lhs The first array to swap (unused parameter, for type deduction only).
 * \param rhs The second array to swap (unused parameter, for type deduction only).
 *
 * \details
 *  - Prevents accidental swaps between arrays of different types or sizes.
 *  - Triggers clear and descriptive static assertions at compile time.
 *
 * \note [SWS_CORE_01296]
 */
template <typename T, std::size_t N, typename U, std::size_t M>
constexpr auto swap(Array<T, N>& /*lhs*/, Array<U, M>& /*rhs*/) noexcept
    -> std::enable_if_t<!(std::is_same_v<T, U> && (N == M)), void>
{
    static_assert(std::is_same_v<T, U>,
        "\n[ERROR] Cannot swap arrays of different types!\n"
        "        (swap(Array<T,N>&, Array<U,M>&)) in ara::core::Array.\n");

    static_assert(N == M,
        "\n[ERROR] Cannot swap arrays of different sizes!\n"
        "        (swap(Array<T,N>&, Array<U,M>&)) in ara::core::Array.\n");
}
 

} // namespace core
} // namespace ara

#endif // OPEN_AA_ADAPTIVE_AUTOSAR_LIBS_INCLUDE_ARA_CORE_ARRAY_H_