/***********************************************************************************************************************
 *  TEST UNSAFE WRAPPER
 *  --------------------------------------------------------------------------------------------------------------------
 *  \file       test_unsafe_wrapper.h
 *  \brief      Simplified unsafe buffer operation wrapper for testing purposes
 *  
 *  \details    Provides minimal wrapper for unsafe operations needed in testing without
 *              requiring the full UnsafeBufferOperation infrastructure
 ***********************************************************************************************************************/
#ifndef TEST_UNSAFE_WRAPPER_H_
#define TEST_UNSAFE_WRAPPER_H_

#include <cstddef>
#include <cstring>
#include <utility>

// Detect Clang's unsafe buffer warning support
#if defined(__clang__)
    #if __has_warning("-Wunsafe-buffer-usage")
        #define TEST_HAS_UNSAFE_BUFFER_WARNING 1
    #else
        #define TEST_HAS_UNSAFE_BUFFER_WARNING 0
    #endif
#else
    #define TEST_HAS_UNSAFE_BUFFER_WARNING 0
#endif

// Define macros for unsafe operations
#if TEST_HAS_UNSAFE_BUFFER_WARNING
    #define TEST_UNSAFE_BEGIN _Pragma("clang unsafe_buffer_usage begin")
    #define TEST_UNSAFE_END   _Pragma("clang unsafe_buffer_usage end")
#else
    #define TEST_UNSAFE_BEGIN
    #define TEST_UNSAFE_END
#endif

namespace test {

/*!
 * \brief Simple unsafe operations wrapper for testing
 * 
 * \details Provides minimal unsafe operation support needed for testing
 *          without complex token-based authorization
 */
struct UnsafeOps {
    
    /*!
     * \brief Unsafe element access for testing
     */
    template<typename T>
    [[nodiscard]] static constexpr inline auto at(T* buffer, std::size_t index) noexcept -> T& {
        TEST_UNSAFE_BEGIN
        return buffer[index];
        TEST_UNSAFE_END
    }
    
    template<typename T>
    [[nodiscard]] static constexpr inline auto at(const T* buffer, std::size_t index) noexcept -> const T& {
        TEST_UNSAFE_BEGIN
        return buffer[index];
        TEST_UNSAFE_END
    }
    
    /*!
     * \brief Unsafe pointer advance for testing
     */
    template<typename T>
    [[nodiscard]] static constexpr inline auto advance(T* ptr, std::ptrdiff_t offset) noexcept -> T* {
        TEST_UNSAFE_BEGIN
        return ptr + offset;
        TEST_UNSAFE_END
    }
    
    template<typename T>
    [[nodiscard]] static constexpr inline auto advance(const T* ptr, std::ptrdiff_t offset) noexcept -> const T* {
        TEST_UNSAFE_BEGIN
        return ptr + offset;
        TEST_UNSAFE_END
    }
    
    /*!
     * \brief Wrapper for unsafe libc calls
     */
    template<typename Func, typename... Args>
    [[nodiscard]] static inline auto call(Func&& func, Args&&... args) 
        noexcept(noexcept(func(std::forward<Args>(args)...)))
        -> decltype(func(std::forward<Args>(args)...)) {
        TEST_UNSAFE_BEGIN
        return func(std::forward<Args>(args)...);
        TEST_UNSAFE_END
    }
};

} // namespace test

#endif // TEST_UNSAFE_WRAPPER_H_