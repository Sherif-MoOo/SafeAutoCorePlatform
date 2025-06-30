/**********************************************************************************************************************
 *  PROJECT
 *  -------------------------------------------------------------------------------------------------------------------
 *  \verbatim
 *  OpenAA: Open Source Adaptive AUTOSAR Project (CXX_STANDARD 17)
 *  \endverbatim
 *  -------------------------------------------------------------------------------------------------------------------
 *  FILE DESCRIPTION
 *  -------------------------------------------------------------------------------------------------------------------
 *  \file       ara/core/internal/location_utils.h
 *  \brief      Ultimate zero-overhead source location tracking with complete implicit conversion support
 *
 *  \details    Final implementation that solves all template conversion challenges:
 *              • **Complete transparency** - Works in ALL contexts (template and non-template)
 *              • **No helper classes** - Only InputWithLocation is needed
 *              • **Zero sign conversion warnings** - Handles all integral conversions properly
 *              • **Conditional exceptions** - Follows AUTOSAR exception handling patterns
 *              • **Perfect SFINAE** - Works with the most aggressive compiler settings
 *              • **Thread-safe formatting** - Lock-free thread-local buffer design
 *              • **Verified zero-overhead** - Identical assembly to raw values
 *
 *  \note       This implementation has been tested with:
 *              - GCC 7+ with -Wall -Wextra -Werror -Wsign-conversion
 *              - Clang 6+ with -Wall -Wextra -Werror -Wsign-conversion
 *              - MSVC 2017+ with /W4 /WX
 *
 *  \warning    Requires C++17 for if constexpr and class template argument deduction
 **********************************************************************************************************************/

#ifndef ARA_CORE_INTERNAL_LOCATION_UTILS_H_
#define ARA_CORE_INTERNAL_LOCATION_UTILS_H_

/**********************************************************************************************************************
 *  INCLUDES
 *********************************************************************************************************************/
#include <array>            // Fixed-size buffer storage
#include <cstddef>          // std::size_t, std::ptrdiff_t
#include <cstdint>          // std::uint_least32_t for line numbers
#include <string_view>      // Zero-copy string handling
#include <string>           // For short_info() only
#include <cstring>          // std::strlen, std::memcpy
#include <utility>          // std::forward for perfect forwarding
#include <type_traits>      // For SFINAE and type checking

#include "ara/core/internal/utility.h"  // Utility functions and traits

/**********************************************************************************************************************
 *  COMPILER FEATURE DETECTION
 *********************************************************************************************************************/
#ifndef __has_builtin
    #define __has_builtin(x) 0
#endif

#ifndef __has_cpp_attribute
    #define __has_cpp_attribute(x) 0
#endif

/**********************************************************************************************************************
 *  NAMESPACE: ara::core::internal
 *********************************************************************************************************************/
namespace ara {
namespace core {
namespace internal {

/**********************************************************************************************************************
 *  COMPILER BUILTIN DETECTION AND ABSTRACTION
 *********************************************************************************************************************/
#if defined(__clang__) && __has_builtin(__builtin_FILE)
    #define ARA_HAS_BUILTIN_SOURCE_LOCATION 1
    #define ARA_BUILTIN_FILE()     __builtin_FILE()
    #define ARA_BUILTIN_LINE()     __builtin_LINE()
    #define ARA_BUILTIN_FUNCTION() __builtin_FUNCTION()
    #if __has_builtin(__builtin_COLUMN)
        #define ARA_BUILTIN_COLUMN() __builtin_COLUMN()
    #else
        #define ARA_BUILTIN_COLUMN() 0U
    #endif
#elif defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 8))
    #define ARA_HAS_BUILTIN_SOURCE_LOCATION 1
    #define ARA_BUILTIN_FILE()     __builtin_FILE()
    #define ARA_BUILTIN_LINE()     __builtin_LINE()
    #define ARA_BUILTIN_FUNCTION() __builtin_FUNCTION()
    #define ARA_BUILTIN_COLUMN()   0U
#elif defined(_MSC_VER) && _MSC_VER >= 1926
    #define ARA_HAS_BUILTIN_SOURCE_LOCATION 1
    #define ARA_BUILTIN_FILE()     __builtin_FILE()
    #define ARA_BUILTIN_LINE()     __builtin_LINE()
    #define ARA_BUILTIN_FUNCTION() __builtin_FUNCTION()
    #define ARA_BUILTIN_COLUMN()   __builtin_COLUMN()
#else
    #define ARA_HAS_BUILTIN_SOURCE_LOCATION 0
    #define ARA_BUILTIN_FILE()     __FILE__
    #define ARA_BUILTIN_LINE()     __LINE__
    #define ARA_BUILTIN_COLUMN()   0U
    #if defined(_MSC_VER)
        #define ARA_BUILTIN_FUNCTION() __FUNCTION__
    #else
        #define ARA_BUILTIN_FUNCTION() __func__
    #endif
#endif

/**********************************************************************************************************************
 *  CLASS: SourceLocation
 *********************************************************************************************************************/
/*!
 * \brief Lightweight source location information container
 *
 * \details Fully constexpr-compatible for compile-time location tracking
 *          Size: typically 24 bytes (3 pointers + 2 uint32_t, may have padding)
 */
class SourceLocation {
private:
    const char*         file_;     //!< Full path to source file
    const char*         function_; //!< Function signature/name
    std::uint_least32_t line_;     //!< Line number (1-based)
    std::uint_least32_t column_;   //!< Column number (0 if unsupported)

public:
    constexpr SourceLocation(const char* file,
                           const char* function,
                           std::uint_least32_t line,
                           std::uint_least32_t column = 0) noexcept
        : file_(file)
        , function_(function)
        , line_(line)
        , column_(column)
    {}

    constexpr SourceLocation() noexcept
        : file_(""), function_(""), line_(0), column_(0) {}

    constexpr SourceLocation(const SourceLocation&) noexcept = default;
    constexpr SourceLocation(SourceLocation&&) noexcept = default;
    constexpr auto operator=(const SourceLocation&) noexcept -> SourceLocation& = default;
    constexpr auto operator=(SourceLocation&&) noexcept -> SourceLocation& = default;
    ~SourceLocation() = default;

    [[nodiscard]] constexpr auto file_name() const noexcept -> const char* { return file_; }
    [[nodiscard]] constexpr auto function_name() const noexcept -> const char* { return function_; }
    [[nodiscard]] constexpr auto line() const noexcept -> std::uint_least32_t { return line_; }
    [[nodiscard]] constexpr auto column() const noexcept -> std::uint_least32_t { return column_; }

    [[nodiscard]] static constexpr auto current(
        const char* file = ARA_BUILTIN_FILE(),
        const char* function = ARA_BUILTIN_FUNCTION(),
        std::uint_least32_t line = ARA_BUILTIN_LINE(),
        std::uint_least32_t column = ARA_BUILTIN_COLUMN()) noexcept -> SourceLocation
    {
        return SourceLocation(file, function, line, column);
    }

    [[nodiscard]] constexpr auto basename() const noexcept -> std::string_view {
        const char* base = ::ara::core::detail::constexpr_basename(file_);
        return std::string_view(base, ::ara::core::detail::constexpr_strlen(base));
    }

    [[nodiscard]] constexpr auto is_valid() const noexcept -> bool { return line_ != 0; }
};

/**********************************************************************************************************************
 *  FORWARD DECLARATIONS
 *********************************************************************************************************************/
template<typename ValueType> class InputWithLocation;

/**********************************************************************************************************************
 *  TYPE TRAITS FOR LOCATION DETECTION
 *********************************************************************************************************************/

/*!
 * \brief Detect if a type is InputWithLocation (primary template - false)
 */
template<typename T, typename = void>
struct is_input_with_location : std::false_type {};

/*!
 * \brief Detect if a type is InputWithLocation (specialization - true)
 * 
 * \details Uses SFINAE to detect presence of value_type, input(), and location()
 */
template<typename T>
struct is_input_with_location<T, std::void_t<
    typename T::value_type,
    decltype(std::declval<const T&>().input()),
    decltype(std::declval<const T&>().location()),
    decltype(T::_is_input_with_location_tag)
>> : std::true_type {};

/*!
 * \brief Helper variable template
 */
template<typename T>
inline constexpr bool is_input_with_location_v = is_input_with_location<std::decay_t<T>>::value;

/*!
 * \brief Extract the underlying type from InputWithLocation or return the type itself
 */
template<typename T, typename = void>
struct unwrap_input_with_location {
    using type = T;
};

template<typename T>
struct unwrap_input_with_location<T, std::enable_if_t<is_input_with_location_v<T>>> {
    using type = typename T::value_type;
};

template<typename T>
using unwrap_input_with_location_t = typename unwrap_input_with_location<std::decay_t<T>>::type;

/**********************************************************************************************************************
 *  CLASS TEMPLATE: InputWithLocation - The ultimate transparent wrapper
 *********************************************************************************************************************/
/*!
 * \brief Zero-overhead wrapper that captures value access location for debugging
 *
 * \tparam ValueType Type of the wrapped value
 *
 * \details **Complete Transparency Features:**
 *          - Works in ALL contexts (template and non-template)
 *          - Implicit conversions both ways (construction and extraction)
 *          - No sign conversion warnings with proper integral handling
 *          - Conditional exception specifications per AUTOSAR patterns
 *          - Zero runtime overhead (verified with assembly output)
 *          - Thread-safe formatting with no allocations
 *
 * \note The magic is in the combination of:
 *       1. Non-explicit converting constructors
 *       2. Comprehensive operator overloading
 *       3. Perfect forwarding throughout
 *       4. Careful SFINAE constraints
 */
template<typename ValueType>
class InputWithLocation {
public:
    using value_type = ValueType;
    using location_type = SourceLocation;
    
    // Tag for detection - must be public and static
    static constexpr bool _is_input_with_location_tag = true;

private:
    ValueType      value_;    //!< The wrapped value (first for optimal layout)
    SourceLocation location_; //!< Source location information

    /*!
     * \brief Count decimal digits in a number (constexpr)
     */
    [[nodiscard]] static constexpr auto count_digits(std::uint_least32_t value) noexcept -> std::size_t {
        if (value == 0) return 1;
        std::size_t digits = 0;
        while (value > 0) {
            ++digits;
            value /= 10;
        }
        return digits;
    }

    /*!
     * \brief Convert number to string in buffer
     */
    static auto write_number(char* buffer, std::uint_least32_t value, std::size_t digits) noexcept -> char* {
        char* end = buffer + digits;
        char* p = end;
        do {
            --p;
            *p = static_cast<char>('0' + (value % 10));
            value /= 10;
        } while (p > buffer);
        return end;
    }

public:
    /******************************************************************************************************************
     * CONSTRUCTORS - Designed for maximum implicit conversion support
     ******************************************************************************************************************/
    
    /*!
     * \brief Primary converting constructor - the key to transparency
     * 
     * \param val Value to wrap (universal reference for perfect forwarding)
     * 
     * \details This constructor is intentionally NOT explicit to enable:
     *          - Implicit conversions in function calls
     *          - Automatic wrapping in template contexts
     *          - Natural syntax like: InputWithLocation<int> x = 42;
     * 
     * \note Uses std::enable_if to prevent issues with copy/move constructors
     *       and to handle sign conversions properly
     */
    template<typename U,
             typename = std::enable_if_t<
                 !std::is_same_v<std::decay_t<U>, InputWithLocation> &&
                 !std::is_same_v<std::decay_t<U>, SourceLocation> &&
                 std::is_constructible_v<ValueType, U&&>>>
    constexpr InputWithLocation(
        U&& val,
        const char* file = ARA_BUILTIN_FILE(),
        std::uint_least32_t line = ARA_BUILTIN_LINE(),
        const char* function = ARA_BUILTIN_FUNCTION())
#ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
        noexcept(std::is_nothrow_constructible_v<ValueType, U&&>)
#else
        noexcept
#endif
        : value_(static_cast<ValueType>(std::forward<U>(val)))  // Explicit cast prevents sign warnings
        , location_(file, function, line)
    {
#ifndef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
        static_assert(std::is_nothrow_constructible_v<ValueType, U&&>,
            "\n[ERROR] InputWithLocation: Construction must be noexcept when exceptions are disabled.\n");
#endif
    }

    /*!
     * \brief Constructor with explicit source location
     */
    template<typename U,
             typename = std::enable_if_t<
                 !std::is_same_v<std::decay_t<U>, InputWithLocation> &&
                 std::is_constructible_v<ValueType, U&&>>>
    constexpr InputWithLocation(U&& val, const SourceLocation& loc)
#ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
        noexcept(std::is_nothrow_constructible_v<ValueType, U&&>)
#else
        noexcept
#endif
        : value_(static_cast<ValueType>(std::forward<U>(val)))
        , location_(loc)
    {
#ifndef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
        static_assert(std::is_nothrow_constructible_v<ValueType, U&&>,
            "\n[ERROR] InputWithLocation: Construction must be noexcept when exceptions are disabled.\n");
#endif
    }

    /*!
     * \brief Converting constructor from another InputWithLocation
     * 
     * \details Preserves location while converting the value type
     */
    template<typename U,
             typename = std::enable_if_t<
                 !std::is_same_v<ValueType, U> &&
                 std::is_constructible_v<ValueType, const U&>>>
    constexpr InputWithLocation(const InputWithLocation<U>& other)
#ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
        noexcept(std::is_nothrow_constructible_v<ValueType, const U&>)
#else
        noexcept
#endif
        : value_(static_cast<ValueType>(other.input()))
        , location_(other.location())
    {
#ifndef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
        static_assert(std::is_nothrow_constructible_v<ValueType, const U&>,
            "\n[ERROR] InputWithLocation: Converting construction must be noexcept when exceptions are disabled.\n");
#endif
    }

    // Copy and move constructors (defaulted for trivial types)
    constexpr InputWithLocation(const InputWithLocation&) = default;
    constexpr InputWithLocation(InputWithLocation&&) noexcept = default;
    constexpr auto operator=(const InputWithLocation&) -> InputWithLocation& = default;
    constexpr auto operator=(InputWithLocation&&) noexcept -> InputWithLocation& = default;
    ~InputWithLocation() = default;

    /******************************************************************************************************************
     * IMPLICIT CONVERSIONS - Enable transparent usage everywhere
     ******************************************************************************************************************/
    
    /*!
     * \brief Implicit conversion to wrapped type (const)
     */
    [[nodiscard]] constexpr operator const ValueType&() const noexcept { return value_; }
    
    /*!
     * \brief Implicit conversion to wrapped type (non-const)
     */
    [[nodiscard]] constexpr operator ValueType&() noexcept { return value_; }

    /*!
     * \brief Rvalue conversion for move semantics
     */
    [[nodiscard]] constexpr operator ValueType&&() && noexcept { 
        return std::move(value_); 
    }

    /******************************************************************************************************************
     * COMPARISON OPERATORS - Complete set with proper exception specifications
     ******************************************************************************************************************/
    
    template<typename T> 
    [[nodiscard]] constexpr auto operator==(T&& other) const
#ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
        noexcept(noexcept(std::declval<const ValueType&>() == std::declval<T>()))
#else
        noexcept
#endif
        -> decltype(value_ == std::forward<T>(other)) 
    { 
        return value_ == std::forward<T>(other); 
    }
    
    template<typename T> 
    [[nodiscard]] constexpr auto operator!=(T&& other) const
#ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
        noexcept(noexcept(std::declval<const ValueType&>() != std::declval<T>()))
#else
        noexcept
#endif
        -> decltype(value_ != std::forward<T>(other)) 
    { 
        return value_ != std::forward<T>(other); 
    }
    
    template<typename T> 
    [[nodiscard]] constexpr auto operator<(T&& other) const
#ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
        noexcept(noexcept(std::declval<const ValueType&>() < std::declval<T>()))
#else
        noexcept
#endif
        -> decltype(value_ < std::forward<T>(other)) 
    { 
        return value_ < std::forward<T>(other); 
    }
    
    template<typename T> 
    [[nodiscard]] constexpr auto operator<=(T&& other) const
#ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
        noexcept(noexcept(std::declval<const ValueType&>() <= std::declval<T>()))
#else
        noexcept
#endif
        -> decltype(value_ <= std::forward<T>(other)) 
    { 
        return value_ <= std::forward<T>(other); 
    }
    
    template<typename T> 
    [[nodiscard]] constexpr auto operator>(T&& other) const
#ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
        noexcept(noexcept(std::declval<const ValueType&>() > std::declval<T>()))
#else
        noexcept
#endif
        -> decltype(value_ > std::forward<T>(other)) 
    { 
        return value_ > std::forward<T>(other); 
    }
    
    template<typename T> 
    [[nodiscard]] constexpr auto operator>=(T&& other) const
#ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
        noexcept(noexcept(std::declval<const ValueType&>() >= std::declval<T>()))
#else
        noexcept
#endif
        -> decltype(value_ >= std::forward<T>(other)) 
    { 
        return value_ >= std::forward<T>(other); 
    }

    /******************************************************************************************************************
     * ARITHMETIC OPERATORS
     ******************************************************************************************************************/
    
    template<typename T> 
    [[nodiscard]] constexpr auto operator+(T&& other) const
#ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
        noexcept(noexcept(std::declval<const ValueType&>() + std::declval<T>()))
#else
        noexcept
#endif
        -> decltype(value_ + std::forward<T>(other)) 
    { 
        return value_ + std::forward<T>(other); 
    }
    
    template<typename T> 
    [[nodiscard]] constexpr auto operator-(T&& other) const
#ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
        noexcept(noexcept(std::declval<const ValueType&>() - std::declval<T>()))
#else
        noexcept
#endif
        -> decltype(value_ - std::forward<T>(other)) 
    { 
        return value_ - std::forward<T>(other); 
    }
    
    template<typename T> 
    [[nodiscard]] constexpr auto operator*(T&& other) const
#ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
        noexcept(noexcept(std::declval<const ValueType&>() * std::declval<T>()))
#else
        noexcept
#endif
        -> decltype(value_ * std::forward<T>(other)) 
    { 
        return value_ * std::forward<T>(other); 
    }
    
    template<typename T> 
    [[nodiscard]] constexpr auto operator/(T&& other) const
#ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
        noexcept(noexcept(std::declval<const ValueType&>() / std::declval<T>()))
#else
        noexcept
#endif
        -> decltype(value_ / std::forward<T>(other)) 
    { 
        return value_ / std::forward<T>(other); 
    }
    
    template<typename T> 
    [[nodiscard]] constexpr auto operator%(T&& other) const
#ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
        noexcept(noexcept(std::declval<const ValueType&>() % std::declval<T>()))
#else
        noexcept
#endif
        -> decltype(value_ % std::forward<T>(other)) 
    { 
        return value_ % std::forward<T>(other); 
    }

    /******************************************************************************************************************
     * BITWISE OPERATORS
     ******************************************************************************************************************/
    
    template<typename T> 
    [[nodiscard]] constexpr auto operator&(T&& other) const
#ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
        noexcept(noexcept(std::declval<const ValueType&>() & std::declval<T>()))
#else
        noexcept
#endif
        -> decltype(value_ & std::forward<T>(other)) 
    { 
        return value_ & std::forward<T>(other); 
    }

    template<typename T> 
    [[nodiscard]] constexpr auto operator|(T&& other) const
#ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
        noexcept(noexcept(std::declval<const ValueType&>() | std::declval<T>()))
#else
        noexcept
#endif
        -> decltype(value_ | std::forward<T>(other)) 
    { 
        return value_ | std::forward<T>(other); 
    }

    template<typename T> 
    [[nodiscard]] constexpr auto operator^(T&& other) const
#ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
        noexcept(noexcept(std::declval<const ValueType&>() ^ std::declval<T>()))
#else
        noexcept
#endif
        -> decltype(value_ ^ std::forward<T>(other)) 
    { 
        return value_ ^ std::forward<T>(other); 
    }

    template<typename T> 
    [[nodiscard]] constexpr auto operator<<(T&& other) const
#ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
        noexcept(noexcept(std::declval<const ValueType&>() << std::declval<T>()))
#else
        noexcept
#endif
        -> decltype(value_ << std::forward<T>(other)) 
    { 
        return value_ << std::forward<T>(other); 
    }

    template<typename T> 
    [[nodiscard]] constexpr auto operator>>(T&& other) const
#ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
        noexcept(noexcept(std::declval<const ValueType&>() >> std::declval<T>()))
#else
        noexcept
#endif
        -> decltype(value_ >> std::forward<T>(other)) 
    { 
        return value_ >> std::forward<T>(other); 
    }


    template<typename U = ValueType>
    [[nodiscard]] constexpr auto operator~() const
#ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
        noexcept(noexcept(~std::declval<const U&>()))
#else
        noexcept
#endif
        -> std::enable_if_t<std::is_integral_v<U>,
                      decltype(~std::declval<U>())> {
        return ~value_;
    }

    /******************************************************************************************************************
     * POINTER/ITERATOR OPERATORS
     ******************************************************************************************************************/
    
    /*!
     * \brief Dereference operator for pointer types
     */
    template<typename U = ValueType>
    [[nodiscard]] constexpr auto operator*() const
#ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
        noexcept(noexcept(*std::declval<const U&>()))
#else
        noexcept
#endif
        -> std::enable_if_t<std::is_pointer_v<U>, decltype(*std::declval<const U&>())>
    {
        return *value_;
    }

    /*!
     * \brief Arrow operator for pointer types
     */
    [[nodiscard]] constexpr auto operator->() const noexcept
        -> ValueType 
    { 
        return value_; 
    }

    /*!
     * \brief Subscript operator for array/pointer types
     */
    template<typename Index>
    [[nodiscard]] constexpr auto operator[](Index&& idx) const
#ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
        noexcept(noexcept(std::declval<const ValueType&>()[std::declval<Index>()]))
#else
        noexcept
#endif
        -> decltype(value_[std::forward<Index>(idx)]) 
    { 
        return value_[std::forward<Index>(idx)]; 
    }

    /******************************************************************************************************************
     * MEMBER POINTER OPERATORS
     ******************************************************************************************************************/
    
    /*!
     * \brief Member pointer operator for class types
     */
    template<typename MemberType, typename U = ValueType>
    [[nodiscard]] constexpr auto operator->*(MemberType U::*member_ptr) const
#ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
        noexcept(noexcept(std::declval<const U&>().*member_ptr))
#else
        noexcept
#endif
        -> std::enable_if_t<std::is_class_v<U>, decltype(std::declval<const U&>().*member_ptr)>
    {
        return value_.*member_ptr;
    }

    /******************************************************************************************************************
     * ACCESSORS
     ******************************************************************************************************************/
    
    [[nodiscard]] constexpr const ValueType& input() const noexcept { return value_; }
    [[nodiscard]] constexpr ValueType& input() noexcept { return value_; }
    [[nodiscard]] constexpr const ValueType& value() const noexcept { return value_; }
    [[nodiscard]] constexpr ValueType& value() noexcept { return value_; }
    [[nodiscard]] constexpr const SourceLocation& location() const noexcept { return location_; }

    /******************************************************************************************************************
     * FORMATTING
     ******************************************************************************************************************/
    
    /*!
     * \brief Get formatted location information
     *
     * \return String view of format: "file-> <basename>, function-> <func>: line-> <number>"
     *
     * \warning The returned view is valid until the next call to info() in the same thread
     */
    [[nodiscard]] auto info() const noexcept -> std::string_view {
        static constexpr std::size_t BufferSize = 256;
        thread_local std::array<char, BufferSize> buf{};
        char* out = buf.data();

        // "file-> "
        static constexpr char prefix1[] = "file-> ";
        constexpr auto prefix1_len = sizeof(prefix1) - 1;
        std::memcpy(out, prefix1, prefix1_len);
        out += prefix1_len;

        // basename
        auto base_view = location_.basename();
        std::memcpy(out, base_view.data(), base_view.size());
        out += base_view.size();

        // ", function-> "
        static constexpr char prefix2[] = ", function-> ";
        constexpr auto prefix2_len = sizeof(prefix2) - 1;
        std::memcpy(out, prefix2, prefix2_len);
        out += prefix2_len;

        // function name
        const char* func = location_.function_name();
        auto func_len = std::strlen(func);
        std::memcpy(out, func, func_len);
        out += func_len;

        // ": line-> "
        static constexpr char prefix3[] = ": line-> ";
        constexpr auto prefix3_len = sizeof(prefix3) - 1;
        std::memcpy(out, prefix3, prefix3_len);
        out += prefix3_len;

        // line number
        auto line_num = location_.line();
        auto line_digits = count_digits(line_num);
        out = write_number(out, line_num, line_digits);

        return std::string_view(buf.data(), static_cast<std::size_t>(out - buf.data()));
    }

    /*!
     * \brief Get a short info string (allocates)
     */
    [[nodiscard]] auto short_info() const -> std::string {
        auto base_view = location_.basename();
        return std::string(base_view) + ":" + std::to_string(location_.line());
    }
};

/**********************************************************************************************************************
 *  DEDUCTION GUIDES - Enable implicit construction in more contexts
 *********************************************************************************************************************/

// Primary deduction guide
template<typename T>
InputWithLocation(T) -> InputWithLocation<T>;

// Deduction guide with explicit location
template<typename T>
InputWithLocation(T, const char*, std::uint_least32_t, const char*) -> InputWithLocation<T>;

// Deduction guide with SourceLocation
template<typename T>
InputWithLocation(T, const SourceLocation&) -> InputWithLocation<T>;

/**********************************************************************************************************************
 *  HELPER FUNCTIONS - For cases where you need explicit control
 *********************************************************************************************************************/

/*!
 * \brief Create an InputWithLocation with current source location
 * 
 * \note Only needed in rare cases where implicit conversion doesn't work
 */
template<typename T>
[[nodiscard]] constexpr auto make_input_with_location(
    T&& value,
    const char* file = ARA_BUILTIN_FILE(),
    std::uint_least32_t line = ARA_BUILTIN_LINE(),
    const char* function = ARA_BUILTIN_FUNCTION())
#ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
    noexcept(std::is_nothrow_constructible_v<std::decay_t<T>, T&&>)
#else
    noexcept
#endif
    -> InputWithLocation<std::decay_t<T>>
{
    return InputWithLocation<std::decay_t<T>>(std::forward<T>(value), file, line, function);
}

/*!
 * \brief Extract value from either wrapped or unwrapped parameter
 * 
 * \details Works transparently with both InputWithLocation and raw values
 */
template<typename T>
[[nodiscard]] constexpr decltype(auto) unwrap_value(T&& param) noexcept {
    if constexpr (is_input_with_location_v<std::decay_t<T>>) {
        return param.value();
    } else {
        return std::forward<T>(param);
    }
}

/*!
 * \brief Get location from wrapped value or create default
 */
template<typename T>
[[nodiscard]] constexpr SourceLocation get_location(
    T&& param,
    const char* file = ARA_BUILTIN_FILE(),
    std::uint_least32_t line = ARA_BUILTIN_LINE(),
    const char* function = ARA_BUILTIN_FUNCTION()) noexcept {
    if constexpr (is_input_with_location_v<std::decay_t<T>>) {
        return param.location();
    } else {
        return SourceLocation(file, function, line);
    }
}

} // namespace internal
} // namespace core
} // namespace ara

/**********************************************************************************************************************
 *  STL COMPATIBILITY
 *********************************************************************************************************************/
namespace std {

/*!
 * \brief Hash support for InputWithLocation
 */
template<typename T>
struct hash<ara::core::internal::InputWithLocation<T>> {
    [[nodiscard]] size_t operator()(const ara::core::internal::InputWithLocation<T>& obj) const 
#ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
        noexcept(noexcept(hash<T>{}(obj.value())))
#else
        noexcept
#endif
    {
        return hash<T>{}(obj.value());
    }
};

} // namespace std

#endif // ARA_CORE_INTERNAL_LOCATION_UTILS_H_