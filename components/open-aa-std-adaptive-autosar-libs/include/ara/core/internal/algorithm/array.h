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
 *  \file       ara/core/internal/algorithm/array.h
 *  \brief      Internal implementation helpers for ara::core::to_array.
 * \details    This file provides implementation details for converting built-in arrays to ara::core::Array.
 * \note       This file is part of the internal implementation and is not intended for public use.
 *            It is designed to support the core platform's requirements and ensure deterministic safe behavior in the
 *            core platform.
 *********************************************************************************************************************/
#ifndef ARA_CORE_INTERNAL_ALGORITHM_ARRAY_H_
#define ARA_CORE_INTERNAL_ALGORITHM_ARRAY_H_

namespace ara {
namespace core {
namespace detail {

/**********************************************************************************************************************
 *  TO_ARRAY IMPLEMENTATION HELPERS
 *********************************************************************************************************************/
/*!
 * \brief  Implementation helper for to_array with lvalue arrays.
 *
 * \details Creates an ara::core::Array by copying elements from a built-in array.
 *          This is used to implement ara::core::to_array for lvalue arrays.
 *
 * \tparam T The element type.
 * \tparam N The number of elements.
 * \tparam I Index sequence for unpacking.
 * \param  a The built-in array to copy from.
 * \return   An ara::core::Array containing copies of the elements.
 */
template <typename T, std::size_t N, std::size_t... I>
[[nodiscard]] constexpr auto to_array_impl(T (&a)[N], std::index_sequence<I...>)
#ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
    noexcept(std::is_nothrow_copy_constructible_v<T>)
#else
    noexcept
#endif
    -> ara::core::Array<std::remove_cv_t<T>, N>
{
#ifndef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
    static_assert(std::is_nothrow_copy_constructible_v<T>,
        "\n[ERROR] detail::to_array_impl: Type T must be nothrow copy constructible when exceptions are disabled.\n");
#endif
    
    return {{a[I]...}};
}

/*!
 * \brief  Implementation helper for to_array with rvalue arrays.
 *
 * \details Creates an ara::core::Array by moving elements from a built-in array.
 *          This is used to implement ara::core::to_array for rvalue arrays.
 *
 * \tparam T The element type.
 * \tparam N The number of elements.
 * \tparam I Index sequence for unpacking.
 * \param  a The built-in array to move from.
 * \return   An ara::core::Array containing moved elements.
 */
template <typename T, std::size_t N, std::size_t... I>
[[nodiscard]] constexpr auto to_array_impl(T (&&a)[N], std::index_sequence<I...>)
#ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
    noexcept(std::is_nothrow_move_constructible_v<T>)
#else
    noexcept
#endif
    -> ara::core::Array<std::remove_cv_t<T>, N>
{
#ifndef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
    static_assert(std::is_nothrow_move_constructible_v<T>,
        "\n[ERROR] detail::to_array_impl: Type T must be nothrow move constructible when exceptions are disabled.\n");
#endif
    
    return {{std::move(a[I])...}};
}

} // namespace detail
} // namespace core
} // namespace ara

#endif // ARA_CORE_INTERNAL_ALGORITHM_ARRAY_H_