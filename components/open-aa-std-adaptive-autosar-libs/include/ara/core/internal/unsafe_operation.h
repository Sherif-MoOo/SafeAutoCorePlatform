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
 *  \file       unsafe_operation.h
 *  \brief      Token-based unsafe buffer operations for Clang's -Wunsafe-buffer-usage.
 *
 *  \details    This file provides a controlled mechanism for performing buffer operations
 *              that would trigger Clang's -Wunsafe-buffer-usage warning (Clang 16+).
 *              Access is restricted through a token-based system to ensure deliberate usage.
 ***********************************************************************************************************************/
#ifndef ARA_CORE_UNSAFE_OPERATION_H_
#define ARA_CORE_UNSAFE_OPERATION_H_

#include <cstddef>                                  // For std::size_t, std::ptrdiff_t
#include <type_traits>                              // For std::enable_if_t, std::is_pointer_v, etc.
#include <utility>                                  // For std::forward, std::move
#include <iterator>                                 // For std::iterator_traits
#include <algorithm>                                // For std::copy, std::fill
#include <cstring>                                  // For std::memcpy, std::memmove
#include <cwchar>                                   // For std::wmemchr

/**********************************************************************************************************************
 *  PLATFORM AND COMPILER DETECTION
 *********************************************************************************************************************/
// Detect Clang version for -Wunsafe-buffer-usage support (introduced in Clang 16)
#if defined(__clang__)
#  if __has_warning("-Wunsafe-buffer-usage")
#    define ARA_HAS_UNSAFE_BUFFER_WARNING 1
#  else
#    define ARA_HAS_UNSAFE_BUFFER_WARNING 0
#  endif
#else
#  define ARA_HAS_UNSAFE_BUFFER_WARNING 0
#endif

/**********************************************************************************************************************
 *  UNSAFE BUFFER OPERATION MACROS
 *********************************************************************************************************************/
// Define macros for unsafe buffer operations based on compiler support
#if ARA_HAS_UNSAFE_BUFFER_WARNING
    #define ARA_CORE_UNSAFE_BUFFER_BEGIN \
        _Pragma("clang unsafe_buffer_usage begin")
    #define ARA_CORE_UNSAFE_BUFFER_END \
        _Pragma("clang unsafe_buffer_usage end")
#else
    // No-op for compilers that don't support the warning
    #define ARA_CORE_UNSAFE_BUFFER_BEGIN
    #define ARA_CORE_UNSAFE_BUFFER_END
#endif

/**********************************************************************************************************************
 *  NAMESPACE: ara::core
 *********************************************************************************************************************/
namespace ara {
namespace core {

/**********************************************************************************************************************
 *  Forward Declarations
 *********************************************************************************************************************/
template <typename T, std::size_t N>
class Array;

template<typename ElementType, std::size_t Extent>
class Span;

template<typename CharT, typename Traits>
class BasicStringView;

template<typename CharT, typename Traits>
constexpr auto char_find(const CharT* str, std::size_t count, CharT ch) noexcept -> const CharT*;

template<class CharT, class Traits>
constexpr auto str_len(const CharT* str) noexcept -> std::size_t;

/**********************************************************************************************************************
 *  NAMESPACE: ara::core::internal
 *********************************************************************************************************************/
namespace internal {


/**********************************************************************************************************************
 *  CLASS: UnsafeBufferToken
 *********************************************************************************************************************/
/*!
 * \brief   Token class for authorizing unsafe buffer operations.
 * 
 * \details This token can only be created by friend classes, providing controlled access
 *          to buffer operations that would trigger -Wunsafe-buffer-usage warnings.
 * 
 * \note    This class cannot be instantiated directly by users.
 */
class UnsafeBufferToken final {
private:
    // Private constructor - only friends can create tokens
    constexpr UnsafeBufferToken() noexcept = default;

    template <typename T, std::size_t N>
    friend class ara::core::Array;

    template<typename CharT, typename Traits>
    friend class ara::core::BasicStringView;

    template<typename ElementType, std::size_t Extent>
    friend class ara::core::Span;

    template<typename C, typename Tr>
    friend constexpr auto ara::core::char_find(const C* str, std::size_t count, C ch) noexcept -> const C*;

    template<class C, class Tr>
    friend constexpr auto ara::core::str_len(const C* str) noexcept -> std::size_t;

public:
    // Deleted copy/move operations to prevent token proliferation
    UnsafeBufferToken(const UnsafeBufferToken&) = delete;
    UnsafeBufferToken& operator=(const UnsafeBufferToken&) = delete;
    UnsafeBufferToken(UnsafeBufferToken&&) = delete;
    UnsafeBufferToken& operator=(UnsafeBufferToken&&) = delete;
    ~UnsafeBufferToken() noexcept = default;
};

/**********************************************************************************************************************
 *  CLASS: UnsafeBufferOperation
 *********************************************************************************************************************/
/*!
 * \brief   Provides controlled access to unsafe buffer operations.
 * 
 * \details This class encapsulates buffer operations that trigger Clang's -Wunsafe-buffer-usage
 *          warning. The warning is designed to encourage use of safe abstractions like span
 *          instead of raw pointer arithmetic.
 * 
 * \warning Use of this class bypasses important buffer safety checks. Only use when
 *          performance requirements or legacy code integration necessitate it.
 */
class UnsafeBufferOperation final {
public:
    // Delete default constructor - operations require a token
    UnsafeBufferOperation() = delete;
    
    /*!
     * \brief   Performs unchecked element access on a buffer.
     * 
     * \tparam  T       Element type
     * \param   token   Authorization token
     * \param   buffer  Pointer to buffer start
     * \param   index   Element index
     * \return  Reference to element
     * 
     * \warning No bounds checking is performed.
     */
    template<typename T>
    [[nodiscard]] static constexpr inline auto unsafe_element_access(
        [[maybe_unused]] const UnsafeBufferToken& /*unused*/,
        T* buffer,
        std::size_t index
    ) noexcept -> T& {
        
        static_assert(!std::is_void_v<T>,
            "\n[ERROR] Cannot access void element.\n");
        static_assert(!std::is_function_v<T>,
            "\n[ERROR] Cannot access function element.\n");

        ARA_CORE_UNSAFE_BUFFER_BEGIN
        return buffer[index];
        ARA_CORE_UNSAFE_BUFFER_END
    }

    template<typename T>
    [[nodiscard]] static constexpr inline auto unsafe_element_access(
        [[maybe_unused]] const UnsafeBufferToken&,
        const T* buffer,
        std::size_t index
    ) noexcept -> const T& {

        static_assert(!std::is_void_v<T>,
            "\n[ERROR] Cannot access void element.\n");
        static_assert(!std::is_function_v<T>,
            "\n[ERROR] Cannot access function element.\n");

        ARA_CORE_UNSAFE_BUFFER_BEGIN
        return buffer[index];
        ARA_CORE_UNSAFE_BUFFER_END
    }

    /*!
     * \brief Increments a pointer without bounds checking.
     * \tparam  T       Element type
     * \param   token   Authorization token
     * \param   ptr     Pointer to increment
     * \return  Incremented pointer
     * \warning No bounds checking is performed.
     */
    template<typename T>
    static constexpr inline auto unsafe_increment(
        [[maybe_unused]] const UnsafeBufferToken& /*unused*/,
        T*& ptr
    ) noexcept -> void {

        static_assert(!std::is_void_v<T>,
            "\n[ERROR] Cannot increment void pointer.\n");
        static_assert(!std::is_function_v<T>,
            "\n[ERROR] Cannot increment function pointer.\n");

        ARA_CORE_UNSAFE_BUFFER_BEGIN
        ++ptr;
        ARA_CORE_UNSAFE_BUFFER_END

    }

    /*!
     * \brief Increments a pointer without bounds checking.
     * \tparam  T       Element type
     * \param   token   Authorization token
     * \param   ptr     Pointer to increment
     * \return  Incremented pointer
     * \warning No bounds checking is performed.
     */
    template<typename T>
    static constexpr inline auto unsafe_increment(
        [[maybe_unused]] const UnsafeBufferToken& /*unused*/,
        const T*& ptr
    ) noexcept -> void {
        
        static_assert(!std::is_void_v<T>,
            "\n[ERROR] Cannot increment void pointer.\n");
        static_assert(!std::is_function_v<T>,
            "\n[ERROR] Cannot increment function pointer.\n");

        ARA_CORE_UNSAFE_BUFFER_BEGIN
        ++ptr;
        ARA_CORE_UNSAFE_BUFFER_END

    }
    
    /*!
     * \brief   Advances a pointer without bounds checking.
     * 
     * \tparam  T       Element type
     * \param   token   Authorization token
     * \param   ptr     Pointer to advance
     * \param   offset  Number of elements to advance
     * \return  Advanced pointer
     * 
     * \warning No bounds checking is performed.
     */
    template<typename T>
    [[nodiscard]] static constexpr inline auto unsafe_advance(
        [[maybe_unused]] const UnsafeBufferToken& /*unused*/,
        T* ptr,
        std::ptrdiff_t offset
    ) noexcept -> T* {

        static_assert(!std::is_void_v<T>,
            "\n[ERROR] Cannot advance void pointer.\n");
        static_assert(!std::is_function_v<T>,
            "\n[ERROR] Cannot advance function pointer.\n");

        ARA_CORE_UNSAFE_BUFFER_BEGIN
        return ptr + offset;
        ARA_CORE_UNSAFE_BUFFER_END

    }

    /*!
     * \brief   Advances a pointer without bounds checking.
     * 
     * \tparam  T       Element type
     * \param   token   Authorization token
     * \param   ptr     Pointer to advance
     * \param   offset  Number of elements to advance
     * \return  Advanced pointer
     * 
     * \warning No bounds checking is performed.
     */
    template<typename T>
    [[nodiscard]] static constexpr inline auto unsafe_advance(
        [[maybe_unused]] const UnsafeBufferToken& /*unused*/,
        const T* ptr,
        std::ptrdiff_t offset
    ) noexcept -> const T* {

        static_assert(!std::is_void_v<T>,
            "\n[ERROR] Cannot advance void pointer.\n");
        static_assert(!std::is_function_v<T>,
            "\n[ERROR] Cannot advance function pointer.\n");

        ARA_CORE_UNSAFE_BUFFER_BEGIN
        return ptr + offset;
        ARA_CORE_UNSAFE_BUFFER_END

    }

    /*!
     * \brief   Generic wrapper for unsafe libc function calls with enhanced type deduction.
     * 
     * \details This template method provides a controlled way to call libc functions
     *          that trigger -Wunsafe-buffer-usage-in-libc-call warnings. It handles
     *          both directly invocable functions.
     *
     * \tparam  Func    The callable type (function pointer, function object, lambda, etc.)
     * \tparam  Args    The argument types (perfectly forwarded)
     * \param   token   Authorization token ensuring controlled access
     * \param   func    The callable to invoke (typically a libc function)
     * \param   args    Arguments to forward to the function
     * \return  Exactly what the wrapped function returns
     * 
     * \warning This bypasses buffer safety checks. Use only when necessary.
     * 
     * \note    For overloaded functions like std::memchr, the generic lambda wrapper
     *          allows the compiler to deduce the correct overload based on the arguments.
     */
    template<typename Func, typename... Args>
    [[nodiscard]] static constexpr inline auto unsafe_libc_call(
        [[maybe_unused]] const UnsafeBufferToken& token,
        Func&& func,
        Args&&... args
    ) noexcept(noexcept(func(std::forward<Args>(args)...)))
    -> decltype(func(std::forward<Args>(args)...))
    {
        // Check for null function pointer
        static_assert(
            !std::is_same_v<std::remove_cv_t<std::remove_reference_t<Func>>, std::nullptr_t>,
            "\n[ERROR] Cannot call a null function pointer.\n"
        );

        ARA_CORE_UNSAFE_BUFFER_BEGIN
        return func(std::forward<Args>(args)...);
        ARA_CORE_UNSAFE_BUFFER_END
    }

};

/**********************************************************************************************************************
 *  CLEANUP: Undefine internal macros
 *********************************************************************************************************************/
#undef ARA_CLANG_VERSION
#undef ARA_HAS_UNSAFE_BUFFER_WARNING
#undef ARA_CORE_UNSAFE_BUFFER_BEGIN
#undef ARA_CORE_UNSAFE_BUFFER_END

} // namespace internal
} // namespace core
} // namespace ara

#endif // ARA_CORE_UNSAFE_OPERATION_H_