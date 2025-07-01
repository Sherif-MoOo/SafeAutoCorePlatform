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

#include <tuple>                        // For std::tuple_size, std::tuple_element, etc.
#include "ara/core/internal/utility.h"  // For utility functions and traits

/**********************************************************************************************************************
 *  TUPLE-LIKE UTILITIES
 *  ---------------------------------------------------------------------------------------------------------------
 *  ara::core::apply      – constexpr replacement for std::apply that works with ADL
 *  ara::core::tuple_cat  – concatenates any number of tuple-protocol objects
 *********************************************************************************************************************/
namespace ara {
namespace core {

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

} // namespace core
} // namespace ara


#endif // OPEN_AA_ADAPTIVE_AUTOSAR_LIBS_INCLUDE_ARA_CORE_ALGORITHM_H_