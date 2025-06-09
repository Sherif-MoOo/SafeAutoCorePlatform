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
 *  \file       ara/core/span.h
 *  \brief      Definition and implementation of the ara::core::Span template class.
 *
 *  \details    The Span class provides a lightweight, non-owning view into a contiguous sequence of elements.
 *              It is designed to be a safer alternative to raw pointers and array references, with
 *              bounds-checking and other safety features.
 *
 *  \note       Based on the Adaptive AUTOSAR SWS (e.g., R24-11) requirements for the "Span" type, especially:
 */
#ifndef OPEN_AA_ADAPTIVE_AUTOSAR_LIBS_INCLUDE_ARA_CORE_SPAN_H_
#define OPEN_AA_ADAPTIVE_AUTOSAR_LIBS_INCLUDE_ARA_CORE_SPAN_H_

#include <cstddef>               // For std::size_t
#include <limits>                // For std::numeric_limits
#include <type_traits>          // For std::remove_cv_t, std::is_same_v
#include <iterator>             // For std::reverse_iterator
#include <utility>              // For std::forward, std::declval


namespace ara {
namespace core {


/*! \brief  The maximum size for a dynamic extent in a span.
 *
 * This constant represents the maximum size that can be used for a span with dynamic extent.
 * It is set to the maximum value of std::size_t, which is typically the largest possible size
 * for an object in memory.
 *
 * \note [SWS_CORE_01901] This is used to indicate that a span can have a dynamic size at runtime.
 */
constexpr std::size_t dynamic_extent =  std::numeric_limits<std::size_t>::max();

/*! \brief  The Span class template.
 *
 * \tparam T The type of elements in the span.
 * \tparam Extent The size of the span (or dynamic_extent for dynamic size).
 *
 * \details
 * - This class provides a non-owning view into a contiguous sequence of elements.
 * - It supports bounds-checking, iteration, and other utilities similar to std::span.
 * - It is designed to be lightweight and efficient, avoiding unnecessary copies.
 *
 * \note [SWS_CORE_01900] This class is a core part of the Adaptive AUTOSAR platform's data handling utilities.
 */
template <typename T, std::size_t Extent = dynamic_extent>
class Span {
public:
    using element_type = T;                                                /*!< [SWS_CORE_01911] The type of elements in the span.                        */
    using value_type = std::remove_cv_t<element_type>;                     /*!< [SWS_CORE_01912] The type of elements in the span, without cv-qualifiers. */ 
    using size_type = std::size_t;                                         /*!< [SWS_CORE_01921] The type used for sizes and indices.                     */
    using difference_type = std::ptrdiff_t;                                /*!< [SWS_CORE_01914] The type used for differences between pointers.          */
    using pointer = element_type*;                                         /*!< [SWS_CORE_01915] Pointer to the first element in the span.                */
    using const_pointer = const element_type*;                             /*!< [SWS_CORE_01922] Pointer to the first element in a const span.            */
    using reference = element_type&;                                       /*!< [SWS_CORE_01916] Reference to an element in the span.                     */
    using const_reference = const element_type&;                           /*!< [SWS_CORE_01923] Reference to an element in a const span.                 */
    using iterator = element_type*;                                        /*!< [SWS_CORE_01917] Iterator type for the span.                              */
    using const_iterator = const element_type*;                            /*!< [SWS_CORE_01918] Const iterator type for the span.                        */
    using reverse_iterator = std::reverse_iterator<iterator>;              /*!< [SWS_CORE_01919] Reverse iterator type for the span.                      */
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;  /*!< [SWS_CORE_01920] Const reverse iterator type for the span.                */

    static constexpr size_type extent = Extent;                            /*!< [SWS_CORE_01931] The size of the span, or dynamic_extent if dynamic.      */

    constexpr Span(const Span& other) noexcept = default;                  /*!< [SWS_CORE_01949] Copy constructor.                                         */


};


/**********************************************************************************************************************
 *  NON-MEMBER FUNCTIONS
 *********************************************************************************************************************/

/*! \brief  Creates a span from a pointer and size.
 *
 * \tparam T The type of elements in the span.
 * \param ptr Pointer to the first element of the span.
 * \param size The number of elements in the span.
 * \return A span object representing the range from ptr to ptr + size.
 *
 * \note   [SWS_CORE_01990] This function is a convenience wrapper to create a span from a pointer and a size.
 */
template <typename T>
constexpr auto MakeSpan(T* ptr, typename Span<T>::size_type size) noexcept -> Span<T> {
    return Span<T>(ptr, size);
}


/*! \brief  Creates a span from two pointers.
 *
 * \tparam T The type of elements in the span.
 * \param firstElem Pointer to the first element of the span.
 * \param lastElem Pointer to one past the last element of the span.
 * \return A span object representing the range from firstElem to lastElem.
 *
 * \note   [SWS_CORE_01991] This function is a convenience wrapper to create a span from two pointers.
 */
template <typename T>
constexpr auto MakeSpan(T* firstElem, T* lastElem) noexcept -> Span<T> {
    return Span<T>(firstElem, lastElem - firstElem);
}

/*! \brief  Creates a span from an array.
 *
 * \tparam T The type of elements in the array.
 * \param arr Reference to the array.
 * \return A span object representing the range of the array.
 *
 * \note   [SWS_CORE_01992] This function is a convenience wrapper to create a span from an array.
 */
template <typename T, std::size_t N>
constexpr auto MakeSpan(T(&arr)[N]) noexcept -> Span<T> {
    return Span<T>(arr, N);
}

/*! \brief  Creates a span from a container.
 *
 * \tparam Container The type of the container (e.g., std::vector, std::array).
 * \param cont Reference to the container.
 * \return A span object representing the range of the container.
 *
 * \note   [SWS_CORE_01993] This function is a convenience wrapper to create a span from a container.
 */
template <typename Container>
constexpr auto MakeSpan(Container& cont) noexcept -> Span<typename Container::value_type> {
    return Span<typename Container::value_type>(cont.data(), cont.size());
}

/*! \brief  Creates a const span from a container.
 *
 * \tparam Container The type of the container (e.g., std::vector, std::array).
 * \param cont Reference to the container.
 * \return A const span object representing the range of the container.
 *
 * \note   [SWS_CORE_01994] This function is a convenience wrapper to create a const span from a container.
 */
template <typename Container>
constexpr auto MakeSpan(const Container& cont) noexcept -> Span<typename Container::value_type const> {
    return Span<typename Container::value_type const>(cont.data(), cont.size());
}

/*! \brief  Converts a span of elements to a span of bytes.
 *
 * \tparam ElementType The type of elements in the span.
 * \tparam Extent The size of the span (or dynamic_extent for dynamic size).
 * \param s The span to convert.
 * \return A span of bytes representing the same data as the original span.
 *
 * \note   [SWS_CORE_01980] This function is used to reinterpret the data in a span as bytes.
 */
template <typename ElementType, std::size Extent>
auto as_bytes(Span<ElementType, Extent> s) noexcept -> Span<const Byte, Extent==dynamic_extent? dynamic_extent : Extent * sizeof(ElementType)> {
    return Span<const Byte, Extent==dynamic_extent? dynamic_extent : Extent * sizeof(ElementType)>(reinterpret_cast<const Byte*>(s.data()), s.size_bytes());
}

/*! \brief  Converts a span of elements to a writable span of bytes.
 *
 * \tparam ElementType The type of elements in the span.
 * \tparam Extent The size of the span (or dynamic_extent for dynamic size).
 * \param s The span to convert.
 * \return A writable span of bytes representing the same data as the original span.
 *
 * \note   [SWS_CORE_01981] This function is used to reinterpret the data in a span as writable bytes.
 */
template <typename ElementType, std::size Extent>
auto as_writable_bytes(Span<ElementType, Extent> s) noexcept -> Span<Byte, Extent==dynamic_extent? dynamic_extent : Extent * sizeof(ElementType)> {
    return Span<Byte, Extent==dynamic_extent? dynamic_extent : Extent * sizeof(ElementType)>(reinterpret_cast<Byte*>(s.data()), s.size_bytes());
}

} // namespace core
} // namespace ara

#endif // OPEN_AA_ADAPTIVE_AUTOSAR_LIBS_INCLUDE_ARA_CORE_SPAN_H_