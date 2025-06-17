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
#include <type_traits>           // For std::remove_cv_t, std::is_same_v
#include <iterator>              // For std::reverse_iterator
#include <utility>               // For std::forward, std::declval


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


    /*! \brief  Default constructor for Span.
     *
     * This constructor initializes an empty span with no elements.
     * It is equivalent to a span of size 0.
     *
     * \note [SWS_CORE_01940] Default constructor initializes an empty span.
     */
    constexpr Span(const Span& other) noexcept = default;  
    
    /*! \brief  Default constructor for Span.
     *
     * This constructor initializes an empty span with no elements.
     * It is equivalent to a span of size 0.
     *
     * \note [SWS_CORE_01941] Default constructor.
     * \note This constructor only participates in overload resolution when:
     *       (Extent == dynamic_extent || Extent == 0) is true
     */
    template <typename = std::enable_if_t<Extent == dynamic_extent || Extent == 0>>
    constexpr Span() noexcept 
        : data_{nullptr}, size_{0} 
    {
    }

    /*! \brief SFINAE constructor for invalid default construction.
     *
     * \note This constructor only participates in overload resolution when:
     *       - Extent != dynamic_extent && Extent != 0
     */
    template <typename T_ = T,
              typename = std::enable_if_t<Extent != dynamic_extent && Extent != 0>,
              typename = void>  // Extra dummy parameter to make signature different
    constexpr Span() noexcept
    {
        static_assert(Extent == dynamic_extent || Extent == 0,
             "\n[ERROR] Cannot default construct Span<T, Extent> when Extent is neither dynamic_extent nor 0.\n"
             "A static extent span with size > 0 must be initialized with data.\n");
    }

    
    constexpr auto operator=(const Span& other) noexcept -> Span& = default;

    /*! \brief  Converting constructor for compatible Span types.
     *
     * This constructor allows construction of a cv-qualified Span from a normal Span,
     * and also of a dynamic_extent Span from a static extent one.
     * 
     * \tparam U The type of elements within the other Span.
     * \tparam N The Extent of the other Span.
     * \param other The other Span instance to convert from.
     *
     * \note [SWS_CORE_01950] Converting constructor.
     * \note This constructor only participates in overload resolution when:
     *       - Extent == dynamic_extent || Extent == N is true
     *       - U(*)[] is convertible to T(*)[]
     *
     * \details Enables conversions like:
     *          - Span<int, 5> to Span<int> (static to dynamic)
     *          - Span<int, 5> to Span<const int, 5> (add const qualification)
     *          - Span<int> to Span<const int> (add const qualification)
     */
    template <typename U, std::size_t N,
              typename = std::enable_if_t<
                  (Extent == dynamic_extent || Extent == N) &&
                  std::is_convertible_v<U(*)[], T(*)[]>
              >>
    constexpr Span(const Span<U, N>& other) noexcept
        : data_{other.data_}, size_{other.size_}
        {

        }
    

    /*! \brief SFINAE constructor for converting Span types.
     *
     * \tparam U The type of elements within the other Span.
     * \tparam N The Extent of the other Span.
     * \note This constructor only participates in overload resolution when:
     *       - Extent != dynamic_extent && Extent != N
     *       - U(*)[] is not convertible to T(*)[]
     */
    template <typename U, std::size_t N,
              typename = std::enable_if_t<!((Extent == dynamic_extent || Extent == N) && std::is_convertible_v<U(*)[], T(*)[]>)>,
              typename = void>  // Extra dummy parameter to make signature different
    constexpr Span(const Span<U, N>& /*other*/) noexcept
        {
            static_assert(Extent == dynamic_extent || Extent == N,
                 "\n[ERROR] Cannot convert Span<U, N> to Span<T, Extent> when Extent is not dynamic_extent or N.\n");
            
            static_assert(std::is_convertible_v<U(*)[], T(*)[]>,
                 "\n[ERROR] Cannot convert Span<U, N> to Span<T, Extent> when U is not convertible to T.\n");     
        }

    

private:

    pointer data_;                                                      
    size_type size_;                                                    

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
    return Span<T>(firstElem, lastElem);
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
constexpr auto MakeSpan(T(&arr)[N]) noexcept -> Span<T, N> {
    return Span<T, N>(arr, N);
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

// /*! \brief  Converts a span of elements to a span of bytes.
//  *
//  * \tparam ElementType The type of elements in the span.
//  * \tparam Extent The size of the span (or dynamic_extent for dynamic size).
//  * \param s The span to convert.
//  * \return A span of bytes representing the same data as the original span.
//  *
//  * \note   [SWS_CORE_01980] This function is used to reinterpret the data in a span as bytes.
//  */
// template <typename ElementType, std::size Extent>
// auto as_bytes(Span<ElementType, Extent> s) noexcept -> Span<const Byte, Extent==dynamic_extent? dynamic_extent : Extent * sizeof(ElementType)> {
//     return Span<const Byte, Extent==dynamic_extent? dynamic_extent : Extent * sizeof(ElementType)>(reinterpret_cast<const Byte*>(s.data()), s.size_bytes());
// }

// /*! \brief  Converts a span of elements to a writable span of bytes.
//  *
//  * \tparam ElementType The type of elements in the span.
//  * \tparam Extent The size of the span (or dynamic_extent for dynamic size).
//  * \param s The span to convert.
//  * \return A writable span of bytes representing the same data as the original span.
//  *
//  * \note   [SWS_CORE_01981] This function is used to reinterpret the data in a span as writable bytes.
//  */
// template <typename ElementType, std::size Extent>
// auto as_writable_bytes(Span<ElementType, Extent> s) noexcept -> Span<Byte, Extent==dynamic_extent? dynamic_extent : Extent * sizeof(ElementType)> {
//     return Span<Byte, Extent==dynamic_extent? dynamic_extent : Extent * sizeof(ElementType)>(reinterpret_cast<Byte*>(s.data()), s.size_bytes());
// }

} // namespace core
} // namespace ara

#endif // OPEN_AA_ADAPTIVE_AUTOSAR_LIBS_INCLUDE_ARA_CORE_SPAN_H_