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
 *  \brief      Definition and implementation of the ara::core::Span type.
 *
 *  \details    This file defines and implements the ara::core::Span type, a non-owning view over a contiguous
 *              sequence of objects. It provides a type-safe, bounds-aware alternative to raw pointers and size
 *              pairs, designed for the OpenAA project. The implementation exceeds standard C++20 span functionality
 *              by providing C++17-compatible ranges support, enhanced safety features, and optimizations for
 *              automotive applications per Adaptive AUTOSAR requirements.
 *
 *  \note       Based on the Adaptive AUTOSAR SWS (e.g., R24-11) requirements for the "Span" type, especially:
 *              - [SWS_CORE_01901] Dynamic extent constant definition
 *              - [SWS_CORE_01911-01920] Type aliases for span
 *              - [SWS_CORE_01931] Static extent member
 *              - [SWS_CORE_01940-01952] Constructors and assignment
 *              - [SWS_CORE_01960-01972] Element access methods
 *              - [SWS_CORE_01980-01994] Utility functions and conversions
 *              - Enhanced with C++26-like features while maintaining C++17 compatibility
 *              - Comprehensive ranges support without C++20 ranges library
 *              - Zero-overhead abstraction with performance for static extents
 *********************************************************************************************************************/

#ifndef OPEN_AA_ADAPTIVE_AUTOSAR_LIBS_INCLUDE_ARA_CORE_SPAN_H_
#define OPEN_AA_ADAPTIVE_AUTOSAR_LIBS_INCLUDE_ARA_CORE_SPAN_H_

/**********************************************************************************************************************
 *  INCLUDES
 *********************************************************************************************************************/
/*!
 * \brief  Standard library headers required for span implementation
 *
 * [SWS_CORE_01901]: cstddef for std::size_t and std::ptrdiff_t
 * [SWS_CORE_01911-01920]: type_traits for type aliases and SFINAE
 * [SWS_CORE_01960-01972]: iterator support for span operations
 */
#include <cstddef>                                  // For std::size_t, std::ptrdiff_t
#include <cstdint>                                  // For fixed-width integer types
#include <type_traits>                              // For type trait checks and SFINAE
#include <iterator>                                 // For iterator categories and traits
#include <limits>                                   // For numeric_limits
#include <utility>                                  // For std::forward, std::move
#include <algorithm>                                // For std::copy, std::equal, std::find
#include <memory>                                   // For std::addressof
#include <initializer_list>                         // For C++26-like initializer_list support
#include <array>                                    // For std::array support

#include "ara/core/ranges.h"                        // For ranges support in span
#include "ara/core/byte.h"                          // For ara::core::Byte type
#include "ara/core/internal/utility.h"              // For utility functions and traits
#include "ara/core/internal/storage/span_storage.h" // For array storage implementation
#include "ara/core/iterator.h"                      // For iterator utilities
#include "ara/core/internal/location_utils.h"       // For capturing file/line details
#include "ara/core/internal/violation_handler.h"    // To trigger violations
#include "ara/core/abort.h"                         // For direct abort on non-specified violations

/**********************************************************************************************************************
 *  NAMESPACE: ara::core
 *********************************************************************************************************************/
/*!
 * \brief  The ara::core namespace, containing AUTOSAR Adaptive Platform core types
 */
namespace ara {
namespace core {

/**********************************************************************************************************************
 *  CLASS: ara::core::Span
 *********************************************************************************************************************/
/*!
 * \brief  A non-owning view over a contiguous sequence of objects
 *
 * \tparam ElementType The type of elements viewed by this span
 * \tparam Extent The number of elements in the sequence, or dynamic_extent if runtime-sized
 *
 * \details
 * - [SWS_CORE_01900]: Core span class template definition
 * - Provides bounds-safe access to contiguous memory sequences
 * - Zero-overhead abstraction when bounds checking is disabled
 * - Static extent spans provide performance improvement
 * - Full compatibility with AUTOSAR containers (Array, Vector)
 * - Enhanced with C++26 features while maintaining C++17 compatibility
 *
 * \note Unlike std::span, this implementation provides additional safety features
 *       and integrations specific to AUTOSAR Adaptive Platform requirements.
 * 
 * \note Thread Safety: All const member functions are thread-safe. Non-const
 *       constructors and assignment operators require external synchronization.
 */
template<typename ElementType, std::size_t Extent>
class Span : private detail::span_storage_base<ElementType, Extent> {
private:
    using storage_type = detail::span_storage_base<ElementType, Extent>;
    using storage_type::data_;
    using storage_type::size;

public:
    // -----------------------------------------------------------------------------------
    // TYPE ALIASES [SWS_CORE_01911-01920]
    // -----------------------------------------------------------------------------------
    using element_type          = ElementType;                         /*!< [SWS_CORE_01911] Type of elements */
    using value_type            = std::remove_cv_t<ElementType>;       /*!< [SWS_CORE_01912] Value type without cv-qualifiers */
    using size_type             = std::size_t;                         /*!< [SWS_CORE_01913] Type for sizes and indices */
    using difference_type       = std::ptrdiff_t;                      /*!< [SWS_CORE_01914] Type for pointer differences */
    using pointer               = element_type*;                       /*!< [SWS_CORE_01915] Pointer to element */
    using const_pointer         = const element_type*;                 /*!< [SWS_CORE_01922] Const pointer to element */
    using reference             = element_type&;                       /*!< [SWS_CORE_01916] Reference to element */
    using const_reference       = const element_type&;                 /*!< [SWS_CORE_01923] Const reference to element */
    
    // Iterator types - implementation defined below
    class iterator;
    class const_iterator;
    
    using reverse_iterator       = std::reverse_iterator<iterator>;         /*!< [SWS_CORE_01919] Reverse iterator */
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;   /*!< [SWS_CORE_01920] Const reverse iterator */

    // -----------------------------------------------------------------------------------
    // STATIC MEMBERS [SWS_CORE_01931]
    // -----------------------------------------------------------------------------------
    static constexpr size_type extent = Extent;   /*!< [SWS_CORE_01931] Static extent value */

    // -----------------------------------------------------------------------------------
    // CONSTRUCTORS [SWS_CORE_01940-01950]
    // -----------------------------------------------------------------------------------
    
    /*!
     * \brief Default constructor - creates empty span
     *
     * \details
     * - [SWS_CORE_01941]: Default construction with empty span
     * - Only participates when Extent == 0 or Extent == dynamic_extent
     * - Results in data() == nullptr and size() == 0
     *
     * \note Compile-time error if called with static extent > 0
     */
    template<bool B = (Extent == 0) || (Extent == dynamic_extent),
             typename = std::enable_if_t<B>>
    constexpr Span() noexcept : storage_type{nullptr, 0} {}

    /*!
     * \brief Construct from pointer and count
     *
     * \param ptr Pointer to first element
     * \param count Number of elements (wrapped with location info)
     *
     * \details
     * - [SWS_CORE_01942]: Iterator pair constructor
     * - For static extent, count must equal Extent
     * - Triggers violation if count != Extent for static spans
     * - Triggers violation if ptr is null but count > 0
     *
     * \note Behavior is undefined if [ptr, ptr + count) is not valid range
     */
    constexpr Span(pointer ptr, const ara::core::internal::InputWithLocation<size_type>& count) noexcept
        : storage_type{ptr, count.input()}
    {
        // Validate count matches static extent
        if constexpr (Extent != dynamic_extent) {
            if (detail::unlikely(count.input() != Extent)) {
                if (!detail::is_constant_evaluated()) {
                        TriggerSizeViolation(count.info(), count.input(), Extent);
                } else {
                    constexpr unsigned char _illegal_count[1] = {};
                    [[maybe_unused]] const auto verify{_illegal_count[1]};
                }
            }
        }

        // Validate null pointer with non-zero count
        if (detail::unlikely(ptr == nullptr && count.input() > 0)) {
            if (!detail::is_constant_evaluated()) {
                TriggerNullPointerViolation(count.info());
            } else {
                constexpr unsigned char _null_pointer_violation[1] = {};
                [[maybe_unused]] const auto verify{_null_pointer_violation[1]};
            }
        }
    }

    /*!
     * \brief Construct from pointer range
     *
     * \param first Pointer to first element
     * \param last  Pointer past last element (wrapped with location info)
     *
     * \details
     * - [SWS_CORE_01943]: Range constructor
     * - Distance must equal Extent for static spans
     * - pointers must denote valid range (first <= last)
     * - Triggers violation if range is invalid or size mismatch
     * \note Behavior is undefined if [first, last) is not valid range
     *       or if ponters are not from the same array.
     *       This is checked at runtime for dynamic extents,
     *       but static extents assume compile-time safety.
     */
    constexpr Span(pointer first, const ara::core::internal::InputWithLocation<pointer>& last) noexcept
        : storage_type{first, static_cast<size_type>(ara::core::distance(first, last.input()))}
    {
        
        if (!first || !last.input()) {

            if (!detail::is_constant_evaluated()) {
                TriggerNullPointerViolation(last.info());
            } else {
                constexpr unsigned char _null_pointer_violation[1] = {};
                [[maybe_unused]] const auto verify{_null_pointer_violation[1]};
            }
        }

        if (first > last.input()) {
            if (!detail::is_constant_evaluated()) {
                TriggerRangeViolation(last.info());
            } else {
                constexpr unsigned char _illegal_range[1] = {};
                [[maybe_unused]] const auto verify{_illegal_range[1]};
            }
        }

        if constexpr (Extent != dynamic_extent) {
            const auto dist = static_cast<size_type>(ara::core::distance(first, last.input()));
            if (detail::unlikely(dist != Extent)) {
                if (!detail::is_constant_evaluated()) {
                    TriggerSizeViolation(last.info(), dist, Extent);
                } else {
                    constexpr unsigned char _illegal_count[1] = {};
                    [[maybe_unused]] const auto verify{_illegal_count[1]};
                }
            }
        }
    }

    /*!
     * \brief Construct from C-style array
     *
     * \tparam N Size of array
     * \param arr Array to view
     *
     * \details
     * - [SWS_CORE_01944]: Array constructor
     * - Automatically deduces size from array
     * - Only participates when N == Extent or dynamic extent
     * - No runtime checks needed as size is compile-time known
     */
    template<std::size_t N,
             typename = std::enable_if_t<
                 (Extent == dynamic_extent || N == Extent) &&
                 detail::is_element_type_compatible_v<element_type*, element_type>>>
    constexpr Span(element_type (&arr)[N]) noexcept
        : storage_type{&arr[0], N} {}

    /*!
     * \brief Construct from std::array
     *
     * \tparam U Element type of array
     * \tparam N Size of array
     * \param arr Array to view
     *
     * \details
     * - Standard array constructor
     * - Supports const-qualification conversions
     * - Only participates when types and sizes are compatible
     * - No runtime checks needed as size is compile-time known
     */
    template<typename U, std::size_t N,
             typename = std::enable_if_t<
                 (Extent == dynamic_extent || N == Extent) &&
                 detail::is_element_type_compatible_v<U*, element_type>>>
    constexpr Span(std::array<U, N>& arr) noexcept
        : storage_type{arr.data(), N} {}

    template<typename U, std::size_t N,
             typename = std::enable_if_t<
                 (Extent == dynamic_extent || N == Extent) &&
                 detail::is_element_type_compatible_v<const U*, element_type>>>
    constexpr Span(const std::array<U, N>& arr) noexcept
        : storage_type{arr.data(), N} {}

    /*!
     * \brief Construct from ara::core::Array
     *
     * \tparam U Element type of array
     * \tparam N Size of array
     * \param arr Array to view
     *
     * \details
     * - Supports ara::core::Array types
     * - Enables seamless integration with Adaptive AUTOSAR containers
     * - Only participates when types and sizes are compatible
     * - No runtime checks needed as size is compile-time known
     */
    template<typename U, std::size_t N,
             typename = std::enable_if_t<
                 (Extent == dynamic_extent || N == Extent) &&
                 detail::is_element_type_compatible_v<U*, element_type>>>
    constexpr Span(ara::core::Array<U, N>& arr) noexcept
        : storage_type{arr.data(), N} {}

    template<typename U, std::size_t N,
             typename = std::enable_if_t<
                 (Extent == dynamic_extent || N == Extent) &&
                 detail::is_element_type_compatible_v<const U*, element_type>>>
    constexpr Span(const ara::core::Array<U, N>& arr) noexcept
        : storage_type{arr.data(), N} {}

    /*!
     * \brief Construct from range (iterator pair)
     *
     * \tparam It Iterator type
     * \tparam End Sentinel type
     * \param first Beginning of range
     * \param last End of range
     * \param loc Source location for error reporting (default captured)
     *
     * \details
     * - Range-based constructor for any iterator pair
     * - Enables construction from ranges namespace views
     * - Validates range order and size compatibility
     */
    template<typename It, typename End,
             typename = std::enable_if_t<
                 !(std::is_same_v<std::decay_t<It>, pointer> &&
                   std::is_same_v<std::decay_t<End>, pointer>) &&
                 !std::is_convertible_v<End, size_type> &&
                 detail::is_iterator_v<It> &&
                 detail::is_iterator_v<End> &&
                 std::is_convertible_v<typename std::iterator_traits<It>::pointer, pointer>>>
    constexpr Span(
            It  first,
            End last,
            const ara::core::internal::InputWithLocation<std::uint8_t> loc =
                  ara::core::internal::make_input_with_location<std::uint8_t>(0)) noexcept
        : storage_type{detail::to_address(first),
                      static_cast<size_type>(ara::core::distance(first, last))}
    {
        // Range order check for random-access iterators
        if constexpr (std::is_same_v<typename std::iterator_traits<It>::iterator_category,
                                    std::random_access_iterator_tag>) {
            if (detail::unlikely(last < first)) {
                if (!detail::is_constant_evaluated()) {
                    TriggerRangeViolation(loc.info());
                } else {
                    constexpr unsigned char _illegal_range[1] = {};
                    [[maybe_unused]] const auto verify{_illegal_range[1]};
                }
            }
        }


        // Extent check for static spans
        if constexpr (Extent != dynamic_extent) {
            const auto cnt = static_cast<size_type>(ara::core::distance(first, last));
            if (detail::unlikely(cnt != Extent)) {
                if (!detail::is_constant_evaluated()) {
                        TriggerSizeViolation(loc.info(), cnt, Extent);
                } else {
                    constexpr unsigned char _illegal_count[1] = {};
                    [[maybe_unused]] const auto verify{_illegal_count[1]};
                }
            }
        }
    }

    /*!
     * \brief Construct from contiguous container (non-const)
     *
     * \tparam Container Contiguous container type
     * \param cont Container to view
     * \param loc Source location for error reporting (default captured)
     *
     * \details
     * - Constructs span from any contiguous container with data() and size()
     * - Excludes spans, arrays, and ranges (handled by other constructors)
     * - Validates size compatibility for static spans
     */
    template <typename Container,
              typename = std::enable_if_t<
                  !detail::is_span_v<std::decay_t<Container>> &&
                  !detail::is_std_array_v<std::decay_t<Container>> &&
                  !detail::is_ara_array_v<std::decay_t<Container>> &&
                  !detail::is_c_array_v<std::decay_t<Container>> &&
                  !detail::is_contiguous_range_v<Container> &&
                  !std::is_const_v<std::remove_reference_t<Container>> &&
                  detail::is_contiguous_container_v<Container> &&
                  detail::is_element_type_compatible_v<
                      std::remove_pointer_t<decltype(std::data(std::declval<Container&>()))>*,
                      element_type>>>
    constexpr Span(Container& cont,
                   const ara::core::internal::InputWithLocation<std::uint8_t> loc =
                         ara::core::internal::make_input_with_location<std::uint8_t>(0)) noexcept
        : storage_type{std::data(cont), std::size(cont)}
    {   
        // Validate size matches static extent
        if constexpr (Extent != dynamic_extent) {
            if (detail::unlikely(std::size(cont) != Extent)) {
                if (!detail::is_constant_evaluated()) {
                    TriggerSizeViolation(loc.info(), std::size(cont), Extent);
                } else {
                    constexpr unsigned char _illegal_count[1] = {};
                    [[maybe_unused]] const auto verify{_illegal_count[1]};
                }
            }
        }
    }

    /*!
     * \brief Construct from contiguous container (const)
     *
     * \tparam Container Contiguous container type
     * \param cont Constant container to view
     * \param loc Source location for error reporting (default captured)
     *
     * \details
     * - Mirrors the non-const container overload
     * - Allows Span<const T> from const std::vector<T>, const std::string, etc.
     * - Excluded when element-type conversion would discard cv-qualification
     */
    template<typename Container,
             typename = std::enable_if_t<
                 !detail::is_span_v<std::decay_t<Container>> &&
                 !detail::is_std_array_v<std::decay_t<Container>> &&
                 !detail::is_ara_array_v<std::decay_t<Container>> &&
                 !detail::is_c_array_v<std::decay_t<Container>> &&
                 !detail::is_contiguous_range_v<Container> &&
                 detail::is_contiguous_container_v<const Container> &&
                 detail::is_element_type_compatible_v<
                     std::remove_pointer_t<
                         decltype(std::data(std::declval<const Container&>()))>*,
                     element_type>>>
    constexpr Span(const Container& cont,
                   const ara::core::internal::InputWithLocation<std::uint8_t> loc =
                         ara::core::internal::make_input_with_location<std::uint8_t>(0)) noexcept
        : storage_type{std::data(cont), std::size(cont)}
    {
        // Validate size matches static extent
        if constexpr (Extent != dynamic_extent) {
            if (detail::unlikely(std::size(cont) != Extent)) {
                if (!detail::is_constant_evaluated()) {
                    TriggerSizeViolation(loc.info(), std::size(cont), Extent);
                } else {
                    constexpr unsigned char _illegal_count[1] = {};
                    [[maybe_unused]] const auto verify{_illegal_count[1]};
                }
            }
        }
    }

    /*!
     * \brief Construct from range-like container
     *
     * \tparam Range Range type
     * \param r Range to view
     * \param loc Source location for error reporting (default captured)
     *
     * \details
     * - Constructs span from any contiguous range
     * - Works with ranges namespace views
     * - Enables natural range integration
     * - Validates size compatibility for static spans
     */
    template<typename Range,
             typename = void,
             typename = std::enable_if_t<
                 !detail::is_span_v<std::decay_t<Range>> &&
                 !detail::is_std_array_v<std::decay_t<Range>> &&
                 !detail::is_ara_array_v<std::decay_t<Range>> &&
                 !detail::is_c_array_v<std::decay_t<Range>> &&
                 !detail::is_contiguous_container_v<std::decay_t<Range>> &&
                 detail::is_contiguous_range_v<std::decay_t<Range>> &&
                 detail::is_element_type_compatible_v<
                     std::remove_pointer_t<decltype(std::data(std::declval<Range&>()))>*,
                     element_type>>>
    constexpr Span(Range&& r,
                   const ara::core::internal::InputWithLocation<std::uint8_t> loc =
                         ara::core::internal::make_input_with_location<std::uint8_t>(0)) noexcept
        : storage_type{std::data(r), std::size(r)}
    {
        // Validate size matches static extent
        if constexpr (Extent != dynamic_extent) {
            if (detail::unlikely(std::size(r) != Extent)) {
                if (!detail::is_constant_evaluated()) {
                    TriggerSizeViolation(loc.info(), std::size(r), Extent);
                } else {
                    constexpr unsigned char _illegal_count[1] = {};
                    [[maybe_unused]] const auto verify{_illegal_count[1]};
                }
            }
        }
    }

    /*!
     * \brief Converting constructor from another span
     *
     * \tparam U Element type of source span
     * \tparam N Extent of source span
     * \param source Source span to convert from
     * \param loc Source location for error reporting (default captured)
     *
     * \details
     * - [SWS_CORE_01950]: Converting constructor
     * - Enables T* to const T* conversions
     * - Static to dynamic extent conversions
     * - Size compatibility checked at compile/runtime
     */
    template <typename U, std::size_t N,
              typename = std::enable_if_t<
                  (Extent == dynamic_extent || N == Extent || N == dynamic_extent) &&
                  detail::is_element_type_compatible_v<U*, element_type>>>
    constexpr Span(const Span<U, N>& source,
                   const ara::core::internal::InputWithLocation<std::uint8_t> loc =
                         ara::core::internal::make_input_with_location<std::uint8_t>(0)) noexcept
        : storage_type{source.data(), source.size()}
    {
        // Validate size when converting dynamic to static extent
        if constexpr (Extent != dynamic_extent && N == dynamic_extent) {
            if (detail::unlikely(source.size() != Extent)) {
                if (!detail::is_constant_evaluated()) {
                    TriggerSizeViolation(loc.info(), source.size(), Extent);
                } else {
                    constexpr unsigned char _illegal_count[1] = {};
                    [[maybe_unused]] const auto verify{_illegal_count[1]};
                }
            }
        }
    }
    // -----------------------------------------------------------------------------------
    // C++26 FEATURES - INITIALIZER LIST SUPPORT
    // -----------------------------------------------------------------------------------
    /*!
     * \brief Construct from initializer list (C++26 feature, P2447R6)
     *
     * \param il Initializer list to view
     * \param loc Source location for error reporting (default captured)
     *
     * \details
     * - C++26 enhancement for natural syntax
     * - Allows span<const T> construction from {1, 2, 3}
     * - Static extent must match initializer list size
     * - Validates size compatibility
     *
     * \note Only available for const element types as initializer_list is immutable
     * \note good compiler options could catch it
     * e.g: ara::core::Span<const int> s1 = {1, 2, 3, 4, 5};
     *  error: may be used uninitialized [-Werror=maybe-uninitialized]
     *  for (int val : s1) {
     * e.g: ara::core::Span<const int, 3> s1 = {10, 20, 30};
     * error: is used uninitialized [-Werror=uninitialized]
        for (int val : s1) {
     */
    template<
        typename U = element_type,
        std::size_t E = Extent,
        typename = std::enable_if_t<
            std::is_const_v<U> &&
            std::is_same_v<value_type, std::remove_cv_t<U>>>>
    constexpr Span(std::initializer_list<value_type> il,
                   const ara::core::internal::InputWithLocation<std::uint8_t> loc =
                         ara::core::internal::make_input_with_location<std::uint8_t>(0)) noexcept
      : storage_type{il.begin(), il.size()}
    {
        // Validate size matches static extent
        if constexpr (E != dynamic_extent) {
            if (detail::unlikely(il.size() != E)) {
                if (!detail::is_constant_evaluated()) {
                    TriggerSizeViolation(loc.info(), il.size(), E);
                } else {
                    constexpr unsigned char _illegal_count[1] = {};
                    [[maybe_unused]] const auto verify{_illegal_count[1]};
                }
            }
        }
    }
    
    /*!
     * \brief Copy constructor
     *
     * \details
     * - [SWS_CORE_01940]: Defaulted copy constructor
     * - Trivially copyable for performance
     * - No validation needed as source is already valid
     */
    constexpr Span(const Span&) noexcept = default;

    /*!
     * \brief Copy assignment operator
     *
     * \details
     * - [SWS_CORE_01952]: Defaulted copy assignment
     * - Trivially copy assignable
     * - No validation needed as source is already valid
     */
    constexpr auto operator=(const Span&) noexcept -> Span& = default;

    /*!
     * \brief Destructor
     *
     * \details
     * - [SWS_CORE_01951]: Defaulted destructor
     * - Trivially destructible
     * - No cleanup needed for non-owning view
     */
    ~Span() noexcept = default;

    // -----------------------------------------------------------------------------------
    // SUBVIEWS [SWS_CORE_01970-01972]
    // -----------------------------------------------------------------------------------
    
    /*!
     * \brief Obtain subspan of first Count elements
     *
     * \tparam Count Number of elements in subspan
     * \param loc Source location for error reporting (default captured)
     * \return Span of first Count elements
     *
     * \details
     * - [SWS_CORE_01970]: Static first subspan
     * - Compile-time bounds checking when possible
     * - Runtime validation that Count doesn't exceed size
     * - Zero-cost abstraction in release builds
     */
    template<std::size_t Count>
    [[nodiscard]] constexpr auto first(
        const ara::core::internal::InputWithLocation<std::uint8_t> loc =
              ara::core::internal::make_input_with_location<std::uint8_t>(0)) const noexcept
        -> Span<element_type, Count>
    {
        static_assert(Count <= Extent || Extent == dynamic_extent,
                      "Count exceeds static extent");
        if (detail::unlikely(Count > size())) {
            if (!detail::is_constant_evaluated()) {
                TriggerSubspanViolation(loc.info(), "first", Count, size());
            } else {
                constexpr unsigned char _count_check[1] = {};
                [[maybe_unused]] const auto verify{_count_check[(Count <= size()) ? 0 : 1]};
            }
        }

        return Span<element_type, Count>(data_, Count);
    }

    /*!
     * \brief Obtain subspan of first count elements (dynamic)
     *
     * \param count Number of elements in subspan (wrapped with location info)
     * \return Dynamic extent span of first count elements
     *
     * \details
     * - [SWS_CORE_01970]: Dynamic first subspan
     * - Runtime bounds checking
     * - Validates count doesn't exceed current size
     */
    [[nodiscard]] constexpr auto first(
        const ara::core::internal::InputWithLocation<size_type>& count) const noexcept
        -> Span<element_type, dynamic_extent>
    {   

        if (count.input() > size()) {
            if (!detail::is_constant_evaluated()) {
                TriggerSubspanViolation(count.info(), "first", count.input(), size()); 
            } else {
                constexpr unsigned char _count_check[1] = {};
                [[maybe_unused]] const auto verify{_count_check[1]};
            }
        }
        return Span<element_type, dynamic_extent>(data_, count.input());
    }

    /*!
     * \brief Obtain subspan of last Count elements
     *
     * \tparam Count Number of elements in subspan
     * \param loc Source location for error reporting (default captured)
     * \return Span of last Count elements
     *
     * \details
     * - [SWS_CORE_01971]: Static last subspan
     * - Compile-time bounds checking when possible
     * - Runtime validation that Count doesn't exceed size
     */
    template<std::size_t Count>
    [[nodiscard]] constexpr auto last(
        const ara::core::internal::InputWithLocation<std::uint8_t> loc =
              ara::core::internal::make_input_with_location<std::uint8_t>(0)) const noexcept
        -> Span<element_type, Count>
    {
        static_assert(Count <= Extent || Extent == dynamic_extent,
                      "Count exceeds static extent");
        if (detail::unlikely(Count > size())) {
            if (!detail::is_constant_evaluated()) {
                TriggerSubspanViolation(loc.info(), "last", Count, size());
            } else {
                constexpr unsigned char _count_check[1] = {};
                [[maybe_unused]] const auto verify{_count_check[1]};
            }
        }
        return Span<element_type, Count>(data_ + size() - Count, Count);
    }

    /*!
     * \brief Obtain subspan of last count elements (dynamic)
     *
     * \param count Number of elements in subspan (wrapped with location info)
     * \return Dynamic extent span of last count elements
     *
     * \details
     * - Dynamic version of last()
     * - Runtime bounds checking
     * - Validates count doesn't exceed current size
     */
    [[nodiscard]] constexpr auto last(
        const ara::core::internal::InputWithLocation<size_type>& count) const noexcept
        -> Span<element_type, dynamic_extent>
    {   

        if (count.input() > size()) {
            if (!detail::is_constant_evaluated()) {
                TriggerSubspanViolation(count.info(), "last", count.input(), size());
            } else {
                constexpr unsigned char _count_check[1] = {};
                [[maybe_unused]] const auto verify{_count_check[1]};
            }
        }
        return Span<element_type, dynamic_extent>(data_ + size() - count.input(), count.input());
    }

    /*!
     * \brief Obtain subspan starting at Offset
     *
     * \tparam Offset Starting position
     * \tparam Count Number of elements (default: all remaining)
     * \param loc Source location for error reporting (default captured)
     * \return Subspan with calculated extent
     *
     * \details
     * - [SWS_CORE_01972]: Static subspan
     * - Extent calculated at compile time
     * - Validates offset and count at compile/runtime
     * - Optimized for static bounds checking
     */
    template<std::size_t Offset, std::size_t Count = dynamic_extent>
    [[nodiscard]] constexpr auto subspan(
        const ara::core::internal::InputWithLocation<std::uint8_t> loc =
              ara::core::internal::make_input_with_location<std::uint8_t>(0)) const noexcept
        -> Span<element_type, detail::subspan_extent<Extent, Offset, Count>::value>
    {
        static_assert(Offset <= Extent || Extent == dynamic_extent,
                      "Offset exceeds static extent");
        static_assert(Count == dynamic_extent || 
                      (Extent != dynamic_extent && Count <= Extent - Offset) || 
                      Extent == dynamic_extent,
                      "Count exceeds remaining elements");

        if (detail::unlikely(Offset > size())) {
            if (!detail::is_constant_evaluated()) {
                TriggerSubspanViolation(loc.info(), "subspan", Offset, size(), true);
            } else {
                constexpr unsigned char _offset_check[1] = {};
                [[maybe_unused]] const auto verify{_offset_check[1]};
            }
        }

        if (detail::unlikely(Count != dynamic_extent && Count > size() - Offset)) {
            if (!detail::is_constant_evaluated()) {
                TriggerSubspanViolation(loc.info(), "subspan", Count, size() - Offset);
            } else {
                constexpr unsigned char _count_check[1] = {};
                [[maybe_unused]] const auto verify{_count_check[1]};
            }
        }
        
        return Span<element_type, detail::subspan_extent<Extent, Offset, Count>::value>(
            data_ + Offset,
            Count == dynamic_extent ? size() - Offset : Count
        );
    }

    /*!
     * \brief Obtain subspan starting at offset (dynamic)
     *
     * \param offset Starting position (wrapped with location info)
     * \param count Number of elements (wrapped with location info, default: all remaining)
     * \return Dynamic extent subspan
     *
     * \details
     * - Dynamic version of subspan()
     * - Runtime bounds checking for both offset and count
     * - Default count means "all remaining elements"
     */
    [[nodiscard]] constexpr auto subspan(
        const ara::core::internal::InputWithLocation<size_type>& offset, 
                                        size_type count = dynamic_extent) const noexcept
        -> Span<element_type, dynamic_extent>
    {
        
        if (detail::unlikely(offset.input() > size())) {
            if (!detail::is_constant_evaluated()) {
                TriggerSubspanViolation(offset.info(), "subspan", offset.input(), size(), true);
            } else {
                constexpr unsigned char _offset_check[1] = {};
                [[maybe_unused]] const auto verify{_offset_check[1]};
            }
        }

        if (detail::unlikely(count != dynamic_extent && count > size() - offset.input())) {
            if (!detail::is_constant_evaluated()) {
                TriggerSubspanViolation(offset.info(), "subspan", count, size() - offset.input());
            } else {
                constexpr unsigned char _count_check[1] = {};
                [[maybe_unused]] const auto verify{_count_check[1]};
            }
        }

        return Span<element_type, dynamic_extent>(
            data_ + offset.input(),
            count == dynamic_extent ? size() - offset.input() : count
        );
    }

    // -----------------------------------------------------------------------------------
    // OBSERVERS [SWS_CORE_01960-01963]
    // -----------------------------------------------------------------------------------
    
    /*!
     * \brief Get number of elements
     *
     * \return Number of elements in span
     *
     * \details
     * - [SWS_CORE_01960]: Size observer
     * - Compile-time constant for static extent
     * - Zero-cost abstraction
     * - No validation needed (always valid)
     */
    [[nodiscard]] constexpr size_type size() const noexcept
    {
        return storage_type::size();
    }

    /*!
     * \brief Get size in bytes
     *
     * \return Total size of all elements in bytes
     *
     * \details
     * - [SWS_CORE_01961]: Size in bytes observer
     * - Equivalent to size() * sizeof(element_type)
     * - May overflow for very large spans (user responsibility)
     */
    [[nodiscard]] constexpr size_type size_bytes() const noexcept
    {
        return size() * sizeof(element_type);
    }

    /*!
     * \brief Check if span is empty
     *
     * \return true if size() == 0
     *
     * \details
     * - [SWS_CORE_01962]: Empty check
     * - Compile-time constant for static extent
     * - More efficient than size() == 0
     */
    [[nodiscard]] constexpr bool empty() const noexcept
    {
        return size() == 0;
    }

    // -----------------------------------------------------------------------------------
    // ELEMENT ACCESS [SWS_CORE_01964-01969]
    // -----------------------------------------------------------------------------------
    
    /*!
     * \brief Access element at index with bounds checking
     *
     * \param idx Index of element (wrapped with location info)
     * \return Reference to element
     *
     * \details
     * - [SWS_CORE_01964]: Element access operator
     * - Always performs bounds checking
     * - Calls violation handler on out-of-bounds
     * - Consistent with at() for safety
     *
     * \note Standard span::operator[] is unchecked, but we prioritize safety
     */
    [[nodiscard]] constexpr reference operator[](
        const ara::core::internal::InputWithLocation<size_type>& idx) const noexcept
    {   

        const size_type& I = idx.input();

        if (detail::unlikely(I >= size())) {
            if (!detail::is_constant_evaluated()) {
                TriggerBoundsViolation(idx.info(), I, size());
            } else {
                constexpr unsigned char _bounds_check[1] = {};
                [[maybe_unused]] const auto verify{_bounds_check[1]};
            }
        }

        return data_[I];
    }

    /*!
     * \brief Access element with explicit bounds checking
     *
     * \param idx Index of element (wrapped with location info)
     * \return Reference to element
     *
     * \details
     * - C++26 enhancement for safety
     * - Always performs bounds checking
     * - Calls violation handler on out-of-bounds
     * - Provides consistent interface with Array and Vector
     */
    [[nodiscard]] constexpr reference at(
        const ara::core::internal::InputWithLocation<size_type>& idx) const
    {
        const size_type& I = idx.input();

        if (detail::unlikely(I >= size())) {
            if (!detail::is_constant_evaluated()) {
                TriggerBoundsViolation(idx.info(), I, size());
            } else {
                constexpr unsigned char _bounds_check[1] = {};
                [[maybe_unused]] const auto verify{_bounds_check[1]};
            }
        }

        return data_[I];
    }

    /*!
     * \brief Access first element
     *
     * \param loc Source location for error reporting (default captured)
     * \return Reference to first element
     *
     * \details
     * - [SWS_CORE_01966]: Front element access
     * - Validates span is not empty
     * - More descriptive error than at(0)
     */
    [[nodiscard]] constexpr reference front(
        const ara::core::internal::InputWithLocation<std::uint8_t> loc =
              ara::core::internal::make_input_with_location<std::uint8_t>(0)) const noexcept
    {
        if (detail::unlikely(empty())) {
            if (!detail::is_constant_evaluated()) {
                TriggerEmptyAccessViolation(loc.info(), "front");
            } else {
                constexpr unsigned char _empty_check[1] = {};
                [[maybe_unused]] const auto verify{_empty_check[1]};
            }
        }
        return data_[0];
    }

    /*!
     * \brief Access last element
     *
     * \param loc Source location for error reporting (default captured)
     * \return Reference to last element
     *
     * \details
     * - [SWS_CORE_01967]: Back element access
     * - Validates span is not empty
     * - More descriptive error than at(size()-1)
     */
    [[nodiscard]] constexpr reference back(
        const ara::core::internal::InputWithLocation<std::uint8_t> loc =
              ara::core::internal::make_input_with_location<std::uint8_t>(0)) const noexcept
    {
        if (detail::unlikely(empty())) {
            if (!detail::is_constant_evaluated()) {
                TriggerEmptyAccessViolation(loc.info(), "back");
            } else {
                constexpr unsigned char _empty_check[1] = {};
                [[maybe_unused]] const auto verify{_empty_check[1]};
            }
        }
        return data_[size() - 1];
    }

    /*!
     * \brief Direct access to underlying array
     *
     * \return Pointer to first element (or nullptr if empty)
     *
     * \details
     * - [SWS_CORE_01965]: Data pointer access
     * - Returns nullptr for empty spans (even if constructed with non-null pointer)
     * - Contiguous memory guarantee
     * - No validation needed (always safe)
     */
    [[nodiscard]] constexpr pointer data() const noexcept
    {
        return data_;
    }

    // -----------------------------------------------------------------------------------
    // ITERATOR SUPPORT [SWS_CORE_01917-01918]
    // -----------------------------------------------------------------------------------
    
    /*!
     * \brief Iterator type for span
     *
     * \details
     * - [SWS_CORE_01917]: Iterator type definition
     * - Satisfies contiguous iterator requirements
     * - Enables vectorization and optimization
     * - No bounds checking in iterators (performance critical)
     */
    class iterator {
    public:
        using iterator_category = std::random_access_iterator_tag;
        using value_type        = std::remove_cv_t<element_type>;
        using difference_type   = std::ptrdiff_t;
        using pointer          = element_type*;
        using reference        = element_type&;
        #if __cplusplus >= 202002L
        using iterator_concept = std::contiguous_iterator_tag;         
        #endif

    private:
        pointer ptr_;

    public:
        constexpr iterator() noexcept : ptr_(nullptr) {}
        constexpr explicit iterator(pointer ptr) noexcept : ptr_(ptr) {}

        [[nodiscard]] constexpr reference operator*() const noexcept { return *ptr_; }
        [[nodiscard]] constexpr pointer operator->() const noexcept { return ptr_; }
        [[nodiscard]] constexpr reference operator[](difference_type n) const noexcept { return ptr_[n]; }

        constexpr iterator& operator++() noexcept { ++ptr_; return *this; }
        constexpr iterator operator++(int) noexcept { iterator tmp = *this; ++ptr_; return tmp; }
        constexpr iterator& operator--() noexcept { --ptr_; return *this; }
        constexpr iterator operator--(int) noexcept { iterator tmp = *this; --ptr_; return tmp; }

        constexpr iterator& operator+=(difference_type n) noexcept { ptr_ += n; return *this; }
        constexpr iterator& operator-=(difference_type n) noexcept { ptr_ -= n; return *this; }

        [[nodiscard]] constexpr iterator operator+(difference_type n) const noexcept 
        { return iterator(ptr_ + n); }
        [[nodiscard]] constexpr iterator operator-(difference_type n) const noexcept 
        { return iterator(ptr_ - n); }
        [[nodiscard]] constexpr difference_type operator-(const iterator& other) const noexcept 
        { return ptr_ - other.ptr_; }

        [[nodiscard]] constexpr bool operator==(const iterator& other) const noexcept 
        { return ptr_ == other.ptr_; }
        [[nodiscard]] constexpr bool operator!=(const iterator& other) const noexcept 
        { return ptr_ != other.ptr_; }
        [[nodiscard]] constexpr bool operator<(const iterator& other) const noexcept 
        { return ptr_ < other.ptr_; }
        [[nodiscard]] constexpr bool operator<=(const iterator& other) const noexcept 
        { return ptr_ <= other.ptr_; }
        [[nodiscard]] constexpr bool operator>(const iterator& other) const noexcept 
        { return ptr_ > other.ptr_; }
        [[nodiscard]] constexpr bool operator>=(const iterator& other) const noexcept 
        { return ptr_ >= other.ptr_; }

        [[nodiscard]] constexpr pointer base() const noexcept { return ptr_; }

        // ADL-findable friend for n + it
        friend constexpr iterator operator+(difference_type n, iterator it) noexcept
        { return it + n; }
    };

    /*!
     * \brief Const iterator type for span
     *
     * \details
     * - [SWS_CORE_01918]: Const iterator type definition
     * - Prevents modification through iterator
     * - Convertible from non-const iterator
     */
    class const_iterator {
    public:
        using iterator_category = std::random_access_iterator_tag;
        using value_type        = std::remove_cv_t<element_type>;
        using difference_type   = std::ptrdiff_t;
        using pointer          = const element_type*;
        using reference        = const element_type&;
        #if __cplusplus >= 202002L
        using iterator_concept = std::contiguous_iterator_tag;         
        #endif
        
    private:
        pointer ptr_;

    public:
        constexpr const_iterator() noexcept : ptr_(nullptr) {}
        constexpr explicit const_iterator(pointer ptr) noexcept : ptr_(ptr) {}
        constexpr const_iterator(const iterator& it) noexcept : ptr_(it.base()) {}

        [[nodiscard]] constexpr reference operator*() const noexcept { return *ptr_; }
        [[nodiscard]] constexpr pointer operator->() const noexcept { return ptr_; }
        [[nodiscard]] constexpr reference operator[](difference_type n) const noexcept { return ptr_[n]; }

        constexpr const_iterator& operator++() noexcept { ++ptr_; return *this; }
        constexpr const_iterator operator++(int) noexcept { const_iterator tmp = *this; ++ptr_; return tmp; }
        constexpr const_iterator& operator--() noexcept { --ptr_; return *this; }
        constexpr const_iterator operator--(int) noexcept { const_iterator tmp = *this; --ptr_; return tmp; }

        constexpr const_iterator& operator+=(difference_type n) noexcept { ptr_ += n; return *this; }
        constexpr const_iterator& operator-=(difference_type n) noexcept { ptr_ -= n; return *this; }

        [[nodiscard]] constexpr const_iterator operator+(difference_type n) const noexcept 
        { return const_iterator(ptr_ + n); }
        [[nodiscard]] constexpr const_iterator operator-(difference_type n) const noexcept 
        { return const_iterator(ptr_ - n); }
        [[nodiscard]] constexpr difference_type operator-(const const_iterator& other) const noexcept 
        { return ptr_ - other.ptr_; }

        [[nodiscard]] constexpr bool operator==(const const_iterator& other) const noexcept 
        { return ptr_ == other.ptr_; }
        [[nodiscard]] constexpr bool operator!=(const const_iterator& other) const noexcept 
        { return ptr_ != other.ptr_; }
        [[nodiscard]] constexpr bool operator<(const const_iterator& other) const noexcept 
        { return ptr_ < other.ptr_; }
        [[nodiscard]] constexpr bool operator<=(const const_iterator& other) const noexcept 
        { return ptr_ <= other.ptr_; }
        [[nodiscard]] constexpr bool operator>(const const_iterator& other) const noexcept 
        { return ptr_ > other.ptr_; }
        [[nodiscard]] constexpr bool operator>=(const const_iterator& other) const noexcept 
        { return ptr_ >= other.ptr_; }

        [[nodiscard]] constexpr pointer base() const noexcept { return ptr_; }
        
        // ADL-findable friend for n + it
        friend constexpr const_iterator operator+(difference_type n, const const_iterator& it) noexcept
        { return it + n; }
    };

    /*!
     * \brief Get iterator to beginning
     *
     * \return Iterator to first element
     *
     * \details
     * - [SWS_CORE_01968]: Begin iterator
     * - Points to first element or equals end() if empty
     * - No validation needed
     */
    [[nodiscard]] constexpr iterator begin() const noexcept { return iterator(data_); }

    /*!
     * \brief Get iterator to end
     *
     * \return Iterator one past last element
     *
     * \details
     * - [SWS_CORE_01969]: End iterator
     * - Points one past last element
     * - Valid for comparison but not dereference
     */
    [[nodiscard]] constexpr iterator end() const noexcept { return iterator(data_ + size()); }

    [[nodiscard]] constexpr const_iterator cbegin() const noexcept { return const_iterator(data_); }
    [[nodiscard]] constexpr const_iterator cend() const noexcept { return const_iterator(data_ + size()); }

    [[nodiscard]] constexpr reverse_iterator rbegin() const noexcept { return reverse_iterator(end()); }
    [[nodiscard]] constexpr reverse_iterator rend() const noexcept { return reverse_iterator(begin()); }
    [[nodiscard]] constexpr const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(cend()); }
    [[nodiscard]] constexpr const_reverse_iterator crend() const noexcept { return const_reverse_iterator(cbegin()); }

    // -----------------------------------------------------------------------------------
    // C++26 ENHANCED FEATURES
    // -----------------------------------------------------------------------------------
    
    /*!
     * \brief Check if span contains a value
     *
     * \param value Value to search for
     * \return true if value is found
     *
     * \details
     * - C++26 enhancement for convenience
     * - Linear search complexity O(n)
     * - Constexpr-compatible for C++17
     * - No validation needed (safe operation)
     */
    [[nodiscard]] constexpr bool contains(const element_type& value) const noexcept
    {
        return ara::core::find(begin(), end(), value) != end();
    }

    /*!
     * \brief Check if span starts with a prefix
     *
     * \tparam U Element type of prefix span
     * \tparam N Extent of prefix span
     * \param prefix Span to check as prefix
     * \return true if this span starts with prefix
     *
     * \details
     * - C++26 enhancement
     * - Efficient prefix checking O(min(n, m))
     * - Fully constexpr in C++17
     * - Returns false if prefix is larger
     */
    template<typename U, std::size_t N>
    [[nodiscard]] constexpr bool starts_with(Span<U, N> prefix) const noexcept
    {
        if (size() < prefix.size()) {
            return false;
        }
        return ara::core::equal(prefix.begin(), prefix.end(), begin());
    }

    /*!
     * \brief Check if span ends with a suffix
     *
     * \tparam U Element type of suffix span
     * \tparam N Extent of suffix span
     * \param suffix Span to check as suffix
     * \return true if this span ends with suffix
     *
     * \details
     * - C++26 enhancement
     * - Efficient suffix checking O(min(n, m))
     * - Fully constexpr in C++17
     * - Returns false if suffix is larger
     */
    template<typename U, std::size_t N>
    [[nodiscard]] constexpr bool ends_with(Span<U, N> suffix) const noexcept
    {
        if (size() < suffix.size()) {
            return false;
        }
        return ara::core::equal(suffix.begin(), suffix.end(),
                                       end() - static_cast<difference_type>(suffix.size()));
    }

    /*!
     * \brief Split span at first occurrence of delimiter
     *
     * \param delimiter Value to split on
     * \return Pair of spans (before delimiter, after delimiter)
     *
     * \details
     * - C++26 enhancement
     * - Returns empty second span if delimiter not found
     * - Delimiter itself is excluded from both spans
     * - Fully constexpr in C++17
     */
    [[nodiscard]] constexpr auto split(const element_type& delimiter) const noexcept
        -> std::pair<Span<element_type, dynamic_extent>, Span<element_type, dynamic_extent>>
    {
        auto it = ara::core::find(begin(), end(), delimiter);
        if (it == end()) {
            return {*this, Span<element_type, dynamic_extent>()};
        }
        return {
            Span<element_type, dynamic_extent>(data_,
                                   static_cast<size_type>(it - begin())),
            Span<element_type, dynamic_extent>(it.base() + 1,
                                   static_cast<size_type>(end() - it - 1))
        };
    }

private:
    // -----------------------------------------------------------------------------------
    // VIOLATION HANDLERS
    // -----------------------------------------------------------------------------------
    
    /*!
     * \brief Trigger size mismatch violation
     *
     * \param location Source location info
     * \param actual Actual size provided
     * \param expected Expected size (from static extent)
     */
    [[noreturn]] static void TriggerSizeViolation(
        std::string_view location,
        std::size_t actual,
        std::size_t expected) noexcept
    {
        auto& handler = ara::core::internal::ViolationHandler::Instance();
        handler.TriggerSpanSizeViolation(
            ara::core::internal::ViolationHandler::SpanKey{},
            location,
            actual,
            expected);
    }

    /*!
     * \brief Trigger null pointer violation
     *
     * \param location Source location info
     */
    [[noreturn]] static void TriggerNullPointerViolation(std::string_view location) noexcept
    {
        auto& handler = ara::core::internal::ViolationHandler::Instance();
        handler.TriggerSpanNullPointerViolation(
            ara::core::internal::ViolationHandler::SpanKey{},
            location);
    }

    /*!
     * \brief Trigger range violation (invalid iterator order)
     *
     * \param location Source location info
     */
    [[noreturn]] static void TriggerRangeViolation(std::string_view location) noexcept
    {
        auto& handler = ara::core::internal::ViolationHandler::Instance();
        handler.TriggerSpanRangeViolation(
            ara::core::internal::ViolationHandler::SpanKey{},
            location);
    }

    /*!
     * \brief Trigger bounds violation (out-of-bounds access)
     *
     * \param location Source location info
     * \param index Requested index
     * \param size Current span size
     */
    [[noreturn]] static void TriggerBoundsViolation(
        std::string_view location,
        std::size_t index,
        std::size_t size) noexcept
    {
        auto& handler = ara::core::internal::ViolationHandler::Instance();
        handler.TriggerSpanBoundsViolation(
            ara::core::internal::ViolationHandler::SpanKey{},
            location,
            index,
            size);
    }

    /*!
     * \brief Trigger empty access violation
     *
     * \param location Source location info
     * \param operation Operation attempted (e.g., "front", "back")
     */
    [[noreturn]] static void TriggerEmptyAccessViolation(
        std::string_view location,
        std::string_view operation) noexcept
    {
        auto& handler = ara::core::internal::ViolationHandler::Instance();
        handler.TriggerSpanEmptyAccessViolation(
            ara::core::internal::ViolationHandler::SpanKey{},
            location,
            operation);
    }

    /*!
     * \brief Trigger subspan violation
     *
     * \param location Source location info
     * \param operation Subspan operation (e.g., "first", "last", "subspan")
     * \param requested Requested size/offset
     * \param available Available size
     * \param is_offset true if the value is an offset, false if it's a count
     */
    [[noreturn]] static void TriggerSubspanViolation(
        std::string_view location,
        std::string_view operation,
        std::size_t requested,
        std::size_t available,
        bool is_offset = false) noexcept
    {
        auto& handler = ara::core::internal::ViolationHandler::Instance();
        handler.TriggerSpanSubspanViolation(
            ara::core::internal::ViolationHandler::SpanKey{},
            location,
            operation,
            requested,
            available,
            is_offset);
    }
};

/**********************************************************************************************************************
 *  CLASS TEMPLATE ARGUMENT DEDUCTION GUIDES  [SWS_CORE_01953]
 **********************************************************************************************************************/

// -----------------------------------------------------------------------------------
// POINTER-BASED CONSTRUCTORS (matching your implementation)
// -----------------------------------------------------------------------------------

// 1) Pointer + count with InputWithLocation
template<typename T>
Span(T*, const ara::core::internal::InputWithLocation<std::size_t>&) 
    -> Span<T, dynamic_extent>;

// 2) Two pointers with InputWithLocation  
template<typename T>
Span(T*, const ara::core::internal::InputWithLocation<T*>&) 
    -> Span<T, dynamic_extent>;

// -----------------------------------------------------------------------------------
// ARRAY CONSTRUCTORS
// -----------------------------------------------------------------------------------

// 3) C-style arrays
template<typename T, std::size_t N>
Span(T (&)[N]) -> Span<T, N>;

// 4) std::array (non-const)
template<typename T, std::size_t N>
Span(std::array<T, N>&) -> Span<T, N>;

// 5) std::array (const)
template<typename T, std::size_t N>
Span(const std::array<T, N>&) -> Span<const T, N>;

// 6) ara::core::Array (non-const)
template<typename T, std::size_t N>
Span(ara::core::Array<T, N>&) -> Span<T, N>;

// 7) ara::core::Array (const)
template<typename T, std::size_t N>
Span(const ara::core::Array<T, N>&) -> Span<const T, N>;

// -----------------------------------------------------------------------------------
// ITERATOR CONSTRUCTORS
// -----------------------------------------------------------------------------------

// 8) Iterator pair with optional InputWithLocation (defaulted parameter)
template<typename It, typename End,
         typename = std::enable_if_t<
             !(std::is_same_v<std::decay_t<It>,  typename Span<It,0>::pointer > &&
               std::is_same_v<std::decay_t<End>, typename Span<End,0>::pointer >) &&
             !std::is_convertible_v<End, typename Span<End,0>::size_type> &&
             detail::is_iterator_v<It> &&
             detail::is_iterator_v<End>>>
Span(It, End, const ara::core::internal::InputWithLocation<std::uint8_t>& = 
              ara::core::internal::make_input_with_location<std::uint8_t>(0))
    -> Span<typename std::iterator_traits<It>::value_type, dynamic_extent>;

// -----------------------------------------------------------------------------------
// CONTAINER CONSTRUCTORS  
// -----------------------------------------------------------------------------------

// 9) Container (non-const) with optional InputWithLocation
template<typename Container,
         typename = std::enable_if_t<
             !detail::is_span_v<std::decay_t<Container>> &&
             !detail::is_std_array_v<std::decay_t<Container>> &&
             !detail::is_ara_array_v<std::decay_t<Container>> &&
             !detail::is_c_array_v<std::decay_t<Container>> &&
             !detail::is_contiguous_range_v<Container> &&
             !std::is_const_v<std::remove_reference_t<Container>> &&
             detail::is_contiguous_container_v<Container>>>
Span(Container&, const ara::core::internal::InputWithLocation<std::uint8_t>& =
                 ara::core::internal::make_input_with_location<std::uint8_t>(0))
    -> Span<std::remove_pointer_t<decltype(std::data(std::declval<Container&>()))>, 
            dynamic_extent>;

// 10) Container (const) with optional InputWithLocation
template<typename Container,
         typename = std::enable_if_t<
             !detail::is_span_v<std::decay_t<Container>> &&
             !detail::is_std_array_v<std::decay_t<Container>> &&
             !detail::is_ara_array_v<std::decay_t<Container>> &&
             !detail::is_c_array_v<std::decay_t<Container>> &&
             !detail::is_contiguous_range_v<Container> &&
             detail::is_contiguous_container_v<const Container>>>
Span(const Container&, const ara::core::internal::InputWithLocation<std::uint8_t>& =
                       ara::core::internal::make_input_with_location<std::uint8_t>(0))
    -> Span<const std::remove_pointer_t<decltype(std::data(std::declval<const Container&>()))>,
            dynamic_extent>;

// -----------------------------------------------------------------------------------
// RANGE CONSTRUCTORS
// -----------------------------------------------------------------------------------

// 11) Range with optional InputWithLocation
template<typename Range,
         typename = std::enable_if_t<
             !detail::is_span_v<std::decay_t<Range>> &&
             !detail::is_std_array_v<std::decay_t<Range>> &&
             !detail::is_ara_array_v<std::decay_t<Range>> &&
             !detail::is_c_array_v<std::decay_t<Range>> &&
             !detail::is_contiguous_container_v<std::decay_t<Range>> &&
             detail::is_contiguous_range_v<std::decay_t<Range>>>>
Span(Range&&, const ara::core::internal::InputWithLocation<std::uint8_t>& =
              ara::core::internal::make_input_with_location<std::uint8_t>(0))
    -> Span<std::remove_pointer_t<decltype(std::data(std::declval<Range&>()))>, 
            dynamic_extent>;

// -----------------------------------------------------------------------------------
// SPAN CONVERSION CONSTRUCTORS
// -----------------------------------------------------------------------------------

// 12) Converting constructor with optional InputWithLocation
template<typename U, std::size_t N>
Span(const Span<U, N>&, const ara::core::internal::InputWithLocation<std::uint8_t>& =
                        ara::core::internal::make_input_with_location<std::uint8_t>(0))
    -> Span<U, N>;

// -----------------------------------------------------------------------------------
// C++26 FEATURES
// -----------------------------------------------------------------------------------

// 13) std::initializer_list with optional InputWithLocation
template<typename T>
Span(std::initializer_list<T>, const ara::core::internal::InputWithLocation<std::uint8_t>& =
                                ara::core::internal::make_input_with_location<std::uint8_t>(0))
    -> Span<const T, dynamic_extent>;

/**********************************************************************************************************************
 *  NON-MEMBER FUNCTIONS [SWS_CORE_01980-01994]
 *********************************************************************************************************************/

/*!
 * \brief Create span from pointer and size
 *
 * \tparam ElementType Type of elements
 * \param ptr Pointer to first element
 * \param count Number of elements
 * \return Dynamic extent span
 *
 * \details
 * - [SWS_CORE_01990]: Factory function for span creation
 * - Alternative to constructor for type deduction
 * - Validates null pointer with non-zero count
 */
template<typename ElementType>
[[nodiscard]] constexpr auto MakeSpan(ElementType* ptr, std::size_t count) noexcept
    -> Span<ElementType>
{
    return Span<ElementType>(ptr, count);
}

/*!
 * \brief Create span from pointer range
 *
 * \tparam ElementType Type of elements
 * \param firstElem Pointer to first element
 * \param lastElem Pointer past last element
 * \return Dynamic extent span
 *
 * \details
 * - [SWS_CORE_01991]: Factory function for iterator pair
 * - Validates range order (first <= last)
 */
template<typename ElementType>
[[nodiscard]] constexpr auto MakeSpan(ElementType* firstElem, ElementType* lastElem) noexcept
    -> Span<ElementType>
{
    return Span<ElementType>(firstElem, lastElem);
}

/*!
 * \brief Create span from C-style array
 *
 * \tparam ElementType Type of elements
 * \tparam N Size of array
 * \param arr Array to view
 * \return Static extent span
 *
 * \details
 * - [SWS_CORE_01992]: Factory function for arrays
 * - Deduces static extent from array size
 * - Zero overhead abstraction
 */
template<typename ElementType, std::size_t N>
[[nodiscard]] constexpr auto MakeSpan(ElementType (&arr)[N]) noexcept
    -> Span<ElementType, N>
{
    return Span<ElementType, N>(arr);
}

/*!
 * \brief Create span from generic container
 *
 * \tparam Container Container type with data() and size()
 * \param cont Container to view
 * \return Dynamic extent span
 *
 * \details
 * - [SWS_CORE_01993]: Generic container factory
 * - Works with any contiguous container including ara::core types
 * - Type deduction for element type
 */
template<typename Container,
         typename = std::enable_if_t<
             detail::has_data_and_size_v<Container> &&
             !detail::is_span_v<std::decay_t<Container>>>>
[[nodiscard]] constexpr auto MakeSpan(Container& cont) noexcept
    -> Span<typename std::remove_pointer_t<
        decltype(std::declval<Container&>().data())>>
{
    using ElementType = std::remove_pointer_t<decltype(cont.data())>;
    return Span<ElementType>(cont);
}

/*!
 * \brief Create const span from generic container
 *
 * \tparam Container Container type with data() and size()
 * \param cont Const container to view
 * \return Dynamic extent span with const elements
 */
template<typename Container,
         typename = std::enable_if_t<
             detail::has_data_and_size_v<const Container> &&
             !detail::is_span_v<std::decay_t<Container>>>>
[[nodiscard]] constexpr auto MakeSpan(const Container& cont) noexcept
    -> Span<const typename std::remove_pointer_t<
        decltype(std::declval<const Container&>().data())>>
{
    using ElementType = const std::remove_pointer_t<decltype(cont.data())>;
    return Span<ElementType>(cont);
}

/*!
 * \brief Get span of object representation as bytes
 *
 * \tparam ElementType Type of elements
 * \tparam Extent Static extent of span
 * \param s Span to reinterpret
 * \return Read-only span of bytes
 *
 * \details
 * - [SWS_CORE_01980]: Byte representation access
 * - Safe reinterpretation as byte sequence
 * - Preserves const-correctness
 * - Size calculated at compile time for static spans
 */
template<typename ElementType, std::size_t Extent>
[[nodiscard]] constexpr auto as_bytes(Span<ElementType, Extent> s) noexcept
    -> Span<const ara::core::Byte, 
            Extent == dynamic_extent ? dynamic_extent : sizeof(ElementType) * Extent>
{
    return Span<const ara::core::Byte, 
                Extent == dynamic_extent ? dynamic_extent : sizeof(ElementType) * Extent>(
        reinterpret_cast<const ara::core::Byte*>(s.data()), s.size_bytes());
}

/*!
 * \brief Get span of object representation as writable bytes
 *
 * \tparam ElementType Type of elements (must not be const)
 * \tparam Extent Static extent of span
 * \param s Span to reinterpret
 * \return Writable span of bytes
 *
 * \details
 * - [SWS_CORE_01981]: Writable byte representation
 * - Only available for non-const element types
 * - Allows byte-level manipulation
 * - SFINAE-disabled for const elements
 */
template<typename ElementType, std::size_t Extent,
         typename = std::enable_if_t<!std::is_const_v<ElementType>>>
[[nodiscard]] constexpr auto as_writable_bytes(Span<ElementType, Extent> s) noexcept
    -> Span<ara::core::Byte, 
            Extent == dynamic_extent ? dynamic_extent : sizeof(ElementType) * Extent>
{
    return Span<ara::core::Byte, 
                Extent == dynamic_extent ? dynamic_extent : sizeof(ElementType) * Extent>(
        reinterpret_cast<ara::core::Byte*>(s.data()), s.size_bytes());
}

/**********************************************************************************************************************
 *  COMPARISON OPERATORS
 *********************************************************************************************************************/

/*!
 * \brief Equality comparison
 *
 * \tparam T1 Element type of first span
 * \tparam E1 Extent of first span
 * \tparam T2 Element type of second span
 * \tparam E2 Extent of second span
 * \param lhs First span
 * \param rhs Second span
 * \return true if spans are equal
 *
 * \details
 * - Spans are equal if they have same size and elements
 * - Uses constexpr-compatible comparison for C++17
 * - Element types must be equality-comparable
 */
template<typename T1, std::size_t E1, typename T2, std::size_t E2>
[[nodiscard]] constexpr bool operator==(Span<T1, E1> lhs, Span<T2, E2> rhs) noexcept
{
    return lhs.size() == rhs.size() && 
           ara::core::equal(lhs.begin(), lhs.end(), rhs.begin());
}

template<typename T1, std::size_t E1, typename T2, std::size_t E2>
[[nodiscard]] constexpr bool operator!=(Span<T1, E1> lhs, Span<T2, E2> rhs) noexcept
{
    return !(lhs == rhs);
}

template<typename T1, std::size_t E1, typename T2, std::size_t E2>
[[nodiscard]] constexpr bool operator<(Span<T1, E1> lhs, Span<T2, E2> rhs) noexcept
{
    return ara::core::lexicographical_compare(lhs.begin(), lhs.end(), 
                                                     rhs.begin(), rhs.end());
}

template<typename T1, std::size_t E1, typename T2, std::size_t E2>
[[nodiscard]] constexpr bool operator<=(Span<T1, E1> lhs, Span<T2, E2> rhs) noexcept
{
    return !(rhs < lhs);
}

template<typename T1, std::size_t E1, typename T2, std::size_t E2>
[[nodiscard]] constexpr bool operator>(Span<T1, E1> lhs, Span<T2, E2> rhs) noexcept
{
    return rhs < lhs;
}

template<typename T1, std::size_t E1, typename T2, std::size_t E2>
[[nodiscard]] constexpr bool operator>=(Span<T1, E1> lhs, Span<T2, E2> rhs) noexcept
{
    return !(lhs < rhs);
}

/**********************************************************************************************************************
 *  RANGES SUPPORT NAMESPACE
 *********************************************************************************************************************/

/*!
 * \brief C++17-compatible ranges support for span
 *
 * \details Provides range-like operations without requiring C++20 ranges library
 */
namespace ranges {

/*!
 * \brief Check if type is a span
 */
template<typename T>
struct is_span : std::false_type {};

template<typename T, std::size_t E>
struct is_span<Span<T, E>> : std::true_type {};

template<typename T>
inline constexpr bool is_span_v = is_span<T>::value;

/*!
 * \brief Create a range view from span
 */
template<typename T, std::size_t E>
[[nodiscard]] constexpr auto view(Span<T, E> s) noexcept
{
    return s;  // Span is already a view
}

/*!
 * \brief Transform view for spans
 */
template<typename T, std::size_t E, typename F>
class span_transform_view {
public:
    using span_type = Span<T, E>;
    using value_type = std::invoke_result_t<F, T&>;
    
private:
    span_type span_;
    F func_;
    
public:
    class iterator {
    private:
        typename span_type::iterator it_;
        const F* func_;
        
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type = std::invoke_result_t<F, T&>;
        using difference_type = std::ptrdiff_t;
        using pointer = void;
        using reference = value_type;
        
        constexpr iterator(typename span_type::iterator it, const F* f) noexcept
            : it_(it), func_(f) {}
        
        [[nodiscard]] constexpr value_type operator*() const noexcept { return (*func_)(*it_); }
        constexpr iterator& operator++() noexcept { ++it_; return *this; }
        constexpr iterator operator++(int) noexcept { iterator tmp = *this; ++*this; return tmp; }
        
        [[nodiscard]] constexpr bool operator==(const iterator& other) const noexcept 
        { return it_ == other.it_; }
        [[nodiscard]] constexpr bool operator!=(const iterator& other) const noexcept 
        { return !(*this == other); }
    };
    
    constexpr span_transform_view(span_type s, F f) noexcept 
        : span_(s), func_(std::move(f)) {}

    [[nodiscard]] constexpr iterator begin() const noexcept 
    { return iterator(span_.begin(), &func_); }
    [[nodiscard]] constexpr iterator end() const noexcept 
    { return iterator(span_.end(), &func_); }
    [[nodiscard]] constexpr auto size() const noexcept { return span_.size(); }
    [[nodiscard]] constexpr bool empty() const noexcept { return span_.empty(); }
};

/*!
 * \brief Create transform view
 */
template<typename T, std::size_t E, typename F>
[[nodiscard]] constexpr auto transform(Span<T, E> s, F&& f) noexcept {
    return span_transform_view<T, E, std::decay_t<F>>(s, std::forward<F>(f));
}

/*!
 * \brief Filter view for spans
 */
template<typename T, std::size_t E, typename Pred>
class span_filter_view {
public:
    using span_type = Span<T, E>;
    
private:
    span_type span_;
    Pred pred_;
    
public:
    class iterator {
    private:
        typename span_type::iterator current_;
        typename span_type::iterator end_;
        const Pred* pred_;
        
        void advance_to_next() noexcept {
            while (current_ != end_ && !(*pred_)(*current_)) {
                ++current_;
            }
        }
        
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = T*;
        using reference = T&;
        
        constexpr iterator(typename span_type::iterator curr,
                          typename span_type::iterator end,
                          const Pred* p) noexcept
            : current_(curr), end_(end), pred_(p) {
            advance_to_next();
        }

        [[nodiscard]] constexpr reference operator*() const noexcept { return *current_; }
        [[nodiscard]] constexpr pointer operator->() const noexcept { return current_.base(); }

        constexpr iterator& operator++() noexcept {
            ++current_;
            advance_to_next();
            return *this;
        }

        constexpr iterator operator++(int) noexcept {
            iterator tmp = *this;
            ++*this;
            return tmp;
        }
        
        [[nodiscard]] constexpr bool operator==(const iterator& other) const noexcept {
            return current_ == other.current_;
        }
        
        [[nodiscard]] constexpr bool operator!=(const iterator& other) const noexcept {
            return !(*this == other);
        }
    };

    constexpr span_filter_view(span_type s, Pred p) noexcept 
        : span_(s), pred_(std::move(p)) {}

    [[nodiscard]] constexpr iterator begin() const noexcept {
        return iterator(span_.begin(), span_.end(), &pred_);
    }

    [[nodiscard]] constexpr iterator end() const noexcept {
        return iterator(span_.end(), span_.end(), &pred_);
    }
};

/*!
 * \brief Create filter view
 */
template<typename T, std::size_t E, typename Pred>
[[nodiscard]] constexpr auto filter(Span<T, E> s, Pred&& p) noexcept {
    return span_filter_view<T, E, std::decay_t<Pred>>(s, std::forward<Pred>(p));
}

/*!
 * \brief Take first n elements
 */
template<typename T, std::size_t E>
[[nodiscard]] constexpr auto take(Span<T, E> s, std::size_t n) noexcept {
    return s.first((std::min)(n, s.size()));
}

/*!
 * \brief Drop first n elements
 */
template<typename T, std::size_t E>
[[nodiscard]] constexpr auto drop(Span<T, E> s, std::size_t n) noexcept {
    return s.subspan((std::min)(n, s.size()));
}

/*!
 * \brief Take elements while predicate is true
 */
template<typename T, std::size_t E, typename Pred>
[[nodiscard]] constexpr auto take_while(Span<T, E> s, Pred&& pred) noexcept {
    auto it = s.begin();
    auto end = s.end();
    while (it != end && pred(*it)) {
        ++it;
    }
    return s.first(static_cast<typename Span<T, E>::size_type>(it - s.begin()));
}

/*!
 * \brief Drop elements while predicate is true
 */
template<typename T, std::size_t E, typename Pred>
[[nodiscard]] constexpr auto drop_while(Span<T, E> s, Pred&& pred) noexcept {
    auto it = s.begin();
    auto end = s.end();
    while (it != end && pred(*it)) {
        ++it;
    }
    return s.subspan(static_cast<typename Span<T, E>::size_type>(it - s.begin()));
}

/*!
 * \brief Create span from range
 */
template<typename Range,
         typename = std::enable_if_t<
             detail::is_contiguous_range_v<std::decay_t<Range>> ||
             detail::is_contiguous_container_v<std::decay_t<Range>>>>
[[nodiscard]] constexpr auto from_range(Range&& r) noexcept
{
    return Span(std::forward<Range>(r));
}

} // namespace ranges

} // namespace core
} // namespace ara

/**********************************************************************************************************************
 *  COMPILE-TIME VERIFICATION
 *********************************************************************************************************************/
namespace ara {
namespace core {

namespace span_test {

// Test types
using TestSpan = Span<int, 5>;
using DynamicTestSpan = Span<int>;
using ConstTestSpan = Span<const int, 5>;

// Memory layout verification
static_assert(sizeof(TestSpan) == sizeof(void*), 
    "Static extent span must be pointer-sized only");

static_assert(sizeof(DynamicTestSpan) == 2 * sizeof(void*), 
    "Dynamic extent span must store pointer and size");

// Type trait verification
static_assert(std::is_trivially_copyable_v<TestSpan>,
    "Span must be trivially copyable");

static_assert(std::is_trivially_destructible_v<TestSpan>,
    "Span must be trivially destructible");

static_assert(std::is_nothrow_move_constructible_v<TestSpan>,
    "Span must be nothrow move constructible");

// Iterator verification
static_assert(std::is_same_v<
    typename std::iterator_traits<TestSpan::iterator>::iterator_category,
    std::random_access_iterator_tag>,
    "Span iterator must be random access");

// Conversion safety verification
static_assert(std::is_constructible_v<ConstTestSpan, TestSpan>,
    "Should be able to construct Span<const T> from Span<T>");

static_assert(!std::is_constructible_v<TestSpan, ConstTestSpan>,
    "Should not be able to construct Span<T> from Span<const T>");

// Constexpr verification
constexpr int test_data[] = {1, 2, 3, 4, 5};
constexpr Span<const int, 5> test_span(test_data);
static_assert(test_span.size() == 5, "Size must work in constexpr");
static_assert(test_span[2] == 3, "Element access must work in constexpr");
static_assert(!test_span.empty(), "Empty check must work in constexpr");
static_assert(test_span.first<2>().size() == 2, "Subspan must work in constexpr");

// C++26 feature verification (initializer list) - simplified for C++17
#if defined(__cpp_constexpr) && __cpp_constexpr >= 201603L
constexpr int init_data[] = {1, 2, 3, 4, 5};
constexpr Span<const int> dynamic_span(init_data, 5);
static_assert(dynamic_span.size() == 5, "Dynamic span construction must work");
#endif

// Range constructor verification
constexpr std::array<int, 5> test_array = {1, 2, 3, 4, 5};
constexpr Span<const int> range_span(test_array);
static_assert(range_span.size() == 5, "Range construction must work");

// C++26 enhanced features verification
static_assert(test_span.contains(3), "Contains must work in constexpr");

constexpr char txt[] = "Hello";
constexpr Span CTAD(txt);


} // span_test namespace

} // namespace core
} // namespace ara

/**********************************************************************************************************************
 *  C++20 ranges: Span is a borrowed_range
 *********************************************************************************************************************/
#if __cplusplus >= 202002L          
namespace std::ranges {
    template<class T, std::size_t E>
    inline constexpr bool enable_borrowed_range<ara::core::Span<T, E>> = true;

    template<class T, std::size_t E>
    inline constexpr bool enable_view<ara::core::Span<T, E>> = true;
}
#endif

#endif // OPEN_AA_ADAPTIVE_AUTOSAR_LIBS_INCLUDE_ARA_CORE_SPAN_H_