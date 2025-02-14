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
 *  INCLUDES
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
#include <cstddef>       // For std::size_t, std::ptrdiff_t
#include <iterator>      // For std::reverse_iterator
#include <algorithm>     // For std::lexicographical_compare, std::swap_ranges, std::fill_n
#include <type_traits>   // For std::is_nothrow_move_constructible, std::is_nothrow_move_assignable, etc.
#include <utility>       // For std::declval, std::move, std::forward

#include "ara/core/internal/location_utils.h"       // For capturing file/line details
#include "ara/core/internal/violation_handler.h"    // To Trigger the violation

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
 *  SECTION: Forward Declaration
 *********************************************************************************************************************/
/*!
 * \brief  Forward declaration of the Array class template.
 */
template <typename T, std::size_t N>
class Array;

/**********************************************************************************************************************
 *  SECTION: Violation Handling
 *********************************************************************************************************************/
/*!
 * \brief  Contains internal details for handling Violations according to AUTOSAR requirements.
 *
 * [SWS_CORE_00090], [SWS_CORE_00091] – Standardized Violations result in abnormal termination,
 * with a diagnostic message describing the process and location.
 *
 * [SWS_CORE_13017] – For Array, the out-of-range message format should mention:
 *   "Violation detected in {processIdentifier} at {location}: Array access out of range..."
 */
namespace detail {

/*!
 * \brief Helper trait to determine if T[N] can be brace-initialized with Args... without narrowing conversions.
 *
 * \tparam T     The type of elements in the array.
 * \tparam N     The size of the array.
 * \tparam Args  The types of arguments provided for initialization.
 *
 * \details
 * - Utilizes SFINAE to check if brace-initialization of T[N] with Args... is possible.
 * - If brace-initialization would cause narrowing conversions, the trait evaluates to false.
 *
 * \note  [SWS_CORE_01241]
 */
template <typename T, std::size_t N, typename... Args>
struct is_brace_initializable_array {
private:
    template <typename U, typename... A>
    static auto test(int) -> decltype(U{std::declval<A>()...}, std::true_type{});

    template <typename, typename...>
    static auto test(...) -> std::false_type;

public:
    static constexpr bool value = decltype(test<T[N], Args...>(0))::value;
};

/*!
 * \brief Helper trait to detect if Args... is exactly one ara::core::Array<T, N>
 *
 * \tparam Args  The types of arguments
 *
 * \note  Prevents the variadic constructor from being selected when an Array is passed as a single argument.
 */
template<typename... Args>
struct is_single_same_array : std::false_type {};

// Partial specialization for a single ara::core::Array<T, N> argument
template<typename T, std::size_t N>
struct is_single_same_array<Array<T, N>> : std::true_type {};

// Variable template for convenience
template<typename... Args>
constexpr bool is_single_same_array_v = is_single_same_array<std::decay_t<Args>...>::value;


/*!
 * \brief Performs a lexicographical comparison between two ranges.
 *
 * \tparam InputIt1 The type of the input iterator for the first range.
 * \tparam InputIt2 The type of the input iterator for the second range.
 * \param first1 An iterator pointing to the first element of the first range.
 * \param last1  An iterator pointing past the last element of the first range.
 * \param first2 An iterator pointing to the first element of the second range.
 * \param last2  An iterator pointing past the last element of the second range.
 * \return \c true if the first range is lexicographically less than the second range; \c false otherwise.
 *
 * \details
 * The function compares the elements in the ranges [first1, last1) and [first2, last2) one by one.
 * - For each pair of corresponding elements, if the element from the first range is less than the element
 *   from the second range, the function returns \c true.
 * - If the element from the second range is less than the element from the first range, the function returns \c false.
 * - If the elements are equal, the comparison continues to the next pair.
 * - When the end of one of the ranges is reached:
 *     - If the first range is exhausted but the second range still has elements, the first range is considered
 *       lexicographically less, so the function returns \c true.
 *     - Otherwise, the function returns \c false.
 *
 * \note The function is declared as \c constexpr, so if both the iterators and the element comparison 
 * are themselves \c constexpr, the entire operation can be evaluated at compile time.
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
 
}  // namespace detail

/**********************************************************************************************************************
 *  SECTION: Array Storage Base Classes
 *********************************************************************************************************************/
/*!
 * \brief  Base class for Array storage.
 * \tparam T  The type of elements.
 * \tparam N  The size of the array.
 * \tparam B  A boolean indicating whether N > 0.
 *
 * \details
 * - Splits out storage to handle partial specialization for N=0 vs N>0.
 *
 * \note  [SWS_CORE_01201]
 */
template <typename T, std::size_t N, bool B = (N > 0)>
struct ArrayStorage;

/*!
 * \brief  Primary template for Array storage when N > 0.
 *
 * \note  [SWS_CORE_01201]
 */
template <typename T, std::size_t N>
struct ArrayStorage<T, N, true>
{
protected:
    /*! \brief Actual storage for N elements of type T. */
    T data_[N]{};

    /*!
     * \brief Variadic constructor to initialize data_ with up to N arguments (brace-initialization).
     * \param args constructor args
     *
     * \note  [SWS_CORE_01201], [SWS_CORE_01214], [SWS_CORE_01215], [SWS_CORE_01241]
     */
    template <typename... Args,
              typename = std::enable_if_t<
                  (sizeof...(Args) <= N) &&
                  std::conjunction_v<std::is_convertible<Args, T>...> &&
                  detail::is_brace_initializable_array<T, N, Args...>::value
              >>
    constexpr ArrayStorage(Args&&... args) 
#ifdef ARA_CORE_ARRAY_ENABLE_CONDITIONAL_EXCEPTIONS
        noexcept(std::conjunction_v<std::is_nothrow_constructible<T, Args&&>...>)
#else
        noexcept
#endif
        : data_{std::forward<Args>(args)...}
    {
        // No further logic needed
    }

    /*! \brief Default constructor for the storage, zero-initializes data_. */
    constexpr ArrayStorage() noexcept = default;
};

/*!
 * \brief  Partial specialization for Array storage when N = 0 => no actual array elements.
 *
 * \note  [SWS_CORE_01201]
 */
template <typename T, std::size_t N>
struct ArrayStorage<T, N, false>
{
protected:
    // For N=0, we have no data_ at all.
    constexpr ArrayStorage() noexcept = default;

    // Variadic constructor for N=0 should not accept any arguments
    template <typename... Args,
              typename = std::enable_if_t<sizeof...(Args) == 0>>
    constexpr ArrayStorage(Args&&...) noexcept
    {
        // No operation needed for zero-sized array
    }
};

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
class Array final : private ArrayStorage<T, N>
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
    static_assert(std::is_nothrow_move_constructible_v<T> &&
                  std::is_nothrow_move_assignable_v<T> &&
                  std::is_nothrow_copy_constructible_v<T> &&
                  std::is_nothrow_copy_assignable_v<T>,
                "\n[ERROR] in ara::core::Array: The type T must be move and copy constructible\n"
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
                  (detail::is_brace_initializable_array<T, N, Args...>::value) &&
                  // Condition #4: Prevent constructor from being selected when Args... is exactly Array<T, N>
                  (!detail::is_single_same_array_v<Args...>)
              >>
    constexpr Array(Args&&... args) 
#ifdef ARA_CORE_ARRAY_ENABLE_CONDITIONAL_EXCEPTIONS
        noexcept(std::conjunction_v<std::is_nothrow_constructible<T, Args&&>...>)
#else
        noexcept
#endif
        : ArrayStorage<T, N>(std::forward<Args>(args)...)
    {
        // Base class handles data_ initialization.
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
             (!detail::is_brace_initializable_array<T, N, Args...>::value))
        >,
        int = 0
    >
    constexpr Array(Args&&...)
    {
        static_assert(sizeof...(Args) <= N,
            "\n[ERROR] Too many arguments passed to Array<T,N> constructor!\n"
            "        Up to N elements are allowed.\n");

        static_assert(std::conjunction_v<std::is_convertible<Args, T>...>,
            "\n[ERROR] One or more arguments cannot be converted to T.\n");

        static_assert(detail::is_brace_initializable_array<T, N, Args...>::value,
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
     * \brief  Unchecked subscript (mutable). Out-of-range => undefined behavior ([SWS_CORE_01266]).
     *
     * \param  idx  The index to access.
     * \return     Reference to the element at index \c idx.
     *
     * \note  [SWS_CORE_01265], [SWS_CORE_01266] Accessing a non-existing element through this operation is undefined behavior. Use the function
     *          at for checked access to the elements.
     */
    constexpr auto operator[](size_type idx) noexcept -> T&
    {
        // Per [SWS_CORE_01266], operator[] does NOT do bound checks. 
        // Accessing out-of-range is undefined behavior.
        return this->at(idx);
    }

    /*!
     * \brief  Unchecked subscript (const). Out-of-range => undefined behavior.
     *
     * \param  idx  The index to access.
     * \return     Const reference to the element at index \c idx.
     *
     * \note  [SWS_CORE_01265], [SWS_CORE_01266] Accessing a non-existing element through this operation is undefined behavior. Use the function
     *          at for checked access to the elements.
     */
    constexpr auto operator[](size_type idx) const noexcept -> const T&
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
    constexpr auto at(size_type idx) noexcept -> T&
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
    constexpr auto at(size_type idx) const noexcept -> const T&
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
    constexpr auto front() noexcept -> T&
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
    constexpr auto front() const noexcept -> const T&
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
    constexpr auto back() noexcept -> T&
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
    constexpr auto back() const noexcept -> const T&
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
    constexpr auto data() noexcept -> T*
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
    constexpr auto data() const noexcept -> const T*
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
    constexpr auto size() const noexcept -> std::size_t
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
    constexpr auto max_size() const noexcept -> std::size_t
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
    constexpr auto fill(const T& val)
#ifdef ARA_CORE_ARRAY_ENABLE_CONDITIONAL_EXCEPTIONS
    noexcept(std::is_nothrow_copy_assignable_v<T>)
#else
    noexcept
#endif
    -> void
    {

#ifndef ARA_CORE_ARRAY_ENABLE_CONDITIONAL_EXCEPTIONS
        // Ensure T's fill is noexcept if exceptions are disabled
        static_assert(std::is_nothrow_copy_assignable_v<T>,
            "\n[ERROR] ara::core::Array: The type T's fill operation must be noexcept when exceptions are disabled.\n");
#endif

        /* If N > 0, loop over the array and assign val to each element.
           This loop is constexpr-friendly so that it can be evaluated at compile time if T's operations are constexpr. */
        if constexpr (N > 0) {
            for (std::size_t i = 0; i < N; ++i) {
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
    constexpr auto swap(Array& other) 
#ifdef ARA_CORE_ARRAY_ENABLE_CONDITIONAL_EXCEPTIONS
        noexcept(noexcept(std::swap(std::declval<T&>(), std::declval<T&>())))
#else
        noexcept
#endif
        -> void
    {   
#ifndef ARA_CORE_ARRAY_ENABLE_CONDITIONAL_EXCEPTIONS
        // Ensure T's swap is noexcept if exceptions are disabled
        static_assert(noexcept(std::swap(std::declval<T&>(), std::declval<T&>())),
            "\n[ERROR] ara::core::Array: The type T's swap operation must be noexcept when exceptions are disabled.\n");
#endif

        for (std::size_t i = 0; i < N; ++i)
        {
            T temp = std::move(this->data_[i]);
            this->data_[i] = std::move(other.data_[i]);
            other.data_[i] = std::move(temp);
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
                                                        std::size_t invalidIndex,
                                                        std::size_t arraySize) const noexcept -> void
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
    return arr[I];
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
    return std::move(arr[I]);
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
    return arr[I];
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

/*!
 * \brief  Attempting to swap arrays of different types or sizes => fail with a user-friendly message.
 *
 * \tparam T  The type of elements in the first array.
 * \tparam N  The number of elements in the first array.
 * \tparam U  The type of elements in the second array.
 * \tparam M  The number of elements in the second array.
 * \param  lhs The first array to swap.
 * \param  rhs The second array to swap.
 *
 * \details
 * - Normally, the overload for \c Array<T,N> won't even match \c Array<U,M> if (T != U) or (N != M).
 *   But if it does, we produce an intentional compile-time error.
 *
 * \note   [SWS_CORE_01296]
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
