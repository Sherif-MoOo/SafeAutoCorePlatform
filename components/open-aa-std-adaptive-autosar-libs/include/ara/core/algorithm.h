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
 *  \file       ara/core/algorithm.h
 *  \brief      Definition and implementation of algorithm-related utilities for the OpenAA project.
 *
 *  \details    This file provides a collection of algorithm-related utilities and enhancements
 *              tailored for the OpenAA project. It includes algorithms for searching, sorting,
 *              and manipulating data structures, with a focus on performance and compliance with
 *              safety critical requirements.
 *********************************************************************************************************************/
#ifndef OPEN_AA_ADAPTIVE_AUTOSAR_LIBS_INCLUDE_ARA_CORE_ALGORITHM_H_
#define OPEN_AA_ADAPTIVE_AUTOSAR_LIBS_INCLUDE_ARA_CORE_ALGORITHM_H_

/**********************************************************************************************************************
 *  INCLUDES
 *********************************************************************************************************************/
#include <algorithm>    // For standard algorithms like std::sort, std::find, etc.
#include <iterator>     // For iterator utilities
#include <type_traits>  // For type traits and SFINAE utilities
#include <utility>      // For std::move, std::forward, etc.
#include <cstddef>      // For std::size_t, std::ptrdiff_t
#include <cstdint>      // For fixed-width integer types
#include <cstring>      // For memcmp, memchr, strlen

#include  <tuple>                                 // For std::tuple_size, std::tuple_element, etc.
#include  "ara/core/internal/utility.h"           // For utility functions and traits
#include  "ara/core/internal/algorithm/array.h"   // For array-like algorithms
#include  "ara/core/internal/algorithm/tuple.h"   // For tuple-like algorithms
#include  "ara/core/internal/algorithm/compare.h" // For comparison algorithms

/**********************************************************************************************************************
 *  NAMESPACE DECLARATION
 *********************************************************************************************************************/
namespace ara {
namespace core {

/**********************************************************************************************************************
 *  ara::core::lexicographical_compare
 *********************************************************************************************************************/
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
 * \note This overload participates in overload resolution only if \c InputIt1 and \c InputIt2 
 *       satisfy the requirements of input iterators and are not random access iterators.
 */
template<typename InputIt1, typename InputIt2,
         typename = std::enable_if_t<
             detail::is_input_iterator_v<InputIt1> &&
             detail::is_input_iterator_v<InputIt2> &&
             !detail::is_random_access_iterator_v<InputIt1> &&
             !detail::is_random_access_iterator_v<InputIt2>
         > >
[[nodiscard]] constexpr auto lexicographical_compare(InputIt1 first1, InputIt1 last1,
                                                     InputIt2 first2, InputIt2 last2)
#ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
    noexcept(noexcept(*first1 < *first2) && noexcept(*first1 == *first2))
#else
    noexcept
#endif
 -> bool
{   
#ifndef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
    static_assert(noexcept(*first1 < *first2) && noexcept(*first1 == *first2),
        "\n[ERROR] in ara::core::constexpr_lexicographical_compare: "
        "The element comparison must be noexcept when exceptions are disabled.\n");
#endif

    for (; (first1 != last1) && (first2 != last2); ++first1, ++first2) {
        if (*first1 < *first2) return true;
        if (*first2 < *first1) return false;
    }
    return (first1 == last1) && (first2 != last2);
}

/*!
 * \brief Performs a lexicographical comparison between two ranges using a custom comparator.
 *
 * This function compares the elements in the ranges \c [first1,last1) and \c [first2,last2)
 * using the provided comparison function object:
 * - For each pair of corresponding elements, if \c comp returns \c true for the element from the first range
 *   compared to the element from the second range, the function returns \c true.
 * - If \c comp returns \c true for the element from the second range compared to the element from the first range,
 *   the function returns \c false.
 * - If neither comparison returns \c true, the comparison continues.
 * - When the end of one of the ranges is reached:
 *     - If the first range is exhausted but the second still has elements, the first range is considered
 *       lexicographically less and the function returns \c true.
 *     - Otherwise, it returns \c false.
 *
 * \tparam InputIt1 The type of the input iterator for the first range.
 * \tparam InputIt2 The type of the input iterator for the second range.
 * \tparam Compare  The type of the comparison function object.
 * \param first1 An iterator pointing to the first element of the first range.
 * \param last1  An iterator pointing past the last element of the first range.
 * \param first2 An iterator pointing to the first element of the second range.
 * \param last2  An iterator pointing past the last element of the second range.
 * \param comp   Binary predicate which returns \c true if the first argument is less than the second.
 * \return \c true if the first range is lexicographically less than the second range according to \c comp;
 *         \c false otherwise.
 *
 * \note The function is declared as \c constexpr so that if the iterators, elements, and comparator
 *       are \c constexpr, the entire operation can be evaluated at compile time.
 * \note This overload participates in overload resolution only if \c InputIt1 and \c InputIt2
 *       satisfy the requirements of input iterators, are not random access iterators, and \c Compare
 *       is invocable with the dereferenced iterator types.
 */
template<typename InputIt1, typename InputIt2, typename Compare,
         typename = std::enable_if_t<
             detail::is_input_iterator_v<InputIt1> &&
             detail::is_input_iterator_v<InputIt2> &&
             !detail::is_random_access_iterator_v<InputIt1> &&
             !detail::is_random_access_iterator_v<InputIt2> &&
             std::is_invocable_r_v<bool, Compare,
                                   detail::iterator_reference_t<InputIt1>,
                                   detail::iterator_reference_t<InputIt2>>>>
[[nodiscard]] constexpr auto lexicographical_compare(InputIt1 first1, InputIt1 last1,
                                                     InputIt2 first2, InputIt2 last2,
                                                     Compare comp)
#ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
    noexcept(noexcept(comp(*first1, *first2)))
#else
    noexcept
#endif
-> bool
{
#ifndef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
    static_assert(noexcept(comp(*first1, *first2)),
        "\n[ERROR] in ara::core::constexpr_lexicographical_compare: "
        "The comparison function must be noexcept when exceptions are disabled.\n");
#endif

    for (; (first1 != last1) && (first2 != last2); ++first1, ++first2) {
        if (comp(*first1, *first2)) return true;
        if (comp(*first2, *first1)) return false;
    }
    return (first1 == last1) && (first2 != last2);
}

/*!
 * \brief Performs a lexicographical comparison between two ranges (optimized for random access iterators).
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
 * \tparam RandomIt1 The type of the random access iterator for the first range.
 * \tparam RandomIt2 The type of the random access iterator for the second range.
 * \param first1 An iterator pointing to the first element of the first range.
 * \param last1  An iterator pointing past the last element of the first range.
 * \param first2 An iterator pointing to the first element of the second range.
 * \param last2  An iterator pointing past the last element of the second range.
 * \return \c true if the first range is lexicographically less than the second range; \c false otherwise.
 *
 * \note The function is declared as \c constexpr so that if both the iterators and the element comparison
 *       are \c constexpr, the entire operation can be evaluated at compile time.
 * \note This overload participates in overload resolution only if \c RandomIt1 and \c RandomIt2
 *       satisfy the requirements of random access iterators.
 * \note This specialization provides better performance by using indexed access instead of iterator increments.
 */
template<typename RandomIt1, typename RandomIt2>
[[nodiscard]] constexpr auto lexicographical_compare(RandomIt1 first1, RandomIt1 last1,
                                                     RandomIt2 first2, RandomIt2 last2)
#ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
    noexcept(noexcept(*first1 < *first2) && noexcept(*first1 == *first2))
#else
    noexcept
#endif
-> std::enable_if_t<
       detail::is_random_access_iterator_v<RandomIt1> &&
       detail::is_random_access_iterator_v<RandomIt2>,
       bool>
{
#ifndef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
    static_assert(noexcept(*first1 < *first2) && noexcept(*first1 == *first2),
        "\n[ERROR] in ara::core::constexpr_lexicographical_compare: "
        "The element comparison must be noexcept when exceptions are disabled.\n");
#endif

    return detail::lexicographical_compare_random_access_impl(first1, last1, first2, last2);
}

/*!
 * \brief Performs a lexicographical comparison between two ranges using a custom comparator (optimized for random access iterators).
 *
 * This function compares the elements in the ranges \c [first1,last1) and \c [first2,last2)
 * using the provided comparison function object:
 * - For each pair of corresponding elements, if \c comp returns \c true for the element from the first range
 *   compared to the element from the second range, the function returns \c true.
 * - If \c comp returns \c true for the element from the second range compared to the element from the first range,
 *   the function returns \c false.
 * - If neither comparison returns \c true, the comparison continues.
 * - When the end of one of the ranges is reached:
 *     - If the first range is exhausted but the second still has elements, the first range is considered
 *       lexicographically less and the function returns \c true.
 *     - Otherwise, it returns \c false.
 *
 * \tparam RandomIt1 The type of the random access iterator for the first range.
 * \tparam RandomIt2 The type of the random access iterator for the second range.
 * \tparam Compare   The type of the comparison function object.
 * \param first1 An iterator pointing to the first element of the first range.
 * \param last1  An iterator pointing past the last element of the first range.
 * \param first2 An iterator pointing to the first element of the second range.
 * \param last2  An iterator pointing past the last element of the second range.
 * \param comp   Binary predicate which returns \c true if the first argument is less than the second.
 * \return \c true if the first range is lexicographically less than the second range according to \c comp;
 *         \c false otherwise.
 *
 * \note The function is declared as \c constexpr so that if the iterators, elements, and comparator
 *       are \c constexpr, the entire operation can be evaluated at compile time.
 * \note This overload participates in overload resolution only if \c RandomIt1 and \c RandomIt2
 *       satisfy the requirements of random access iterators and \c Compare is invocable with
 *       the dereferenced iterator types.
 * \note This specialization provides better performance by using indexed access instead of iterator increments.
 */
template<typename RandomIt1, typename RandomIt2, typename Compare>
[[nodiscard]] constexpr auto lexicographical_compare(RandomIt1 first1, RandomIt1 last1,
                                                     RandomIt2 first2, RandomIt2 last2,
                                                     Compare comp)
#ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
    noexcept(noexcept(comp(*first1, *first2)))
#else
    noexcept
#endif
-> std::enable_if_t<
       detail::is_random_access_iterator_v<RandomIt1> &&
       detail::is_random_access_iterator_v<RandomIt2> &&
       std::is_invocable_r_v<bool, Compare,
                             detail::iterator_reference_t<RandomIt1>,
                             detail::iterator_reference_t<RandomIt2>>,
       bool>
{
#ifndef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
    static_assert(noexcept(comp(*first1, *first2)),
        "\n[ERROR] in ara::core::constexpr_lexicographical_compare: "
        "The comparison function must be noexcept when exceptions are disabled.\n");
#endif

    return detail::lexicographical_compare_random_access_impl(first1, last1, first2, last2, comp);
}

/*!
 * \brief Performs a lexicographical comparison between two ranges (specialized for pointers to arithmetic types).
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
 * \tparam T The arithmetic type pointed to by the pointers.
 * \param first1 A pointer to the first element of the first range.
 * \param last1  A pointer pointing past the last element of the first range.
 * \param first2 A pointer to the first element of the second range.
 * \param last2  A pointer pointing past the last element of the second range.
 * \return \c true if the first range is lexicographically less than the second range; \c false otherwise.
 *
 * \note The function is declared as \c constexpr so that the entire operation can be evaluated at compile time.
 * \note This overload participates in overload resolution only if \c T is an arithmetic type.
 * \note This specialization provides optimal performance for built-in types using direct memory access.
 */
template<typename T,
         typename = std::enable_if_t<std::is_arithmetic_v<T>>>
[[nodiscard]] constexpr auto lexicographical_compare(const T* first1, const T* last1,
                                                     const T* first2, const T* last2) noexcept -> bool
{
    auto const len1 = static_cast<std::size_t>(last1 - first1);
    auto const len2 = static_cast<std::size_t>(last2 - first2);
    auto const min_len = (len1 < len2) ? len1 : len2;
    
    // For built-in types, we can use a simple loop with direct indexing
    for (std::size_t i = 0; i < min_len; ++i) {
        if (first1[i] < first2[i]) return true;
        if (first2[i] < first1[i]) return false;
    }
    
    return len1 < len2;
}

/**********************************************************************************************************************
 *  ara::core::find
 *********************************************************************************************************************/
/*!
 * \brief Searches for an element equal to value in the range [first, last).
 *
 * This function examines each element in the range \c [first,last) and returns an iterator
 * to the first element that is equal to \c value. If no such element is found, \c last is returned.
 *
 * \tparam InputIt The type of the input iterator.
 * \tparam T The type of the value to search for.
 * \param first An iterator pointing to the first element of the range to search.
 * \param last An iterator pointing past the last element of the range to search.
 * \param value The value to search for.
 * \return An iterator to the first element equal to \c value, or \c last if not found.
 *
 * \note The function is declared as \c constexpr so that if the iterators and comparison
 *       are \c constexpr, the entire operation can be evaluated at compile time.
 * \note This overload participates in overload resolution only if \c InputIt
 *       satisfies the requirements of an input iterator.
 */
template<typename InputIt, typename T,
         typename = std::enable_if_t<detail::is_input_iterator_v<InputIt>>>
[[nodiscard]] constexpr auto find(InputIt first, InputIt last, const T& value)
#ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
    noexcept(noexcept(*first == value))
#else
    noexcept
#endif
-> InputIt
{
#ifndef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
    static_assert(noexcept(*first == value),
        "\n[ERROR] in ara::core::find: "
        "The element comparison must be noexcept when exceptions are disabled.\n");
#endif

    for (; first != last; ++first) {
        if (*first == value) {
            return first;
        }
    }
    return last;
}

/*!
 * \brief Searches for an element satisfying a predicate in the range [first, last).
 *
 * This function examines each element in the range \c [first,last) and returns an iterator
 * to the first element for which predicate \c p returns \c true. If no such element is found,
 * \c last is returned.
 *
 * \tparam InputIt The type of the input iterator.
 * \tparam UnaryPredicate The type of the predicate function object.
 * \param first An iterator pointing to the first element of the range to search.
 * \param last An iterator pointing past the last element of the range to search.
 * \param p Unary predicate which returns \c true for the required element.
 * \return An iterator to the first element for which the predicate returns \c true,
 *         or \c last if not found.
 *
 * \note The function is declared as \c constexpr so that if the iterators and predicate
 *       are \c constexpr, the entire operation can be evaluated at compile time.
 * \note This overload participates in overload resolution only if \c InputIt
 *       satisfies the requirements of an input iterator and \c UnaryPredicate
 *       is invocable with the dereferenced iterator type.
 */
template<typename InputIt, typename UnaryPredicate,
         typename = std::enable_if_t<
             detail::is_input_iterator_v<InputIt> &&
             std::is_invocable_r_v<bool, UnaryPredicate,
                                   detail::iterator_reference_t<InputIt>>>>
[[nodiscard]] constexpr auto find_if(InputIt first, InputIt last, UnaryPredicate p)
#ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
    noexcept(noexcept(p(*first)))
#else
    noexcept
#endif
-> InputIt
{
#ifndef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
    static_assert(noexcept(p(*first)),
        "\n[ERROR] in ara::core::find_if: "
        "The predicate must be noexcept when exceptions are disabled.\n");
#endif

    for (; first != last; ++first) {
        if (p(*first)) {
            return first;
        }
    }
    return last;
}

/**********************************************************************************************************************
 *  ara::core::equal
 *********************************************************************************************************************/
/*!
 * \brief Tests whether two ranges are equal.
 *
 * This function compares the elements in the range \c [first1,last1) with those in the range
 * beginning at \c first2, and returns \c true if all corresponding pairs of elements are equal.
 *
 * \tparam InputIt1 The type of the input iterator for the first range.
 * \tparam InputIt2 The type of the input iterator for the second range.
 * \param first1 An iterator pointing to the first element of the first range.
 * \param last1 An iterator pointing past the last element of the first range.
 * \param first2 An iterator pointing to the first element of the second range.
 * \return \c true if all corresponding elements are equal; \c false otherwise.
 *
 * \note The function is declared as \c constexpr so that if the iterators and comparison
 *       are \c constexpr, the entire operation can be evaluated at compile time.
 * \note The behavior is undefined if the second range is shorter than the first range.
 * \note This overload participates in overload resolution only if \c InputIt1 and \c InputIt2
 *       satisfy the requirements of input iterators.
 */
template<typename InputIt1, typename InputIt2,
         typename = std::enable_if_t<
             detail::is_input_iterator_v<InputIt1> &&
             detail::is_input_iterator_v<InputIt2>>>
[[nodiscard]] constexpr auto equal(InputIt1 first1, InputIt1 last1, InputIt2 first2)
#ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
    noexcept(noexcept(*first1 == *first2))
#else
    noexcept
#endif
-> bool
{
#ifndef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
    static_assert(noexcept(*first1 == *first2),
        "\n[ERROR] in ara::core::equal: "
        "The element comparison must be noexcept when exceptions are disabled.\n");
#endif

    for (; first1 != last1; ++first1, ++first2) {
        if (!(*first1 == *first2)) {
            return false;
        }
    }
    return true;
}

/*!
 * \brief Tests whether two ranges are equal.
 *
 * This function compares the elements in the range \c [first1,last1) with those in the range
 * \c [first2,last2), and returns \c true if both ranges have the same number of elements
 * and all corresponding pairs of elements are equal.
 *
 * \tparam InputIt1 The type of the input iterator for the first range.
 * \tparam InputIt2 The type of the input iterator for the second range.
 * \param first1 An iterator pointing to the first element of the first range.
 * \param last1 An iterator pointing past the last element of the first range.
 * \param first2 An iterator pointing to the first element of the second range.
 * \param last2 An iterator pointing past the last element of the second range.
 * \return \c true if the ranges have the same size and all corresponding elements are equal;
 *         \c false otherwise.
 *
 * \note The function is declared as \c constexpr so that if the iterators and comparison
 *       are \c constexpr, the entire operation can be evaluated at compile time.
 * \note This overload participates in overload resolution only if \c InputIt1 and \c InputIt2
 *       satisfy the requirements of input iterators.
 */
template<typename InputIt1, typename InputIt2,
         typename = std::enable_if_t<
             detail::is_input_iterator_v<InputIt1> &&
             detail::is_input_iterator_v<InputIt2>>>
[[nodiscard]] constexpr auto equal(InputIt1 first1, InputIt1 last1,
                                   InputIt2 first2, InputIt2 last2)
#ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
    noexcept(noexcept(*first1 == *first2))
#else
    noexcept
#endif
-> bool
{
#ifndef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
    static_assert(noexcept(*first1 == *first2),
        "\n[ERROR] in ara::core::equal: "
        "The element comparison must be noexcept when exceptions are disabled.\n");
#endif

    for (; first1 != last1 && first2 != last2; ++first1, ++first2) {
        if (!(*first1 == *first2)) {
            return false;
        }
    }
    return first1 == last1 && first2 == last2;
}

/**********************************************************************************************************************
 *  ara::core::search
 *********************************************************************************************************************/
/*!
 * \brief Searches for the first occurrence of a subsequence in a range.
 *
 * This function searches the range \c [first1,last1) for the first occurrence of the sequence
 * defined by the range \c [first2,last2). It returns an iterator to the beginning of the first
 * such occurrence, or \c last1 if no occurrences are found.
 *
 * \tparam ForwardIt1 The type of the forward iterator for the range to search in.
 * \tparam ForwardIt2 The type of the forward iterator for the pattern to search for.
 * \param first1 An iterator pointing to the first element of the range to search in.
 * \param last1 An iterator pointing past the last element of the range to search in.
 * \param first2 An iterator pointing to the first element of the pattern to search for.
 * \param last2 An iterator pointing past the last element of the pattern to search for.
 * \return An iterator to the beginning of the first occurrence of the pattern,
 *         or \c last1 if the pattern is not found.
 *
 * \note The function is declared as \c constexpr so that if the iterators and comparison
 *       are \c constexpr, the entire operation can be evaluated at compile time.
 * \note If \c [first2,last2) is empty, \c first1 is returned.
 * \note This overload participates in overload resolution only if \c ForwardIt1 and \c ForwardIt2
 *       satisfy the requirements of forward iterators.
 */
template<typename ForwardIt1, typename ForwardIt2,
         typename = std::enable_if_t<
             detail::is_input_iterator_v<ForwardIt1> &&
             detail::is_input_iterator_v<ForwardIt2>>>
[[nodiscard]] constexpr auto search(ForwardIt1 first1, ForwardIt1 last1,
                                    ForwardIt2 first2, ForwardIt2 last2)
#ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
    noexcept(noexcept(*first1 == *first2))
#else
    noexcept
#endif
-> ForwardIt1
{
#ifndef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
    static_assert(noexcept(*first1 == *first2),
        "\n[ERROR] in ara::core::search: "
        "The element comparison must be noexcept when exceptions are disabled.\n");
#endif

    if (first2 == last2) return first1;  // Empty pattern always matches
    
    for (; first1 != last1; ++first1) {
        auto it1 = first1;
        auto it2 = first2;
        
        for (; it1 != last1 && it2 != last2 && *it1 == *it2; ++it1, ++it2) {
            // Continue matching
        }
        
        if (it2 == last2) {
            return first1;  // Found complete match
        }
    }
    
    return last1;
}

/**********************************************************************************************************************
 *  Character sequence algorithms
 *********************************************************************************************************************/
/*!
 * \brief Finds the first occurrence of a character in a character sequence.
 *
 * This function searches for the first occurrence of character \c ch in the first \c count
 * characters of the array pointed to by \c str.
 *
 * \tparam CharT The character type.
 * \tparam Traits The character traits type (default: typename detail::default_traits<CharT>::type).
 * \param str Pointer to the character string to examine.
 * \param count Number of characters to examine.
 * \param ch Character to search for.
 * \return Pointer to the first occurrence of \c ch, or \c nullptr if not found.
 *
 * \note The function is declared as \c constexpr so that it can be evaluated at compile time.
 * \note Uses optimized standard library functions (memchr/wmemchr) when possible at runtime.
 */
template<typename CharT, typename Traits = typename detail::default_traits<CharT>::type>
[[nodiscard]] constexpr auto char_find(const CharT* str, std::size_t count, CharT ch) noexcept -> const CharT*
{
    if (count == 0 || str == nullptr) {
        return nullptr;
    }

    if (!detail::is_constant_evaluated()) {
        if constexpr (std::is_same_v<CharT, char> &&
                      detail::is_default_char_traits_v<CharT, Traits>) {
            return static_cast<const CharT*>(std::memchr(str, ch, count));
        } else if constexpr (std::is_same_v<CharT, wchar_t> &&
                             detail::is_default_char_traits_v<CharT, Traits>) {
            return static_cast<const CharT*>(std::wmemchr(str, ch, count));
        }
    }

    std::size_t i = 0;
    for (; i + 4 <= count; i += 4) {
        if (Traits::eq(str[i + 0], ch)) return str + i + 0;
        if (Traits::eq(str[i + 1], ch)) return str + i + 1;
        if (Traits::eq(str[i + 2], ch)) return str + i + 2;
        if (Traits::eq(str[i + 3], ch)) return str + i + 3;
    }

    for (; i < count; ++i) {
        if (Traits::eq(str[i], ch)) return str + i;
    }

    return nullptr;
}

/*!
 * \brief Compares two character sequences.
 *
 * This function lexicographically compares the first \c count characters of the arrays
 * pointed to by \c s1 and \c s2.
 *
 * \tparam CharT The character type.
 * \tparam Traits The character traits type (default: std::char_traits<CharT>).
 * \param s1 Pointer to the first character string to compare.
 * \param s2 Pointer to the second character string to compare.
 * \param count Number of characters to compare.
 * \return Negative value if \c s1 is less than \c s2, positive value if \c s1 is greater
 *         than \c s2, zero if the strings are equal.
 *
 * \note The function is declared as \c constexpr so that it can be evaluated at compile time.
 * \note Uses optimized standard library functions (memcmp/wmemcmp) when possible at runtime.
 */
template<typename CharT, typename Traits>
[[nodiscard]] constexpr auto char_compare(const CharT* s1, const CharT* s2, size_t count) noexcept -> int {
    if (!detail::is_constant_evaluated() && count > 0) {
        // Runtime: use optimized comparison
        if constexpr (detail::is_default_char_traits_v<CharT, Traits>) {
            if constexpr (std::is_same_v<CharT, char>) {
                return std::memcmp(s1, s2, count);
            } else if constexpr (std::is_same_v<CharT, wchar_t>) {
                return std::wmemcmp(s1, s2, count);
            }
        }
    }
    
    // Compile-time or generic: use traits
    return Traits::compare(s1, s2, count);
}

/*!
 * \brief Calculates the length of a null-terminated string.
 *
 * This function returns the length of the null-terminated character string pointed to by \c str,
 * that is, the number of characters that precede the terminating null character.
 *
 * \tparam CharT The character type.
 * \param str Pointer to the null-terminated character string to be examined.
 * \return The length of the null-terminated string, or 0 if \c str is \c nullptr.
 *
 * \note The function is declared as \c constexpr so that it can be evaluated at compile time.
 * \note Uses compiler built-ins when available for better performance.
 */
template<typename CharT>
[[nodiscard]] constexpr auto strlen(const CharT* str)
    noexcept -> std::size_t
{
    if (str == nullptr) return 0;

#if defined(__has_builtin)
    #if __has_builtin(__builtin_strlen)
        if constexpr (std::is_same_v<CharT, char>) {
            return __builtin_strlen(str);
        }
    #endif
#endif

    std::size_t len = 0;
    while (str[len] != CharT{}) {
        ++len;
    }
    return len;
}

/*!
 * \brief Extracts the filename portion from a file path.
 *
 * This function returns a pointer to the filename component of the null-terminated
 * path string pointed to by \c path. The filename component is the part after the
 * last directory separator ('/' or '\\').
 *
 * \param path Full file path.
 * \return Pointer to the filename portion of the path, or empty string if \c path is \c nullptr.
 *
 * \note The function is declared as \c constexpr so that it can be evaluated at compile time.
 * \note Handles both forward and backward slashes as path separators.
 */
[[nodiscard]] constexpr auto basename(const char* path) noexcept -> const char*
{
    if (path == nullptr) return "";

    const char* last_separator = path;
    const char* current = path;

    while (*current != '\0') {
        if (*current == '/' || *current == '\\') {
            last_separator = current + 1;
        }
        ++current;
    }

    return last_separator;
}

/*!
 * \brief Constexpr search implementation for substring search
 *
 * \tparam CharT Character type
 * \tparam Traits Character traits type
 * \param first Beginning of the string to search in
 * \param last End of the string to search in
 * \param s_first Beginning of the substring to find
 * \param s_last End of the substring to find
 * \return Pointer to the first occurrence of the substring, or last if not found
 * \details Simplified Boyer-Moore algorithm that works in constexpr context
 */
template<typename CharT, typename Traits>
[[nodiscard]] constexpr auto char_search(
    const CharT* first1, const CharT* last1,
    const CharT* first2, const CharT* last2) noexcept -> const CharT* {
    
    const std::size_t len1 = static_cast<std::size_t>(last1 - first1);
    const std::size_t len2 = static_cast<std::size_t>(last2 - first2);

    if (len2 == 0)                 return first1;
    if (len2 > len1)               return last1;
    
    // For small patterns, use simple search
    if (len2 <= 4) {
        for (std::size_t i = 0; i <= len1 - len2; ++i) {
            if (Traits::compare(first1 + i, first2, len2) == 0)
                return first1 + i;
        }
        return last1;
    }
    
    // For larger patterns, use optimized search with first/last char check
    const CharT first_char = *first2;
    const CharT last_char  = *(last2 - 1);

    for (std::size_t i = 0; i <= len1 - len2; ++i) {
        if (Traits::eq(first1[i],           first_char) &&
            Traits::eq(first1[i + len2 - 1], last_char )) {
            if (Traits::compare(first1 + i, first2, len2) == 0)
                return first1 + i;
        }
    }
    return last1;
}

/**********************************************************************************************************************
 *  ara::core::apply
 *********************************************************************************************************************/
/*!
 * \brief  Applies a callable to the elements of a tuple-like object.
 *
 * \details
 * This is a constexpr, SFINAE-friendly replacement for std::apply that works with
 * any tuple-like type via ADL. It unpacks the tuple-like object and invokes the
 * callable with its elements as arguments.
 *
 * Features:
 * - Works with any tuple-like type (std::tuple, std::pair, ara::core::Array, etc.)
 * - Preserves value categories when forwarding arguments
 * - Fully constexpr and noexcept aware
 * - SFINAE-friendly with proper enable_if constraints
 *
 * \tparam F     The type of the callable (function, lambda, functor, etc.).
 * \tparam Tuple The type of the tuple-like object.
 * \param  f     The callable to apply.
 * \param  tup   The tuple-like object containing the arguments.
 * \return       The result of calling f with the unpacked elements of tup.
 *
 * \pre Tuple must be a tuple-like type (satisfy is_tuple_like_v).
 * \pre F must be invocable with the unpacked elements of Tuple.
 *
 * Example usage:
 * \code
 * ara::core::Array<int, 3> arr{10, 20, 30};
 * auto sum = ara::core::apply([](int a, int b, int c) { return a + b + c; }, arr);
 * // sum == 60
 * 
 * // Works with perfect forwarding
 * auto concat = ara::core::apply(
 *     [](auto&&... args) { return (... + args); },
 *     std::make_tuple(std::string("Hello"), ' ', std::string("World"))
 * );
 * // concat == "Hello World"
 * \endcode
 */
template<typename F, typename Tuple>
[[nodiscard]] constexpr auto apply(F&& f, Tuple&& tup)
#ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
    noexcept(noexcept(detail::apply_impl(
        std::forward<F>(f),
        std::forward<Tuple>(tup),
        std::make_index_sequence<std::tuple_size_v<
            std::remove_cv_t<std::remove_reference_t<Tuple>>>>{})))
#else
    noexcept
#endif
-> std::enable_if_t<
    detail::is_tuple_like_v<std::remove_reference_t<Tuple>>,
    decltype(detail::apply_impl(
        std::forward<F>(f),
        std::forward<Tuple>(tup),
        std::make_index_sequence<std::tuple_size_v<
            std::remove_cv_t<std::remove_reference_t<Tuple>>>>{}))>
{
    static_assert(detail::is_tuple_like_v<std::remove_reference_t<Tuple>>,
        "\n[ERROR] ara::core::apply: The second argument must be a tuple-like type.\n");
    
    constexpr std::size_t N = std::tuple_size_v<
        std::remove_cv_t<std::remove_reference_t<Tuple>>>;
    
#ifndef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
    static_assert(noexcept(detail::apply_impl(
        std::forward<F>(f),
        std::forward<Tuple>(tup),
        std::make_index_sequence<N>{})),
        "\n[ERROR] ara::core::apply: The callable invocation must be noexcept when exceptions are disabled.\n");
#endif
    
    return detail::apply_impl(std::forward<F>(f),
                              std::forward<Tuple>(tup),
                              std::make_index_sequence<N>{});
}

/*!
 * \brief  Concatenates any number of tuple-like objects into a single std::tuple.
 *
 * \details
 * This function concatenates multiple tuple-like objects into a single std::tuple,
 * following the same semantics as std::tuple_cat. It creates a new tuple containing
 * values (not references) of all elements from the input objects.
 *
 * Behavior:
 * - For lvalue arguments: elements are copied
 * - For rvalue arguments: elements are moved
 * - All types are decayed (cv-qualifiers and references removed)
 * - Works in constexpr contexts
 * - Fully noexcept aware
 *
 * This implementation correctly handles the C++ standard semantics where tuple_cat
 * creates a tuple of values, enabling use in constexpr contexts and avoiding
 * dangling reference issues.
 *
 * \tparam Tuples The types of the tuple-like objects to concatenate.
 * \param  tpls   The tuple-like objects to concatenate.
 * \return        A std::tuple containing values of all elements.
 *
 * \pre All types in Tuples must be tuple-like (satisfy is_tuple_like_v).
 *
 * Example usage:
 * \code
 * ara::core::Array<int, 2> arr{1, 2};
 * std::pair<double, char> pr{3.14, 'x'};
 * std::tuple<bool> tup{true};
 * 
 * auto combined = ara::core::tuple_cat(arr, pr, tup);
 * // combined is std::tuple<int, int, double, char, bool>{1, 2, 3.14, 'x', true}
 * 
 * // Works in constexpr contexts
 * constexpr ara::core::Array<int, 3> c_arr{10, 20, 30};
 * constexpr auto c_result = ara::core::tuple_cat(c_arr, std::make_pair(40, 50));
 * static_assert(std::get<0>(c_result) == 10);
 * static_assert(std::get<4>(c_result) == 50);
 * \endcode
 */
template<typename... Tuples>
[[nodiscard]] constexpr auto tuple_cat(Tuples&&... tpls)
#ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
    noexcept(noexcept(std::tuple_cat(detail::to_std_tuple(std::forward<Tuples>(tpls))...)))
#else
    noexcept
#endif
-> std::enable_if_t<
    (detail::is_tuple_like_v<std::remove_reference_t<Tuples>> && ...),
    decltype(std::tuple_cat(detail::to_std_tuple(std::forward<Tuples>(tpls))...))>
{
    static_assert((detail::is_tuple_like_v<std::remove_reference_t<Tuples>> && ...),
        "\n[ERROR] ara::core::tuple_cat: All arguments must be tuple-like types.\n");
    
#ifndef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
    static_assert(noexcept(std::tuple_cat(detail::to_std_tuple(std::forward<Tuples>(tpls))...)),
        "\n[ERROR] ara::core::tuple_cat: The tuple concatenation must be noexcept when exceptions are disabled.\n");
#endif
    
    return std::tuple_cat(detail::to_std_tuple(std::forward<Tuples>(tpls))...);
}

/**********************************************************************************************************************
 *  ara::core::exchange
 *********************************************************************************************************************/
/*!
 * \brief  Exchanges the value of an object with a new value.
 *
 * \details
 * This function replaces the value of the given object with a new value and returns the old value.
 * It is similar to std::exchange but provides additional compile-time checks for noexcept
 * and SFINAE constraints to ensure the types are suitable for exchange operations.
 *
 * \tparam T The type of the object to exchange.
 * \tparam U The type of the new value (default is T).
 * \param obj The object whose value will be exchanged.
 * \param new_value The new value to assign to the object.
 * \return The old value of the object before the exchange.
 *
 * \note This function is designed to be noexcept when exceptions are disabled,
 *       ensuring that it can be used in contexts where exception safety is a concern.
 * \pre T must be nothrow-move-constructible and nothrow-assignable from U
 *      when exceptions are disabled.
 * \warning If exceptions are disabled, T must be nothrow-move-constructible
 *          and nothrow-assignable from U.
 *          If these conditions are not met, static_assert will trigger a compile-time error.
 */
template <typename T, typename U = T>
[[nodiscard]] constexpr auto exchange(T& obj, U&& new_value)
#ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
    noexcept(std::is_nothrow_move_constructible_v<T> &&
             std::is_nothrow_assignable_v<T&, U>)
#else
    noexcept
#endif
-> std::enable_if_t<
    std::is_move_constructible_v<T> &&
    std::is_assignable_v<T&, U>,
    T>
{
#ifndef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
    static_assert(std::is_nothrow_move_constructible_v<T>,
        "\n[ERROR] ara::core::exchange: T must be nothrow-move-constructible when exceptions are disabled.\n");
    static_assert(std::is_nothrow_assignable_v<T&, U>,
        "\n[ERROR] ara::core::exchange: T must be nothrow-assignable from U when exceptions are disabled.\n");
#endif

    T old  = std::move(obj);                 /* Step 1 – capture old value           */
    obj    = std::forward<U>(new_value);     /* Step 2 – assign new value to *obj*   */
    return old;                              /* Step 3 – return the old value        */
}

} // namespace core
} // namespace ara

#endif // OPEN_AA_ADAPTIVE_AUTOSAR_LIBS_INCLUDE_ARA_CORE_ALGORITHM_H_