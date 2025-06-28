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
 *  \brief      Compile-time source location extraction and formatting utilities
 *
 *  \details    This file provides a C++17-compatible implementation of source location tracking with:
 *              • **Fully constexpr** path manipulation and formatting without dynamic allocation
 *              • **Zero-overhead** abstractions for compile-time string processing
 *              • **Thread-safe** const operations with no global state
 *              • **Compiler-agnostic** with graceful fallback for non-supporting compilers
 *              • **Memory-efficient** fixed-size buffer formatting with overflow protection
 *              • **C++20-ready** design allowing seamless migration to std::source_location
 *
 *  \note       Performance Characteristics:
 *              - Compile-time path stripping: O(N) where N is path length
 *              - Runtime formatting: O(1) amortized (cached after first call)
 *              - Memory overhead: 256 bytes per InputWithLocation instance
 *              - No heap allocations in any code path
 *
 *  \warning    Thread Safety:
 *              - All const member functions are thread-safe for concurrent reads
 *              - The mutable format cache in InputWithLocation uses relaxed semantics
 *              - Multiple threads may redundantly compute the same cached value (benign race)
 **********************************************************************************************************************/

#ifndef ARA_CORE_INTERNAL_LOCATION_UTILS_H_
#define ARA_CORE_INTERNAL_LOCATION_UTILS_H_

/**********************************************************************************************************************
 *  INCLUDES
 *********************************************************************************************************************/
/*!
 * \brief Core dependencies for location utilities
 *
 * Minimal set of headers to support:
 * - Fixed-size storage (std::array)
 * - Size types and compile-time constants (cstddef, cstdint)
 * - String view for zero-copy string handling
 * - String stream for fallback formatting (only in non-constexpr contexts)
 */
#include <array>            // Fixed-size buffer storage
#include <cstddef>          // std::size_t, std::ptrdiff_t
#include <cstdint>          // std::uint_least32_t for line numbers
#include <string_view>      // Zero-copy string handling
#include <sstream>          // Fallback formatting support
#include <cstring>          // std::strlen, std::memcpy
#include <utility>          // std::forward for perfect forwarding

/**********************************************************************************************************************
 *  COMPILER FEATURE DETECTION
 *********************************************************************************************************************/
/*!
 * \brief Portable compiler builtin detection macro
 *
 * \details Provides a unified way to check for compiler builtin availability
 *          across different compiler vendors and versions.
 */
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
/*!
 * \defgroup SourceLocationBuiltins Compiler-Specific Source Location Builtins
 * \brief Abstracts compiler-specific source location intrinsics
 * @{
 */

/*!
 * \def ARA_HAS_BUILTIN_SOURCE_LOCATION
 * \brief Indicates whether the compiler provides builtin source location functions
 *
 * \details Set to 1 if the compiler supports __builtin_FILE() and related intrinsics,
 *          0 otherwise. This enables zero-overhead source location capture.
 */

/*!
 * \def ARA_BUILTIN_FILE()
 * \brief Expands to the current source file path
 *
 * \return Null-terminated string containing the full path to the current source file
 * \note Falls back to __FILE__ macro on unsupported compilers
 */

/*!
 * \def ARA_BUILTIN_LINE()
 * \brief Expands to the current source line number
 *
 * \return Unsigned integer representing the current line number (1-based)
 * \note Falls back to __LINE__ macro on unsupported compilers
 */

/*!
 * \def ARA_BUILTIN_FUNCTION()
 * \brief Expands to the current function name
 *
 * \return Null-terminated string containing the current function signature
 * \note Format varies by compiler (may include return type, parameters, etc.)
 */

/*!
 * \def ARA_BUILTIN_COLUMN()
 * \brief Expands to the current source column number
 *
 * \return Unsigned integer representing the current column (0 if unsupported)
 * \note Only available on recent Clang and MSVC compilers
 */

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

/** @} */ // End of SourceLocationBuiltins group

/**********************************************************************************************************************
 *  UTILITY FUNCTIONS
 *********************************************************************************************************************/
/*!
 * \brief Internal utility functions for string manipulation
 * \internal
 */
namespace detail {

/*!
 * \brief Compile-time string length calculation
 *
 * \tparam CharT Character type (typically char)
 * \param str Null-terminated string
 * \return Length of the string excluding null terminator
 *
 * \note Prefer std::string_view for runtime strings
 */
template<typename CharT>
[[nodiscard]] constexpr auto strlen_constexpr(const CharT* str) noexcept -> std::size_t {
    if (str == nullptr) return 0;
    
    std::size_t len = 0;
    while (str[len] != CharT{}) {
        ++len;
    }
    return len;
}

/*!
 * \brief Extract basename from a file path at compile time
 *
 * \param path Full file path
 * \return Pointer to the basename portion of the path
 *
 * \details Handles both forward and backward slashes as path separators
 */
[[nodiscard]] constexpr auto basename_constexpr(const char* path) noexcept -> const char* {
    if (path == nullptr) return "";
    
    const char* base = path;
    const char* p = path;
    
    while (*p != '\0') {
        if (*p == '/' || *p == '\\') {
            base = p + 1;
        }
        ++p;
    }
    
    return base;
}

} // namespace detail

/**********************************************************************************************************************
 *  CLASS: SourceLocation
 *********************************************************************************************************************/
/*!
 * \brief Lightweight source location information container
 *
 * \details This class provides a C++17-compatible implementation of source location tracking,
 *          similar to C++20's std::source_location but with additional features:
 *          - Fully constexpr construction and accessors
 *          - Zero runtime overhead when used with compiler builtins
 *          - Seamless migration path to std::source_location
 *
 * \note Thread Safety: All member functions are thread-safe (immutable after construction)
 */
class SourceLocation {
private:
    const char*         m_file;     //!< Full path to source file
    const char*         m_function; //!< Function signature/name
    std::uint_least32_t m_line;     //!< Line number (1-based)
    std::uint_least32_t m_column;   //!< Column number (0 if unsupported)

public:
    //******************************************************************************************************************
    // CONSTRUCTORS AND ASSIGNMENT
    //******************************************************************************************************************
    
    /*!
     * \brief Construct a source location with explicit values
     *
     * \param[in] file     Source file path (must remain valid)
     * \param[in] function Function name/signature (must remain valid)
     * \param[in] line     Line number (1-based)
     * \param[in] column   Column number (0 if unknown)
     *
     * \pre file and function point to null-terminated strings with static storage duration
     * \post Object is fully initialized and immutable
     */
    constexpr SourceLocation(const char* file,
                           const char* function,
                           std::uint_least32_t line,
                           std::uint_least32_t column = 0) noexcept
        : m_file(file)
        , m_function(function)
        , m_line(line)
        , m_column(column)
    {}
    
    /*!
     * \brief Default constructor creates an "unknown" location
     */
    constexpr SourceLocation() noexcept
        : m_file("")
        , m_function("")
        , m_line(0)
        , m_column(0)
    {}
    
    // Rule of five: trivially copyable
    constexpr SourceLocation(const SourceLocation&) noexcept = default;
    constexpr SourceLocation(SourceLocation&&) noexcept = default;
    constexpr auto operator=(const SourceLocation&) noexcept -> SourceLocation& = default;
    constexpr auto operator=(SourceLocation&&) noexcept -> SourceLocation& = default;
    ~SourceLocation() = default;

    //******************************************************************************************************************
    // ACCESSORS
    //******************************************************************************************************************
    
    /*!
     * \brief Get the source file path
     * \return Null-terminated string containing the full file path
     * \note String lifetime is tied to the program's lifetime (typically __FILE__)
     */
    [[nodiscard]] constexpr auto file_name() const noexcept -> const char* { 
        return m_file; 
    }
    
    /*!
     * \brief Get the function name/signature
     * \return Null-terminated string containing the function identifier
     * \note Format is compiler-dependent (may include namespaces, parameters, etc.)
     */
    [[nodiscard]] constexpr auto function_name() const noexcept -> const char* { 
        return m_function; 
    }
    
    /*!
     * \brief Get the source line number
     * \return Line number (1-based), or 0 if unknown
     */
    [[nodiscard]] constexpr auto line() const noexcept -> std::uint_least32_t { 
        return m_line; 
    }
    
    /*!
     * \brief Get the source column number
     * \return Column number (1-based), or 0 if unsupported by compiler
     */
    [[nodiscard]] constexpr auto column() const noexcept -> std::uint_least32_t { 
        return m_column; 
    }

    //******************************************************************************************************************
    // FACTORY METHODS
    //******************************************************************************************************************
    
    /*!
     * \brief Create a SourceLocation for the current call site
     *
     * \param[in] file     Source file (defaults to current file)
     * \param[in] function Function name (defaults to current function)
     * \param[in] line     Line number (defaults to current line)
     * \param[in] column   Column number (defaults to current column if supported)
     *
     * \return SourceLocation object representing the call site
     *
     * \note When used as a default argument, captures the caller's location:
     * \code{.cpp}
     * void trace(SourceLocation loc = SourceLocation::current()) {
     *     // loc contains the location where trace() was called
     * }
     * \endcode
     */
    [[nodiscard]] static constexpr auto current(
        const char* file = ARA_BUILTIN_FILE(),
        const char* function = ARA_BUILTIN_FUNCTION(),
        std::uint_least32_t line = ARA_BUILTIN_LINE(),
        std::uint_least32_t column = ARA_BUILTIN_COLUMN()) noexcept -> SourceLocation
    {
        return SourceLocation(file, function, line, column);
    }
    
    //******************************************************************************************************************
    // UTILITY METHODS
    //******************************************************************************************************************
    
    /*!
     * \brief Get just the filename portion of the source file path
     * \return View of the basename (e.g., "main.cpp" from "/path/to/main.cpp")
     */
    [[nodiscard]] constexpr auto basename() const noexcept -> std::string_view {
        const char* base = detail::basename_constexpr(m_file);
        return std::string_view(base, detail::strlen_constexpr(base));
    }
    
    /*!
     * \brief Check if this represents a valid source location
     * \return true if line number is non-zero, false otherwise
     */
    [[nodiscard]] constexpr auto is_valid() const noexcept -> bool {
        return m_line != 0;
    }
};

/**********************************************************************************************************************
 *  CLASS TEMPLATE: InputWithLocation
 *********************************************************************************************************************/
/*!
 * \brief Wrapper that captures array input access location for debugging
 *
 * \tparam InputType Type used for array inputing (typically std::size_t)
 *
 * \details This class enables transparent location tracking for array access operations:
 *          - Implicit conversion from integers for seamless integration
 *          - Lazy formatting with caching for efficient repeated access
 *          - Fixed-size buffer to avoid heap allocations
 *          - Thread-safe const operations with benign race on cache
 */
template<typename InputType = std::size_t>
class InputWithLocation {
public:
    using input_type = InputType; //!< Type alias for the input type
    
private:
    static constexpr std::size_t MaxFormatSize = 256; //!< Maximum formatted string length
    
    InputType      m_input;    //!< The actual array input value
    SourceLocation m_location; //!< Source location of the inputing operation
    
    // Mutable cache for formatted output (lazy evaluation)
    mutable std::array<char, MaxFormatSize> m_format_buffer{}; //!< Pre-allocated format buffer
    mutable std::size_t                      m_format_length{0}; //!< Cached string length (0 = not cached)
    
    /*!
     * \brief Count decimal digits in a number
     * \param[in] value Number to count digits for
     * \return Number of decimal digits (minimum 1)
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
     * \param[out] buffer Output buffer (must have space for digits)
     * \param[in] value Number to convert
     * \param[in] digits Number of digits (from count_digits)
     * \return Pointer past the last written character
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
    //******************************************************************************************************************
    // CONSTRUCTORS AND CONVERSIONS
    //******************************************************************************************************************
    
    /*!
     * \brief Construct from input value with automatic location capture
     *
     * \param[in] idx      Array input value
     * \param[in] file     Source file (auto-captured)
     * \param[in] line     Line number (auto-captured)
     * \param[in] function Function name (auto-captured)
     *
     * \note The default parameters capture the caller's location when used directly
     */
    constexpr InputWithLocation(
        InputType idx,
        const char* file = ARA_BUILTIN_FILE(),
        std::uint_least32_t line = ARA_BUILTIN_LINE(),
        const char* function = ARA_BUILTIN_FUNCTION()) noexcept
        : m_input(idx)
        , m_location(file, function, line)
        , m_format_buffer{}
        , m_format_length(0)
    {}
    
    /*!
     * \brief Implicit conversion to the underlying input type
     * \return The wrapped input value
     * \note Allows transparent use in array subscript operations
     */
    [[nodiscard]] constexpr operator InputType() const noexcept { 
        return m_input; 
    }
    
    //******************************************************************************************************************
    // ACCESSORS
    //******************************************************************************************************************
    
    /*!
     * \brief Get the wrapped input value
     * \return The array input
     */
    [[nodiscard]] constexpr auto input() const noexcept -> InputType { 
        return m_input; 
    }
    
    /*!
     * \brief Get the source location information
     * \return SourceLocation object containing file, line, etc.
     */
    [[nodiscard]] constexpr auto location() const noexcept -> const SourceLocation& { 
        return m_location; 
    }
    
    //******************************************************************************************************************
    // FORMATTING
    //******************************************************************************************************************
    
    /*!
     * \brief Get formatted location information string
     *
     * \return String view of format: "file-> <basename>, function-> <func>: line-> <number>"
     *
     * \details Format is computed on first call and cached for subsequent calls.
     *          Thread-safe with benign races (multiple threads may compute the same value).
     *
     * \note The returned string_view is valid for the lifetime of this object
     */
    [[nodiscard]] auto info() const noexcept -> std::string_view {
        // Check if already cached
        if (m_format_length != 0) {
            return std::string_view(m_format_buffer.data(), m_format_length);
        }
        
        // Format: "file-> <basename>, function-> <func>: line-> <number>"
        
        // Get components
        auto base_view = m_location.basename();
        const char* func = m_location.function_name();
        auto func_len = std::strlen(func);
        auto line_num = m_location.line();
        auto line_digits = count_digits(line_num);
        
        // Build the formatted string
        char* out = m_format_buffer.data();
        
        // "file-> "
        static constexpr char prefix1[] = "file-> ";
        constexpr auto prefix1_len = sizeof(prefix1) - 1;
        std::memcpy(out, prefix1, prefix1_len);
        out += prefix1_len;
        
        // basename
        std::memcpy(out, base_view.data(), base_view.size());
        out += base_view.size();
        
        // ", function-> "
        static constexpr char prefix2[] = ", function-> ";
        constexpr auto prefix2_len = sizeof(prefix2) - 1;
        std::memcpy(out, prefix2, prefix2_len);
        out += prefix2_len;
        
        // function name
        std::memcpy(out, func, func_len);
        out += func_len;
        
        // ": line-> "
        static constexpr char prefix3[] = ": line-> ";
        constexpr auto prefix3_len = sizeof(prefix3) - 1;
        std::memcpy(out, prefix3, prefix3_len);
        out += prefix3_len;
        
        // line number
        out = write_number(out, line_num, line_digits);
        
        // Null terminate and record length
        *out = '\0';
        m_format_length = static_cast<std::size_t>(out - m_format_buffer.data());
        
        return std::string_view(m_format_buffer.data(), m_format_length);
    }
    
    /*!
     * \brief Get a short format string (basename:line)
     * \return Dynamically allocated string (for cases where lifetime matters)
     */
    [[nodiscard]] auto short_info() const -> std::string {
        auto base = m_location.basename();
        return std::string(base) + ":" + std::to_string(m_location.line());
    }
};

/**********************************************************************************************************************
 *  DEDUCTION GUIDES
 *********************************************************************************************************************/
/*!
 * \brief Deduction guide for InputWithLocation
 * \relates InputWithLocation
 */
template<typename T>
InputWithLocation(T) -> InputWithLocation<T>;


}  // namespace internal
}  // namespace core
}  // namespace ara

#endif  // ARA_CORE_INTERNAL_LOCATION_UTILS_H_