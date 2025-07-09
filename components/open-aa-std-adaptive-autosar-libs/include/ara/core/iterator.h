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
 *  \file       ara/core/iterator.h
 *  \brief      Definition and implementation of iterator-related utilities for the OpenAA project.
 *
 *  \details    This file provides a collection of iterator-related utilities and enhancements
 *              tailored for the OpenAA project. It includes safe iterator operations such as
 *              distance calculation, advance, next, and prev functions with comprehensive
 *              overflow protection and noexcept specifications for safety-critical applications.
 *********************************************************************************************************************/
#ifndef OPEN_AA_ADAPTIVE_AUTOSAR_LIBS_INCLUDE_ARA_CORE_ITERATOR_H_
#define OPEN_AA_ADAPTIVE_AUTOSAR_LIBS_INCLUDE_ARA_CORE_ITERATOR_H_

/**********************************************************************************************************************
 *  INCLUDES
 *********************************************************************************************************************/
#include <iterator>     // For std::iterator_traits, iterator tags
#include <type_traits>  // For type traits and SFINAE utilities
#include <limits>       // For std::numeric_limits
#include <utility>      // For std::move, std::forward
#include <cstddef>      // For std::size_t, std::ptrdiff_t

#include "ara/core/internal/utility.h"  // For utility functions and traits
#include "ara/core/abort.h"             // For ara::core::Abort function

/**********************************************************************************************************************
 *  ITERATOR TYPE TRAITS AND UTILITIES
 *  ---------------------------------------------------------------------------------------------------------------
 *  Helper traits for iterator operations and C++20 backports
 *********************************************************************************************************************/
namespace ara {
namespace core {

namespace detail {

/*!
 * \brief  Helper to detect iterator overflow conditions.
 * \tparam DiffType The difference type to check.
 * \param  current  Current counter value.
 * \return true if incrementing would cause overflow.
 */
template<typename DiffType>
[[nodiscard]] constexpr auto would_overflow_increment(DiffType current) noexcept -> bool {
    static_assert(std::is_integral_v<DiffType>, 
        "\n[ERROR] ara::core::detail::would_overflow_increment: Distance calculation requires integral difference type.\n");
    return current >= std::numeric_limits<DiffType>::max();
}

/*!
 * \brief  Safe distance calculation for random access iterators.
 * \tparam RandomAccessIterator Iterator type with random access capability.
 * \param  first Beginning iterator.
 * \param  last  End iterator.
 * \return Distance between iterators.
 */
template<typename RandomAccessIterator>
[[nodiscard]] constexpr auto distance_impl(RandomAccessIterator first, 
                                          RandomAccessIterator last,
                                          std::random_access_iterator_tag) 
    noexcept(noexcept(last - first)) 
    -> typename std::iterator_traits<RandomAccessIterator>::difference_type 
{
    using difference_type = typename std::iterator_traits<RandomAccessIterator>::difference_type;
    
    static_assert(std::is_signed_v<difference_type>, 
        "\n[ERROR] ara::core::detail::distance_impl: Random access iterator difference_type must be signed for safe arithmetic.\n");
    
    return last - first;
}

/*!
 * \brief  Safe distance calculation for input iterators.
 * \tparam InputIterator Iterator type with input capability.
 * \param  first Beginning iterator.
 * \param  last  End iterator.
 * \return Distance between iterators.
 */
template<typename InputIterator>
[[nodiscard]] constexpr auto distance_impl(InputIterator first, 
                                          InputIterator last,
                                          std::input_iterator_tag) 
#ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
    noexcept(noexcept(++std::declval<InputIterator&>()) && 
             noexcept(std::declval<InputIterator>() != std::declval<InputIterator>()))
#else
    noexcept
#endif
    -> typename std::iterator_traits<InputIterator>::difference_type
{
    using difference_type = typename std::iterator_traits<InputIterator>::difference_type;
    
    static_assert(std::is_integral_v<difference_type>, 
        "\n[ERROR] ara::core::detail::distance_impl: Iterator difference_type must be integral for distance calculation.\n");
    static_assert(std::is_signed_v<difference_type>,
        "\n[ERROR] ara::core::detail::distance_impl: Iterator difference_type must be signed to handle negative distances.\n");
    
#ifndef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
    static_assert(noexcept(++std::declval<InputIterator&>()),
        "\n[ERROR] ara::core::detail::distance_impl: Iterator increment must be noexcept when exceptions are disabled.\n");
    static_assert(noexcept(std::declval<InputIterator>() != std::declval<InputIterator>()),
        "\n[ERROR] ara::core::detail::distance_impl: Iterator comparison must be noexcept when exceptions are disabled.\n");
#endif
    
    difference_type n = 0;
    
    while (first != last) {
        if (would_overflow_increment(n)) {
            ara::core::Abort("Iterator distance overflow detected");
        }
        ++first;
        ++n;
    }
    
    return n;
}

/*!
 * \brief  Safe advance implementation for random access iterators.
 * \tparam RandomAccessIterator Iterator type with random access capability.
 * \param  it Iterator to advance.
 * \param  n  Number of positions to advance.
 */
template<typename RandomAccessIterator>
constexpr auto advance_impl(RandomAccessIterator& it,
                           typename std::iterator_traits<RandomAccessIterator>::difference_type n,
                           std::random_access_iterator_tag)
    noexcept(noexcept(it += n)) -> void
{
    it += n;
}

/*!
 * \brief  Safe advance implementation for bidirectional iterators.
 * \tparam BidirectionalIterator Iterator type with bidirectional capability.
 * \param  it Iterator to advance.
 * \param  n  Number of positions to advance.
 */
template<typename BidirectionalIterator>
constexpr auto advance_impl(BidirectionalIterator& it,
                           typename std::iterator_traits<BidirectionalIterator>::difference_type n,
                           std::bidirectional_iterator_tag)
#ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
    noexcept(noexcept(++std::declval<BidirectionalIterator&>()) && 
             noexcept(--std::declval<BidirectionalIterator&>()))
#else
    noexcept
#endif
    -> void
{
#ifndef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
    static_assert(noexcept(++std::declval<BidirectionalIterator&>()),
        "\n[ERROR] ara::core::detail::advance_impl: Iterator increment must be noexcept when exceptions are disabled.\n");
    static_assert(noexcept(--std::declval<BidirectionalIterator&>()),
        "\n[ERROR] ara::core::detail::advance_impl: Iterator decrement must be noexcept when exceptions are disabled.\n");
#endif

    if (n >= 0) {
        while (n-- > 0) {
            ++it;
        }
    } else {
        while (n++ < 0) {
            --it;
        }
    }
}

/*!
 * \brief  Safe advance implementation for input iterators.
 * \tparam InputIterator Iterator type with input capability.
 * \param  it Iterator to advance.
 * \param  n  Number of positions to advance.
 */
template<typename InputIterator>
constexpr auto advance_impl(InputIterator& it,
                           typename std::iterator_traits<InputIterator>::difference_type n,
                           std::input_iterator_tag)
#ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
    noexcept(noexcept(++std::declval<InputIterator&>()))
#else
    noexcept
#endif
    -> void
{
    static_assert(std::is_signed_v<typename std::iterator_traits<InputIterator>::difference_type>,
        "\n[ERROR] ara::core::detail::advance_impl: Iterator difference_type must be signed.\n");
    
#ifndef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
    static_assert(noexcept(++std::declval<InputIterator&>()),
        "\n[ERROR] ara::core::detail::advance_impl: Iterator increment must be noexcept when exceptions are disabled.\n");
#endif

    if (n < 0) {
#ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
        ara::core::Abort("Cannot advance input iterator by negative amount");
#else
        // In no-exception mode, this is a critical error that cannot be recovered from
        static_assert(sizeof(InputIterator) == 0,
            "\n[ERROR] ara::core::detail::advance_impl: Cannot advance input iterator by negative amount.\n");
#endif
    }
    
    while (n-- > 0) {
        ++it;
    }
}

} // namespace detail

/**********************************************************************************************************************
 *  ara::core::distance
 *********************************************************************************************************************/
/*!
 * \brief  Calculate distance between iterators with comprehensive safety.
 *
 * \details
 * This function calculates the number of increments needed to reach last from first.
 * It provides optimal complexity for different iterator categories and includes
 * comprehensive overflow protection and noexcept specifications.
 *
 * Features:
 * - O(1) complexity for random access iterators
 * - O(n) complexity for other iterator categories
 * - Overflow detection and protection
 * - Conditional noexcept based on iterator operations
 * - SFINAE-friendly with proper enable_if constraints
 *
 * \tparam InputIterator Iterator type meeting InputIterator requirements.
 * \param  first Beginning of range.
 * \param  last  End of range.
 * \return Number of increments needed to reach last from first.
 *
 * \pre Iterator must be dereferenceable and incrementable.
 * \pre Iterator difference_type must be integral and signed.
 * \pre last must be reachable from first.
 *
 * \note When exceptions are disabled, returns numeric_limits::max() on overflow.
 *
 * Example usage:
 * \code
 * std::vector<int> v{1, 2, 3, 4, 5};
 * auto d1 = ara::core::distance(v.begin(), v.end());  // d1 == 5
 * 
 * std::list<int> l{1, 2, 3};
 * auto d2 = ara::core::distance(l.begin(), l.end());  // d2 == 3
 * \endcode
 */
template<typename InputIterator>
[[nodiscard]] constexpr auto distance(InputIterator first, InputIterator last) 
#ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
    noexcept(noexcept(detail::distance_impl(first, last, 
                     typename std::iterator_traits<InputIterator>::iterator_category{})))
#else
    noexcept
#endif
    -> std::enable_if_t<
        std::is_base_of_v<std::input_iterator_tag, 
                         typename std::iterator_traits<InputIterator>::iterator_category>,
        typename std::iterator_traits<InputIterator>::difference_type
    >
{
    static_assert(detail::is_iterator_v<InputIterator>,
        "\n[ERROR] ara::core::distance: Template parameter must be a valid iterator type.\n");
    
    using traits = std::iterator_traits<InputIterator>;
    static_assert(std::is_integral_v<typename traits::difference_type>,
        "\n[ERROR] ara::core::distance: Iterator difference_type must be integral.\n");
    static_assert(std::is_signed_v<typename traits::difference_type>,
        "\n[ERROR] ara::core::distance: Iterator difference_type must be signed.\n");
    
    return detail::distance_impl(first, last, typename traits::iterator_category{});
}

/**********************************************************************************************************************
 *  ara::core::advance
 *********************************************************************************************************************/
/*!
 * \brief  Advances an iterator by a given number of positions.
 *
 * \details
 * This function advances the given iterator by n positions. For bidirectional
 * and random access iterators, n may be negative. The function provides
 * optimal complexity based on iterator category.
 *
 * Features:
 * - O(1) complexity for random access iterators
 * - O(n) complexity for other iterator categories
 * - Supports negative advancement for bidirectional iterators
 * - Conditional noexcept based on iterator operations
 *
 * \tparam InputIterator Iterator type meeting InputIterator requirements.
 * \param  it Iterator to advance.
 * \param  n  Number of positions to advance (may be negative for bidirectional).
 *
 * \pre Iterator must be incrementable n times if n > 0.
 * \pre For bidirectional iterators, must be decrementable |n| times if n < 0.
 * \pre For input iterators, n must be non-negative.
 *
 * Example usage:
 * \code
 * std::vector<int> v{1, 2, 3, 4, 5};
 * auto it = v.begin();
 * ara::core::advance(it, 3);  // it now points to v[3] (value 4)
 * 
 * std::list<int> l{1, 2, 3, 4, 5};
 * auto lit = l.end();
 * ara::core::advance(lit, -2);  // lit now points to second-to-last element
 * \endcode
 */
template<typename InputIterator>
constexpr auto advance(InputIterator& it, 
                      typename std::iterator_traits<InputIterator>::difference_type n)
#ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
    noexcept(noexcept(detail::advance_impl(it, n,
                     typename std::iterator_traits<InputIterator>::iterator_category{})))
#else
    noexcept
#endif
    -> void
{
    static_assert(detail::is_iterator_v<InputIterator>,
        "\n[ERROR] ara::core::advance: Template parameter must be a valid iterator type.\n");
    
    detail::advance_impl(it, n, typename std::iterator_traits<InputIterator>::iterator_category{});
}

/**********************************************************************************************************************
 *  ara::core::next
 *********************************************************************************************************************/
/*!
 * \brief  Returns an iterator to the element n positions after the given iterator.
 *
 * \details
 * This function returns a new iterator pointing to the element n positions
 * after the given iterator. The original iterator is not modified.
 *
 * Features:
 * - Non-modifying operation (original iterator unchanged)
 * - Supports negative values for bidirectional iterators
 * - Optimal complexity based on iterator category
 * - Conditional noexcept specifications
 *
 * \tparam InputIterator Iterator type meeting InputIterator requirements.
 * \param  it Iterator to compute next position from.
 * \param  n  Number of positions to advance (default = 1).
 * \return New iterator pointing n positions after it.
 *
 * \pre Iterator must be incrementable n times if n > 0.
 * \pre For bidirectional iterators, must be decrementable |n| times if n < 0.
 *
 * Example usage:
 * \code
 * std::vector<int> v{1, 2, 3, 4, 5};
 * auto it1 = v.begin();
 * auto it2 = ara::core::next(it1);      // it2 points to v[1]
 * auto it3 = ara::core::next(it1, 3);   // it3 points to v[3]
 * // it1 still points to v[0]
 * \endcode
 */
template<typename InputIterator>
[[nodiscard]] constexpr auto next(InputIterator it,
                                 typename std::iterator_traits<InputIterator>::difference_type n = 1)
#ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
    noexcept(std::is_nothrow_copy_constructible_v<InputIterator> &&
             noexcept(advance(std::declval<InputIterator&>(), n)))
#else
    noexcept
#endif
    -> InputIterator
{
    static_assert(detail::is_iterator_v<InputIterator>,
        "\n[ERROR] ara::core::next: Template parameter must be a valid iterator type.\n");
    
#ifndef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
    static_assert(std::is_nothrow_copy_constructible_v<InputIterator>,
        "\n[ERROR] ara::core::next: Iterator must be nothrow-copy-constructible when exceptions are disabled.\n");
#endif
    
    advance(it, n);
    return it;
}

/**********************************************************************************************************************
 *  ara::core::prev
 *********************************************************************************************************************/
/*!
 * \brief  Returns an iterator to the element n positions before the given iterator.
 *
 * \details
 * This function returns a new iterator pointing to the element n positions
 * before the given iterator. The original iterator is not modified.
 *
 * Features:
 * - Non-modifying operation (original iterator unchanged)
 * - Requires bidirectional iterator capability
 * - Optimal complexity based on iterator category
 * - Conditional noexcept specifications
 *
 * \tparam BidirectionalIterator Iterator type meeting BidirectionalIterator requirements.
 * \param  it Iterator to compute previous position from.
 * \param  n  Number of positions to go back (default = 1).
 * \return New iterator pointing n positions before it.
 *
 * \pre Iterator must be decrementable n times.
 * \pre Iterator must be at least a BidirectionalIterator.
 *
 * Example usage:
 * \code
 * std::vector<int> v{1, 2, 3, 4, 5};
 * auto it1 = v.end();
 * auto it2 = ara::core::prev(it1);      // it2 points to v[4]
 * auto it3 = ara::core::prev(it1, 3);   // it3 points to v[2]
 * // it1 still points to v.end()
 * \endcode
 */
template<typename BidirectionalIterator>
[[nodiscard]] constexpr auto prev(BidirectionalIterator it,
                                 typename std::iterator_traits<BidirectionalIterator>::difference_type n = 1)
#ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
    noexcept(std::is_nothrow_copy_constructible_v<BidirectionalIterator> &&
             noexcept(advance(std::declval<BidirectionalIterator&>(), -n)))
#else
    noexcept
#endif
    -> BidirectionalIterator
{
    static_assert(detail::is_iterator_v<BidirectionalIterator>,
        "\n[ERROR] ara::core::prev: Template parameter must be a valid iterator type.\n");
    
    static_assert(std::is_base_of_v<std::bidirectional_iterator_tag,
                                   typename std::iterator_traits<BidirectionalIterator>::iterator_category>,
        "\n[ERROR] ara::core::prev: Iterator must be at least bidirectional.\n");
    
#ifndef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
    static_assert(std::is_nothrow_copy_constructible_v<BidirectionalIterator>,
        "\n[ERROR] ara::core::prev: Iterator must be nothrow-copy-constructible when exceptions are disabled.\n");
#endif
    
    advance(it, -n);
    return it;
}

/**********************************************************************************************************************
 *  ITERATOR TYPE ALIASES (C++20 BACKPORTS)
 *********************************************************************************************************************/
#if __cplusplus < 202002L
/*!
 * \brief  Backport of C++20 iter_difference_t for C++17.
 * \tparam Iterator Iterator type.
 */
template<typename Iterator>
using iter_difference_t = typename std::iterator_traits<Iterator>::difference_type;

/*!
 * \brief  Backport of C++20 iter_value_t for C++17.
 * \tparam Iterator Iterator type.
 */
template<typename Iterator>
using iter_value_t = typename std::iterator_traits<Iterator>::value_type;

/*!
 * \brief  Backport of C++20 iter_reference_t for C++17.
 * \tparam Iterator Iterator type.
 */
template<typename Iterator>
using iter_reference_t = typename std::iterator_traits<Iterator>::reference;
#else
using std::iter_difference_t;
using std::iter_value_t;
using std::iter_reference_t;
#endif

} // namespace core
} // namespace ara

#endif // OPEN_AA_ADAPTIVE_AUTOSAR_LIBS_INCLUDE_ARA_CORE_ITERATOR_H_