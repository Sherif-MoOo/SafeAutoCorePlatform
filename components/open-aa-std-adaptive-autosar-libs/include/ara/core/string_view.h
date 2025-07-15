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
 *  \file       ara/core/string_view.h
 *  \brief      Definition and implementation of the ara::core::StringView type.
 *
 *  \details    This file defines and implements the ara::core::StringView type and related string view types,
 *              providing non-owning views over contiguous sequences of characters. It delivers a lightweight,
 *              read-only alternative to std::string for string manipulation without memory allocation, designed
 *              for the OpenAA project. The implementation exceeds standard C++17 string_view functionality by
 *              providing C++26 features, enhanced safety, and optimizations for automotive applications per
 *              Adaptive AUTOSAR requirements.
 *
 *  \note       Based on the Adaptive AUTOSAR SWS (e.g., R24-11) requirements for the "StringView" type:
 *              - [SWS_CORE_03001] String view class template definition
 *              - [SWS_CORE_03011-03022] Type aliases for string view
 *              - [SWS_CORE_03031] Static member npos
 *              - [SWS_CORE_03040-03052] Constructors and assignment
 *              - [SWS_CORE_03060-03072] Element access methods
 *              - [SWS_CORE_03080-03094] Utility functions and modifiers
 *              - [SWS_CORE_03100-03130] String operations and searching
 *              - Enhanced with C++26-like features while maintaining C++17 compatibility
 *              - Comprehensive ranges support without C++20 ranges library
 *              - Zero-overhead abstraction with deterministic performance
 *********************************************************************************************************************/

#ifndef OPEN_AA_ADAPTIVE_AUTOSAR_LIBS_INCLUDE_ARA_CORE_STRING_VIEW_H_
#define OPEN_AA_ADAPTIVE_AUTOSAR_LIBS_INCLUDE_ARA_CORE_STRING_VIEW_H_

/**********************************************************************************************************************
 *  INCLUDES
 *********************************************************************************************************************/
/*!
 * \brief  Standard library headers required for string view implementation
 *
 * [SWS_CORE_03001]: cstddef for std::size_t and std::ptrdiff_t
 * [SWS_CORE_03011-03022]: type_traits for type aliases and SFINAE
 * [SWS_CORE_03060-03072]: iterator support for string view operations
 */
#include <cstddef>                                  // For std::size_t, std::ptrdiff_t
#include <cstdint>                                  // For fixed-width integer types
#include <cstring>                                  // For std::memcmp, std::strlen, char_traits
#include <cwchar>                                   // For wchar_t support
#include <string>                                   // For std::char_traits
#include <type_traits>                              // For type trait checks and SFINAE
#include <iterator>                                 // For iterator categories and traits
#include <limits>                                   // For numeric_limits
#include <utility>                                  // For std::forward, std::move
#include <algorithm>                                // For std::min, std::search
#include <memory>                                   // For std::addressof
#include <ostream>                                  // For operator<<
#include <functional>                               // For std::hash

#include "ara/core/ranges.h"                        // For ranges support in string view
#include "ara/core/internal/utility.h"              // For utility functions and traits
#include "ara/core/algorithm.h"                     // For algorithm utilities
#include "ara/core/internal/location_utils.h"       // For capturing file/line details
#include "ara/core/internal/violation_handler.h"    // To trigger violations
#include "ara/core/internal/xxh3_minimal.h"         // For XXH3 hashing support

/**********************************************************************************************************************
 *  NAMESPACE: ara::core
 *********************************************************************************************************************/
/*!
 * \brief  The ara::core namespace, containing AUTOSAR Adaptive Platform core types
 */
namespace ara {
namespace core {

/**********************************************************************************************************************
 *  CLASS: ara::core::BasicStringView
 *********************************************************************************************************************/
/*!
 * \brief  A non-owning view over a contiguous sequence of characters
 *
 * \tparam CharT Character type
 * \tparam Traits Character traits type (default: std::char_traits<CharT>)
 *
 * \details
 * - [SWS_CORE_03000]: Core string view class template definition
 * - Provides bounds-safe access to character sequences
 * - Zero-overhead abstraction with no dynamic allocation
 * - Full compatibility with std::string and C-strings
 * - Enhanced with C++26 features while maintaining C++17 compatibility
 *
 * \note Unlike std::string_view, this implementation provides additional safety features
 *       and integrations specific to AUTOSAR Adaptive Platform requirements.
 * 
 * \note Thread Safety: All member functions are thread-safe for const operations.
 *       Modifying operations (remove_prefix, remove_suffix, swap) require external synchronization.
 */
template<typename CharT, typename Traits = typename detail::default_traits<CharT>::type>
class BasicStringView {
public:
    // -----------------------------------------------------------------------------------
    // TYPE ALIASES [SWS_CORE_03011-03022]
    // -----------------------------------------------------------------------------------
    using traits_type            = Traits;                                 /*!< [SWS_CORE_03011] Character traits type */
    using value_type             = CharT;                                  /*!< [SWS_CORE_03012] Character type */
    using pointer                = CharT*;                                 /*!< [SWS_CORE_03013] Pointer to character */
    using const_pointer          = const CharT*;                           /*!< [SWS_CORE_03014] Const pointer to character */
    using reference              = CharT&;                                 /*!< [SWS_CORE_03015] Reference to character */
    using const_reference        = const CharT&;                           /*!< [SWS_CORE_03016] Const reference to character */
    using const_iterator         = const_pointer;                           /*!< [SWS_CORE_03017] Const iterator type */
    using iterator               = const_iterator;                         /*!< [SWS_CORE_03018] Iterator type (same as const) */
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;  /*!< [SWS_CORE_03019] Const reverse iterator */
    using reverse_iterator       = const_reverse_iterator;                 /*!< [SWS_CORE_03020] Reverse iterator */
    using size_type              = std::size_t;                            /*!< [SWS_CORE_03021] Size type */
    using difference_type        = std::ptrdiff_t;                         /*!< [SWS_CORE_03022] Difference type */

    // -----------------------------------------------------------------------------------
    // STATIC MEMBERS [SWS_CORE_03031]
    // -----------------------------------------------------------------------------------
    static constexpr size_type npos = std::numeric_limits<size_type>::max(); /*!< [SWS_CORE_03031] Invalid position indicator */

    // -----------------------------------------------------------------------------------
    // CONSTRUCTORS [SWS_CORE_03040-03050]
    // -----------------------------------------------------------------------------------
    
    /*!
     * \brief Default constructor - creates empty view
     *
     * \details
     * - [SWS_CORE_03041]: Default construction with empty view
     * - Results in data() == nullptr and size() == 0
     */
    constexpr BasicStringView() noexcept
        : data_{nullptr}, size_{0} {}

    /*!
     * \brief Copy constructor
     *
     * \details
     * - [SWS_CORE_03042]: Copy construction
     * - Trivially copyable for performance
     */
    constexpr BasicStringView(const BasicStringView&) noexcept = default;

    /*!
     * \brief Construct from pointer and count
     *
     * \param s Pointer to character sequence
     * \param count Number of characters (wrapped with location info)
     *
     * \details
     * - [SWS_CORE_03043]: Pointer and count constructor
     * - Validates null pointer with non-zero count
     * - Behavior is undefined if [s, s + count) is not valid
     * 
     * \note Using InputWithLocation for enhanced debugging
     */
    constexpr BasicStringView(const_pointer s, 
                             const ara::core::internal::InputWithLocation<size_type>& count) noexcept
        : data_{s}, size_{count.input()}
    {
        // Validate null pointer with non-zero count
        if (detail::unlikely(s == nullptr && count.input() > 0)) {
            if (!detail::is_constant_evaluated()) {
                TriggerNullptrViolation(count.info());
            } else {
                constexpr unsigned char _null_pointer_violation[1] = {};
                [[maybe_unused]] const auto verify{_null_pointer_violation[1]};
            }
        }
    }

    /*!
     * \brief Construct from C-string
     *
     * \param s Null-terminated character string
     *
     * \details
     * - [SWS_CORE_03044]: C-string constructor
     * - Length determined by Traits::length()
     * - C++26 feature: nullptr check for safety
     */
    template< class Ptr,
              std::enable_if_t<detail::is_real_pointer_v<Ptr>, int> = 0 >
    constexpr BasicStringView(const Ptr s,
                              const ara::core::internal::InputWithLocation<std::uint8_t> loc =
                                    ara::core::internal::make_input_with_location<std::uint8_t>(0)) noexcept
        : data_{s}, size_{s ? Traits::length(s) : 0}
    {
        if (detail::unlikely(!s)) {
            if (!detail::is_constant_evaluated()) {
                TriggerNullptrViolation(loc.info());
            } else {
                constexpr unsigned char _null_pointer_violation[1] = {};
                [[maybe_unused]] const auto verify{_null_pointer_violation[1]};
            }
        }
    }

    /*!
     * \brief C++26 feature: Delete nullptr constructor for safety
     *
     * \details
     * - [SWS_CORE_03045]: Nullptr safety
     * - Prevents dangerous null pointer construction
     */
    BasicStringView(std::nullptr_t) noexcept = delete;

    /*!
     * \brief *Hard-error* constructor from r-value std::basic_string
     *
     *        Triggers a friendly static_assert instead of silent UB.
     *        Example that will now fail:
     *            auto sv = ara::core::StringView{std::string("tmp")};
     */
    template<typename Traits2, typename Alloc2>
    constexpr BasicStringView(std::basic_string<CharT, Traits2, Alloc2>&&) noexcept {
        static_assert(detail::dependent_false_v<CharT>,
            "\n[ERROR] ara::core::BasicStringView: "
            "Constructing a view from a temporary std::basic_string would dangle.\n");
    }

    /*!
     * \brief construct from r-value container
     * \tparam Container Contiguous container type
     * \param cont Container to view
     * \details
     */
    template<
        typename Container,
        typename = std::enable_if_t<
            detail::is_contiguous_container_v<Container> &&
            !detail::is_contiguous_range_v<Container> &&
            !detail::is_basic_string_v<std::decay_t<Container>> &&
            !std::is_same_v< BasicStringView,
                        detail::remove_cvref_t<Container>>
        >
    >
    constexpr BasicStringView(const Container&&) noexcept
    {
        static_assert(detail::dependent_false_v<CharT>,
            "\n[ERROR] ara::core::BasicStringView: "
            "Cannot construct a view from an r-value container. Use lvalue containers only.\n");
    }

    /*!
     * \brief Construct from lvalue string
     *
     * \tparam Traits2 Traits of the string (may differ)
     * \tparam Alloc2 Allocator type of the string
     * \param str String to view
     *
     * \details
     * - Safe construction from lvalue strings only
     * - Allows different traits/allocators
     * - Zero-cost view creation
     */
    template<typename Traits2, typename Alloc2>
    constexpr BasicStringView(const std::basic_string<CharT, Traits2, Alloc2>& str) noexcept
        : data_{str.data()}, size_{str.size()} {}


    /*!
     * \brief Construct from C-style array
     *
     * \tparam N Size of array (including null terminator)
     * \param arr Array to view
     *
     * \details
     * - Automatically deduces size
     * - Excludes null terminator from view
     * - Compile-time size calculation
     */
    template<size_t N>
    constexpr BasicStringView(const CharT (&arr)[N]) noexcept
        : data_{arr}, size_{N > 0 ? N - 1 : 0}  // Exclude null terminator
    {
        static_assert(N > 0,
            "\n[ERROR] ara::core::BasicStringView: "
            "Cannot create string view from zero-sized array.\n");
    }

    /*!
     * \brief Construct from ara::core::Array
     *
     * \tparam N Size of array
     * \param arr Array to view
     * \param loc Source location for error reporting
     *
     * \details
     * - Seamless integration with ara::core types
     * - Validates against null terminators in data
     * - Compile-time size known
     */
    template<size_t N>
    constexpr BasicStringView(const ara::core::Array<CharT, N>& arr) noexcept
        : data_{arr.data()},
          size_{strlen(arr.data())}  // Use strlen to find actual length
    {

    }

    /*!
     * \brief Construct from std::array
     *
     * \tparam N Size of array
     * \param arr array to view
     * \param loc Source location for error reporting
     *
     * \details
     * - Seamless integration with ara::core types
     * - Validates against null terminators in data
     * - Compile-time size known
     */
    template<size_t N>
    constexpr BasicStringView(const std::array<CharT, N>& arr) noexcept
        : data_{arr.data()}
    {
        // Find actual string length (up to first null)
        size_type actual_len = 0;
        for (size_type i = 0; i < N; ++i) {
            if (arr[i] == CharT{}) break;
            ++actual_len;
        }
        size_ = actual_len;
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
    template<
        typename Container,
        typename = std::enable_if_t<
            !detail::is_std_array_v<std::decay_t<Container>> &&
            !detail::is_ara_array_v<std::decay_t<Container>> &&
            !detail::is_c_array_v<std::decay_t<Container>> &&
            !detail::is_contiguous_range_v<Container> &&
            !detail::is_basic_string_v<std::decay_t<Container>> &&
            !std::is_const_v<std::remove_reference_t<Container>> &&
             detail::is_contiguous_container_v<Container> &&
             detail::is_string_view_compatible_v<Container, CharT>
        >
    >     
    constexpr BasicStringView(Container& cont,
                              const ara::core::internal::InputWithLocation<std::uint8_t> loc =
                                    ara::core::internal::make_input_with_location<std::uint8_t>(0))
#ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
        noexcept(noexcept(std::data(cont)) && noexcept(std::size(cont)))
#else
        noexcept
#endif
        : data_{std::data(cont)}, size_{std::size(cont)}
    {
#ifndef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
        static_assert(noexcept(std::data(cont)) && noexcept(std::size(cont)),
                "\n[ERROR] BasicStringView constructor must be noexcept for all string-like types.\n");
#endif
        if (detail::unlikely(data_ == nullptr && size_ > 0)) {
            if (!detail::is_constant_evaluated()) {
                TriggerNullptrViolation(loc.info());
            } else {
                constexpr unsigned char _null_pointer_violation[1] = {};
                [[maybe_unused]] const auto verify{_null_pointer_violation[1]};
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
     * - Constructs span from any contiguous container with data() and size()
     * - Excludes spans, arrays, and ranges (handled by other constructors)
     */
    template <typename Container,
              typename = std::enable_if_t<
                  !detail::is_std_array_v<std::decay_t<Container>> &&
                  !detail::is_ara_array_v<std::decay_t<Container>> &&
                  !detail::is_c_array_v<std::decay_t<Container>> &&
                  !detail::is_contiguous_range_v<Container> &&
                  !detail::is_basic_string_v<std::decay_t<Container>> &&
             detail::is_contiguous_container_v<Container> &&
             detail::is_string_view_compatible_v<Container, CharT>
        >
    >   
    constexpr BasicStringView(const Container& cont,
                              const ara::core::internal::InputWithLocation<std::uint8_t> loc =
                                    ara::core::internal::make_input_with_location<std::uint8_t>(0))
#ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
        noexcept(noexcept(std::data(cont)) && noexcept(std::size(cont)))
#else
        noexcept
#endif
        : data_{std::data(cont)}, size_{std::size(cont)}
    {

#ifndef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
        static_assert(noexcept(std::data(cont)) && noexcept(std::size(cont)),
                "\n[ERROR] BasicStringView constructor must be noexcept for all string-like types.\n");
#endif
        if (detail::unlikely(data_ == nullptr && size_ > 0)) {
            if (!detail::is_constant_evaluated()) {
                TriggerNullptrViolation(loc.info());
            } else {
                constexpr unsigned char _null_pointer_violation[1] = {};
                [[maybe_unused]] const auto verify{_null_pointer_violation[1]};
            }
        }

    }

    /*!
     * \brief Construct from string-like container
     *
     * \tparam Range String-like type with data() and size()
     * \param str String-like object
     * \param loc Source location for error reporting
     *
     * \details
     * - [SWS_CORE_03046]: Range constructor
     * - Works with std::string, std::vector<CharT>, etc.
     * - SFINAE-protected for type safety
     */
    template<typename Range,
             typename = std::enable_if_t<
                 !std::is_same_v<std::decay_t<Range>, BasicStringView> &&
                 !std::is_pointer_v<std::decay_t<Range>> &&
                 !detail::is_std_array_v<std::decay_t<Range>> &&
                 !detail::is_ara_array_v<std::decay_t<Range>> &&
                 !detail::is_c_array_v<std::decay_t<Range>> &&
                 !detail::is_basic_string_v<std::remove_reference_t<Range>> &&
                 detail::is_string_view_compatible_v<Range, CharT> &&
                 std::is_lvalue_reference_v<Range&&>
        >
    >   
    constexpr BasicStringView(Range&& str,
                              const ara::core::internal::InputWithLocation<std::uint8_t> loc =
                                    ara::core::internal::make_input_with_location<std::uint8_t>(0))
#ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
        noexcept(noexcept(std::data(str)) && noexcept(std::size(str)))
#else
        noexcept
#endif
        : data_{std::data(str)}, size_{std::size(str)}
    {
#ifndef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
        static_assert(noexcept(std::data(str)) && noexcept(std::size(str)),
                "\n[ERROR] BasicStringView constructor must be noexcept for all string-like types.\n");
#endif
        
        // Validate data pointer
        if (detail::unlikely(data_ == nullptr && size_ > 0)) {
            if (!detail::is_constant_evaluated()) {
                TriggerNullptrViolation(loc.info());
            } else {
                constexpr unsigned char _null_pointer_violation[1] = {};
                [[maybe_unused]] const auto verify{_null_pointer_violation[1]};
            }
        }
    }

    /*!
     * \brief Destructor
     *
     * \details
     * - [SWS_CORE_03051]: Defaulted destructor
     * - Trivially destructible
     */
    ~BasicStringView() noexcept = default;

    // -----------------------------------------------------------------------------------
    // ASSIGNMENT [SWS_CORE_03052]
    // -----------------------------------------------------------------------------------
    
    /*!
     * \brief Copy assignment operator
     *
     * \details
     * - [SWS_CORE_03052]: Copy assignment
     * - Trivially copy assignable
     */
    constexpr BasicStringView& operator=(const BasicStringView&) noexcept = default;

    // -----------------------------------------------------------------------------------
    // ITERATOR SUPPORT [SWS_CORE_03053-03058]
    // -----------------------------------------------------------------------------------
    
    [[nodiscard]] constexpr auto begin() const noexcept -> const_iterator
    { return data_; }
    
    [[nodiscard]] constexpr auto end() const noexcept -> const_iterator
    { return data_ + size_; }
    
    [[nodiscard]] constexpr auto cbegin() const noexcept -> const_iterator
    { return begin(); }
    
    [[nodiscard]] constexpr auto cend() const noexcept -> const_iterator
    { return end(); }
    
    [[nodiscard]] constexpr auto rbegin() const noexcept -> const_reverse_iterator
    { return const_reverse_iterator(end()); }
    
    [[nodiscard]] constexpr auto rend() const noexcept -> const_reverse_iterator
    { return const_reverse_iterator(begin()); }
    
    [[nodiscard]] constexpr auto crbegin() const noexcept -> const_reverse_iterator
    { return rbegin(); }
    
    [[nodiscard]] constexpr auto crend() const noexcept -> const_reverse_iterator
    { return rend(); }

    // -----------------------------------------------------------------------------------
    // ELEMENT ACCESS [SWS_CORE_03060-03065]
    // -----------------------------------------------------------------------------------
    
    /*!
     * \brief Access character at index (unchecked in release)
     *
     * \param pos Index of character
     * \return Reference to character
     *
     * \details
     * - [SWS_CORE_03060]: Unchecked element access
     * - Zero-cost in release builds
     */
    [[nodiscard]] constexpr auto operator[](const ara::core::internal::InputWithLocation<size_type>& pos) const noexcept -> const_reference
    {   
        const size_type& I = pos.input();
        if (detail::unlikely(I >= size_)) {
            if (!detail::is_constant_evaluated()) {
                TriggerIndexViolation(pos.info(), I, size_);
            } else {
                constexpr unsigned char _bounds_check[1] = {};
                [[maybe_unused]] const auto verify{_bounds_check[1]};
            }
        }

        return data_[pos];
    }

    /*!
     * \brief Access character with bounds checking
     *
     * \param pos Index of character
     * \return Reference to character
     *
     * \details
     * - [SWS_CORE_03061]: Checked element access
     * - Always performs bounds checking
     * - Calls violation handler on out-of-bounds
     */
    [[nodiscard]] constexpr auto at(const ara::core::internal::InputWithLocation<size_type>& pos) const noexcept -> const_reference
    {
        const size_type& I = pos.input();
        if (detail::unlikely(I >= size_)) {
            if (!detail::is_constant_evaluated()) {
                TriggerBoundsViolation(pos.info(), I, size_);
            } else {
                constexpr unsigned char _bounds_check[1] = {};
                [[maybe_unused]] const auto verify{_bounds_check[1]};
            }
        }
        return data_[pos];
    }

    /*!
     * \brief Access first character
     *
     * \return Reference to first character
     *
     * \details
     * - [SWS_CORE_03062]: Front element access
     */
    [[nodiscard]] constexpr auto front(const ara::core::internal::InputWithLocation<std::uint8_t> loc =
                                    ara::core::internal::make_input_with_location<std::uint8_t>(0)) const noexcept -> const_reference
    {   

        if (detail::unlikely(empty())) {
            if (!detail::is_constant_evaluated()) {
                TriggerEmptyAccessViolation(loc.info(), "front");
            } else {
                constexpr unsigned char _empty_access_violation[1] = {};
                [[maybe_unused]] const auto verify{_empty_access_violation[1]};
            }
        }
        return data_[0];
    }

    /*!
     * \brief Access last character
     *
     * \return Reference to last character
     *
     * \details
     * - [SWS_CORE_03063]: Back element access
     */
    [[nodiscard]] constexpr auto back(const ara::core::internal::InputWithLocation<std::uint8_t> loc =
                                      ara::core::internal::make_input_with_location<std::uint8_t>(0)) const noexcept -> const_reference
    {
        
        if (detail::unlikely(empty())) {
            if (!detail::is_constant_evaluated()) {
                TriggerEmptyAccessViolation(loc.info(), "back");
            } else {
                constexpr unsigned char _empty_access_violation[1] = {};
                [[maybe_unused]] const auto verify{_empty_access_violation[1]};
            }
        }

        return data_[size_ - 1];
    }

    /*!
     * \brief Direct access to underlying array
     *
     * \return Pointer to first character
     *
     * \details
     * - [SWS_CORE_03064]: Data pointer access
     * - May return nullptr for empty views
     */
    [[nodiscard]] constexpr auto data() const noexcept -> const_pointer
    { return data_; }

    // -----------------------------------------------------------------------------------
    // CAPACITY [SWS_CORE_03070-03074]
    // -----------------------------------------------------------------------------------
    
    /*!
     * \brief Get number of characters
     *
     * \return Number of characters in view
     *
     * \details
     * - [SWS_CORE_03070]: Size observer
     */
    [[nodiscard]] constexpr auto size() const noexcept -> size_type
    { return size_; }
    
    /*!
     * \brief Get length (same as size)
     *
     * \return Number of characters in view
     *
     * \details
     * - [SWS_CORE_03071]: Length observer
     */
    [[nodiscard]] constexpr auto length() const noexcept -> size_type
    { return size_; }
    
    /*!
     * \brief Get maximum possible size
     *
     * \return Maximum size value
     *
     * \details
     * - [SWS_CORE_03072]: Max size observer
     */
    [[nodiscard]] constexpr auto max_size() const noexcept -> size_type
    { return (std::numeric_limits<size_type>::max)() / sizeof(CharT); }
    
    /*!
     * \brief Check if view is empty
     *
     * \return true if size() == 0
     *
     * \details
     * - [SWS_CORE_03073]: Empty check
     */
    [[nodiscard]] constexpr auto empty() const noexcept -> bool
    { return size_ == 0; }

    // -----------------------------------------------------------------------------------
    // MODIFIERS [SWS_CORE_03080-03083]
    // -----------------------------------------------------------------------------------
    
    /*!
     * \brief Remove n characters from the beginning
     *
     * \param n Number of characters to remove
     *
     * \details
     * - [SWS_CORE_03080]: Remove prefix
     * - Behavior undefined if n > size()
     */
    constexpr auto remove_prefix(const ara::core::internal::InputWithLocation<size_type>& n) noexcept -> void
    {
        const size_type count = n.input();
        
        if (detail::unlikely(count > size_)) {
            if (!detail::is_constant_evaluated()) {
                TriggerRemoveViolation(n.info(), count, size_, "remove_prefix");
            } else {
                constexpr unsigned char _remove_violation[1] = {};
                [[maybe_unused]] const auto verify{_remove_violation[1]};
            }
        }
        
        data_ += count;
        size_ -= count;
    }

    /*!
     * \brief Remove n characters from the end
     *
     * \param n Number of characters to remove
     *
     * \details
     * - [SWS_CORE_03081]: Remove suffix
     * - Behavior undefined if n > size()
     */
    constexpr auto remove_suffix(const ara::core::internal::InputWithLocation<size_type>& n) noexcept -> void
    {
        const size_type count = n.input();
        
        if (detail::unlikely(count > size_)) {
            if (!detail::is_constant_evaluated()) {
                TriggerRemoveViolation(n.info(), count, size_, "remove_suffix");
            } else {
                constexpr unsigned char _remove_violation[1] = {};
                [[maybe_unused]] const auto verify{_remove_violation[1]};
            }
        }
        
        size_ -= count;
    }

    /*!
     * \brief Swap with another string view
     *
     * \param v String view to swap with
     *
     * \details
     * - [SWS_CORE_03082]: Swap operation
     */
    constexpr auto swap(BasicStringView& other) noexcept -> void
    {
        other = ara::core::exchange(*this, other);
    }

    // -----------------------------------------------------------------------------------
    // STRING OPERATIONS [SWS_CORE_03090-03097]
    // -----------------------------------------------------------------------------------
    
    /*!
     * \brief Copy characters to buffer
     *
     * \param dest Destination buffer
     * \param count Number of characters to copy
     * \param pos Starting position
     * \return Number of characters copied
     *
     * \details
     * - [SWS_CORE_03090]: Copy operation
     * - Triggers violation if pos > size()
     */
    constexpr auto copy(CharT* dest, const ara::core::internal::InputWithLocation<size_type>& count, size_type pos = 0) const noexcept -> size_type
    {
        // Validate destination
        if (detail::unlikely(dest == nullptr)) {
            if (!detail::is_constant_evaluated()) {
                TriggerNullptrViolation(count.info());
            } else {
                constexpr unsigned char _null_dest[1] = {};
                [[maybe_unused]] const auto verify{_null_dest[1]};
            }
        }
        
        // Validate position
        if (detail::unlikely(pos > size_)) {
            if (!detail::is_constant_evaluated()) {
                TriggerPosViolation(count.info(), pos, size_);
            } else {
                constexpr unsigned char _pos_violation[1] = {};
                [[maybe_unused]] const auto verify{_pos_violation[1]};
            }
        }
        
        const size_type rlen = (std::min)(count.input(), size_ - pos);
        
        if (rlen > 0) {
            if (!detail::is_constant_evaluated()) {
                // Runtime: use optimized copy
                Traits::copy(dest, data_ + pos, rlen);
            } else {
                // Compile-time: manual copy
                for (size_type i = 0; i < rlen; ++i) {
                    Traits::assign(dest[i], data_[pos + i]);
                }
            }
        }
        
        return rlen;
    }

    /*!
     * \brief Get substring view
     *
     * \param pos Starting position
     * \param count Number of characters (default: npos)
     * \return Substring view
     *
     * \details
     * - [SWS_CORE_03091]: Substring operation
     * - Triggers violation if pos > size()
     */
    [[nodiscard]] constexpr auto substr(const ara::core::internal::InputWithLocation<size_type>& pos = 0, size_type count = npos) const
        noexcept -> BasicStringView
    {
        
        const size_type position = pos.input();
        
        if (detail::unlikely(position > size_)) {
            if (!detail::is_constant_evaluated()) {
                TriggerPosViolation(pos.info(), position, size_);
            } else {
                constexpr unsigned char _pos_violation[1] = {};
                [[maybe_unused]] const auto verify{_pos_violation[1]};
            }
        }

        const size_type rlen = (std::min)(count, size_ - position);
        return BasicStringView(data_ + position, rlen);
    }

    /*!
     * \brief Compare with another view
     *
     * \param v View to compare with
     * \return Negative if less, 0 if equal, positive if greater
     *
     * \details
     * - [SWS_CORE_03092]: Compare operation
     */
    [[nodiscard]] constexpr auto compare(BasicStringView v) const noexcept -> int
    {
        const size_type rlen = (std::min)(size_, v.size_);
        
        if (rlen > 0) {
            const int ret = ara::core::char_compare<CharT, Traits>(data_, v.data_, rlen);
            if (ret != 0) return ret;
        }
        
        // Compare sizes if prefixes are equal
        return (size_ == v.size_) ? 0 : (size_ < v.size_ ? -1 : 1);
    }

    [[nodiscard]] constexpr auto compare(size_type pos1, size_type count1,
                                        BasicStringView v) const noexcept -> int
    {
        return substr(pos1, count1).compare(v);
    }

    [[nodiscard]] constexpr auto compare(size_type pos1, size_type count1,
                                        BasicStringView v,
                                        size_type pos2, size_type count2) const noexcept -> int
    {
        return substr(pos1, count1).compare(v.substr(pos2, count2));
    }

    [[nodiscard]] constexpr auto compare(const_pointer s) const noexcept -> int
    {
        return compare(BasicStringView(s));
    }

    [[nodiscard]] constexpr auto compare(size_type pos1, size_type count1,
                                        const_pointer s) const noexcept -> int
    {
        return substr(pos1, count1).compare(BasicStringView(s));
    }

    [[nodiscard]] constexpr auto compare(size_type pos1, size_type count1,
                                        const_pointer s, size_type count2) const noexcept -> int
    {
        return substr(pos1, count1).compare(BasicStringView(s, count2));
    }

    // -----------------------------------------------------------------------------------
    // C++26 FEATURES - ENHANCED OPERATIONS
    // -----------------------------------------------------------------------------------
    
    /*!
     * \brief Check if view starts with a prefix (C++20/26 feature)
     *
     * \param sv String view to check as prefix
     * \return true if this view starts with sv
     *
     * \details
     * - C++26 enhancement for convenience
     * - Efficient prefix checking
     */
    [[nodiscard]] constexpr auto starts_with(BasicStringView sv) const noexcept -> bool
    {
        return size_ >= sv.size_ && 
               (sv.empty() || ara::core::char_compare<CharT, Traits>(data_, sv.data_, sv.size_) == 0);
    }

    [[nodiscard]] constexpr auto starts_with(CharT ch) const noexcept -> bool
    {
        return !empty() && Traits::eq(front(), ch);
    }

    [[nodiscard]] constexpr auto starts_with(const_pointer s) const noexcept -> bool
    {
        return starts_with(BasicStringView(s));
    }

    /*!
     * \brief Check if view ends with a suffix (C++20/26 feature)
     *
     * \param sv String view to check as suffix
     * \return true if this view ends with sv
     *
     * \details
     * - C++26 enhancement for convenience
     * - Efficient suffix checking
     */
    [[nodiscard]] constexpr auto ends_with(BasicStringView sv) const noexcept -> bool
    {
        return size_ >= sv.size_ && 
               (sv.empty() || ara::core::char_compare<CharT, Traits>(
                   data_ + size_ - sv.size_, sv.data_, sv.size_) == 0);
    }

    [[nodiscard]] constexpr auto ends_with(CharT ch) const noexcept -> bool
    {
        return !empty() && Traits::eq(back(), ch);
    }

    [[nodiscard]] constexpr auto ends_with(const_pointer s) const noexcept -> bool
    {
        return ends_with(BasicStringView(s));
    }

    /*!
     * \brief Check if view contains a substring (C++23/26 feature)
     *
     * \param sv String view to search for
     * \return true if sv is found
     *
     * \details
     * - C++26 enhancement for convenience
     * - Linear search complexity
     */
    [[nodiscard]] constexpr auto contains(BasicStringView sv) const noexcept -> bool
    {
        return find(sv) != npos;
    }

    [[nodiscard]] constexpr auto contains(CharT ch) const noexcept -> bool
    {
        return find(ch) != npos;
    }

    [[nodiscard]] constexpr auto contains(const_pointer s) const noexcept -> bool
    {
        return find(s) != npos;
    }

    // -----------------------------------------------------------------------------------
    // SEARCHING [SWS_CORE_03100-03120]
    // -----------------------------------------------------------------------------------
    
    /*!
     * \brief Find first occurrence of substring
     *
     * \param v Substring to search for
     * \param pos Starting position
     * \return Position of first occurrence or npos
     *
     * \details
     * - [SWS_CORE_03100]: Find substring
     */
    [[nodiscard]] constexpr auto find(BasicStringView v, size_type pos = 0) const noexcept -> size_type
    {
        // Early exit conditions
        if (pos > size_ || v.size_ > size_ - pos) {
            return npos;
        }
        
        if (v.empty()) {
            return pos;
        }
        
        // Use optimized search
        const auto result = ara::core::char_search<CharT, Traits>(
            data_ + pos, data_ + size_,
            v.data_, v.data_ + v.size_
        );
        
        return (result == data_ + size_) ? npos : static_cast<size_type>(result - data_);
    }

    [[nodiscard]] constexpr auto find(CharT ch, size_type pos = 0) const noexcept -> size_type
    {
        if (pos >= size_) {
            return npos;
        }
        
        const auto result = ara::core::char_find<CharT, Traits>(
            data_ + pos, size_ - pos, ch
        );
        
        return result ? static_cast<size_type>(result - data_) : npos;
    }

    [[nodiscard]] constexpr auto find(const_pointer s, size_type pos, size_type count) const noexcept -> size_type
    {
        return find(BasicStringView(s, count), pos);
    }

    [[nodiscard]] constexpr auto find(const_pointer s, size_type pos = 0) const noexcept -> size_type
    {
        return find(BasicStringView(s), pos);
    }

    /*!
     * \brief Find last occurrence of substring
     *
     * \param v Substring to search for
     * \param pos Starting position
     * \return Position of last occurrence or npos
     *
     * \details
     * - [SWS_CORE_03105]: Reverse find substring
     */
    [[nodiscard]] constexpr auto rfind(BasicStringView v, size_type pos = npos) const noexcept -> size_type
    {
        if (v.size_ > size_) {
            return npos;
        }
        
        if (v.empty()) {
            return (std::min)(pos, size_);
        }
        
        const size_type last = (std::min)(pos, size_ - v.size_);
        
        // Search backwards with optimization for single character
        if (v.size_ == 1) {
            for (size_type i = last + 1; i > 0; --i) {
                if (Traits::eq(data_[i - 1], v.data_[0])) {
                    return i - 1;
                }
            }
        } else {
            // Multi-character search
            for (size_type i = last + 1; i > 0; --i) {
                if (ara::core::char_compare<CharT, Traits>(
                        data_ + i - 1, v.data_, v.size_) == 0) {
                    return i - 1;
                }
            }
        }
        
        return npos;
    }

    [[nodiscard]] constexpr auto rfind(CharT ch, size_type pos = npos) const noexcept -> size_type
    {
        if (empty()) return npos;
        
        const size_type last = (std::min)(pos, size_ - 1);
        
        for (size_type i = last + 1; i > 0; --i) {
            if (Traits::eq(data_[i - 1], ch)) {
                return i - 1;
            }
        }
        
        return npos;
    }

    [[nodiscard]] constexpr auto rfind(const_pointer s, size_type pos, size_type count) const noexcept -> size_type
    {
        return rfind(BasicStringView(s, count), pos);
    }

    [[nodiscard]] constexpr auto rfind(const_pointer s, size_type pos = npos) const noexcept -> size_type
    {
        return rfind(BasicStringView(s), pos);
    }

    /*!
     * \brief Find first occurrence of any character in set
     *
     * \param v Set of characters to search for
     * \param pos Starting position
     * \return Position of first occurrence or npos
     *
     * \details
     * - [SWS_CORE_03110]: Find first of set
     */
    [[nodiscard]] constexpr auto find_first_of(BasicStringView v, size_type pos = 0) const noexcept
        -> size_type
    {
        if (pos >= size_ || v.empty()) {
            return npos;
        }
    
        /* Single character → delegate to find() */
        if (v.size_ == 1) {
            return this->find(v.front(), pos);
        }
    
        /* Very small set (≤ 8) → current double loop */
        if (v.size_ <= 8) {
            for (size_type i = pos; i < size_; ++i) {
                for (size_type j = 0; j < v.size_; ++j) {
                    if (Traits::eq(data_[i], v.data_[j])) {
                        return i;
                    }
                }
            }
            return npos;
        }
    
    
        /* constexpr evaluation cannot use a lookup table */
        if (detail::is_constant_evaluated() || sizeof(value_type) != 1) {
            /* Fallback O(n·m) */
            for (size_type i = pos; i < size_; ++i) {
                if (v.find(data_[i]) != npos) {
                    return i;
                }
            }
            return npos;
        }
    
    
        bool present[256] = {};                       /* 256-byte stack array      */
        for (size_type j = 0; j < v.size_; ++j) {     /* O(m) */
            present[static_cast<unsigned char>(v.data_[j])] = true;
        }
    
        for (size_type i = pos; i < size_; ++i) {     /* O(n) */
            if (present[static_cast<unsigned char>(data_[i])]) {
                return i;
            }
        }
        return npos;
    }

    [[nodiscard]] constexpr auto find_first_of(CharT ch, size_type pos = 0) const noexcept -> size_type
    {
        return find(ch, pos);
    }

    [[nodiscard]] constexpr auto find_first_of(const_pointer s, size_type pos, size_type count) const
       noexcept -> size_type
    {
        return find_first_of(BasicStringView(s, count), pos);
    }

    [[nodiscard]] constexpr auto find_first_of(const_pointer s, size_type pos = 0) const noexcept -> size_type
    {
        return find_first_of(BasicStringView(s), pos);
    }

    /*!
     * \brief Find last occurrence of any character in set
     *
     * \param v Set of characters to search for
     * \param pos Starting position
     * \return Position of last occurrence or npos
     *
     * \details
     * - [SWS_CORE_03111]: Find last of set
     */
    [[nodiscard]] constexpr auto find_last_of(BasicStringView v, size_type pos = npos) const noexcept
        -> size_type
    {
        if (empty() || v.empty()) {
            return npos;
        }
    
        const size_type last = (std::min)(pos, size_ - 1);
    
        /* Single character → just rfind */
        if (v.size_ == 1) {
            return this->rfind(v.data_[0], last);
        }
    
        constexpr size_type small_set_threshold = 8;          
        if (v.size_ <= small_set_threshold) {
            for (size_type i = last + 1; i-- > 0; ) {
                for (size_type j = 0; j < v.size_; ++j) {
                    if (Traits::eq(data_[i], v.data_[j])) {
                        return i;
                    }
                }
            }
            return npos;
        }
    
        /* Constant-evaluation or wide chars → stay with O(n·m) */
        if (detail::is_constant_evaluated() || sizeof(value_type) != 1) {
            for (size_type i = last + 1; i-- > 0; ) {
                if (v.find(data_[i]) != npos) {
                    return i;
                }
            }
            return npos;
        }
    
        /* 8-bit fast path: build presence table once, scan backward */
        bool present[256] = {};
        for (size_type j = 0; j < v.size_; ++j) {
            present[static_cast<unsigned char>(v.data_[j])] = true;
        }
        for (size_type i = last + 1; i-- > 0; ) {
            if (present[static_cast<unsigned char>(data_[i])]) {
                return i;
            }
        }
        return npos;
    }

    [[nodiscard]] constexpr auto find_last_of(CharT ch, size_type pos = npos) const noexcept -> size_type
    {
        return rfind(ch, pos);
    }

    [[nodiscard]] constexpr auto find_last_of(const_pointer s, size_type pos, size_type count) const
        -> size_type
    {
        return find_last_of(BasicStringView(s, count), pos);
    }

    [[nodiscard]] constexpr auto find_last_of(const_pointer s, size_type pos = npos) const noexcept -> size_type
    {
        return find_last_of(BasicStringView(s), pos);
    }

    /*!
     * \brief Find first character not in set
     *
     * \param v Set of characters to exclude
     * \param pos Starting position
     * \return Position of first non-matching character or npos
     *
     * \details
     * - [SWS_CORE_03115]: Find first not of set
     */
    [[nodiscard]] constexpr auto find_first_not_of(BasicStringView v, size_type pos = 0) const noexcept
        -> size_type
    {

        if (pos >= size_) {
            return npos;
        }
        if (v.empty()) {
            return pos;
        }
    
        if (v.size_ == 1) {
            const value_type ch = v.data_[0];
            for (size_type i = pos; i < size_; ++i) {
                if (!Traits::eq(data_[i], ch)) {
                    return i;
                }
            }
            return npos;
        }
    
        constexpr size_type small_set_threshold = 8;
        if (v.size_ <= small_set_threshold) {
            for (size_type i = pos; i < size_; ++i) {
                bool found = false;
                for (size_type j = 0; j < v.size_; ++j) {
                    if (Traits::eq(data_[i], v.data_[j])) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    return i;
                }
            }
            return npos;
        }
    
        /* constexpr evaluation or wide characters → fall back to O(n·m) loop */
        if (detail::is_constant_evaluated() || sizeof(value_type) != 1) {
            for (size_type i = pos; i < size_; ++i) {
                if (v.find(data_[i]) == npos) {
                    return i;
                }
            }
            return npos;
        }
    
        /* 8-bit fast path: build presence table */
        bool present[256] = {};                           /*256-byte stack buffer*/
        for (size_type j = 0; j < v.size_; ++j) {         /*O(m)*/
            present[static_cast<unsigned char>(v.data_[j])] = true;
        }
        for (size_type i = pos; i < size_; ++i) {         /*O(n)*/
            if (!present[static_cast<unsigned char>(data_[i])]) {
                return i;
            }
        }
        return npos;
    }

    [[nodiscard]] constexpr auto find_first_not_of(CharT ch, size_type pos = 0) const noexcept
        -> size_type
    {
        for (size_type i = pos; i < size_; ++i) {
            if (!Traits::eq(data_[i], ch)) {
                return i;
            }
        }
        return npos;
    }

    [[nodiscard]] constexpr auto find_first_not_of(const_pointer s, size_type pos, size_type count) const
       noexcept -> size_type
    {
        return find_first_not_of(BasicStringView(s, count), pos);
    }

    [[nodiscard]] constexpr auto find_first_not_of(const_pointer s, size_type pos = 0) const noexcept -> size_type
    {
        return find_first_not_of(BasicStringView(s), pos);
    }

    /*!
     * \brief Find last character not in set
     *
     * \param v Set of characters to exclude
     * \param pos Starting position
     * \return Position of last non-matching character or npos
     *
     * \details
     * - [SWS_CORE_03116]: Find last not of set
     */
    [[nodiscard]] constexpr auto find_last_not_of(BasicStringView v, size_type pos = npos) const noexcept
        -> size_type
    {
        if (empty()) {
            return npos;
        }
    
        const size_type last = (std::min)(pos, size_ - 1);
    
        if (v.empty()) {
            return last;
        }
    
        if (v.size_ == 1) {
            const value_type ch = v.data_[0];
            for (size_type i = last + 1; i-- > 0; ) {
                if (!Traits::eq(data_[i - 1], ch)) {
                    return i - 1;
                }
            }
            return npos;
        }
    
        constexpr size_type small_set_threshold = 8;
        if (v.size_ <= small_set_threshold) {
            for (size_type i = last + 1; i-- > 0; ) {
                const value_type cur = data_[i - 1];
                bool found = false;
                for (size_type j = 0; j < v.size_; ++j) {
                    if (Traits::eq(cur, v.data_[j])) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    return i - 1;
                }
            }
            return npos;
        }
    
        /* constexpr evaluation (C++17) or wide characters -> fallback O(n·m) loop */
        if (detail::is_constant_evaluated() || sizeof(value_type) != 1) {
            for (size_type i = last + 1; i-- > 0; ) {
                if (v.find(data_[i - 1]) == npos) {
                    return i - 1;
                }
            }
            return npos;
        }
    
        /* 8-bit fast path: build presence table once, then scan backward */
        bool present[256] = {};                          /* 256-byte stack array*/
        for (size_type j = 0; j < v.size_; ++j) {        /* O(m)*/
            present[static_cast<unsigned char>(v.data_[j])] = true;
        }
        for (size_type i = last + 1; i-- > 0; ) {        /* O(n)*/
            if (!present[static_cast<unsigned char>(data_[i - 1])]) {
                return i - 1;
            }
        }
        return npos;
    }

    [[nodiscard]] constexpr auto find_last_not_of(CharT ch, size_type pos = npos) const noexcept
        -> size_type
    {
        if (empty()) {
            return npos;
        }
        
        const size_type last = (std::min)(pos, size_ - 1);
        
        for (size_type i = last + 1; i > 0; --i) {
            if (!Traits::eq(data_[i - 1], ch)) {
                return i - 1;
            }
        }
        return npos;
    }

    [[nodiscard]] constexpr auto find_last_not_of(const_pointer s, size_type pos, size_type count) const
        noexcept -> size_type
    {
        return find_last_not_of(BasicStringView(s, count), pos);
    }

    [[nodiscard]] constexpr auto find_last_not_of(const_pointer s, size_type pos = npos) const
        noexcept -> size_type
    {
        return find_last_not_of(BasicStringView(s), pos);
    }

private:
    const_pointer data_;     /*!< Pointer to character sequence */
    size_type size_;        /*!< Number of characters */

    /*!
     * \brief Trigger bounds violation
     */
    [[noreturn]] static void TriggerBoundsViolation(
        std::string_view location,
        std::size_t index,
        std::size_t size) noexcept
    {
        auto& handler = ara::core::internal::ViolationHandler::Instance();
        handler.TriggerStringViewBoundsViolation(
            ara::core::internal::ViolationHandler::StringViewKey{},
            location,
            index,
            size);
    }

    /*!
     * \brief Trigger position violation
     */
    [[noreturn]] static void TriggerPosViolation(
        std::string_view location,
        std::size_t pos,
        std::size_t size) noexcept
    {
        auto& handler = ara::core::internal::ViolationHandler::Instance();
        handler.TriggerStringViewPosViolation(
            ara::core::internal::ViolationHandler::StringViewKey{},
            location,
            pos,
            size);
    }

    /*!
     * \brief Trigger nullptr construction violation
     */
    [[noreturn]] static void TriggerNullptrViolation(
        std::string_view location) noexcept
    {
        auto& handler = ara::core::internal::ViolationHandler::Instance();
        handler.TriggerStringViewNullptrViolation(
            ara::core::internal::ViolationHandler::StringViewKey{},
            location);
    }

    /*!
     * \brief Trigger empty access violation
     */
    [[noreturn]] static void TriggerEmptyAccessViolation(
        std::string_view location,
        const char* operation) noexcept
    {
        auto& handler = ara::core::internal::ViolationHandler::Instance();
        handler.TriggerStringViewEmptyViolation(
            ara::core::internal::ViolationHandler::StringViewKey{},
            location,
            operation);
    }

    /*!
     * \brief Trigger index violation (for operator[])
     */
    [[noreturn]] static void TriggerIndexViolation(
        std::string_view location,
        std::size_t index,
        std::size_t size) noexcept
    {
        auto& handler = ara::core::internal::ViolationHandler::Instance();
        handler.TriggerStringViewIndexViolation(
            ara::core::internal::ViolationHandler::StringViewKey{},
            location,
            index,
            size);
    }

    /*!
     * \brief Trigger remove operation violation
     */
    [[noreturn]] static void TriggerRemoveViolation(
        std::string_view location,
        std::size_t n,
        std::size_t size,
        const char* operation) noexcept
    {
        auto& handler = ara::core::internal::ViolationHandler::Instance();
        handler.TriggerStringViewRemoveViolation(
            ara::core::internal::ViolationHandler::StringViewKey{},
            location,
            n,
            size,
            operation);
    }
};

/**********************************************************************************************************************
 *  TYPE ALIASES [SWS_CORE_03130-03135]
 *********************************************************************************************************************/

/*!
 * \brief String view for char
 * [SWS_CORE_03130]: Standard char string view type
 */
using StringView = BasicStringView<char>;

/*!
 * \brief String view for wchar_t
 * [SWS_CORE_03131]: Wide character string view type
 */
using WStringView = BasicStringView<wchar_t>;

#if __cplusplus > 201703L || defined(__cpp_char8_t)
/*!
 * \brief String view for char8_t (C++20 feature)
 * [SWS_CORE_03132]: UTF-8 character string view type
 */
using U8StringView = BasicStringView<char8_t>;
#endif

/*!
 * \brief String view for char16_t
 * [SWS_CORE_03133]: UTF-16 character string view type
 */
using U16StringView = BasicStringView<char16_t>;

/*!
 * \brief String view for char32_t
 * [SWS_CORE_03134]: UTF-32 character string view type
 */
using U32StringView = BasicStringView<char32_t>;

/**********************************************************************************************************************
 *  DEDUCTION GUIDES [SWS_CORE_03140]
 *********************************************************************************************************************/

/*!
 * \brief Deduction guide for iterator pair
 *
 * \details Enables: BasicStringView sv(begin, end);
 */
template<typename It, typename End>
BasicStringView(It, End) -> BasicStringView<typename std::iterator_traits<It>::value_type>;

/*!
 * \brief Deduction guide for C-style array
 *
 * \details Enables: BasicStringView sv("Hello");
 */
template<typename CharT, std::size_t N>
BasicStringView(const CharT(&)[N]) -> BasicStringView<CharT>;

/*!
 * \brief Deduction guide for pointer
 *
 * \details Enables: BasicStringView sv(ptr);
 */
template<typename CharT>
BasicStringView(const CharT*) -> BasicStringView<CharT>;

/*!
 * \brief Deduction guide for basic_string
 *
 * \details Enables: BasicStringView sv(str);
 */
template<typename CharT, typename Traits, typename Alloc>
BasicStringView(const std::basic_string<CharT, Traits, Alloc>&) 
    -> BasicStringView<CharT, Traits>;

/**********************************************************************************************************************
 *  NON-MEMBER FUNCTIONS [SWS_CORE_03150-03160]
 *********************************************************************************************************************/

/*!
 * \brief Swap two string views
 *
 * [SWS_CORE_03150]: Non-member swap
 */
template<typename CharT, typename Traits>
constexpr auto swap(BasicStringView<CharT, Traits>& lhs,
                   BasicStringView<CharT, Traits>& rhs) noexcept -> void
{
    lhs.swap(rhs);
}

/**********************************************************************************************************************
 *  COMPARISON OPERATORS [SWS_CORE_03161-03166]
 *********************************************************************************************************************/

template<typename CharT, typename Traits>
[[nodiscard]] constexpr auto operator==(BasicStringView<CharT, Traits> lhs,
                                       BasicStringView<CharT, Traits> rhs) noexcept -> bool
{
    return lhs.size() == rhs.size() && lhs.compare(rhs) == 0;
}

template<typename CharT, typename Traits>
[[nodiscard]] constexpr auto operator!=(BasicStringView<CharT, Traits> lhs,
                                       BasicStringView<CharT, Traits> rhs) noexcept -> bool
{
    return !(lhs == rhs);
}

template<typename CharT, typename Traits>
[[nodiscard]] constexpr auto operator<(BasicStringView<CharT, Traits> lhs,
                                      BasicStringView<CharT, Traits> rhs) noexcept -> bool
{
    return lhs.compare(rhs) < 0;
}

template<typename CharT, typename Traits>
[[nodiscard]] constexpr auto operator<=(BasicStringView<CharT, Traits> lhs,
                                       BasicStringView<CharT, Traits> rhs) noexcept -> bool
{
    return lhs.compare(rhs) <= 0;
}

template<typename CharT, typename Traits>
[[nodiscard]] constexpr auto operator>(BasicStringView<CharT, Traits> lhs,
                                      BasicStringView<CharT, Traits> rhs) noexcept -> bool
{
    return lhs.compare(rhs) > 0;
}

template<typename CharT, typename Traits>
[[nodiscard]] constexpr auto operator>=(BasicStringView<CharT, Traits> lhs,
                                       BasicStringView<CharT, Traits> rhs) noexcept -> bool
{
    return lhs.compare(rhs) >= 0;
}

// String literal comparisons
template<typename CharT, typename Traits>
[[nodiscard]] constexpr auto operator==(BasicStringView<CharT, Traits> lhs,
                                        const CharT* rhs) noexcept -> bool
{
    return lhs == BasicStringView<CharT, Traits>(rhs);
}

template<typename CharT, typename Traits>
[[nodiscard]] constexpr auto operator==(const CharT* lhs,
                                        BasicStringView<CharT, Traits> rhs) noexcept -> bool
{
    return BasicStringView<CharT, Traits>(lhs) == rhs;
}

template<typename CharT, typename Traits>
[[nodiscard]] constexpr auto operator!=(BasicStringView<CharT, Traits> lhs,
                                        const CharT* rhs) noexcept -> bool
{
    return !(lhs == rhs);
}

template<typename CharT, typename Traits>
[[nodiscard]] constexpr auto operator!=(const CharT* lhs,
                                        BasicStringView<CharT, Traits> rhs) noexcept -> bool
{
    return !(lhs == rhs);
}

template<typename CharT, typename Traits>
[[nodiscard]] constexpr auto operator<(BasicStringView<CharT, Traits> lhs,
                                       const CharT* rhs) noexcept -> bool
{
    return lhs < BasicStringView<CharT, Traits>(rhs);
}

template<typename CharT, typename Traits>
[[nodiscard]] constexpr auto operator<(const CharT* lhs,
                                       BasicStringView<CharT, Traits> rhs) noexcept -> bool
{
    return BasicStringView<CharT, Traits>(lhs) < rhs;
}

template<typename CharT, typename Traits>
[[nodiscard]] constexpr auto operator<=(BasicStringView<CharT, Traits> lhs,
                                        const CharT* rhs) noexcept -> bool
{
    return lhs <= BasicStringView<CharT, Traits>(rhs);
}

template<typename CharT, typename Traits>
[[nodiscard]] constexpr auto operator<=(const CharT* lhs,
                                        BasicStringView<CharT, Traits> rhs) noexcept -> bool
{
    return BasicStringView<CharT, Traits>(lhs) <= rhs;
}

template<typename CharT, typename Traits>
[[nodiscard]] constexpr auto operator>(BasicStringView<CharT, Traits> lhs,
                                       const CharT* rhs) noexcept -> bool
{
    return lhs > BasicStringView<CharT, Traits>(rhs);
}

template<typename CharT, typename Traits>
[[nodiscard]] constexpr auto operator>(const CharT* lhs,
                                       BasicStringView<CharT, Traits> rhs) noexcept -> bool
{
    return BasicStringView<CharT, Traits>(lhs) > rhs;
}

template<typename CharT, typename Traits>
[[nodiscard]] constexpr auto operator>=(BasicStringView<CharT, Traits> lhs,
                                        const CharT* rhs) noexcept -> bool
{
    return lhs >= BasicStringView<CharT, Traits>(rhs);
}

template<typename CharT, typename Traits>
[[nodiscard]] constexpr auto operator>=(const CharT* lhs,
                                        BasicStringView<CharT, Traits> rhs) noexcept -> bool
{
    return BasicStringView<CharT, Traits>(lhs) >= rhs;
}

/**********************************************************************************************************************
 *  STREAM OPERATORS [SWS_CORE_03170]
 *********************************************************************************************************************/
/*!
 * \brief Stream insertion operator
 *
 * \param os Output stream
 * \param sv String view to output
 * \return Reference to stream
 *
 * \details
 * - [SWS_CORE_03170]: Stream output support
 * - Respects stream formatting (width, alignment)
 * - Efficient single write operation when possible
 */
template<typename CharT, typename Traits>
inline auto operator<<(
    std::basic_ostream<CharT, Traits>& os,
    BasicStringView<CharT, Traits> sv) -> std::basic_ostream<CharT, Traits>& {
    // Get current stream state
    const auto width = os.width();
    const auto fill_char = os.fill();
    
    // Reset width to prevent double application
    os.width(0);
    
    // Handle width and alignment
    const auto str_len = static_cast<std::streamsize>(sv.size());
    
    if (width <= str_len) {
        // No padding needed - direct output
        return os.write(sv.data(), str_len);
    }
    
    // Calculate padding
    const auto padding = width - str_len;
    const bool left_align = (os.flags() & std::ios_base::adjustfield) == std::ios_base::left;
    
    // Output with padding
    if (!left_align) {
        // Right align - pad first
        for (std::streamsize i = 0; i < padding; ++i) {
            os.put(fill_char);
        }
    }
    
    os.write(sv.data(), str_len);
    
    if (left_align) {
        // Left align - pad after
        for (std::streamsize i = 0; i < padding; ++i) {
            os.put(fill_char);
        }
    }
    
    return os;
}

/**********************************************************************************************************************
 *  HASH SUPPORT
 *********************************************************************************************************************/
/*!
 * \brief High-quality hash functor for BasicStringView
 *
 * \details
 * Provides a hybrid hashing approach optimized for different contexts:
 * - Compile-time: Uses FNV-1a algorithm (constexpr-compatible)
 * - Runtime: Uses XXH3-64 algorithm (high performance, excellent distribution)
 * 
 * The implementation ensures:
 * - Deterministic hashing (same input always produces same output)
 * - Good distribution properties for hash tables
 * - High performance for runtime hashing
 * - Compatibility with std::hash interface
 * 
 * \note The runtime path uses a deterministic seed based on the functor's address
 *       to provide some protection against hash flooding attacks while maintaining
 *       determinism within a process.
 */
struct hash_string_view {
private:
    /*!
     * \brief Generate deterministic seed for XXH3
     *
     * \return 64-bit seed value unique per process
     *
     * \details
     * Uses the address of a static variable XORed with a constant to create
     * a seed that's deterministic within a process but varies between runs.
     * This provides some protection against hash collision attacks.
     */
    [[nodiscard]] static auto get_seed() noexcept -> uint64_t {
        // Golden ratio constant for better bit distribution
        constexpr uint64_t golden_ratio = 0x9E3779B97F4A7C15ULL;
        
        // Use address of static variable for per-process uniqueness
        static const char seed_anchor = 0;
        
        return golden_ratio ^ 
               reinterpret_cast<std::uintptr_t>(&seed_anchor);
    }
    
    /*!
     * \brief Constexpr FNV-1a hash implementation
     *
     * \tparam CharT Character type
     * \param data Pointer to character data
     * \param size Number of characters
     * \return 64-bit hash value
     *
     * \details
     * FNV-1a (Fowler-Noll-Vo) algorithm:
     * - Simple and fast
     * - Good distribution for small strings
     * - Fully constexpr-compatible
     */
    template<typename CharT>
    [[nodiscard]] static constexpr auto fnv1a_hash(
        const CharT* data, 
        std::size_t size) noexcept -> uint64_t {
        
        // FNV-1a 64-bit offset basis
        uint64_t hash = 14695981039346656037ULL;
        
        // FNV-1a 64-bit prime
        constexpr uint64_t prime = 1099511628211ULL;
        
        // Process each byte
        for (std::size_t i = 0; i < size; ++i) {
            // Ensure unsigned conversion to avoid sign extension
            using unsigned_char_t = std::make_unsigned_t<CharT>;
            const auto byte = static_cast<uint64_t>(
                static_cast<unsigned_char_t>(data[i])
            );
            
            hash ^= byte;
            hash *= prime;
        }
        
        return hash;
    }
    
    /*!
     * \brief High-quality mixer for hash finalization
     *
     * \param x Value to mix
     * \return Mixed value with improved bit distribution
     *
     * \details
     * Uses MurmurHash3 finalizer for excellent avalanche properties.
     * Ensures all bits of input affect all bits of output.
     */
    [[nodiscard]] static constexpr auto mix_bits(uint64_t x) noexcept -> uint64_t {
        // MurmurHash3 finalizer
        x ^= x >> 33;
        x *= 0xFF51AFD7ED558CCDULL;
        x ^= x >> 33;
        x *= 0xC4CEB9FE1A85EC53ULL;
        x ^= x >> 33;
        return x;
    }
    
public:
    /*!
     * \brief Hash a string view
     *
     * \tparam CharT Character type
     * \tparam Traits Character traits
     * \param sv String view to hash
     * \return Hash value as std::size_t
     *
     * \details
     * Dual-path implementation:
     * 1. Compile-time: Uses FNV-1a for constexpr compatibility
     * 2. Runtime: Uses XXH3-64 for superior performance and distribution
     * 
     * Both paths produce high-quality hash values suitable for hash tables.
     */
    template<typename CharT, typename Traits>
    [[nodiscard]] constexpr auto operator()(
        BasicStringView<CharT, Traits> sv) const noexcept -> std::size_t {
        
        // -----------------------------------------------------------------------------------
        // COMPILE-TIME PATH - FNV-1a
        // -----------------------------------------------------------------------------------
        if (detail::is_constant_evaluated()) {
            const uint64_t raw_hash = fnv1a_hash(sv.data(), sv.size());
            const uint64_t mixed_hash = mix_bits(raw_hash);
            
            // Fold to size_t
            if constexpr (sizeof(std::size_t) == sizeof(uint64_t)) {
                return static_cast<std::size_t>(mixed_hash);
            } else {
                // 32-bit platform: XOR-fold the halves
                const uint32_t hi = static_cast<uint32_t>(mixed_hash >> 32);
                const uint32_t lo = static_cast<uint32_t>(mixed_hash);
                return static_cast<std::size_t>(hi ^ lo);
            }
        }
        
        // -----------------------------------------------------------------------------------
        // RUNTIME PATH - XXH3-64
        // -----------------------------------------------------------------------------------
        
        // Convert to byte view for hashing
        const void* data = static_cast<const void*>(sv.data());
        const std::size_t byte_size = sv.size() * sizeof(CharT);
        
        // Get high-quality hash from XXH3
        const uint64_t hash64 = detail::xxh3_64bits_withSeed(
            data, 
            byte_size, 
            get_seed()
        );
        
        // Apply final mixing for better distribution
        const uint64_t mixed = mix_bits(hash64);
        
        // Convert to size_t
        if constexpr (sizeof(std::size_t) == sizeof(uint64_t)) {
            return static_cast<std::size_t>(mixed);
        } else {
            // 32-bit platform: XOR-fold with additional mixing
            const uint32_t hi = static_cast<uint32_t>(mixed >> 32);
            const uint32_t lo = static_cast<uint32_t>(mixed);
            const uint32_t folded = hi ^ lo;
            
            // Additional mixing for 32-bit result
            return mix_bits(folded);
        }
    }
    
    /*!
     * \brief Transparent hash support (C++20 feature backported)
     *
     * \details
     * Allows heterogeneous lookup in unordered containers when enabled.
     * This means you can look up a std::string key using a StringView.
     */
    using is_transparent = void;
};

} // namespace core
} // namespace ara

// -----------------------------------------------------------------------------------
// STD::HASH SPECIALIZATION
// -----------------------------------------------------------------------------------

namespace std {

/*!
 * \brief Hash specialization for ara::core::BasicStringView
 *
 * \tparam CharT Character type
 * \tparam Traits Character traits type
 *
 * \details
 * Enables ara::core::BasicStringView to be used as key in std::unordered_map
 * and other unordered associative containers.
 * 
 * Inherits from ara::core::hash_string_view to provide the implementation
 * while satisfying the std::hash interface requirements.
 * 
 * \note This specialization is injected into std namespace as required by
 *       the C++ standard for user-defined types.
 */
template<typename CharT, typename Traits>
struct hash<ara::core::BasicStringView<CharT, Traits>> 
    : ara::core::hash_string_view {
    
    // Inherit all functionality from hash_string_view
    using ara::core::hash_string_view::operator();
    using ara::core::hash_string_view::is_transparent;
};

} // namespace std

namespace ara {
namespace core {

/**********************************************************************************************************************
 *  USER-DEFINED LITERALS [SWS_CORE_03190-03195]
 *********************************************************************************************************************/

/*!
 * \brief Literal operators namespace
 */
inline namespace literals {
inline namespace string_view_literals {

/*!
 * \brief Create string view from string literal
 *
 * \param str String literal
 * \param len Length of string
 * \return String view
 *
 * \details
 * - [SWS_CORE_03190]: User-defined literal for char
 * - Enables "Hello"_sv syntax
 */
[[nodiscard]] constexpr auto operator""_sv(const char* str, std::size_t len) noexcept -> StringView
{
    return StringView{str, len};
}

/*!
 * \brief Create wide string view from string literal
 *
 * \param str Wide string literal
 * \param len Length of string
 * \return Wide string view
 *
 * \details
 * - [SWS_CORE_03191]: User-defined literal for wchar_t
 * - Enables L"Hello"_sv syntax
 */
[[nodiscard]] constexpr auto operator""_sv(const wchar_t* str, std::size_t len) noexcept -> WStringView
{
    return WStringView{str, len};
}

#if __cplusplus > 201703L || defined(__cpp_char8_t)
/*!
 * \brief Create UTF-8 string view from string literal
 *
 * \param str UTF-8 string literal
 * \param len Length of string
 * \return UTF-8 string view
 *
 * \details
 * - [SWS_CORE_03192]: User-defined literal for char8_t
 * - Enables u8"Hello"_sv syntax
 */
[[nodiscard]] constexpr auto operator""_sv(const char8_t* str, std::size_t len) noexcept -> U8StringView
{
    return U8StringView{str, len};
}
#endif

/*!
 * \brief Create UTF-16 string view from string literal
 *
 * \param str UTF-16 string literal
 * \param len Length of string
 * \return UTF-16 string view
 *
 * \details
 * - [SWS_CORE_03193]: User-defined literal for char16_t
 * - Enables u"Hello"_sv syntax
 */
[[nodiscard]] constexpr auto operator""_sv(const char16_t* str, std::size_t len) noexcept -> U16StringView
{
    return U16StringView{str, len};
}

/*!
 * \brief Create UTF-32 string view from string literal
 *
 * \param str UTF-32 string literal
 * \param len Length of string
 * \return UTF-32 string view
 *
 * \details
 * - [SWS_CORE_03194]: User-defined literal for char32_t
 * - Enables U"Hello"_sv syntax
 */
[[nodiscard]] constexpr auto operator""_sv(const char32_t* str, std::size_t len) noexcept -> U32StringView
{
    return U32StringView{str, len};
}

} // namespace string_view_literals
} // namespace literals

/**********************************************************************************************************************
 *  FACTORY FUNCTIONS [SWS_CORE_03200-03202]
 *********************************************************************************************************************/

/*!
 * \brief Create string view from pointer and size
 *
 * \tparam CharT Character type
 * \param str Pointer to characters
 * \param len Number of characters
 * \return String view
 *
 * \details
 * - [SWS_CORE_03200]: Factory function for pointer and size
 */
template<typename CharT>
[[nodiscard]] constexpr auto MakeStringView(const CharT* str, std::size_t len) noexcept
    -> BasicStringView<CharT>
{
    return BasicStringView<CharT>(str, len);
}

/*!
 * \brief Create string view from C-string
 *
 * \tparam CharT Character type
 * \param str Null-terminated string
 * \return String view
 *
 * \details
 * - [SWS_CORE_03201]: Factory function for C-string
 */
template<typename CharT>
[[nodiscard]] constexpr auto MakeStringView(const CharT* str) noexcept
    -> BasicStringView<CharT>
{
    return BasicStringView<CharT>(str);
}

/*!
 * \brief Create string view from string-like container
 *
 * \tparam Container String-like type
 * \param cont Container with data() and size()
 * \return String view
 *
 * \details
 * - [SWS_CORE_03202]: Factory function for containers
 */
template<typename Container,
         typename = std::enable_if_t<detail::has_string_like_interface_v<Container>>>
[[nodiscard]] constexpr auto MakeStringView(const Container& cont) noexcept
    -> BasicStringView<
         std::remove_cv_t<
           std::remove_pointer_t<
             decltype(std::data(std::declval<const Container&>()))>>>
{
    using RawChar =
        std::remove_cv_t<
          std::remove_pointer_t<
            decltype(std::data(cont))>>;
    return BasicStringView<RawChar>(cont);
}

/**********************************************************************************************************************
 *  RANGES SUPPORT NAMESPACE
 *********************************************************************************************************************/

/*!
 * \brief C++17-compatible ranges support for string_view
 */
namespace ranges {

/*!
 * \brief Check if type is a string_view
 */
template<typename T>
struct is_string_view : std::false_type {};

template<typename CharT, typename Traits>
struct is_string_view<BasicStringView<CharT, Traits>> : std::true_type {};

template<typename T>
inline constexpr bool is_string_view_v = is_string_view<T>::value;

/*!
 * \brief Split view for string views
 */
template< typename CharT, typename Traits >
class split_view {
public:
    using string_view_type = BasicStringView< CharT, Traits >;

private:
    string_view_type view_;   /* the complete input                    */
    CharT            delim_;  /* delimiter character                   */

    /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *  iterator – single-pass forward iterator
     *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
    class iterator
    {
        string_view_type remaining_;    /* input still to scan              */
        string_view_type current_;      /* sub-view we are on               */
        CharT            delim_;        /* delimiter                         */
        bool             done_{ false };/* true  → iterator equals end()    */

        /* advance to next non-empty token ----------------------------------*/
        constexpr void next() noexcept
        {
            while ( !done_ )
            {
                if ( remaining_.empty() )               /* nothing left – mark end */
                {
                    done_ = true;
                    return;
                }

                const auto pos = remaining_.find( delim_ );

                if ( pos == string_view_type::npos )    /* last token */
                {
                    current_   = remaining_;
                    remaining_ = {};                    /* exhaust input           */
                    /* `done_` stays false so the caller can consume this token   */
                    return;
                }

                current_ = remaining_.substr( 0, pos ); /* token before delimiter  */
                remaining_.remove_prefix( pos + 1 );    /* skip delimiter          */

                if ( !current_.empty() )                /* skip consecutive delimiters */
                    return;
                /* else: current_ was empty → loop again */
            }
        }

    public:
        /* iterator traits ---------------------------------------------------*/
        using iterator_category = std::forward_iterator_tag;
        using value_type        = string_view_type;
        using difference_type   = std::ptrdiff_t;
        using pointer           = const value_type*;
        using reference         = const value_type&;

        /* construction ------------------------------------------------------*/
        constexpr iterator() noexcept
          : remaining_{}, current_{}, delim_{ CharT{} }, done_{ true }
        {}

        constexpr iterator(string_view_type s, CharT d) noexcept
          : remaining_{ s }, current_{}, delim_{ d }, done_{ false }
        {
            next();
        }
        
        /* dereference -------------------------------------------------------*/
        [[nodiscard]] constexpr reference operator*()  const noexcept { return current_; }
        [[nodiscard]] constexpr pointer   operator->() const noexcept { return &current_; }

        /* increment ---------------------------------------------------------*/
        constexpr iterator& operator++() noexcept { next(); return *this; }
        constexpr iterator  operator++( int ) noexcept
        { iterator tmp = *this; ++(*this); return tmp; }

        /* comparison --------------------------------------------------------*/
        [[nodiscard]] friend constexpr bool operator==( const iterator& lhs,
                                                       const iterator& rhs ) noexcept
        { return lhs.done_ == rhs.done_; }

        [[nodiscard]] friend constexpr bool operator!=( const iterator& lhs,
                                                       const iterator& rhs ) noexcept
        { return !( lhs == rhs ); }
    };

public:
    /* split_view construction ----------------------------------------------*/
    constexpr split_view( string_view_type sv, CharT d ) noexcept
        : view_{ sv }, delim_{ d } {}

    /* begin / end ----------------------------------------------------------*/
    [[nodiscard]] constexpr iterator begin() const noexcept { return iterator{ view_, delim_ }; }
    [[nodiscard]] constexpr iterator end()   const noexcept { return iterator{}; }
};

/* factory helper ------------------------------------------------------------*/
template< typename CharT, typename Traits >
[[nodiscard]] constexpr auto split( BasicStringView< CharT, Traits > sv,
                                    CharT                            delimiter ) noexcept
    -> split_view< CharT, Traits >
{
    return split_view< CharT, Traits >( sv, delimiter );
}
/*!
 * \brief Trim whitespace from both ends
 */
template<typename CharT, typename Traits>
[[nodiscard]] constexpr auto trim(BasicStringView<CharT, Traits> sv) noexcept
    -> BasicStringView<CharT, Traits>
{
    constexpr auto is_ws = [](CharT c) noexcept {
        return c == CharT{' '}  || c == CharT{'\t'} ||
               c == CharT{'\n'} || c == CharT{'\r'} ||
               c == CharT{'\f'} || c == CharT{'\v'};
    };

    std::size_t first = 0;
    while (first < sv.size() && is_ws(sv[first]))  ++first;

    std::size_t last  = sv.size();
    while (last  > first && is_ws(sv[last - 1]))   --last;

    return sv.substr(first, last - first);
}

/*!
 * \brief Transform view for string views
 */
template<typename CharT, typename Traits, typename F>
class sv_transform_view {
public:
    using string_view_type = BasicStringView<CharT, Traits>;
    using value_type = std::invoke_result_t<F, CharT>;
    
private:
    string_view_type view_;
    F func_;
    
public:
    class iterator {
    private:
        typename string_view_type::const_iterator it_;
        const F* func_;
        
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type = std::invoke_result_t<F, CharT>;
        using difference_type = std::ptrdiff_t;
        using pointer = void;
        using reference = value_type;
        
        constexpr iterator(typename string_view_type::const_iterator it, const F* f)  noexcept
            : it_(it), func_(f) {}

        [[nodiscard]] constexpr auto operator*() const noexcept -> value_type {
            return (*func_)(*it_);
        }

        constexpr auto operator++() noexcept -> iterator& {
            ++it_;
            return *this;
        }

        constexpr auto operator++(int) noexcept -> iterator {
            iterator tmp = *this;
            ++*this;
            return tmp;
        }
        
        [[nodiscard]] constexpr auto operator==(const iterator& other) const noexcept -> bool {
            return it_ == other.it_;
        }
        
        [[nodiscard]] constexpr auto operator!=(const iterator& other) const noexcept -> bool {
            return !(*this == other);
        }
    };
    
    constexpr sv_transform_view(string_view_type v, F f) noexcept : view_(v), func_(std::move(f)) {}
    
    [[nodiscard]] constexpr auto begin() const noexcept -> iterator { return iterator(view_.begin(), &func_); }
    [[nodiscard]] constexpr auto end() const noexcept -> iterator { return iterator(view_.end(), &func_); }
    [[nodiscard]] constexpr auto size() const noexcept -> std::size_t { return view_.size(); }
    [[nodiscard]] constexpr auto empty() const noexcept -> bool { return view_.empty(); }
};

/*!
 * \brief Create transform view
 */
template<typename CharT, typename Traits, typename F>
[[nodiscard]] constexpr auto transform(BasicStringView<CharT, Traits> sv, F&& f)
    noexcept -> sv_transform_view<CharT, Traits, std::decay_t<F>>
{
    return sv_transform_view<CharT, Traits, std::decay_t<F>>(sv, std::forward<F>(f));
}

/*!
 * \brief Filter view for string views
 */
template<typename CharT, typename Traits, typename Pred>
class sv_filter_view {
public:
    using string_view_type = BasicStringView<CharT, Traits>;
    
private:
    string_view_type view_;
    Pred pred_;
    
public:
    class iterator {
    private:
        typename string_view_type::const_iterator current_;
        typename string_view_type::const_iterator end_;
        const Pred* pred_;
        
        auto advance_to_next() noexcept -> void {
            while (current_ != end_ && !(*pred_)(*current_)) {
                ++current_;
            }
        }
        
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = CharT;
        using difference_type = std::ptrdiff_t;
        using pointer = const CharT*;
        using reference = const CharT&;
        
        constexpr iterator(typename string_view_type::const_iterator curr,
                          typename string_view_type::const_iterator end,
                          const Pred* p) noexcept
            : current_(curr), end_(end), pred_(p) {
            advance_to_next();
        }
        
        [[nodiscard]] constexpr auto operator*() const noexcept -> reference { return *current_; }
        [[nodiscard]] constexpr auto operator->() const noexcept -> pointer { return current_; }

        constexpr auto operator++() noexcept -> iterator& {
            ++current_;
            advance_to_next();
            return *this;
        }

        constexpr auto operator++(int) noexcept -> iterator {
            iterator tmp = *this;
            ++*this;
            return tmp;
        }
        
        [[nodiscard]] constexpr auto operator==(const iterator& other) const noexcept -> bool {
            return current_ == other.current_;
        }
        
        [[nodiscard]] constexpr auto operator!=(const iterator& other) const noexcept -> bool {
            return !(*this == other);
        }
    };
    
    constexpr sv_filter_view(string_view_type v, Pred p) noexcept : view_(v), pred_(std::move(p)) {}

    [[nodiscard]] constexpr auto begin() const noexcept -> iterator {
        return iterator(view_.begin(), view_.end(), &pred_);
    }

    [[nodiscard]] constexpr auto end() const noexcept -> iterator {
        return iterator(view_.end(), view_.end(), &pred_);
    }
};

/*!
 * \brief Create filter view
 */
template<typename CharT, typename Traits, typename Pred>
[[nodiscard]] constexpr auto filter(BasicStringView<CharT, Traits> sv, Pred&& p)
    noexcept -> sv_filter_view<CharT, Traits, std::decay_t<Pred>>
{
    return sv_filter_view<CharT, Traits, std::decay_t<Pred>>(sv, std::forward<Pred>(p));
}

/*!
 * \brief Take first n characters
 */
template<typename CharT, typename Traits>
[[nodiscard]] constexpr auto take(BasicStringView<CharT, Traits> sv, std::size_t n) noexcept
    -> BasicStringView<CharT, Traits>
{
    return sv.substr(0, (std::min)(n, sv.size()));
}

/*!
 * \brief Drop first n characters
 */
template<typename CharT, typename Traits>
[[nodiscard]] constexpr auto drop(BasicStringView<CharT, Traits> sv, std::size_t n) noexcept
    -> BasicStringView<CharT, Traits>
{
    return sv.substr((std::min)(n, sv.size()));
}

/*!
 * \brief Take characters while predicate is true
 */
template<typename CharT, typename Traits, typename Pred>
[[nodiscard]] constexpr auto take_while(BasicStringView<CharT, Traits> sv, Pred&& pred) noexcept
    -> BasicStringView<CharT, Traits>
{
    std::size_t count = 0;
    for (CharT ch : sv) {
        if (!pred(ch)) break;
        ++count;
    }
    return sv.substr(0, count);
}

/*!
 * \brief Drop characters while predicate is true
 */
template<typename CharT, typename Traits, typename Pred>
[[nodiscard]] constexpr auto drop_while(BasicStringView<CharT, Traits> sv, Pred&& pred) noexcept
    -> BasicStringView<CharT, Traits>
{
    std::size_t count = 0;
    for (CharT ch : sv) {
        if (!pred(ch)) break;
        ++count;
    }
    return sv.substr(count);
}

/*!
 * \brief Create string view from range
 */
template<typename Range,
         typename = std::enable_if_t<detail::is_string_view_compatible_v<
             Range, typename std::remove_pointer_t<
                 decltype(std::data(std::declval<const Range&>()))>>>>
[[nodiscard]] constexpr auto from_range(const Range& r) noexcept
{
    return BasicStringView(r);
}

} // namespace ranges

} // namespace core
} // namespace ara

/**********************************************************************************************************************
 *  COMPILE-TIME VERIFICATION
 *********************************************************************************************************************/
/*!
 * \brief Compile-time verification of AUTOSAR requirements for ara::core::StringView
 * 
 * \details These static assertions verify:
 * 
 * 1. **Memory layout optimization**: StringView uses only 16 bytes (pointer + size)
 *    matching std::string_view layout for zero-cost interoperability.
 * 
 * 2. **Type trait requirements**: StringView must be trivially copyable and destructible
 *    for efficient pass-by-value and deterministic performance.
 * 
 * 3. **Iterator requirements**: Iterators must be pointers for maximum efficiency
 *    and vectorization opportunities.
 * 
 * 4. **Safety features**: nullptr constructor is deleted preventing dangerous
 *    construction patterns common in automotive software.
 * 
 * 5. **Constexpr support**: All operations work in constexpr context for
 *    compile-time string processing and validation.
 */

namespace ara {
namespace core {

namespace sv_test {

// Test types
using TestStringView = StringView;
using TestWStringView = WStringView;

// Memory layout verification
static_assert(sizeof(TestStringView) == 2 * sizeof(void*), 
    "\n[ERROR] ara::core::StringView: "
    "StringView must store pointer and size only (16 bytes on 64-bit).\n");

static_assert(alignof(TestStringView) == alignof(void*),
    "\n[ERROR] ara::core::StringView: "
    "StringView must have pointer alignment.\n");

// Type trait verification
static_assert(std::is_trivially_copyable_v<TestStringView>,
    "\n[ERROR] ara::core::StringView: "
    "StringView must be trivially copyable.\n");

static_assert(std::is_trivially_destructible_v<TestStringView>,
    "\n[ERROR] ara::core::StringView: "
    "StringView must be trivially destructible.\n");

static_assert(std::is_nothrow_move_constructible_v<TestStringView>,
    "\n[ERROR] ara::core::StringView: "
    "StringView must be nothrow move constructible.\n");

// Iterator verification
static_assert(std::is_same_v<TestStringView::iterator, const char*>,
    "\n[ERROR] ara::core::StringView: "
    "StringView iterator must be const char*.\n");

static_assert(std::is_same_v<TestStringView::const_iterator, const char*>,
    "\n[ERROR] ara::core::StringView: "
    "StringView const_iterator must be const char*.\n");

// Safety verification - nullptr constructor deleted
static_assert(!std::is_constructible_v<TestStringView, std::nullptr_t>,
    "\n[ERROR] ara::core::StringView: "
    "StringView must not be constructible from nullptr.\n");

// Constexpr verification
constexpr const char test_data[] = "Hello, AUTOSAR!";
constexpr StringView test_sv(test_data);
static_assert(test_sv.size() == 15, "Size must work in constexpr");
static_assert(test_sv[7] == 'A', "Element access must work in constexpr");
static_assert(!test_sv.empty(), "Empty check must work in constexpr");
static_assert(test_sv.substr(7, 7) == "AUTOSAR", "Substr must work in constexpr");

// C++26 feature verification
static_assert(test_sv.starts_with("Hello"), "starts_with must work in constexpr");
static_assert(test_sv.ends_with("!"), "ends_with must work in constexpr");
static_assert(test_sv.contains("AUTOSAR"), "contains must work in constexpr");

// Comparison verification
constexpr StringView sv1 = "abc";
constexpr StringView sv2 = "def";
static_assert(sv1 < sv2, "Comparison must work in constexpr");
static_assert(sv1 != sv2, "Inequality must work in constexpr");

// Find operations verification
static_assert(test_sv.find("AUTOSAR")      == 7 , "Find must work in constexpr");
static_assert(test_sv.rfind('A')           == 12, "Rfind must work in constexpr");
static_assert(test_sv.find_first_of("AS")  == 7 , "Find_first_of must work in constexpr");

// Factory function verification
constexpr auto made_sv = MakeStringView("Test");
static_assert(made_sv.size() == 4, "MakeStringView must work in constexpr");

// User-defined literal verification
using namespace ara::core::literals::string_view_literals;
constexpr auto literal_sv = "Hello World!"_sv;
static_assert(literal_sv.size() == 12, "String literal must work in constexpr");
static_assert(literal_sv == "Hello World!", "String literal comparison must work");

// Ranges support verification
constexpr auto trimmed = ranges::trim("  test  "_sv);
static_assert(trimmed == "test", "Trim must work in constexpr");

// String literal comparison operators
static_assert("test"_sv == "test", "String literal equality must work");
static_assert("test" == "test"_sv, "Reverse string literal equality must work");
static_assert("abc"_sv < "def", "String literal less than must work");
static_assert("abc" < "def"_sv, "Reverse string literal less than must work");

} // sv_test namespace

} // namespace core
} // namespace ara

/**********************************************************************************************************************
 *  C++20 ranges: StringView is a borrowed_range
 *********************************************************************************************************************/
#if __cplusplus >= 202002L          
namespace std::ranges {
    template<class CharT, class Traits>
    inline constexpr bool enable_borrowed_range<ara::core::BasicStringView<CharT, Traits>> = true;
    
    template<class CharT, class Traits>
    inline constexpr bool enable_view<ara::core::BasicStringView<CharT, Traits>> = true;
}
#endif

#endif // OPEN_AA_ADAPTIVE_AUTOSAR_LIBS_INCLUDE_ARA_CORE_STRING_VIEW_H_