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
 *  \file       ara/core/ranges.h
 *  \brief      Definition and implementation of the ara::core::ranges namespace for span operations.
 *  \details    This file provides utilities for working with spans, including functions for creating,
 *               manipulating, and querying spans in a safe and efficient manner.
 *
 *  \note       Based on the Adaptive AUTOSAR SWS (e.g., R24-11) requirements.
 */

#ifndef OPEN_AA_ADAPTIVE_AUTOSAR_LIBS_INCLUDE_ARA_CORE_RANGES_H_
#define OPEN_AA_ADAPTIVE_AUTOSAR_LIBS_INCLUDE_ARA_CORE_RANGES_H_

#include <cstddef>                                  // For std::size_t
#include <iterator>                                 // For std::iterator_traits
#include <type_traits>                              // For std::enable_if, std::is_same
#include <utility>                                  // For std::declval

namespace ara {
namespace core {

template<typename T, std::size_t E>
class Span;

template<typename CharT, typename Traits>
class BasicStringView;    

namespace ranges {


/*!
 * \brief Span satisfies borrowed_range concept
 */
template<typename T>
struct enable_borrowed_range : std::false_type {};

template<typename T, std::size_t E>
struct enable_borrowed_range<Span<T, E>> : std::true_type {};

template<typename CharT, typename Traits>
struct enable_borrowed_range<BasicStringView<CharT, Traits>> : std::true_type {};



} // namespace ranges
} // namespace core
} // namespace ara


#endif // OPEN_AA_ADAPTIVE_AUTOSAR_LIBS_INCLUDE_ARA_CORE_RANGES_H_