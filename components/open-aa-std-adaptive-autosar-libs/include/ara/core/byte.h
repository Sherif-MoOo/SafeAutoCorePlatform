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
 *  \file       ara/core/byte.h
 *  \brief      Definition and implementation of the ara::core::Byte type.
 *
 *  \details    This file defines and implements the ara::core::Byte type, a strong type-safe byte representation
 *              designed for the OpenAA project. It provides functionality similar to std::byte with additional
 *              enhancements and safety features to meet Adaptive AUTOSAR requirements (e.g., [SWS_CORE_10104],
 *              [SWS_CORE_10105], [SWS_CORE_10106], [SWS_CORE_10107], etc.), including violation handling and
 *              enhanced operations beyond the base specification.
 *
 *  \note       Based on the Adaptive AUTOSAR SWS (e.g., R24-11) requirements for the "Byte" type, especially:
 *              - [SWS_CORE_10102] (Value range requirements)
 *              - [SWS_CORE_10104] (Default construction)
 *              - [SWS_CORE_10105] (Trivial destructor)
 *              - [SWS_CORE_10106] (No implicit conversion from other types)
 *              - [SWS_CORE_10107] (No implicit conversion to other types)
 *              - [SWS_CORE_10108] (Additional conversion restrictions)
 *              - Enhanced with C++26-like features while maintaining C++17 compatibility
 *********************************************************************************************************************/

#ifndef OPEN_AA_ADAPTIVE_AUTOSAR_LIBS_INCLUDE_ARA_CORE_BYTE_H_
#define OPEN_AA_ADAPTIVE_AUTOSAR_LIBS_INCLUDE_ARA_CORE_BYTE_H_

/**********************************************************************************************************************
 *  CONFIGURATION MACROS
 *********************************************************************************************************************/
/*!
 * \brief  Configuration macros for optional features
 *
 * ENABLE_PLATFORM_CONDITIONAL_EXCEPTION - Enable conditional noexcept based on operations
 * ARA_CORE_BYTE_ENABLE_IOSTREAM - Enable stream operators for debugging
 */

/**********************************************************************************************************************
 *  INCLUDES: files that required by the byte type
 *********************************************************************************************************************/
/*!
 * \brief  Includes necessary standard headers for byte operations, plus any additional
 *         headers for violation handling or type traits (per AUTOSAR guidelines).
 *
 * [SWS_CORE_10102]: cstdint for std::uint8_t underlying type
 * [SWS_CORE_10104]: type_traits for trivial construction checks
 * [SWS_CORE_10105]: type_traits for trivial destructor verification
 * [SWS_CORE_10106], [SWS_CORE_10107]: we prevent implicit conversions
 */
#include <cstdint>                                  // For std::uint8_t
#include <cstddef>                                  // For std::size_t
#include <type_traits>                              // For type trait checks
#include <iosfwd>                                   // For stream forward declarations
#include <limits>                                   // For numeric_limits
#include <bitset>                                   // For bitset in test code

#include "ara/core/internal/utility.h"              // For utility functions and traits
#include "ara/core/internal/location_utils.h"       // For capturing file/line details
#include "ara/core/internal/violation_handler.h"    // To trigger violations
#include "ara/core/abort.h"                         // For direct abort on non-specified violations

/**********************************************************************************************************************
 *  NAMESPACE: ara::core
 *********************************************************************************************************************/
/*!
 * \brief  The ara::core namespace, within which our AUTOSAR Adaptive Platform
 *         data types and utilities reside.
 */
namespace ara {
namespace core {

/**********************************************************************************************************************
 *  CLASS: ara::core::Byte
 *********************************************************************************************************************/
/*!
 * \brief  A type-safe byte representation for the Adaptive AUTOSAR platform.
 *
 * \details
 * - [SWS_CORE_10102]: Represents values in the range [0, 255]
 * - [SWS_CORE_10104]: Default constructible with indeterminate value at no runtime cost
 * - [SWS_CORE_10105]: Trivial destructor
 * - [SWS_CORE_10106]: No implicit conversions from other types
 * - [SWS_CORE_10107]: No implicit conversions to other types (including bool)
 * - Enhanced beyond AUTOSAR spec with modern C++26-like features
 * - Provides comprehensive bitwise and comparison operators
 * - Strict type safety with violation handling for invalid operations
 *
 * \note Unlike std::byte, this implementation provides more ergonomic construction
 *       and enhanced safety features while maintaining zero-overhead abstraction.
 * 
 * \note Thread Safety: All const member functions are thread-safe. Non-const member
 *       functions require external synchronization when accessed from multiple threads.
 */
class Byte final
{
public:
    // -----------------------------------------------------------------------------------
    // TYPE ALIASES (public)
    // -----------------------------------------------------------------------------------
    using underlying_type = std::uint8_t;  /*!< Expose underlying type for explicit access */

    // -----------------------------------------------------------------------------------
    // CONSTRUCTORS AND DESTRUCTOR
    // -----------------------------------------------------------------------------------
    
    /*!
     * \brief Default constructor - creates byte with indeterminate value.
     *
     * \details
     * - [SWS_CORE_10104]: Default construction with no initializer
     * - No runtime cost incurred
     * - Value is indeterminate (not zero-initialized)
     *
     * \note This follows AUTOSAR spec exactly - use Byte{} or Byte{0} for zero initialization
     * \note Cannot be constexpr in C++17 due to uninitialized member
     */
    Byte() noexcept = default;

    /*!
     * \brief Construct from integral value with bounds checking.
     *
     * \tparam IntegralType Type of the integral value (must be integral)
     * \param val The value to initialize the byte with
     *
     * \details
     * - Explicit constructor prevents implicit conversions [SWS_CORE_10106]
     * - Compile-time bounds checking for constant expressions
     * - Runtime violation for out-of-range values in non-constant contexts
     * - Enhanced beyond AUTOSAR: accepts any integral type with proper checking
     *
     * \note Values outside [0, 255] trigger violation handler
     */
    template <typename IntegralType,
              typename = std::enable_if_t<std::is_integral_v<IntegralType>>>
    constexpr explicit Byte(IntegralType val) noexcept
        : value_(static_cast<underlying_type>(val))
    {
        
        // C++17 compatible bounds checking
        if constexpr (std::is_signed_v<IntegralType>) {
            if (!detail::is_constant_evaluated()) {
                if (val < 0 || val > 255) {
                    TriggerByteRangeViolation(ARA_CORE_INTERNAL_FILELINE, val);
                }
            }
        } else {
            if (!detail::is_constant_evaluated()) {
                if (val > 255) {
                    TriggerByteRangeViolation(ARA_CORE_INTERNAL_FILELINE, val);
                }
            }
        }
    }

    /*!
     * \brief Rejecting constructor for non-integral types.
     *
     * \tparam T Type that is not integral
     * \param Type parameter (unused)
     *
     * \details
     * - Catches attempts to construct Byte from non-integral types
     * - Provides clear compile-time error message
     * - Enforces [SWS_CORE_10106] - no implicit conversions
     */
    template <typename T,
              typename = std::enable_if_t<!std::is_integral_v<T>>,
              int = 0>
    constexpr explicit Byte([[maybe_unused]] T t) noexcept
    {
        static_assert(std::is_integral_v<T>,
            "\n[ERROR] Cannot construct ara::core::Byte from non-integral type!\n"
            "        Byte can only be constructed from integral types.\n"
            "        Use explicit casting if you need to convert from other types.\n");
    }

    /*!
     * \brief Destructor - trivial as required by [SWS_CORE_10105]
     * \note Cannot be constexpr in C++17
     */
    ~Byte() noexcept = default;

    /*!
     * \brief Copy constructor - defaulted for trivial copyability
     */
    constexpr Byte(const Byte&) noexcept = default;

    /*!
     * \brief Move constructor - defaulted for trivial movability
     */
    constexpr Byte(Byte&&) noexcept = default;

    /*!
     * \brief Copy assignment - defaulted for trivial assignability
     */
    constexpr auto operator=(const Byte&) noexcept -> Byte& = default;

    /*!
     * \brief Move assignment - defaulted for trivial assignability
     */
    constexpr auto operator=(Byte&&) noexcept -> Byte& = default;

    // -----------------------------------------------------------------------------------
    // EXPLICIT CONVERSIONS [SWS_CORE_10107]
    // -----------------------------------------------------------------------------------
    
    /*!
     * \brief Explicit conversion to integral types.
     *
     * \tparam IntegralType Target integral type
     * \return The byte value as the requested integral type
     *
     * \details
     * - [SWS_CORE_10107]: No implicit conversions allowed
     * - Must use static_cast or explicit conversion
     * - Safe for any integral type (proper value preservation)
     *
     * \note Enhanced: Works with any integral type, not just uint8_t
     */
    template <typename IntegralType,
              typename = std::enable_if_t<std::is_integral_v<IntegralType>>>
    constexpr explicit operator IntegralType() const noexcept
    {
        return static_cast<IntegralType>(value_);
    }

    /*!
     * \brief Deleted bool conversion operator.
     *
     * \details
     * - [SWS_CORE_10107]: Explicitly prevents implicit conversion to bool
     * - Provides clear compile-time error if attempted
     */
    explicit operator bool() const = delete;

    /*!
     * \brief Get underlying value explicitly.
     *
     * \return The underlying uint8_t value
     *
     * \details
     * - Provides named access to underlying value
     * - Alternative to static_cast for clarity
     * - Enhanced beyond AUTOSAR for better API ergonomics
     */
    [[nodiscard]] constexpr auto to_integer() const noexcept -> underlying_type
    {
        return value_;
    }

    /*!
     * \brief Get underlying value via template function.
     *
     * \tparam IntegralType Target integral type
     * \return The byte value as the requested type
     *
     * \details
     * - Template version of to_integer for any integral type
     * - Enhanced API for modern C++ usage patterns
     */
    template <typename IntegralType,
              typename = std::enable_if_t<std::is_integral_v<IntegralType>>>
    [[nodiscard]] constexpr auto to_integer() const noexcept -> IntegralType
    {
        return static_cast<IntegralType>(value_);
    }

    // -----------------------------------------------------------------------------------
    // BITWISE OPERATORS (COMPOUND ASSIGNMENT)
    // -----------------------------------------------------------------------------------
    
    /*!
     * \brief Bitwise AND assignment operator.
     *
     * \param other The byte to AND with
     * \return Reference to this byte after operation
     *
     * \details
     * - Modifies this byte in-place
     * - Part of comprehensive bitwise operator set
     * - Enhanced beyond base AUTOSAR requirements
     */
    constexpr auto operator&=(Byte other) noexcept -> Byte&
    {
        value_ = static_cast<underlying_type>(value_ & other.value_);
        return *this;
    }

    /*!
     * \brief Bitwise OR assignment operator.
     *
     * \param other The byte to OR with
     * \return Reference to this byte after operation
     */
    constexpr auto operator|=(Byte other) noexcept -> Byte&
    {
        value_ = static_cast<underlying_type>(value_ | other.value_);
        return *this;
    }

    /*!
     * \brief Bitwise XOR assignment operator.
     *
     * \param other The byte to XOR with
     * \return Reference to this byte after operation
     */
    constexpr auto operator^=(Byte other) noexcept -> Byte&
    {
        value_ = static_cast<underlying_type>(value_ ^ other.value_);
        return *this;
    }

    /*!
     * \brief Left shift assignment operator.
     *
     * \param shift Number of positions to shift
     * \return Reference to this byte after operation
     *
     * \details
     * - Shift amount is masked to valid range [0, 7]
     * - Prevents undefined behavior from over-shifting
     * - Enhanced safety beyond standard requirements
     */
    template <typename IntegralType,
              typename = std::enable_if_t<std::is_integral_v<IntegralType>>>
    constexpr auto operator<<=(IntegralType shift) noexcept -> Byte&
    {
        if (!detail::is_constant_evaluated()) {
            if (shift < 0 || shift >= 8) {
                ara::core::Abort("Byte shift amount out of range [0, 7]");
            }
        }
        value_ = static_cast<underlying_type>(
            static_cast<underlying_type>(value_ << (static_cast<unsigned>(shift) & 0x7u))
        );
        return *this;
    }

    /*!
     * \brief Rejecting left shift assignment for non-integral types.
     *
     * \tparam T Non-integral type
     * \param shift Invalid shift type (unused)
     *
     * \details
     * - Provides clear compile-time error message
     * - Prevents accidental misuse of shift assignment
     */
    template <typename T,
              typename = std::enable_if_t<!std::is_integral_v<T>>,
              int = 0>
    constexpr auto operator<<=([[maybe_unused]] T shift) noexcept -> Byte&
    {
        static_assert(std::is_integral_v<T>,
            "\n[ERROR] Cannot shift-assign ara::core::Byte with non-integral type!\n"
            "        Shift amount must be an integral type.\n");
        return *this;  // Never reached
    }

    /*!
     * \brief Right shift assignment operator.
     *
     * \param shift Number of positions to shift
     * \return Reference to this byte after operation
     *
     * \details
     * - Shift amount is masked to valid range [0, 7]
     * - Prevents undefined behavior from over-shifting
     * - Enhanced safety beyond standard requirements
     */
    template <typename IntegralType,
              typename = std::enable_if_t<std::is_integral_v<IntegralType>>>
    constexpr auto operator>>=(IntegralType shift) noexcept -> Byte&
    {
        if (!detail::is_constant_evaluated()) {
            if (shift < 0 || shift >= 8) {
                ara::core::Abort("Byte shift amount out of range [0, 7]");
            }
        }
        value_ = static_cast<underlying_type>(
            static_cast<underlying_type>(value_ >> (static_cast<unsigned>(shift) & 0x7u))
        );
        return *this;
    }

    /*!
     * \brief Rejecting right shift assignment for non-integral types.
     *
     * \tparam T Non-integral type
     * \param shift Invalid shift type (unused)
     *
     * \details
     * - Provides clear compile-time error message
     * - Prevents accidental misuse of shift assignment
     */
    template <typename T,
              typename = std::enable_if_t<!std::is_integral_v<T>>,
              int = 0>
    constexpr auto operator>>=([[maybe_unused]] T shift) noexcept -> Byte&
    {
        static_assert(std::is_integral_v<T>,
            "\n[ERROR] Cannot shift-assign ara::core::Byte with non-integral type!\n"
            "        Shift amount must be an integral type.\n");
        return *this;  // Never reached
    }

    // -----------------------------------------------------------------------------------
    // ENHANCED FEATURES (BEYOND AUTOSAR)
    // -----------------------------------------------------------------------------------
    
    /*!
     * \brief Test if specific bit is set.
     *
     * \param pos Bit position to test [0, 7]
     * \return true if bit is set, false otherwise
     *
     * \details
     * - Enhanced API for bit manipulation
     * - Bounds checked for safety
     * - Modern C++ style interface
     */
    [[nodiscard]] constexpr auto test(std::size_t pos) const noexcept -> bool
    {
        if (pos >= 8) {
            if (!detail::is_constant_evaluated()) {
                ara::core::Abort("Bit position out of range [0, 7]");
            }
            return false;
        }
        return (value_ & static_cast<underlying_type>(1u << pos)) != 0;
    }

    /*!
     * \brief Set specific bit to 1.
     *
     * \param pos Bit position to set [0, 7]
     * \return Reference to this byte
     *
     * \details
     * - Enhanced API for bit manipulation
     * - Bounds checked for safety
     * - Chainable operation
     */
    constexpr auto set(std::size_t pos) noexcept -> Byte&
    {
        if (pos >= 8) {
            if (!detail::is_constant_evaluated()) {
                ara::core::Abort("Bit position out of range [0, 7]");
            }
            return *this;
        }
        value_ = static_cast<underlying_type>(value_ | static_cast<underlying_type>(1u << pos));
        return *this;
    }

    /*!
     * \brief Reset specific bit to 0.
     *
     * \param pos Bit position to reset [0, 7]
     * \return Reference to this byte
     *
     * \details
     * - Enhanced API for bit manipulation
     * - Bounds checked for safety
     * - Chainable operation
     */
    constexpr auto reset(std::size_t pos) noexcept -> Byte&
    {
        if (pos >= 8) {
            if (!detail::is_constant_evaluated()) {
                ara::core::Abort("Bit position out of range [0, 7]");
            }
            return *this;
        }
        value_ = static_cast<underlying_type>(value_ & static_cast<underlying_type>(~(1u << pos)));
        return *this;
    }

    /*!
     * \brief Flip specific bit.
     *
     * \param pos Bit position to flip [0, 7]
     * \return Reference to this byte
     *
     * \details
     * - Enhanced API for bit manipulation
     * - Bounds checked for safety
     * - Chainable operation
     */
    constexpr auto flip(std::size_t pos) noexcept -> Byte&
    {
        if (pos >= 8) {
            if (!detail::is_constant_evaluated()) {
                ara::core::Abort("Bit position out of range [0, 7]");
            }
            return *this;
        }
        value_ = static_cast<underlying_type>(value_ ^ static_cast<underlying_type>(1u << pos));
        return *this;
    }

    /*!
     * \brief Set all bits to 1.
     *
     * \return Reference to this byte
     */
    constexpr auto set() noexcept -> Byte&
    {
        value_ = 0xFF;
        return *this;
    }

    /*!
     * \brief Reset all bits to 0.
     *
     * \return Reference to this byte
     */
    constexpr auto reset() noexcept -> Byte&
    {
        value_ = 0;
        return *this;
    }

    /*!
     * \brief Flip all bits.
     *
     * \return Reference to this byte
     */
    constexpr auto flip() noexcept -> Byte&
    {
        value_ = static_cast<underlying_type>(~value_);
        return *this;
    }

    /*!
     * \brief Count number of set bits (popcount).
     *
     * \return Number of bits set to 1
     *
     * \details
     * - Enhanced feature for bit manipulation
     * - Efficient implementation using bit tricks
     * - Useful for embedded/systems programming
     */
    [[nodiscard]] constexpr auto count() const noexcept -> std::size_t
    {
        // Brian Kernighan's algorithm
        std::size_t count = 0;
        auto v = value_;
        while (v) {
            v = static_cast<underlying_type>(v & static_cast<underlying_type>(v - 1));
            ++count;
        }
        return count;
    }

    /*!
     * \brief Check if all bits are set.
     *
     * \return true if all bits are 1, false otherwise
     */
    [[nodiscard]] constexpr auto all() const noexcept -> bool
    {
        return value_ == 0xFF;
    }

    /*!
     * \brief Check if any bit is set.
     *
     * \return true if at least one bit is 1, false otherwise
     */
    [[nodiscard]] constexpr auto any() const noexcept -> bool
    {
        return value_ != 0;
    }

    /*!
     * \brief Check if no bits are set.
     *
     * \return true if all bits are 0, false otherwise
     */
    [[nodiscard]] constexpr auto none() const noexcept -> bool
    {
        return value_ == 0;
    }

private:

    underlying_type value_;  /*!< Underlying storage - exactly 8 bits as per [SWS_CORE_10102] */

    /*!
     * \brief Trigger violation for out-of-range byte construction.
     *
     * \param location Source location information
     * \param value The invalid value attempted
     *
     * \details
     * - Uses AUTOSAR violation handler for consistency
     * - Provides detailed error information
     * - [[noreturn]] ensures proper compiler optimization
     */
    template <typename T>
    [[noreturn]] static auto TriggerByteRangeViolation(
        std::string_view location,
        T value) noexcept -> void
    {
        auto& handler = ara::core::internal::ViolationHandler::Instance();
        handler.TriggerByteRangeViolation(
            ara::core::internal::ViolationHandler::ByteKey{},
            location,
            static_cast<long long>(value));
    }
};

/**********************************************************************************************************************
 *  NON-MEMBER FUNCTIONS - BITWISE OPERATORS
 *********************************************************************************************************************/

/*!
 * \brief Bitwise AND operator.
 *
 * \param lhs Left operand
 * \param rhs Right operand
 * \return Result of bitwise AND
 */
[[nodiscard]] constexpr auto operator&(Byte lhs, Byte rhs) noexcept -> Byte
{
    return Byte{static_cast<std::uint8_t>(
        static_cast<std::uint8_t>(lhs) & static_cast<std::uint8_t>(rhs))};
}

/*!
 * \brief Bitwise OR operator.
 *
 * \param lhs Left operand
 * \param rhs Right operand
 * \return Result of bitwise OR
 */
[[nodiscard]] constexpr auto operator|(Byte lhs, Byte rhs) noexcept -> Byte
{
    return Byte{static_cast<std::uint8_t>(
        static_cast<std::uint8_t>(lhs) | static_cast<std::uint8_t>(rhs))};
}

/*!
 * \brief Bitwise XOR operator.
 *
 * \param lhs Left operand
 * \param rhs Right operand
 * \return Result of bitwise XOR
 */
[[nodiscard]] constexpr auto operator^(Byte lhs, Byte rhs) noexcept -> Byte
{
    return Byte{static_cast<std::uint8_t>(
        static_cast<std::uint8_t>(lhs) ^ static_cast<std::uint8_t>(rhs))};
}

/*!
 * \brief Bitwise NOT operator.
 *
 * \param b Operand to negate
 * \return Result of bitwise NOT
 */
[[nodiscard]] constexpr auto operator~(Byte b) noexcept -> Byte
{
    return Byte{static_cast<std::uint8_t>(~static_cast<std::uint8_t>(b))};
}

/*!
 * \brief Left shift operator.
 *
 * \param b Byte to shift
 * \param shift Number of positions to shift
 * \return Result of left shift
 */
template <typename IntegralType,
          typename = std::enable_if_t<std::is_integral_v<IntegralType>>>
[[nodiscard]] constexpr auto operator<<(Byte b, IntegralType shift) noexcept -> Byte
{
    return Byte{static_cast<std::uint8_t>(
        static_cast<std::uint8_t>(b) << (static_cast<unsigned>(shift) & 0x7u))};
}

/*!
 * \brief Rejecting left shift operator for non-integral shift amounts.
 *
 * \tparam T Non-integral type
 * \param b Byte operand (unused)
 * \param shift Invalid shift type (unused)
 *
 * \details
 * - Provides clear compile-time error for invalid shift operations
 * - Prevents accidental misuse of shift operators
 */
template <typename T,
          typename = std::enable_if_t<!std::is_integral_v<T>>,
          int = 0>
[[nodiscard]] constexpr auto operator<<([[maybe_unused]] Byte b, [[maybe_unused]] T shift) noexcept -> Byte
{
    static_assert(std::is_integral_v<T>,
        "\n[ERROR] Cannot shift ara::core::Byte by non-integral type!\n"
        "        Shift amount must be an integral type.\n");
    return Byte{0};  // Never reached
}

/*!
 * \brief Right shift operator.
 *
 * \param b Byte to shift
 * \param shift Number of positions to shift
 * \return Result of right shift
 */
template <typename IntegralType,
          typename = std::enable_if_t<std::is_integral_v<IntegralType>>>
[[nodiscard]] constexpr auto operator>>(Byte b, IntegralType shift) noexcept -> Byte
{
    return Byte{static_cast<std::uint8_t>(
        static_cast<std::uint8_t>(b) >> (static_cast<unsigned>(shift) & 0x7u))};
}

/*!
 * \brief Rejecting right shift operator for non-integral shift amounts.
 *
 * \tparam T Non-integral type
 * \param b Byte operand (unused)
 * \param shift Invalid shift type (unused)
 *
 * \details
 * - Provides clear compile-time error for invalid shift operations
 * - Prevents accidental misuse of shift operators
 */
template <typename T,
          typename = std::enable_if_t<!std::is_integral_v<T>>,
          int = 0>
[[nodiscard]] constexpr auto operator>>([[maybe_unused]] Byte b, [[maybe_unused]] T shift) noexcept -> Byte
{
    static_assert(std::is_integral_v<T>,
        "\n[ERROR] Cannot shift ara::core::Byte by non-integral type!\n"
        "        Shift amount must be an integral type.\n");
    return Byte{0};  // Never reached
}

/**********************************************************************************************************************
 *  NON-MEMBER FUNCTIONS - COMPARISON OPERATORS
 *********************************************************************************************************************/

/*!
 * \brief Equality comparison operator.
 *
 * \param lhs Left operand
 * \param rhs Right operand
 * \return true if bytes are equal, false otherwise
 */
[[nodiscard]] constexpr auto operator==(Byte lhs, Byte rhs) noexcept -> bool
{
    return static_cast<std::uint8_t>(lhs) == static_cast<std::uint8_t>(rhs);
}

/*!
 * \brief Inequality comparison operator.
 *
 * \param lhs Left operand
 * \param rhs Right operand
 * \return true if bytes are not equal, false otherwise
 */
[[nodiscard]] constexpr auto operator!=(Byte lhs, Byte rhs) noexcept -> bool
{
    return !(lhs == rhs);
}

/*!
 * \brief Less-than comparison operator.
 *
 * \param lhs Left operand
 * \param rhs Right operand
 * \return true if lhs < rhs, false otherwise
 */
[[nodiscard]] constexpr auto operator<(Byte lhs, Byte rhs) noexcept -> bool
{
    return static_cast<std::uint8_t>(lhs) < static_cast<std::uint8_t>(rhs);
}

/*!
 * \brief Less-than-or-equal comparison operator.
 *
 * \param lhs Left operand
 * \param rhs Right operand
 * \return true if lhs <= rhs, false otherwise
 */
[[nodiscard]] constexpr auto operator<=(Byte lhs, Byte rhs) noexcept -> bool
{
    return !(rhs < lhs);
}

/*!
 * \brief Greater-than comparison operator.
 *
 * \param lhs Left operand
 * \param rhs Right operand
 * \return true if lhs > rhs, false otherwise
 */
[[nodiscard]] constexpr auto operator>(Byte lhs, Byte rhs) noexcept -> bool
{
    return rhs < lhs;
}

/*!
 * \brief Greater-than-or-equal comparison operator.
 *
 * \param lhs Left operand
 * \param rhs Right operand
 * \return true if lhs >= rhs, false otherwise
 */
[[nodiscard]] constexpr auto operator>=(Byte lhs, Byte rhs) noexcept -> bool
{
    return !(lhs < rhs);
}

/**********************************************************************************************************************
 *  SFINAE PROTECTION - PREVENTING INVALID OPERATIONS
 *********************************************************************************************************************/

/*!
 * \brief Prevent comparison between Byte and other types (left variant).
 *
 * \tparam T Non-Byte type
 * \param lhs Byte operand (unused)
 * \param rhs Other type operand (unused)
 *
 * \details
 * - Enforces [SWS_CORE_10107] - no implicit conversions
 * - Provides clear compile-time error message
 */
template <typename T,
          typename = std::enable_if_t<!std::is_same_v<T, Byte>>>
constexpr auto operator==([[maybe_unused]] Byte lhs, [[maybe_unused]] const T& rhs) noexcept -> bool
{
    static_assert(std::is_same_v<T, Byte>,
        "\n[ERROR] Cannot compare ara::core::Byte with other types!\n"
        "        No implicit conversions are allowed for Byte comparisons.\n"
        "        Use explicit casting if comparison is intended.\n");
    return false;
}

/*!
 * \brief Prevent comparison between Byte and other types (right variant).
 *
 * \tparam T Non-Byte type
 * \param lhs Other type operand (unused)
 * \param rhs Byte operand (unused)
 */
template <typename T,
          typename = std::enable_if_t<!std::is_same_v<T, Byte>>>
constexpr auto operator==([[maybe_unused]] const T& lhs, [[maybe_unused]] Byte rhs) noexcept -> bool
{
    static_assert(std::is_same_v<T, Byte>,
        "\n[ERROR] Cannot compare ara::core::Byte with other types!\n"
        "        No implicit conversions are allowed for Byte comparisons.\n"
        "        Use explicit casting if comparison is intended.\n");
    return false;
}

/*!
 * \brief Prevent bitwise operations between Byte and other types (AND).
 *
 * \tparam T Non-Byte type
 * \param lhs Byte operand (unused)
 * \param rhs Other type operand (unused)
 */
template <typename T,
          typename = std::enable_if_t<!std::is_same_v<T, Byte>>>
constexpr auto operator&([[maybe_unused]] Byte lhs, [[maybe_unused]] const T& rhs) noexcept -> Byte
{
    static_assert(std::is_same_v<T, Byte>,
        "\n[ERROR] Cannot perform bitwise AND between ara::core::Byte and other types!\n"
        "        Bitwise operations require both operands to be Byte.\n"
        "        Use explicit Byte construction for the other operand.\n");
    return Byte{0};
}

/*!
 * \brief Prevent bitwise operations between Byte and other types (OR).
 *
 * \tparam T Non-Byte type
 * \param lhs Byte operand (unused)
 * \param rhs Other type operand (unused)
 */
template <typename T,
          typename = std::enable_if_t<!std::is_same_v<T, Byte>>>
constexpr auto operator|([[maybe_unused]] Byte lhs, [[maybe_unused]] const T& rhs) noexcept -> Byte
{
    static_assert(std::is_same_v<T, Byte>,
        "\n[ERROR] Cannot perform bitwise OR between ara::core::Byte and other types!\n"
        "        Bitwise operations require both operands to be Byte.\n"
        "        Use explicit Byte construction for the other operand.\n");
    return Byte{0};
}

/*!
 * \brief Prevent bitwise operations between Byte and other types (XOR).
 *
 * \tparam T Non-Byte type
 * \param lhs Byte operand (unused)
 * \param rhs Other type operand (unused)
 */
template <typename T,
          typename = std::enable_if_t<!std::is_same_v<T, Byte>>>
constexpr auto operator^([[maybe_unused]] Byte lhs, [[maybe_unused]] const T& rhs) noexcept -> Byte
{
    static_assert(std::is_same_v<T, Byte>,
        "\n[ERROR] Cannot perform bitwise XOR between ara::core::Byte and other types!\n"
        "        Bitwise operations require both operands to be Byte.\n"
        "        Use explicit Byte construction for the other operand.\n");
    return Byte{0};
}

/*!
 * \brief Prevent arithmetic addition with Byte.
 *
 * \param lhs Left operand (unused)
 * \param rhs Right operand (unused)
 */
constexpr auto operator+([[maybe_unused]] Byte lhs, [[maybe_unused]] Byte rhs) noexcept -> Byte = delete;

/*!
 * \brief Prevent arithmetic subtraction with Byte.
 *
 * \param lhs Left operand (unused)
 * \param rhs Right operand (unused)
 */
constexpr auto operator-([[maybe_unused]] Byte lhs, [[maybe_unused]] Byte rhs) noexcept -> Byte = delete;

/*!
 * \brief Prevent arithmetic multiplication with Byte.
 *
 * \param lhs Left operand (unused)
 * \param rhs Right operand (unused)
 */
constexpr auto operator*([[maybe_unused]] Byte lhs, [[maybe_unused]] Byte rhs) noexcept -> Byte = delete;

/*!
 * \brief Prevent arithmetic division with Byte.
 *
 * \param lhs Left operand (unused)
 * \param rhs Right operand (unused)
 */
constexpr auto operator/([[maybe_unused]] Byte lhs, [[maybe_unused]] Byte rhs) noexcept -> Byte = delete;

/*!
 * \brief Prevent arithmetic modulo with Byte.
 *
 * \param lhs Left operand (unused)
 * \param rhs Right operand (unused)
 */
constexpr auto operator%([[maybe_unused]] Byte lhs, [[maybe_unused]] Byte rhs) noexcept -> Byte = delete;

/**********************************************************************************************************************
 *  NON-MEMBER FUNCTIONS - UTILITY FUNCTIONS
 *********************************************************************************************************************/

/*!
 * \brief Convert integer to Byte with explicit notation.
 *
 * \tparam IntegralType Type of the integral value
 * \param value The value to convert
 * \return Byte representation of the value
 *
 * \details
 * - Factory function for clearer code
 * - Alternative to constructor for readability
 * - Enhanced beyond AUTOSAR for better API
 */
template <typename IntegralType,
          typename = std::enable_if_t<std::is_integral_v<IntegralType>>>
[[nodiscard]] constexpr auto to_byte(IntegralType value) noexcept -> Byte
{
    return Byte{value};
}

/*!
 * \brief Rejecting to_byte for non-integral types.
 *
 * \tparam T Non-integral type
 * \param value Invalid value type (unused)
 *
 * \details
 * - Provides clear compile-time error message
 * - Enforces type safety for byte creation
 */
template <typename T,
          typename = std::enable_if_t<!std::is_integral_v<T> && !std::is_same_v<T, char>>,
          int = 0>
[[nodiscard]] constexpr auto to_byte([[maybe_unused]] T value) noexcept -> Byte
{
    static_assert(std::is_integral_v<T> || std::is_same_v<T, char>,
        "\n[ERROR] Cannot convert non-integral type to ara::core::Byte!\n"
        "        to_byte() only accepts integral types or char.\n"
        "        Use explicit casting if conversion is intended.\n");
    return Byte{0};  // Never reached
}

/*!
 * \brief Create Byte from character literal.
 *
 * \param c Character to convert
 * \return Byte representation of the character
 *
 * \details
 * - Convenience function for character data
 * - Common use case in protocol implementations
 * - Enhanced API feature
 */
[[nodiscard]] constexpr auto to_byte(char c) noexcept -> Byte
{
    return Byte{static_cast<unsigned char>(c)};
}

/**********************************************************************************************************************
 *  NON-MEMBER FUNCTIONS - ENHANCED INTEGER LITERAL SUPPORT (C++26-like)
 *********************************************************************************************************************/

/*!
 * \brief User-defined literal for Byte creation.
 *
 * \tparam Chars Character pack representing the literal
 * \return Byte value from the literal
 *
 * \details
 * - Enables syntax like: auto b = 0xFF_byte;
 * - Compile-time parsing and validation
 * - Enhanced feature inspired by C++26 proposals
 * - Supports decimal, hex (0x), octal (0), and binary (0b) literals
 */
inline namespace literals {
inline namespace byte_literals {

namespace parser {

/* template-dependent false for assertions */
template<char... Cs> struct always_false : std::false_type {};

/* sentinel codes (must all be >255) */
constexpr std::uint16_t INVALID_DIGIT = 0x1FF;
constexpr std::uint16_t OVERFLOW      = 0x1FE;
constexpr std::uint16_t NO_DIGITS     = 0x1FD;

template<char... Cs>
struct parse_byte
{
private:
    static constexpr std::uint16_t compute() noexcept
    {
        constexpr char txt[] { Cs..., '\0' };
        constexpr std::size_t n = sizeof...(Cs);

        std::size_t pos = 0;
        int base        = 10;

        /* prefix handling --------------------------------------------------*/
        if (n >= 2 && txt[0] == '0')
        {
            switch (txt[1]) {
                case 'x': case 'X': base = 16; pos = 2; break;
                case 'b': case 'B': base =  2; pos = 2; break;
                default:            base =  8; pos = 1; break; // 0-leading
            }
        }

        auto digit = [](char c) constexpr -> int {
            return (c >= '0' && c <= '9') ? c - '0' :
                   (c >= 'a' && c <= 'f') ? 10 + c - 'a' :
                   (c >= 'A' && c <= 'F') ? 10 + c - 'A' : -1;
        };

        std::uint16_t val = 0;
        bool have_digit   = false;

        for (; pos < n; ++pos)
        {
            int d = digit(txt[pos]);
            if (d < 0 || d >= base)           return INVALID_DIGIT;
            val = static_cast<std::uint16_t>(val * base + d);
            if (val > 255)                    return OVERFLOW;
            have_digit = true;
        }
        return have_digit ? val : NO_DIGITS;
    }

public:
    static constexpr std::uint16_t raw = compute();

    /* one assert per failure-code – evaluated exactly once per literal -----*/
    static_assert(raw != INVALID_DIGIT,
        "\n[ERROR] invalid digit for chosen base in _byte literal");
    static_assert(raw != OVERFLOW,
        "\n[ERROR] _byte literal value exceeds 255");
    static_assert(raw != NO_DIGITS,
        "\n[ERROR] _byte literal contains no digits after the prefix");

    static constexpr std::uint8_t value =
        static_cast<std::uint8_t>(raw);   // safe: raw is 0-255
};

} // namespace parser

/* user-defined literal ------------------------------------------------------*/
template<char... Cs>
[[nodiscard]] constexpr ara::core::Byte operator ""_byte() noexcept
{
    return ara::core::Byte{ parser::parse_byte<Cs...>::value };
}

} // namespace byte_literals
} // namespace literals

// Bring byte literals into ara::core namespace for convenience

/**********************************************************************************************************************
 *  STREAM OPERATORS (ENHANCED FEATURE)
 *********************************************************************************************************************/

#ifdef ARA_CORE_BYTE_ENABLE_IOSTREAM
#include <iostream>
#include <iomanip>

/*!
 * \brief Stream insertion operator for debugging.
 *
 * \param os Output stream
 * \param b Byte to output
 * \return Reference to the stream
 *
 * \details
 * - Outputs byte as hex value (e.g., "0xFF")
 * - Enhanced feature for debugging and logging
 * - Not required by AUTOSAR but useful in practice
 * - Note: Cannot be constexpr due to I/O operations
 *
 * \note Conditional compilation based on ARA_CORE_BYTE_ENABLE_IOSTREAM
 */
inline auto operator<<(std::ostream& os, Byte b) -> std::ostream&
{
    const auto saved_flags = os.flags();
    const auto saved_fill = os.fill();
    
    os << "0x" << std::hex << std::uppercase << std::setfill('0') 
       << std::setw(2) << static_cast<unsigned>(static_cast<std::uint8_t>(b));
    
    // Restore stream state
    os.flags(saved_flags);
    os.fill(saved_fill);
    
    return os;
}
#endif

/**********************************************************************************************************************
 *  TYPE TRAITS SUPPORT
 *********************************************************************************************************************/

} // namespace core
} // namespace ara

// Type traits specializations in std namespace
namespace std {

/*!
 * \brief Specialize numeric_limits for ara::core::Byte.
 *
 * \details
 * - Provides standard numeric properties
 * - Enhanced feature for generic programming
 * - Enables use in template algorithms
 */
template <>
class numeric_limits<ara::core::Byte> {
public:
    static constexpr bool is_specialized = true;
    static constexpr bool is_signed = false;
    static constexpr bool is_integer = true;
    static constexpr bool is_exact = true;
    static constexpr bool has_infinity = false;
    static constexpr bool has_quiet_NaN = false;
    static constexpr bool has_signaling_NaN = false;
    static constexpr std::float_denorm_style has_denorm = std::denorm_absent;
    static constexpr bool has_denorm_loss = false;
    static constexpr std::float_round_style round_style = std::round_toward_zero;
    static constexpr bool is_iec559 = false;
    static constexpr bool is_bounded = true;
    static constexpr bool is_modulo = true;
    static constexpr int digits = 8;
    static constexpr int digits10 = 2;
    static constexpr int max_digits10 = 0;
    static constexpr int radix = 2;
    static constexpr int min_exponent = 0;
    static constexpr int min_exponent10 = 0;
    static constexpr int max_exponent = 0;
    static constexpr int max_exponent10 = 0;
    static constexpr bool traps = false;
    static constexpr bool tinyness_before = false;

    static constexpr auto min() noexcept -> ara::core::Byte { 
        return ara::core::Byte{0}; 
    }
    
    static constexpr auto lowest() noexcept -> ara::core::Byte { 
        return ara::core::Byte{0}; 
    }
    
    static constexpr auto max() noexcept -> ara::core::Byte { 
        return ara::core::Byte{255}; 
    }
    
    static constexpr auto epsilon() noexcept -> ara::core::Byte { 
        return ara::core::Byte{0}; 
    }
    
    static constexpr auto round_error() noexcept -> ara::core::Byte { 
        return ara::core::Byte{0}; 
    }
    
    static constexpr auto infinity() noexcept -> ara::core::Byte { 
        return ara::core::Byte{0}; 
    }
    
    static constexpr auto quiet_NaN() noexcept -> ara::core::Byte { 
        return ara::core::Byte{0}; 
    }
    
    static constexpr auto signaling_NaN() noexcept -> ara::core::Byte { 
        return ara::core::Byte{0}; 
    }
    
    static constexpr auto denorm_min() noexcept -> ara::core::Byte { 
        return ara::core::Byte{0}; 
    }
};

} // namespace std

/**********************************************************************************************************************
 *  COMPILE-TIME VERIFICATION
 *********************************************************************************************************************/
/*!
 * \brief Compile-time verification of AUTOSAR requirements for ara::core::Byte
 * 
 * \details How these static assertions work:
 * 
 * 1. **Placement after class definition**: These assertions are placed AFTER the complete
 *    definition of the Byte class. This is crucial because the compiler needs the full
 *    class definition to evaluate type traits.
 * 
 * 2. **Type trait evaluation**: The compiler evaluates these at compile time:
 *    - sizeof(Byte): Checks the actual memory size of the class
 *    - is_trivially_default_constructible: Verifies Byte() = default is trivial
 *    - is_trivially_destructible: Verifies ~Byte() = default is trivial
 *    - is_standard_layout: Ensures no virtual functions, single access control, etc.
 * 
 * 3. **Conversion checks**: 
 *    - is_convertible<int, Byte>: Would be true if you could write: Byte b = 42;
 *    - Since constructor is explicit, this is false (good!)
 *    - is_convertible<Byte, int>: Would be true if you could write: int i = someByte;
 *    - Since conversion operator is explicit, this is false (good!)
 * 
 * 4. **Compile-time guarantee**: If ANY of these assertions fail, compilation stops
 *    immediately with the specified error message, preventing non-compliant code from
 *    being built.
 * 
 * Example of what makes these work:
 * - Making constructor explicit prevents: is_convertible<int, Byte> 
 * - Using = default for destructor ensures: is_trivially_destructible
 * - Having only one data member (value_) helps ensure: is_standard_layout
 */

namespace ara {
namespace core {

// Verify that ara::core::Byte meets all AUTOSAR requirements
static_assert(sizeof(Byte) == 1, 
    "ara::core::Byte must be exactly 1 byte in size [SWS_CORE_10102]");

static_assert(std::is_trivially_default_constructible_v<Byte>,
    "ara::core::Byte must be trivially default constructible [SWS_CORE_10104]");

static_assert(std::is_trivially_destructible_v<Byte>,
    "ara::core::Byte must have trivial destructor [SWS_CORE_10105]");

static_assert(std::is_trivially_copy_constructible_v<Byte>,
    "ara::core::Byte must be trivially copy constructible");

static_assert(std::is_trivially_move_constructible_v<Byte>,
    "ara::core::Byte must be trivially move constructible");

static_assert(std::is_standard_layout_v<Byte>,
    "ara::core::Byte must have standard layout");

static_assert(!std::is_convertible_v<int, Byte>,
    "ara::core::Byte must not be implicitly convertible from other types [SWS_CORE_10106]");

static_assert(!std::is_convertible_v<Byte, int>,
    "ara::core::Byte must not be implicitly convertible to other types [SWS_CORE_10107]");

static_assert(!std::is_convertible_v<Byte, bool>,
    "ara::core::Byte must not be implicitly convertible to bool [SWS_CORE_10107]");

} // namespace core
} // namespace ara

#endif // OPEN_AA_ADAPTIVE_AUTOSAR_LIBS_INCLUDE_ARA_CORE_BYTE_H_