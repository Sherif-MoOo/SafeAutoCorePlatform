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
 *  \file       ara/core/internal/algorithm/tuple.h
 *  \brief      Internal utilities for tuple-like algorithms.
 *  \details    This file defines algorithms for working with tuple-like types, such as ara::core::Array and std::tuple.
 *              It includes functions for applying callables to tuples,
 *              concatenating tuples, and converting tuple-like types to std::tuple.
 *  \note       This file is part of the internal implementation and is not intended for public
 *              use. It is designed to support the core platform's requirements and ensure
 *              deterministic safe behavior in the core platform.
 *********************************************************************************************************************/
#ifndef ARA_CORE_INTERNAL_ALGORITHM_TUPLE_H_
#define ARA_CORE_INTERNAL_ALGORITHM_TUPLE_H_

#include <tuple>                        // For std::tuple, std::make_tuple, std::tuple_cat
#include <type_traits>                  // For std::enable_if_t, std::is_tuple_like
#include <utility>                      // For std::forward, std::move, std::index_sequence
#include "ara/core/internal/utility.h"  // For utility functions and traits

namespace ara {
namespace core {
namespace detail {

/**********************************************************************************************************************
 *  TUPLE CONVERSION UTILITIES
 *********************************************************************************************************************/
/*!
 * \brief  Implementation helper to convert a tuple-like type to std::tuple.
 *
 * \details Creates a tuple of values (not references) from a tuple-like object.
 *          This matches the behavior of std::tuple_cat which uses value semantics:
 *          - For lvalues: copies the elements
 *          - For rvalues: moves the elements
 *          - Always decays types (removes cv-qualifiers and references)
 *
 * \tparam Src The source tuple-like type.
 * \tparam I   Index sequence for unpacking.
 * \param  src The source tuple-like object.
 * \return     A std::tuple containing values (copies/moves) of the elements.
 *
 * \note Uses unqualified get for proper ADL lookup.
 */
template<typename Src, std::size_t... I>
[[nodiscard]] constexpr auto to_std_tuple_impl(Src&& src, std::index_sequence<I...>)
#ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
    noexcept(noexcept(std::make_tuple(get<I>(std::forward<Src>(src))...)))
#else
    noexcept
#endif
    -> decltype(std::make_tuple(get<I>(std::forward<Src>(src))...))
{
#ifndef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
    static_assert(noexcept(std::make_tuple(get<I>(std::forward<Src>(src))...)),
        "\n[ERROR] detail::to_std_tuple_impl: Tuple construction must be noexcept when exceptions are disabled.\n");
#endif
    
    return std::make_tuple(get<I>(std::forward<Src>(src))...);
}

/*!
 * \brief  Converts a tuple-like type to std::tuple.
 *
 * \details Converts any tuple-like type (e.g., ara::core::Array, std::array, std::pair)
 *          into a std::tuple with value semantics. This is designed to work with
 *          std::tuple_cat and similar utilities that expect value semantics.
 *
 * \tparam Src The source tuple-like type.
 * \param  src The source tuple-like object.
 * \return     A std::tuple containing values (copies/moves) of the elements.
 *
 * \pre The type std::remove_reference_t<Src> must satisfy is_tuple_like_v.
 */
template<typename Src>
[[nodiscard]] constexpr auto to_std_tuple(Src&& src)
#ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
    noexcept(noexcept(to_std_tuple_impl(
        std::forward<Src>(src),
        std::make_index_sequence<std::tuple_size_v<
            std::remove_cv_t<std::remove_reference_t<Src>>>>{})))
#else
    noexcept
#endif
    -> decltype(to_std_tuple_impl(
        std::forward<Src>(src),
        std::make_index_sequence<std::tuple_size_v<
            std::remove_cv_t<std::remove_reference_t<Src>>>>{}))
{
    static_assert(is_tuple_like_v<std::remove_reference_t<Src>>,
        "\n[ERROR] detail::to_std_tuple: Type must be tuple-like (have tuple_size, tuple_element, and get).\n");
    
    constexpr std::size_t N = std::tuple_size_v<
        std::remove_cv_t<std::remove_reference_t<Src>>>;
    
#ifndef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
    static_assert(noexcept(to_std_tuple_impl(
        std::forward<Src>(src),
        std::make_index_sequence<N>{})),
        "\n[ERROR] detail::to_std_tuple: Conversion must be noexcept when exceptions are disabled.\n");
#endif
    
    return to_std_tuple_impl(std::forward<Src>(src),
                             std::make_index_sequence<N>{});
}

/**********************************************************************************************************************
 *  APPLY IMPLEMENTATION
 *********************************************************************************************************************/
/*!
 * \brief  Implementation helper for apply function.
 *
 * \details Expands the tuple-like object and invokes the callable with its elements.
 *          Perfectly forwards the elements to preserve value categories when calling
 *          the function. Uses unqualified get for proper ADL lookup.
 *
 * \tparam F     The callable type.
 * \tparam Tuple The tuple-like type.
 * \tparam I     Index sequence for unpacking.
 * \param  f     The callable to invoke.
 * \param  tup   The tuple-like object containing arguments.
 * \return       The result of invoking f with the tuple elements.
 *
 * \warning The result of this function should not be discarded if the callable
 *          returns a value that needs to be handled.
 */
template<typename F, typename Tuple, std::size_t... I>
[[nodiscard]] constexpr auto apply_impl(F&& f, Tuple&& tup, std::index_sequence<I...>)
#ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
    noexcept(noexcept(std::forward<F>(f)(get<I>(std::forward<Tuple>(tup))...)))
#else
    noexcept
#endif
    -> invoke_result_t<F&&, decltype(get<I>(std::forward<Tuple>(tup)))...>
{
#ifndef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
    static_assert(noexcept(std::forward<F>(f)(get<I>(std::forward<Tuple>(tup))...)),
        "\n[ERROR] detail::apply_impl: Callable invocation must be noexcept when exceptions are disabled.\n");
#endif
    
    return std::forward<F>(f)(get<I>(std::forward<Tuple>(tup))...);
}

} // namespace detail
} // namespace core
} // namespace ara

#endif // ARA_CORE_INTERNAL_ALGORITHM_TUPLE_H_