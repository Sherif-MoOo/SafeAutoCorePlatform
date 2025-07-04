/**********************************************************************************************************************
 *  PROJECT
 *  -------------------------------------------------------------------------------------------------------------------
 *  \verbatim
 *  OpenAA: Open Source Adaptive AUTOSAR Project (CXX_STANDARD 17)
 *  Author: Sherif Mohamed
 *  \endverbatim
 *  -------------------------------------------------------------------------------------------------------------------
 *  FILE DESCRIPTION
 *  -------------------------------------------------------------------------------------------------------------------
 *  \file       ara/core/array.h
 *  \brief      Definition and implementation of the ara::core::Array template class.
 *
 *  \details    This file defines and implements the ara::core::Array template class, a fixed-size array container
 *              designed for the OpenAA project. It provides functionalities similar to std::array with additional
 *              customizations to meet Adaptive AUTOSAR requirements, including violation handling and optimized 
 *              memory allocation. The implementation exceeds AUTOSAR requirements by providing C++26 features
 *              backported to C++17, comprehensive SFINAE for user-friendly errors, and extensive compiler
 *              compatibility.
 *
 *  \note       Based on the Adaptive AUTOSAR SWS R24-11 requirements for the "Array" type:
 *              - [SWS_CORE_01201] Definition of ara::core::Array
 *              - [SWS_CORE_01210-01220] Type aliases
 *              - [SWS_CORE_01240-01242] Constructors and operations  
 *              - [SWS_CORE_01250-01261] Iterator support
 *              - [SWS_CORE_01262-01274] Element access
 *              - [SWS_CORE_01280-01285] Tuple interface
 *              - [SWS_CORE_01290-01296] Comparison operators
 *              - [SWS_CORE_00040] No exceptions - violation handling
 *              - [SWS_CORE_11200] Behaves like std::array except for differences
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
#include <tuple>                                    // For std::tuple_size / tuple_element declarations
#include <cstring>                                  // For std::memcpy, std::memset
#include <iterator>                                 // For std::reverse_iterator

#include "ara/core/internal/utility.h"              // For utility functions and traits
#include "ara/core/algorithm.h"                     // For algorithm utilities
#include "ara/core/internal/location_utils.h"       // For capturing file/line details
#include "ara/core/internal/violation_handler.h"    // To Trigger the violation


/**********************************************************************************************************************
 *  TUPLE: INTERFACE SPECIALISATIONS
 *  ---------------------------------------------------------------------------------------------------------------
 *  std::tuple_size          [SWS_CORE_01280]
 *  std::tuple_element       [SWS_CORE_01281 / SWS_CORE_01285]
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

namespace std {

/*---------------------------------------------------------------------------------------------------------------
 *  ADL-enabled get() functions for ara::core::Array
 *-------------------------------------------------------------------------------------------------------------*/
template<size_t I, typename T, size_t N>
[[nodiscard]] constexpr auto get(ara::core::Array<T, N>& a) noexcept -> T& {
    return ara::core::get<I>(a);
}

template<size_t I, typename T, size_t N>
[[nodiscard]] constexpr auto get(const ara::core::Array<T, N>& a) noexcept -> const T& {
    return ara::core::get<I>(a);
}

template<size_t I, typename T, size_t N>
[[nodiscard]] constexpr auto get(ara::core::Array<T, N>&& a) noexcept -> T&& {
    return ara::core::get<I>(std::move(a));
}

template<size_t I, typename T, size_t N>
[[nodiscard]] constexpr auto get(const ara::core::Array<T, N>&& a) noexcept -> const T&& {
    return ara::core::get<I>(std::move(a));
}
  
/*---------------------------------------------------------------------------------------------------------------
 *  (1) tuple_size – "How many elements?"
 *-------------------------------------------------------------------------------------------------------------*/
/*!
 * \brief   Primary trait giving the fixed size N of ara::core::Array<T,N>.
 *
 * \tparam  T  Element type stored in the Array.
 * \tparam  N  Number of elements (array extent).
 *
 * \note    [SWS_CORE_01280] – must model a C++14 UnaryTypeTrait whose BaseCharacteristic
 *          is std::integral_constant<std::size_t,N>.
 */
template<typename T, std::size_t N>
struct tuple_size<ara::core::Array<T, N>>
    : std::integral_constant<std::size_t, N>  // UnaryTypeTrait
{};

/*---------------------------------------------------------------------------------------------------------------
 *  (2) tuple_element – "What is the type of element I?"
 *-------------------------------------------------------------------------------------------------------------*/
/*!
 * \brief   Yields the element **type** of ara::core::Array<T,N> at compile-time index \c I.
 *
 * \details
 *   • If I ≥ N the implementation triggers a compile-time error as mandated by the spec
 *     ("shall flag the condition I >= N as a compile error" – SWS_CORE_01281).\n
 *   • Because every element of ara::core::Array<T,N> has the same type \c T, the alias
 *     simply forwards `using type = T;` (SWS_CORE_01285).
 *
 * \tparam  I  Zero-based element index requested at compile time.
 * \tparam  T  Element type stored in the Array.
 * \tparam  N  Number of elements in the Array.
 */
template<std::size_t I, typename T, std::size_t N>
struct tuple_element<I, ara::core::Array<T, N>>
{
    static_assert(I < N,
        "\n[ERROR] std::tuple_element<I, ara::core::Array<T,N>> : "
        "index I is out of range (I >= N).\n");

    using type = T;  // every element is T
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
#ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
    // In Conditional Safe Mode, allow potentially-throwing types
#else
    /*!
     * \brief Enforce that T cannot throw exceptions during move or copy operations.
     *
     * \note  [SWS_CORE_00040]
     */
    static_assert(std::is_nothrow_destructible_v<T>,
                "\n[ERROR] in ara::core::Array: The type T must have a noexcept destructor.\n");

    static_assert(!std::is_default_constructible_v<T> || std::is_nothrow_constructible_v<T>,
                "\n[ERROR] in ara::core::Array: The type T must be noexcept default constructible or not default constructible.\n");

    static_assert(!std::is_move_constructible_v<T> || std::is_nothrow_move_constructible_v<T>,
                "\n[ERROR] in ara::core::Array: The type T must be noexcept move constructible or not move constructible.\n");

    static_assert(!std::is_move_assignable_v<T> || std::is_nothrow_move_assignable_v<T>,
                "\n[ERROR] in ara::core::Array: The type T must be noexcept move assignable or not move assignable.\n");

    static_assert(!std::is_copy_constructible_v<T> || std::is_nothrow_copy_constructible_v<T>,
                "\n[ERROR] in ara::core::Array: The type T must be noexcept copy constructible or not copy constructible.\n");

    static_assert(!std::is_copy_assignable_v<T> || std::is_nothrow_copy_assignable_v<T>,
                "\n[ERROR] in ara::core::Array: The type T must be noexcept copy assignable or not copy assignable.\n");
#endif

    using detail::ArrayStorage<T, N>::ArrayStorage; /*!< Inherit constructors from ArrayStorage */

    // -----------------------------------------------------------------------------------
    // TYPE ALIASES (public) [SWS_CORE_01210..01220]
    // -----------------------------------------------------------------------------------
    using value_type             = T;                                                   /*!< [SWS_CORE_01216]: Type of the elements   */
    using size_type              = std::size_t;                                         /*!< [SWS_CORE_01214]: Used for indexing      */
    using difference_type        = std::ptrdiff_t;                                      /*!< [SWS_CORE_01215]: Used for pointer diffs */
    using reference              = T&;                                                  /*!< [SWS_CORE_01210]: Type of a reference    */
    using const_reference        = const T&;                                            /*!< [SWS_CORE_01211]: Type of a const-ref    */
    using pointer                = T*;                                                  /*!< [SWS_CORE_01217]: Pointer to element     */
    using const_pointer          = const T*;                                            /*!< [SWS_CORE_01218]: Const pointer          */
    // Iterator type - raw pointer automatically satisfying all C++ standard requirements:
    // - Until C++17: LegacyRandomAccessIterator and LegacyContiguousIterator
    // - C++17+: Additionally LiteralType  
    // - C++20+: Additionally contiguous_iterator concept and ConstexprIterator   
    using iterator               = T*;                                                  /*!< [SWS_CORE_01212]: Iterator type          */
    using const_iterator         = const T*;                                            /*!< [SWS_CORE_01213]: Const iterator type    */
    using reverse_iterator       = std::reverse_iterator<iterator>;                     /*!< [SWS_CORE_01219]: Reverse iterator       */
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;               /*!< [SWS_CORE_01220]: Const reverse iterator */

    // -----------------------------------------------------------------------------------
    // 3) DEFAULT AND COPY/MOVE OPERATIONS [SWS_CORE_01201]
    // -----------------------------------------------------------------------------------
    constexpr Array()
    #ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
        noexcept(std::is_nothrow_default_constructible_v<T>)
    #else
        noexcept
    #endif
        = default; /*!< [SWS_CORE_01201]: Default constructor */
    
    constexpr Array(const Array&)
        #ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
            noexcept(std::is_nothrow_copy_constructible_v<T>)
        #else
            noexcept
        #endif
        = default; /*!< [SWS_CORE_01201]: Copy constructor */

    constexpr Array(Array&&)
        #ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
            noexcept(std::is_nothrow_move_constructible_v<T>)
        #else
            noexcept
        #endif
        = default; /*!< [SWS_CORE_01201]: Move constructor */
    
    constexpr auto operator=(const Array&)
        #ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
            noexcept(std::is_nothrow_copy_assignable_v<T>)
        #else
            noexcept
        #endif
        -> Array& = default; /*!< [SWS_CORE_01201]: Copy assignment */

    constexpr auto operator=(Array&&)
        #ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
            noexcept(std::is_nothrow_move_assignable_v<T>)
        #else
            noexcept
        #endif
        -> Array& = default; /*!< [SWS_CORE_01201]: Move assignment */
    
    ~Array()
        #ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
            noexcept(std::is_nothrow_destructible_v<T>)
        #else
            noexcept
        #endif
        = default; /*!< [SWS_CORE_01201]: Destructor */

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
    constexpr auto operator[](const ara::core::internal::InputWithLocation<size_type>& idx) noexcept -> reference
    {        
        static_assert(N > 0,
            "\n[ERROR] operator[] called on zero-sized Array!\n");
        
        const size_type& I = idx.input();

        if (detail::unlikely(I >= N)) {

            if (!detail::is_constant_evaluated()) {
                TriggerOutOfRangeViolation(
                    idx.info(),
                    I,
                    N
                );
            }

        }

        /*! \note compiler smart enough to bound check in consteval scope */
        return this->data_[I];
    }

    /*!
     * \brief  [old req before the update] Unchecked subscript (const). Out-of-range => undefined behavior.
     *
     * \param  idx  The index to access.
     * \return     Const reference to the element at index \c idx.
     *
     * \note  [SWS_CORE_01265], [SWS_CORE_01266] Accessing a non-existing element through this operation now will trigger a violation
     */
    constexpr auto operator[](const ara::core::internal::InputWithLocation<size_type>& idx) const noexcept -> const_reference
    {

        static_assert(N > 0,
            "\n[ERROR] operator[] called on zero-sized Array!\n");
        
        const size_type& I = idx.input();

        if (detail::unlikely(I >= N)) {

            if (!detail::is_constant_evaluated()) {
                TriggerOutOfRangeViolation(
                    idx.info(),
                    I,
                    N
                );
            }

        }

        /*! \note compiler smart enough to bound check in consteval scope */
        return this->data_[I];
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
    constexpr auto at(const ara::core::internal::InputWithLocation<size_type>& idx) noexcept -> reference
    {
        static_assert(N > 0,
            "\n[ERROR] at() called on zero-sized Array!\n");

        const size_type& I = idx.input();

        if (detail::unlikely(I >= N)) {

            if (!detail::is_constant_evaluated()) {
                TriggerOutOfRangeViolation(
                    idx.info(),
                    I,
                    N
                );
            }

        }

        /*! \note compiler smart enough to bound check in consteval scope */
        return this->data_[I];
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
    constexpr auto at(const ara::core::internal::InputWithLocation<size_type>& idx) const noexcept -> const_reference
    {

        static_assert(N > 0,
            "\n[ERROR] at() called on zero-sized Array!\n");

        const size_type& I = idx.input();

        if (detail::unlikely(I >= N)) {

            if (!detail::is_constant_evaluated()) {
                TriggerOutOfRangeViolation(
                    idx.info(),
                    I,
                    N
                );
            }

        }

        /*! \note compiler smart enough to bound check in consteval scope */
        return this->data_[I];
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
    [[nodiscard]] constexpr auto size() const noexcept -> size_type
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
    [[nodiscard]] constexpr auto max_size() const noexcept -> size_type
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
    [[nodiscard]] constexpr auto empty() const noexcept -> bool
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
#ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
    noexcept(std::is_nothrow_copy_assignable_v<T>)
#else
    noexcept
#endif
    -> std::enable_if_t<std::is_trivially_copyable_v<U> && 
                        std::is_trivially_constructible_v<U> && (M > 0), void>
    {

#ifndef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
        // Ensure T's fill is noexcept if exceptions are disabled
        static_assert(std::is_nothrow_copy_assignable_v<T>,
            "\n[ERROR] ara::core::Array: The type T's fill operation must be noexcept when exceptions are disabled.\n");
#endif
        
        if (!detail::is_constant_evaluated()) {
            // If not evaluated at compile time, use memset for performance
            if constexpr (sizeof(T) == 1) {
                // Byte-sized elements
                if constexpr (std::is_same_v<T, bool>) {
                    std::memset(this->data_, val ? 0x01 : 0x00, N);
                } else {
                    // Safe for char, unsigned char, int8_t, uint8_t
                    std::memset(this->data_, static_cast<unsigned char>(val), N);
                }
            } else if (val == T{}) {
                // Zero-fill optimization
                std::memset(this->data_, 0, N * sizeof(T));
            } else {
                // General case
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
#ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
    noexcept(std::is_nothrow_copy_assignable_v<T>)
#else
    noexcept
#endif
    -> std::enable_if_t<std::is_trivially_copyable_v<U> && 
                        !std::is_trivially_constructible_v<U> && (M > 0), void>
    {

#ifndef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
        // Ensure T's fill is noexcept if exceptions are disabled
        static_assert(std::is_nothrow_copy_assignable_v<T>,
            "\n[ERROR] ara::core::Array: The type T's fill operation must be noexcept when exceptions are disabled.\n");
#endif
        
        if (!detail::is_constant_evaluated()) {
            if constexpr (sizeof(T) == 1) {
                // Byte-sized elements
                if constexpr (std::is_same_v<T, bool>) {
                    std::memset(this->data_, val ? 0x01 : 0x00, N);
                } else {
                    // Safe for char, unsigned char, int8_t, uint8_t
                    std::memset(this->data_, static_cast<unsigned char>(val), N);
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
#ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
    noexcept(std::is_nothrow_copy_assignable_v<T>)
#else
    noexcept
#endif
    -> std::enable_if_t<!std::is_trivially_copyable_v<U> || (M == 0), void>
    {

#ifndef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
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
#ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
        noexcept(noexcept(std::swap(std::declval<T&>(), std::declval<T&>())))
#else
        noexcept
#endif
    -> std::enable_if_t<std::is_trivially_copyable_v<U> && (M > 0), void>
    {   
#ifndef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
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
                other.data_[i] = ara::core::exchange(this->data_[i],
                                                     std::move(other.data_[i]));
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
#ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
        noexcept(noexcept(std::swap(std::declval<T&>(), std::declval<T&>())))
#else
        noexcept
#endif
    -> std::enable_if_t<!std::is_trivially_copyable_v<U> || (M == 0), void>
    {   
#ifndef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
        // Ensure T's swap is noexcept if exceptions are disabled
        static_assert(noexcept(std::swap(std::declval<T&>(), std::declval<T&>())),
            "\n[ERROR] ara::core::Array: The type T's swap operation must be noexcept when exceptions are disabled.\n");
#endif

        if constexpr (M > 0)
        {
            for (size_type i = 0; i < N; ++i)
            {
                other.data_[i] = ara::core::exchange(this->data_[i],
                                                     std::move(other.data_[i]));
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
        violation_trigger.TriggerArrayAccessOutOfRangeViolation(
            ara::core::internal::ViolationHandler::ArrayKey{}, 
            location, 
            invalidIndex, 
            arraySize
        );
    }

};

/**********************************************************************************************************************
 *  DEDUCTION GUIDES (C++17)
 *********************************************************************************************************************/
/*!
 * \brief  Class template argument deduction guide for Array.
 *
 * \note   Enhancement beyond AUTOSAR requirements - enables: ara::core::Array arr{1, 2, 3, 4};
 */
template<typename T, typename... U>
Array(T, U...) -> Array<T, 1 + sizeof...(U)>;

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
[[nodiscard]] constexpr auto get(Array<T, N>& arr) noexcept -> T&
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
 */
template <std::size_t I, typename T, std::size_t N>
[[nodiscard]] constexpr auto get(Array<T, N>&& arr) noexcept -> T&&
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
 */
template <std::size_t I, typename T, std::size_t N>
[[nodiscard]] constexpr auto get(const Array<T, N>& arr) noexcept -> const T&
{
    static_assert(I < N,
        "\n[ERROR] get<I>() out of range!\n"
        "        I must be less than N in ara::core::Array.\n");
    return arr.data()[I];
}

/*!
 * \brief   Retrieves the I-th element (const rvalue reference) from a const rvalue \c arr.
 *
 * \tparam  I   The compile-time index.
 * \tparam  T   The element type.
 * \tparam  N   The array size.
 *
 * \param   arr The const rvalue array from which to retrieve the element.
 * \return      Const rvalue reference to the I-th element.
 *
 * \note  Enhancement for completeness.
 */
template <std::size_t I, typename T, std::size_t N>
[[nodiscard]] constexpr auto get(const Array<T, N>&& arr) noexcept -> const T&&
{
    static_assert(I < N,
        "\n[ERROR] get<I>() out of range!\n"
        "        I must be less than N in ara::core::Array.\n");
    return std::move(arr.data()[I]);
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
[[nodiscard]] constexpr auto operator==(const Array<T, N>& lhs, const Array<T, N>& rhs)
#ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
    noexcept(noexcept(std::declval<T&>() == std::declval<T&>()))
#else
    noexcept
#endif
-> std::enable_if_t<detail::has_bitwise_equality_v<T>, bool>
{
#ifndef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
    static_assert(noexcept(std::declval<T&>() == std::declval<T&>()),
        "\n[ERROR] in ara::core::Array: The type T's operator== must be marked 'noexcept' when exceptions are disabled.\n");
#endif

    if constexpr (N == 0) {
        return true;  // Empty arrays are always equal
    }
    
    if (!detail::is_constant_evaluated()) { 

       return std::memcmp(lhs.data(), rhs.data(), N * sizeof(T)) == 0;

    } else {
        for (std::size_t i = 0; i < N; ++i) {
            if (!(lhs[i] == rhs[i])) {
                return false;
            }
        }
    }
    return true;
}

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
[[nodiscard]] constexpr auto operator==(const Array<T, N>& lhs, const Array<T, N>& rhs)
#ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
    noexcept(noexcept(std::declval<T&>() == std::declval<T&>()))
#else
    noexcept
#endif
-> std::enable_if_t<!detail::has_bitwise_equality_v<T>, bool>
{
#ifndef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
    static_assert(noexcept(std::declval<T&>() == std::declval<T&>()),
        "\n[ERROR] in ara::core::Array: The type T's operator== must be marked 'noexcept' when exceptions are disabled.\n");
#endif

    if constexpr (N == 0) {
        return true;  // Empty arrays are always equal
    }

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
[[nodiscard]] constexpr auto operator!=(const Array<T, N>& lhs, const Array<T, N>& rhs)
#ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
    noexcept(noexcept(!(lhs == rhs)))
#else
    noexcept
#endif
-> bool
{
#ifndef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
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
[[nodiscard]] constexpr auto operator<(const Array<T, N>& lhs, const Array<T, N>& rhs)
#ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
    noexcept(noexcept(std::declval<T&>() < std::declval<T&>()))
#else
    noexcept
#endif
-> std::enable_if_t<detail::byte_comparable_v<T>, bool>
{
#ifndef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
    static_assert(noexcept(std::declval<T&>() < std::declval<T&>()),
        "\n[ERROR] in ara::core::Array: The type T's operator< must be marked 'noexcept' when exceptions are disabled.\n");
#endif

    if (!detail::is_constant_evaluated()) {

        return std::memcmp(lhs.data(), rhs.data(), N) < 0;
    }

    return detail::constexpr_lexicographical_compare(lhs.begin(), lhs.end(),
                                                     rhs.begin(), rhs.end());
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
[[nodiscard]] constexpr auto operator<(const Array<T, N>& lhs, const Array<T, N>& rhs)
#ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
    noexcept(noexcept(std::declval<T&>() < std::declval<T&>()))
#else
    noexcept
#endif
-> std::enable_if_t<!detail::byte_comparable_v<T>, bool>
{
#ifndef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
    static_assert(noexcept(std::declval<T&>() < std::declval<T&>()),
        "\n[ERROR] in ara::core::Array: The type T's operator< must be marked 'noexcept' when exceptions are disabled.\n");
#endif

    return detail::constexpr_lexicographical_compare(lhs.begin(), lhs.end(),
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
[[nodiscard]] constexpr auto operator<=(const Array<T, N>& lhs, const Array<T, N>& rhs)
#ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
    noexcept(noexcept(!(rhs < lhs)))
#else
    noexcept
#endif
-> bool
{
#ifndef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
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
[[nodiscard]] constexpr auto operator>(const Array<T, N>& lhs, const Array<T, N>& rhs)
#ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
    noexcept(noexcept(rhs < lhs))
#else
    noexcept
#endif
-> bool
{
#ifndef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
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
[[nodiscard]] constexpr auto operator>=(const Array<T, N>& lhs, const Array<T, N>& rhs)
#ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
    noexcept(noexcept(!(lhs < rhs)))
#else
    noexcept
#endif
-> bool
{
#ifndef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
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
#ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
    noexcept(noexcept(lhs.swap(rhs)))
#else
    noexcept
#endif
-> void
{
#ifndef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
    // Ensure T's swap is noexcept if exceptions are disabled
    static_assert(noexcept(lhs.swap(rhs)),
        "\n[ERROR] ara::core::Array: The type T's swap operation must be noexcept when exceptions are disabled.\n");
#endif

    lhs.swap(rhs);
}

/********************************************************************************************
 *  to_array - Modern C++ utility [Enhancement beyond AUTOSAR spec]
 ********************************************************************************************/
/*!
 * \brief  Creates an ara::core::Array from a built-in array (lvalue).
 *
 * \tparam T  The element type.
 * \tparam N  The number of elements.
 * \param  a  The built-in array to convert.
 * \return    An ara::core::Array containing copies of the elements.
 *
 * \details This is similar to std::to_array from C++20, adapted for ara::core::Array.
 *          Enables: auto arr = ara::core::to_array({1, 2, 3, 4});
 *
 * \note   This is an enhancement beyond base AUTOSAR requirements for modern C++ compatibility.
 */
template <typename T, std::size_t N>
[[nodiscard]] constexpr auto to_array(T (&a)[N]) 
#ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
    noexcept(std::is_nothrow_copy_constructible_v<T>) 
#else
    noexcept
#endif
    -> Array<std::remove_cv_t<T>, N>
{
#ifndef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
    static_assert(std::is_nothrow_copy_constructible_v<T>,
        "\n[ERROR] ara::core::to_array: The type T must be nothrow copy constructible when exceptions are disabled.\n");
#endif
    
    return detail::to_array_impl(a, std::make_index_sequence<N>{});
}

/*!
 * \brief  Creates an ara::core::Array from a built-in array (rvalue).
 *
 * \tparam T  The element type.
 * \tparam N  The number of elements.
 * \param  a  The built-in array to move from.
 * \return    An ara::core::Array containing moved elements.
 *
 * \details This is similar to std::to_array from C++20, adapted for ara::core::Array.
 *          Enables move semantics for array creation.
 *
 * \note   This is an enhancement beyond base AUTOSAR requirements for modern C++ compatibility.
 */
template <typename T, std::size_t N>
[[nodiscard]] constexpr auto to_array(T (&&a)[N]) 
#ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
    noexcept(std::is_nothrow_move_constructible_v<T>) 
#else
    noexcept
#endif
    -> Array<std::remove_cv_t<T>, N>
{
#ifndef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
    static_assert(std::is_nothrow_move_constructible_v<T>,
        "\n[ERROR] ara::core::to_array: The type T must be nothrow move constructible when exceptions are disabled.\n");
#endif
    
    return detail::to_array_impl(std::move(a), std::make_index_sequence<N>{});
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
 
/**********************************************************************************************************************
 *  COMPILE-TIME VERIFICATION – ara::core::Array
 *********************************************************************************************************************/

/*
 *  These `static_assert`s guarantee that the container preserves all low-level
 *  properties you normally expect from a plain “T[N]” whenever the element type
 *  itself has them.
 *
 *  ───────────────  WHAT WE CHECK  ───────────────
 *  • Size/layout ——  sizeof(Array<T,N>)   == N * sizeof(T)
 *                    is_standard_layout   ⇐⇒  same for T
 *  • Triviality  ——  is_trivial           ⇐⇒  same for T
 *                    is_trivially_(copy|move)_(constructible|assignable)
 *  • No hidden conversion surprises.
 *
 *  The assertions are instantiated for:
 *       A trivial POD (`int`)              → everything should remain trivial.
 *       A non-trivial type (`std::string`) → container must *not* pretend to be trivial.
 */

// ─────────────────────────────────────────────────────────────────────────────
//  Case 1 : trivial element type
// ─────────────────────────────────────────────────────────────────────────────
static_assert(sizeof(Array<int, 4>) == 4 * sizeof(int),
    "Array<int,4> size must equal 4 * sizeof(int)");

static_assert(std::is_standard_layout_v<Array<int, 4>>,
    "Array<int,4> should have standard layout");

static_assert(std::is_standard_layout_v<Array<int, 0>>,
    "Array<int,0> should have standard layout (empty class rule)");

static_assert(std::is_trivial_v<Array<int, 4>>,
    "Array<int,4> should be trivial because int is trivial");

static_assert(std::is_trivial_v<Array<int, 0>>,
    "Array<int,0> should be trivial (empty class rule)");

static_assert(std::is_trivially_copy_constructible_v<Array<int, 4>>,
    "Array<int,4> should be trivially copy-constructible");

static_assert(std::is_trivially_copy_constructible_v<Array<int, 0>>,
    "Array<int,0> should be trivially copy-constructible (empty class rule)");

static_assert(std::is_trivially_move_assignable_v<Array<int, 4>>,
    "Array<int,4> should be trivially move-assignable");

static_assert(std::is_trivially_move_assignable_v<Array<int, 0>>,
    "Array<int,0> should be trivially move-assignable (empty class rule)");

// No implicit element-wise conversions
static_assert(!std::is_convertible_v<Array<int,4>, int*>,
    "Array should not implicitly decay to pointer");

static_assert(!std::is_convertible_v<Array<int,0>, int*>,
    "Array<int,0> should not implicitly decay to pointer (empty class rule)");

static_assert(!std::is_convertible_v<int*, Array<int,4>>,
    "Pointer should not implicitly convert to Array");

static_assert(!std::is_convertible_v<int*, Array<int,0>>,
    "Pointer should not implicitly convert to Array<int,0> (empty class rule)");

// ─────────────────────────────────────────────────────────────────────────────
//  Case 2 : non-trivial element type
// ─────────────────────────────────────────────────────────────────────────────
#ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
static_assert(!std::is_trivial_v<Array<std::string, 2>>,
    "Array<std::string,2> must NOT be trivial because std::string is not");

static_assert(!std::is_trivially_copy_constructible_v<Array<std::string, 2>>,
    "Array<std::string,2> must NOT be trivially copy-constructible");

static_assert(sizeof(Array<std::string,2>) == 2 * sizeof(std::string),
    "Size check for non-trivial element type");

#endif // ENABLE_PLATFORM_CONDITIONAL_EXCEPTION


// ─────────────────────────────────────────────────────────────────────────────
//  Sanity: triviality tracks the element type
// ─────────────────────────────────────────────────────────────────────────────
template<typename T>
constexpr bool array_triviality_matches_element =
    std::is_trivial_v<Array<T,1>> == std::is_trivial_v<T>;

static_assert(array_triviality_matches_element<int>,
    "Triviality tracking failed for int");

#ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
static_assert(array_triviality_matches_element<std::string>,
    "Triviality tracking failed for std::string");
#endif // ENABLE_PLATFORM_CONDITIONAL_EXCEPTION


// ─────────────────────────────────────────────────────────────────────────────
//  Case 3 : empty array (N=0)
// ─────────────────────────────────────────────────────────────────────────────
static_assert(sizeof(Array<int, 0>) == 1,
    "Array<int,0> should occupy exactly one byte (empty-class rule)");


} // namespace core
} // namespace ara

#endif // OPEN_AA_ADAPTIVE_AUTOSAR_LIBS_INCLUDE_ARA_CORE_ARRAY_H_