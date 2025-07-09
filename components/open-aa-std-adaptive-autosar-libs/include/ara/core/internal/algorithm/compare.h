/***********************************************************************************************************************
 *  PROJECT
 *  --------------------------------------------------------------------------------------------------------------------
 *  \verbatim
 *  OpenAA: Open Source Adaptive AUTOSAR Project (CXX_STANDARD 17)
 *  Author: Sherif Mohamed
 *  \endverbatim
 *  --------------------------------------------------------------------------------------------------------------------
 *  FILE DESCRIPTION
 *  --------------------------------------------------------------------------------------------------------------------
 *  \file       ara/core/internal/algorithm/compare.h
 *  \brief      Internal utilities for comparison algorithms.
 *  \details    This file defines comparison algorithms 
  *  \note       This file is part of the internal implementation and is not intended for public
 *              use. It is designed to support the core platform's requirements and ensure
 *              deterministic safe behavior in the core platform.
 *********************************************************************************************************************/
#ifndef ARA_CORE_INTERNAL_ALGORITHM_COMPARE_H_
#define ARA_CORE_INTERNAL_ALGORITHM_COMPARE_H_

#include <tuple>                        // For std::tuple, std::make_tuple, std::tuple_cat
#include <type_traits>                  // For std::enable_if_t
#include <utility>                      // For std::forward, std::move, std::index_sequence
#include "ara/core/internal/utility.h"  // For utility functions and traits

namespace ara {
namespace core {
namespace detail {

/***********************************************************************************************************************
 *  LEXICOGRAPHICAL COMPARISON IMPLEMENTATION
 **********************************************************************************************************************/
/*! 
 * \brief  Implementation helper for lexicographical comparison of two ranges.
 *
 * \details Compares two ranges element-wise in lexicographical order:
 * - If the first range is less than the second, returns true
 * - If the first range is greater than the second, returns false
 * - If they are equal, compares their lengths
 *
 * \tparam RandomIt1  Iterator type for the first range
 * \tparam RandomIt2  Iterator type for the second range
 * \param  first1     Start iterator for the first range
 * \param  last1      End iterator for the first range
 * \param  first2     Start iterator for the second range
 * \param  last2      End iterator for the second range
 * \return            True if first range is lexicographically less than second, false otherwise
 */
template<typename RandomIt1, typename RandomIt2>
[[nodiscard]] constexpr auto lexicographical_compare_random_access_impl(
    RandomIt1 first1, RandomIt1 last1,
    RandomIt2 first2, RandomIt2 last2)
#ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
    noexcept(noexcept(*first1 < *first2) && noexcept(*first1 == *first2))
#else
    noexcept
#endif
-> bool
{
    using diff_t1 = typename std::iterator_traits<RandomIt1>::difference_type;
    
    auto const len1 = last1 - first1;
    auto const len2 = last2 - first2;
    auto const min_len = (len1 < len2) ? len1 : len2;
    
    for (diff_t1 i = 0; i < min_len; ++i) {
        if (first1[i] < first2[i]) return true;
        if (first2[i] < first1[i]) return false;
    }
    
    return len1 < len2;
}
 
/***********************************************************************************************************************
 *  LEXICOGRAPHICAL COMPARISON WITH CUSTOM COMPARATOR
 **********************************************************************************************************************/
/*!
 * \brief  Implementation helper for lexicographical comparison of two ranges with a custom comparator.
 * \details Compares two ranges element-wise in lexicographical order using a custom comparator:
 * - If the first range is less than the second, returns true
 * - If the first range is greater than the second, returns false
 * - If they are equal, compares their lengths
 * \tparam RandomIt1  Iterator type for the first range
 * \tparam RandomIt2  Iterator type for the second range
 * \tparam Compare    Type of the comparison function
 * \param  first1     Start iterator for the first range
 * \param  last1      End iterator for the first range
 * \param  first2     Start iterator for the second range
 * \param  last2      End iterator for the second range
 * \param  comp       Comparison function that takes two elements and returns true if the first is less than the second
 * \return            True if first range is lexicographically less than second, false otherwise
 * \note The comparison function must be noexcept if exceptions are disabled.
 */
template<typename RandomIt1, typename RandomIt2, typename Compare>
[[nodiscard]] constexpr auto lexicographical_compare_random_access_impl(
    RandomIt1 first1, RandomIt1 last1,
    RandomIt2 first2, RandomIt2 last2,
    Compare comp)
#ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
    noexcept(noexcept(comp(*first1, *first2)))
#else
    noexcept
#endif
-> bool
{
    using diff_t1 = typename std::iterator_traits<RandomIt1>::difference_type;
    
    auto const len1 = last1 - first1;
    auto const len2 = last2 - first2;
    auto const min_len = (len1 < len2) ? len1 : len2;
    
    for (diff_t1 i = 0; i < min_len; ++i) {
        if (comp(first1[i], first2[i])) return true;
        if (comp(first2[i], first1[i])) return false;
    }
    
    return len1 < len2;
}

} // namespace detail
} // namespace core
} // namespace ara

#endif // ARA_CORE_INTERNAL_ALGORITHM_COMPARE_H_