/***********************************************************************************************************************
 *  PROJECT
 *  --------------------------------------------------------------------------------------------------------------------
 *  \verbatim
 *  OpenAA: Open Source Adaptive AUTOSAR Project (CXX_STANDARD 17)
 *  \endverbatim
 *  --------------------------------------------------------------------------------------------------------------------
 *  FILE DESCRIPTION
 *  --------------------------------------------------------------------------------------------------------------------
 *  \file       utility.h
 *  \brief      Internal utilities for compile-time.
 *
 *  \details    This file defines helper functions and traits.
 ***********************************************************************************************************************/
#ifndef ARA_CORE_INTERNAL_UTILITY_H_
#define ARA_CORE_INTERNAL_UTILITY_H_

#include <mutex>                                    // For std::mutex, std::lock_guard
#include <type_traits>                              // For std::is_nothrow_move_constructible, std::is_nothrow_move_assignable, etc.
#include <utility>                                  // For std::declval, std::move, std::forward
#include <cstddef>                                  // For std::size_t, std::ptrdiff_t
#include <string>                                   // for std::basic_string
#include <string_view>                              // for std::basic_string_view
#include <string>                                   // for std::basic_string
#include <string_view>                              // for std::basic_string_view
/**********************************************************************************************************************
 *  SECTION: Forward Declaration
 *********************************************************************************************************************/
namespace ara::core {

/**********************************************************************************************************************
 *  CONSTANTS
 *********************************************************************************************************************/
/*!
 * \brief  A constant of type std::size_t denoting that the span has dynamic extent
 *
 * \details
 * - [SWS_CORE_01901]: Used to indicate runtime-determined span size
 * - Set to maximum value of size_t as per standard specification
 * - Enables distinction between static and dynamic extent spans
 *
 * \note This matches std::dynamic_extent from C++20
 */
inline constexpr std::size_t dynamic_extent = std::numeric_limits<std::size_t>::max();

/*!
 * \brief  Forward declaration of the Array class template.
 */
template <typename T, std::size_t N>
class Array;    

template<typename ElementType, std::size_t Extent = dynamic_extent>
class Span;

template<typename CharT, typename Traits>
class BasicStringView;

/*!
 * \brief  Forward declaration of the get function template.
 */
template <std::size_t I, typename T, std::size_t N>
[[nodiscard]] constexpr auto get(ara::core::Array<T,N>&) noexcept -> T&;

/*!
 * \brief  Forward declaration of the get function template.
 */
template <std::size_t I, typename T, std::size_t N>
[[nodiscard]] constexpr auto get(const ara::core::Array<T,N>&) noexcept -> const T&;

/*!
 * \brief  Forward declaration of the get function template.
 */
template <std::size_t I, typename T, std::size_t N>
[[nodiscard]] constexpr auto get(ara::core::Array<T,N>&&) noexcept -> T&&;

} // namespace ara::core


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
 *  SECTION: Internal Utilities
 *********************************************************************************************************************/
/*!
 * \brief Contains internal details for handling internal utilities.
 *
 * This namespace encapsulates helper traits and functions that are used internally by the \c ara::core::
 * implementation. These details are subject to change and are not part of the public API.
 * all translation units that will share the same instance of these variables.
 * This means that the same AbortHandler array, count, and mutex are used across your entire program.
 * \note This not proposed by the Specification of Adaptive Platform Core
 */
namespace detail {

using AbortHandler = auto (*)() noexcept -> void;

/***********************************************************************************************************************
 *  INTERNAL: STORAGE & SYNCHRONIZATION FOR ABORT HANDLERS
***********************************************************************************************************************/
/* Maximum number of AbortHandlers supported. */
inline constexpr std::size_t kMaxAbortHandlers{8U};

/* Storage for installed AbortHandlers.
 * This fixed-size C-style array holds the pointers to the installed AbortHandlers.
 *
 * Note: We use inline variables (introduced in C++17) so that there is exactly one instance
 *       shared among all translation units that include this header. Using 'static' here would
 *       create a separate copy in each translation unit, which is not desired.
 */
inline AbortHandler g_abortHandlers[kMaxAbortHandlers]{};

/* Current count of installed AbortHandlers. */
inline std::size_t g_abortHandlerCount{0U};

/* Mutex to synchronize calls to Abort() and handler modifications.
 * While a call to Abort() is in progress, other threads will block.
 */
inline std::mutex g_abortMutex{};

/***********************************************************************************************************************
 *  INTERNAL: IS_CONSTANT_EVALUATED
 ***********************************************************************************************************************/
/*!
 * \brief  Helper function to determine if the current context is a constant evaluation.
 *
 * This function checks if the current context is a constant evaluation using
 * compiler-specific builtins or standard library features.
 *
 * \return  true if the context is a constant evaluation, false otherwise.
 *
 * \note    This function is used internally to determine if certain operations
 *          can be performed at compile-time or runtime.
 */
[[nodiscard]] constexpr auto is_constant_evaluated() noexcept -> bool {
    #if defined(__cpp_lib_is_constant_evaluated) \
        && (__cpp_lib_is_constant_evaluated >= 202002L)
        return std::is_constant_evaluated();              // C++20 standard API
    #elif defined(__has_builtin) && __has_builtin(__builtin_is_constant_evaluated)
        return __builtin_is_constant_evaluated();         // GCC/Clang builtin in C++17
    #elif defined(_MSC_VER)
        return __is_constant_evaluated();                 // MSVC intrinsic
    #else
        return false;                                     // fallback: always run-time
    #endif
}


/***********************************************************************************************************************
 *  INTERNAL: IS_TUPLE_LIKE
 ***********************************************************************************************************************/
/*!
 * \brief  Trait to detect if a type is tuple-like.
 *
 * \details This trait checks if the type T has a valid std::tuple_size specialization,
 *          indicating that it behaves like a tuple (supports structured bindings, get<I>, etc.).
 *          A tuple-like type must support:
 *          - std::tuple_size<T>
 *          - std::tuple_element<I, T>
 *          - get<I>(t) via ADL or std::get<I>
 *
 * \tparam T The type to check.
 *
 * \note This trait is SFINAE-friendly and won't cause hard errors for non-tuple-like types.
 */
template<typename T, typename = void>
struct is_tuple_like : std::false_type {};

template<typename T>
struct is_tuple_like<T,
        std::void_t<
            decltype(std::tuple_size<std::remove_cv_t<T>>::value),
            std::enable_if_t<(std::tuple_size<std::remove_cv_t<T>>::value >= 0)>
        >>
    : std::true_type {};

/*!
 * \brief  Helper variable template for is_tuple_like.
 */
template<typename T>
inline constexpr bool is_tuple_like_v = is_tuple_like<T>::value;

/**********************************************************************************************************************
 *  INVOKE RESULT HELPER (for better SFINAE)
 *********************************************************************************************************************/
/*!
 * \brief  Helper to safely detect the result of invoking a callable.
 *
 * \details Provides SFINAE-friendly way to detect if a callable can be invoked
 *          with given arguments and what the result type would be.
 */
template<typename F, typename... Args>
using invoke_result_t = decltype(std::declval<F>()(std::declval<Args>()...));

/**********************************************************************************************************************
 *  TUPLE CONVERSION UTILITIES
 *********************************************************************************************************************/
/*!
 * \brief  Implementation helper to convert a tuple-like type to std::tuple.
 *
 * \details Creates a tuple of values (not references) from a tuple-like object.
 *          This matches the behavior of std::tuple_cat which uses value semantics:
 *          - For lvalues: copies the elements
 *          - For rvalues: moves the elements
 *          - Always decays types (removes cv-qualifiers and references)
 *
 * \tparam Src The source tuple-like type.
 * \tparam I   Index sequence for unpacking.
 * \param  src The source tuple-like object.
 * \return     A std::tuple containing values (copies/moves) of the elements.
 *
 * \note Uses unqualified get for proper ADL lookup.
 */
template<typename Src, std::size_t... I>
[[nodiscard]] constexpr auto to_std_tuple_impl(Src&& src, std::index_sequence<I...>)
#ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
    noexcept(noexcept(std::make_tuple(get<I>(std::forward<Src>(src))...)))
#else
    noexcept
#endif
    -> decltype(std::make_tuple(get<I>(std::forward<Src>(src))...))
{
    // Using unqualified get for ADL - finds the best get via argument-dependent lookup
    // std::make_tuple applies std::decay to each argument:
    // - T& -> T (copy)
    // - const T& -> T (copy)
    // - T&& -> T (move)
    // - const T&& -> T (move)
    
#ifndef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
    static_assert(noexcept(std::make_tuple(get<I>(std::forward<Src>(src))...)),
        "\n[ERROR] detail::to_std_tuple_impl: Tuple construction must be noexcept when exceptions are disabled.\n");
#endif
    
    return std::make_tuple(get<I>(std::forward<Src>(src))...);
}

/*!
 * \brief  Converts a tuple-like type to std::tuple.
 *
 * \details Converts any tuple-like type (e.g., ara::core::Array, std::array, std::pair)
 *          into a std::tuple with value semantics. This is designed to work with
 *          std::tuple_cat and similar utilities that expect value semantics.
 *
 * \tparam Src The source tuple-like type.
 * \param  src The source tuple-like object.
 * \return     A std::tuple containing values (copies/moves) of the elements.
 *
 * \pre The type std::remove_reference_t<Src> must satisfy is_tuple_like_v.
 */
template<typename Src>
[[nodiscard]] constexpr auto to_std_tuple(Src&& src)
#ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
    noexcept(noexcept(to_std_tuple_impl(
        std::forward<Src>(src),
        std::make_index_sequence<std::tuple_size_v<
            std::remove_cv_t<std::remove_reference_t<Src>>>>{})))
#else
    noexcept
#endif
    -> decltype(to_std_tuple_impl(
        std::forward<Src>(src),
        std::make_index_sequence<std::tuple_size_v<
            std::remove_cv_t<std::remove_reference_t<Src>>>>{}))
{
    static_assert(is_tuple_like_v<std::remove_reference_t<Src>>,
        "\n[ERROR] detail::to_std_tuple: Type must be tuple-like (have tuple_size, tuple_element, and get).\n");
    
    constexpr std::size_t N = std::tuple_size_v<
        std::remove_cv_t<std::remove_reference_t<Src>>>;
    
#ifndef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
    static_assert(noexcept(to_std_tuple_impl(
        std::forward<Src>(src),
        std::make_index_sequence<N>{})),
        "\n[ERROR] detail::to_std_tuple: Conversion must be noexcept when exceptions are disabled.\n");
#endif
    
    return to_std_tuple_impl(std::forward<Src>(src),
                             std::make_index_sequence<N>{});
}

/**********************************************************************************************************************
 *  APPLY IMPLEMENTATION
 *********************************************************************************************************************/
/*!
 * \brief  Implementation helper for apply function.
 *
 * \details Expands the tuple-like object and invokes the callable with its elements.
 *          Perfectly forwards the elements to preserve value categories when calling
 *          the function. Uses unqualified get for proper ADL lookup.
 *
 * \tparam F     The callable type.
 * \tparam Tuple The tuple-like type.
 * \tparam I     Index sequence for unpacking.
 * \param  f     The callable to invoke.
 * \param  tup   The tuple-like object containing arguments.
 * \return       The result of invoking f with the tuple elements.
 *
 * \warning The result of this function should not be discarded if the callable
 *          returns a value that needs to be handled.
 */
template<typename F, typename Tuple, std::size_t... I>
[[nodiscard]] constexpr auto apply_impl(F&& f, Tuple&& tup, std::index_sequence<I...>)
#ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
    noexcept(noexcept(std::forward<F>(f)(get<I>(std::forward<Tuple>(tup))...)))
#else
    noexcept
#endif
    -> invoke_result_t<F&&, decltype(get<I>(std::forward<Tuple>(tup)))...>
{
    // Using unqualified get for ADL - this allows user-defined types to provide
    // their own get function that will be found via argument-dependent lookup
    
#ifndef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
    static_assert(noexcept(std::forward<F>(f)(get<I>(std::forward<Tuple>(tup))...)),
        "\n[ERROR] detail::apply_impl: Callable invocation must be noexcept when exceptions are disabled.\n");
#endif
    
    return std::forward<F>(f)(get<I>(std::forward<Tuple>(tup))...);
}

/**********************************************************************************************************************
 *  TO_ARRAY IMPLEMENTATION HELPERS
 *********************************************************************************************************************/
/*!
 * \brief  Implementation helper for to_array with lvalue arrays.
 *
 * \details Creates an ara::core::Array by copying elements from a built-in array.
 *          This is used to implement ara::core::to_array for lvalue arrays.
 *
 * \tparam T The element type.
 * \tparam N The number of elements.
 * \tparam I Index sequence for unpacking.
 * \param  a The built-in array to copy from.
 * \return   An ara::core::Array containing copies of the elements.
 */
template <typename T, std::size_t N, std::size_t... I>
[[nodiscard]] constexpr auto to_array_impl(T (&a)[N], std::index_sequence<I...>)
#ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
    noexcept(std::is_nothrow_copy_constructible_v<T>)
#else
    noexcept
#endif
    -> ara::core::Array<std::remove_cv_t<T>, N>
{
#ifndef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
    static_assert(std::is_nothrow_copy_constructible_v<T>,
        "\n[ERROR] detail::to_array_impl: Type T must be nothrow copy constructible when exceptions are disabled.\n");
#endif
    
    return {{a[I]...}};
}

/*!
 * \brief  Implementation helper for to_array with rvalue arrays.
 *
 * \details Creates an ara::core::Array by moving elements from a built-in array.
 *          This is used to implement ara::core::to_array for rvalue arrays.
 *
 * \tparam T The element type.
 * \tparam N The number of elements.
 * \tparam I Index sequence for unpacking.
 * \param  a The built-in array to move from.
 * \return   An ara::core::Array containing moved elements.
 */
template <typename T, std::size_t N, std::size_t... I>
[[nodiscard]] constexpr auto to_array_impl(T (&&a)[N], std::index_sequence<I...>)
#ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
    noexcept(std::is_nothrow_move_constructible_v<T>)
#else
    noexcept
#endif
    -> ara::core::Array<std::remove_cv_t<T>, N>
{
#ifndef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
    static_assert(std::is_nothrow_move_constructible_v<T>,
        "\n[ERROR] detail::to_array_impl: Type T must be nothrow move constructible when exceptions are disabled.\n");
#endif
    
    return {{std::move(a[I])...}};
}
/*!
 * \brief Trait to detect whether an array of type T[N] can be list‑initialized
 *        with arguments of types Args... without narrowing conversions.
 *
 * This trait uses SFINAE to check the validity of the list‑initialization expression:
 *   T[N]{ std::declval<Args>()... }
 * If well‑formed (and no narrowing occurs), it inherits from std::true_type;
 * otherwise, from std::false_type.
 *
 * \tparam T    The element type of the array.
 * \tparam N    The number of elements in the array.
 * \tparam Args The types of the initializer arguments.
 *
 * \note Conforms to AUTOSAR C++ Guidelines (SWS_CORE_11200).
 * \since C++17
 */
template <typename T, std::size_t N, typename... Args>
struct is_brace_initializable_array
{
private:
    /* Selected if U[N]{ Args... } is well‑formed */
    template <typename U, typename = decltype(U{ std::declval<Args>()... })>
    [[nodiscard]] static auto test(int) noexcept -> std::true_type;

    /* Fallback if substitution in the above fails */
    template <typename...>
    [[nodiscard]] static auto test(...) noexcept -> std::false_type;

public:
    /* Integral constant type: std::true_type or std::false_type */
    using type = decltype(test<T[N]>(0));

    /* Shorthand for the boolean result (for trait compatibility) */
    using value_type = bool;

    /* The raw boolean result of the check */
    static constexpr value_type value = type::value;
};

/*!
 * \brief Partial specialization for zero‑length arrays.
 *
 * For N == 0, no elements exist, so list‑initialization with any Args...
 * is considered invalid. This specialization always inherits from
 * std::false_type without attempting SFINAE on T[0].
 *
 * \tparam T    The element type of the array.
 * \tparam Args The types of the initializer arguments.
 *
 * \note Conforms to AUTOSAR C++ Guidelines (SWS_CORE_11200).
 * \since C++17
 */
template <typename T, typename... Args>
struct is_brace_initializable_array<T, 0, Args...> : std::false_type
{
    using type       = std::false_type;
    using value_type = bool;
    static constexpr value_type value = false;
};

/*!
 * \brief Variable template for is_brace_initializable_array.
 *
 * Simplifies usage:
 *   if constexpr (is_brace_initializable_array_v<MyType, 3, int, double, char>) { … }
 *
 * \tparam T    The element type of the array.
 * \tparam N    The number of elements in the array.
 * \tparam Args The types of the initializer arguments.
 */
template <typename T, std::size_t N, typename... Args>
inline constexpr bool is_brace_initializable_array_v =
    is_brace_initializable_array<T, N, Args...>::value;



/*!
 * \brief Primary helper trait to detect if a type is an \c ara::core::Array.
 *
 * By default, any type is not considered an \c ara::core::Array.
 *
 * \tparam T The type to check.
 */
template <typename...>
struct is_array : std::false_type {};

/*!
 * \brief Specialization for \c ara::core::Array.
 *
 * If a type matches \c ara::core::Array<T, N> for any \c T and \c N,
 * this trait yields \c std::true_type.
 *
 * \tparam T The element type.
 * \tparam N The size of the array.
 */
template <typename T, std::size_t N>
struct is_array<ara::core::Array<T, N>> : std::true_type {};

/*!
 * \brief Trait to detect if the parameter pack \c Args contains exactly one argument
 *        and that (after decay) is an \c ara::core::Array.
 *
 * This trait evaluates to \c true if:
 * - The number of arguments is exactly one, and
 * - The decayed type of that argument is recognized as an \c ara::core::Array.
 *
 * The fold expression (\c is_array<std::decay_t<Args>>::value && ...) applies the check to the
 * argument (in this case just one) and returns \c true only if the condition holds.
 *
 * \tparam Args The types of the arguments.
 */
template <typename... Args>
struct is_single_same_array 
    : std::bool_constant<
          (sizeof...(Args) == 1) && (is_array<std::decay_t<Args>>::value && ...)
      > 
{};

/*!
 * \brief Convenience variable template for \c is_single_same_array.
 *
 * This variable template allows for a simplified syntax:
 *
 * \code
 * if constexpr (is_single_same_array_v<ArgType>)
 * {
 *     // ...
 * }
 * \endcode
 *
 * \tparam Args The types of the arguments.
 */
template <typename... Args>
inline constexpr bool is_single_same_array_v = is_single_same_array<Args...>::value;

/*!
 * \brief  Trait to detect if a type has bitwise equality.
 *
 * This trait checks if a type can be compared for equality using bitwise operations.
 * It requires the type to be trivially copyable, standard layout, and not a floating-point type or bool.
 *
 * \tparam T The type to check.
 */
template<typename T>
struct has_bitwise_equality 
    : std::conjunction<
        std::is_trivially_copyable<T>,
        std::is_standard_layout<T>,
        std::negation<std::is_floating_point<T>>,
        std::negation<std::is_same<T, bool>>
      > {};

/*! \brief Variable template for has_bitwise_equality.
 * This variable template allows for a simplified syntax to check if a type has bitwise equality.
 * \tparam T The type to check.
 * \details
 * - Evaluates to true if the type is trivially copyable, standard layout,
 *   and not a floating-point type or bool.
 * - Can be used in constexpr contexts to enable compile-time checks.
 */
template<typename T>
inline constexpr bool has_bitwise_equality_v = has_bitwise_equality<T>::value;

/*!
 * \brief  Trait to detect if a type is byte-comparable.
 *
 * This trait checks if a type can be compared byte-wise, which is typically the case for
 * unsigned integral types of size 1 (e.g., uint8_t).
 *
 * \tparam T The type to check.
 */
template<typename T>
struct byte_comparable
    : std::bool_constant<
          std::is_trivially_copyable_v<T> &&
          std::is_standard_layout_v<T>   &&
          std::is_unsigned_v<T>          &&
          sizeof(T) == 1
      > {};

/*! \brief Variable template for byte_comparable.
 * This variable template allows for a simplified syntax to check if a type is byte-comparable.
 * \tparam T The type to check.
 * \details
 * - Evaluates to true if the type is trivially copyable, standard layout,
 *   unsigned, and has a size of 1 byte.
 * - Can be used in constexpr contexts to enable compile-time checks.
 */
template<typename T>
inline constexpr bool byte_comparable_v = byte_comparable<T>::value;


template<typename CharT, typename = void>
struct default_traits
{
    /*  Pull the traits through a system-header alias – no warnings here. */
    using type = typename std::basic_string_view<CharT>::traits_type;
};

/*  const-qualified forms forward to the base type (AUTOSAR-style). */
template<typename CharT>
struct default_traits<const CharT, void> : default_traits<CharT> {};

/*!
 * \brief Performs a lexicographical comparison between two ranges.
 *
 * This function compares the elements in the ranges \c [first1,last1) and \c [first2,last2)
 * one by one:
 * - For each pair of corresponding elements, if the element from the first range is less than the element
 *   from the second range, the function returns \c true.
 * - If the element from the second range is less than the element from the first range, the function returns \c false.
 * - If the elements are equal, the comparison continues.
 * - When the end of one of the ranges is reached:
 *     - If the first range is exhausted but the second still has elements, the first range is considered
 *       lexicographically less and the function returns \c true.
 *     - Otherwise, it returns \c false.
 *
 * \tparam InputIt1 The type of the input iterator for the first range.
 * \tparam InputIt2 The type of the input iterator for the second range.
 * \param first1 An iterator pointing to the first element of the first range.
 * \param last1  An iterator pointing past the last element of the first range.
 * \param first2 An iterator pointing to the first element of the second range.
 * \param last2  An iterator pointing past the last element of the second range.
 * \return \c true if the first range is lexicographically less than the second range; \c false otherwise.
 *
 * \note The function is declared as \c constexpr so that if both the iterators and the element comparison
 *       are \c constexpr, the entire operation can be evaluated at compile time.
 */
template<typename InputIt1, typename InputIt2>
[[nodiscard]] constexpr bool constexpr_lexicographical_compare(InputIt1 first1, InputIt1 last1,
                                                                InputIt2 first2, InputIt2 last2)
#ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
    noexcept(noexcept(*first1 < *first2) && noexcept(*first1 == *first2))
#else
    noexcept
#endif
{   

#ifndef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
    static_assert(noexcept(*first1 < *first2) && noexcept(*first1 == *first2),
        "\n[ERROR] in ara::core::constexpr_lexicographical_compare: "
        "The element comparison must be noexcept when exceptions are disabled.\n");
#endif

    for (; (first1 != last1) && (first2 != last2); ++first1, ++first2) {
        if (*first1 < *first2) return true;
        if (*first2 < *first1) return false;
    }
    return (first1 == last1) && (first2 != last2);
}

/*!
 * \brief  Constexpr-compatible find implementation for C++17
 *
 * \tparam InputIt Iterator type
 * \tparam T Value type to find
 * \param first Beginning of the range to search
 * \param last End of the range to search
 * \param value Value to find
 * \return Iterator to the first element equal to value, or last if not found
 */
template<typename InputIt, typename T>
[[nodiscard]] constexpr InputIt constexpr_find(InputIt first, InputIt last, const T& value) {
    for (; first != last; ++first) {
        if (*first == value) {
            return first;
        }
    }
    return last;
}

/*!
 * \brief  Constexpr-compatible equal implementation
 *
 * \tparam InputIt1 First range iterator type
 * \tparam InputIt2 Second range iterator type
 * \param first1 Beginning of the first range
 * \param last1 End of the first range
 * \param first2 Beginning of the second range
 * \return true if all corresponding elements are equal, false otherwise
 */
template<typename InputIt1, typename InputIt2>
[[nodiscard]] constexpr bool constexpr_equal(InputIt1 first1, InputIt1 last1, InputIt2 first2) {
    for (; first1 != last1; ++first1, ++first2) {
        if (!(*first1 == *first2)) {
            return false;
        }
    }
    return true;
}

/*!
 * \brief Constexpr strlen implementation for C++17
 *
 * \tparam CharT Character type
 * \param str Null-terminated string
 * \return Length of the string, excluding the null terminator
 */
template<typename CharT>
[[nodiscard]] constexpr std::size_t constexpr_strlen(const CharT* str) noexcept
{
    if (str == nullptr) return 0;              // or omit for stricter C semantics

    if constexpr (std::is_same_v<CharT, char> &&
                  __has_builtin(__builtin_strlen)) {
        return __builtin_strlen(str);          // fast, constexpr-friendly path
    } else {
        std::size_t len = 0;
        while (str[len] != CharT{}) ++len;
        return len;
    }
}

 /*!
 * \brief Extract basename from a file path at compile time
 *
 * \param path Full file path
 * \return Pointer to the basename portion of the path
 *
 * \details Handles both forward and backward slashes as path separators
 */
[[nodiscard]] constexpr auto constexpr_basename(const char* path) noexcept -> const char* {
    if (path == nullptr) return "";

    const char* last_separator = path;
    const char* current = path;

    while (*current != '\0') {
        if (*current == '/' || *current == '\\') {
            last_separator = current + 1;
        }
        ++current;
    }

    return last_separator;
}

/*!
 * \brief Constexpr char_traits::compare for C++17
 *
 * \tparam CharT Character type
 * \tparam Traits Character traits type
 * \param s1 First string to compare
 * \param s2 Second string to compare
 * \param count Number of characters to compare
 * \return Negative if s1 < s2, positive if s1 > s2, zero if equal
 */
template<typename CharT, typename Traits>
[[nodiscard]] constexpr auto constexpr_compare(const CharT* s1, const CharT* s2, std::size_t count) noexcept -> int
{
    for (std::size_t i = 0; i < count; ++i) {
        if (Traits::lt(s1[i], s2[i])) return -1;
        if (Traits::lt(s2[i], s1[i])) return 1;
    }
    return 0;
}

/*!
 * \brief Constexpr find implementation for character sequences
 *
 * \tparam CharT Character type
 * \tparam Traits Character traits type
 * \param str String to search in
 * \param count Length of the string
 * \param ch Character to find
 * \return Pointer to the first occurrence of ch, or nullptr if not found
 */
template<typename CharT, typename Traits>
[[nodiscard]] constexpr auto constexpr_find(const CharT* str, std::size_t count, CharT ch) noexcept -> const CharT*
{
    for (std::size_t i = 0; i < count; ++i) {
        if (Traits::eq(str[i], ch)) {
            return str + i;
        }
    }
    return nullptr;
}

/*!
 * \brief Constexpr search implementation for substring search
 *
 * \tparam CharT Character type
 * \tparam Traits Character traits type
 * \param first Beginning of the string to search in
 * \param last End of the string to search in
 * \param s_first Beginning of the substring to find
 * \param s_last End of the substring to find
 * \return Pointer to the first occurrence of the substring, or last if not found
 */
template<typename CharT, typename Traits>
[[nodiscard]] constexpr auto constexpr_search(const CharT* first, const CharT* last,
                                              const CharT* s_first, const CharT* s_last) noexcept -> const CharT*
{
    const auto len = static_cast<std::size_t>(last - first);
    const auto s_len = static_cast<std::size_t>(s_last - s_first);
    
    if (s_len == 0) return first;
    if (s_len > len) return last;
    
    for (std::size_t i = 0; i <= len - s_len; ++i) {
        bool found = true;
        for (std::size_t j = 0; j < s_len; ++j) {
            if (!Traits::eq(first[i + j], s_first[j])) {
                found = false;
                break;
            }
        }
        if (found) return first + i;
    }
    return last;
}

/**********************************************************************************************************************
 *  SPAN: IMPLEMENTATION DETAILS - TYPE TRAITS AND SFINAE HELPERS
 *********************************************************************************************************************/
 
/*!
 * \brief  void_t helper for detection idiom
 */
template<typename...>
using void_t = void;

/*!
 * \brief  Detection helper for C-style arrays
 */
template<typename T>
struct is_c_array : std::false_type {};

template<typename T, std::size_t N>
struct is_c_array<T[N]> : std::true_type {};

template<typename T>
inline constexpr bool is_c_array_v = is_c_array<T>::value;

/*!
 * \brief  Detection helper for std::array
 */
template<typename T>
struct is_std_array : std::false_type {};

template<typename T, std::size_t N>
struct is_std_array<std::array<T, N>> : std::true_type {};

template<typename T>
inline constexpr bool is_std_array_v = is_std_array<T>::value;

/*!
 * \brief  Detection helper for ara::core::Array
 */
template<typename T>
struct is_ara_array : std::false_type {};

template<typename T, std::size_t N>
struct is_ara_array<ara::core::Array<T, N>> : std::true_type {};

template<typename T>
inline constexpr bool is_ara_array_v = is_ara_array<T>::value;

/*!
 * \brief  Detection helper for Span specializations
 */
template<typename T>
struct is_span : std::false_type {};

template<typename T, std::size_t E>
struct is_span<Span<T, E>> : std::true_type {};

template<typename T>
inline constexpr bool is_span_v = is_span<T>::value;

/*!
 * \brief  Check if type has contiguous storage (data() and size() members)
 */
template<typename T, typename = void>
struct has_data_and_size : std::false_type {};

template<typename T>
struct has_data_and_size<T, void_t<
    decltype(std::declval<T&>().data()),
    decltype(std::declval<T&>().size()),
    std::enable_if_t<std::is_pointer_v<decltype(std::declval<T&>().data())>>
>> : std::true_type {};

template<typename T>
inline constexpr bool has_data_and_size_v = has_data_and_size<T>::value;

/*!
 * \brief  Check if type is a range (has begin() and end())
 */
template<typename T, typename = void>
struct is_range : std::false_type {};

template<typename T>
struct is_range<T, void_t<
    decltype(std::begin(std::declval<T&>())),
    decltype(std::end(std::declval<T&>()))
>> : std::true_type {};

template<typename T>
inline constexpr bool is_range_v = is_range<T>::value;

template<typename, typename = void>
struct has_member_pointer : std::false_type { };

template<typename T>
struct has_member_pointer<T, std::void_t<typename std::decay_t<T>::pointer>>
    : std::true_type { };

template<typename T>
inline constexpr bool has_member_pointer_v = has_member_pointer<T>::value;

/**********************************************************************************************************************
 *  detail::is_contiguous_container_v   (owning, STL-style container)
 *      – requires  data() / size() *and* a  “pointer”  typedef
 *********************************************************************************************************************/
template<typename T, typename = void>
struct is_contiguous_container : std::false_type { };

template<typename T>
struct is_contiguous_container<T, std::void_t<
    decltype(std::declval<T&>().data()),            /* member .data()      */
    decltype(std::declval<T&>().size()),            /* member .size()      */
    std::enable_if_t<
        std::is_pointer_v<decltype(std::declval<T&>().data())>>,
    std::enable_if_t<
        std::is_integral_v<decltype(std::declval<T&>().size())>>,
    std::enable_if_t<has_member_pointer_v<T>>       /* <- NEW requirement */
>> : std::true_type { };

template<typename T>
inline constexpr bool is_contiguous_container_v =
    is_contiguous_container<std::remove_cv_t<std::remove_reference_t<T>>>::value;

/**********************************************************************************************************************
 *  detail::is_contiguous_range_v   (view / subrange / span / string_view …)
 *      – contiguous range  *without* a “pointer” typedef (so it’s NOT a container)
 *********************************************************************************************************************/
template<typename T, typename = void>
struct is_contiguous_range : std::false_type { };

template<typename T>
struct is_contiguous_range<T, std::void_t<
    std::enable_if_t<!is_contiguous_container_v<T>>,/* exclude containers  */
    decltype(std::begin(std::declval<T&>())),
    decltype(std::end  (std::declval<T&>())),
    decltype(std::data (std::declval<T&>())),
    decltype(std::size (std::declval<T&>())),
    std::enable_if_t<
        std::is_pointer_v<decltype(std::data(std::declval<T&>()))>>,
    std::enable_if_t<
        std::is_integral_v<decltype(std::size(std::declval<T&>()))>>
>> : std::true_type { };

template<typename T>
inline constexpr bool is_contiguous_range_v = is_contiguous_range<T>::value;

/*!
 * \brief  Check element type compatibility for span construction
 */
template<typename From, typename To>
struct is_element_type_compatible : std::false_type {};

template<typename From, typename To>
struct is_element_type_compatible<From*, To> : std::bool_constant<
    std::is_convertible_v<From(*)[], To(*)[]>
> {};

template<typename From, typename To>
inline constexpr bool is_element_type_compatible_v = is_element_type_compatible<From, To>::value;

/*!
 * \brief  Always-false helper for static_asserts in dependent contexts
 */
template<typename...>
inline constexpr bool dependent_false_v = false;

/*!
 * \brief  Detect whether \c T provides \c data() and \c size() that are both noexcept.
 *
 *         Raw pointers are deliberately *excluded* from being considered string-like.
 */
template<typename T, typename = void>
struct has_string_like_interface : std::false_type {};

template<typename T>
struct has_string_like_interface<T,
    std::void_t<
        decltype(std::data(std::declval<const T&>())),
        decltype(std::size(std::declval<const T&>()))
    >
> : std::true_type {};

/*! \brief  Raw pointers are *not* string-like. */
template<typename T>
struct has_string_like_interface<T*, void> : std::false_type {};

template<typename T>
inline constexpr bool has_string_like_interface_v =
    has_string_like_interface<T>::value;

/*!
 * \brief  Determine whether \c T can be converted into a pointer to \c CharT via
 *         its \c data() member (only checked if the type passed the previous test).
 */
template<typename T, typename CharT, bool = has_string_like_interface_v<T>>
struct is_string_view_compatible_impl : std::false_type {};

template<typename T, typename CharT>
struct is_string_view_compatible_impl<T, CharT, true>
    : std::bool_constant<
          std::is_convertible_v<
              decltype(std::data(std::declval<const T&>())),
              const CharT*
          >
      > {};

template<typename T, typename CharT>
inline constexpr bool is_string_view_compatible_v =
    is_string_view_compatible_impl<T, CharT>::value;

/*!
 * \brief  Always-false helper for static_asserts in dependent contexts
 */
template<typename...>
inline constexpr bool dependent_false_v = false;

/*!
 * \brief  Detect whether \c T provides \c data() and \c size() that are both noexcept.
 *
 *         Raw pointers are deliberately *excluded* from being considered string-like.
 */
template<typename T, typename = void>
struct has_string_like_interface : std::false_type {};

template<typename T>
struct has_string_like_interface<T,
    std::void_t<
        decltype(std::data(std::declval<const T&>())),
        decltype(std::size(std::declval<const T&>()))
    >
> : std::true_type {};

/*! \brief  Raw pointers are *not* string-like. */
template<typename T>
struct has_string_like_interface<T*, void> : std::false_type {};

template<typename T>
inline constexpr bool has_string_like_interface_v =
    has_string_like_interface<T>::value;

/*!
 * \brief  Determine whether \c T can be converted into a pointer to \c CharT via
 *         its \c data() member (only checked if the type passed the previous test).
 */
template<typename T, typename CharT, bool = has_string_like_interface_v<T>>
struct is_string_view_compatible_impl : std::false_type {};

template<typename T, typename CharT>
struct is_string_view_compatible_impl<T, CharT, true>
    : std::bool_constant<
          std::is_convertible_v<
              decltype(std::data(std::declval<const T&>())),
              const CharT*
          >
      > {};

template<typename T, typename CharT>
inline constexpr bool is_string_view_compatible_v =
    is_string_view_compatible_impl<T, CharT>::value;


/**********************************************************************************************************************
 *  SECTION: Array Storage Base Classes
 *********************************************************************************************************************/
/*!
* \brief Base class for array storage.
*
* Splits out storage to handle partial specialization for \c N > 0 versus \c N == 0.
*
* \tparam T The type of elements.
* \tparam N The number of elements in the array.
* \tparam B A boolean indicating whether \c N > 0.
*
* \note [SWS_CORE_01201]
*/
template <typename T, std::size_t N, bool B = (N > 0)>
struct ArrayStorage;

/*!
 * \brief Primary template for Array storage when \c N > 0.
 *
 * Provides the actual storage for \c N elements of type \c T and supports direct brace‑initialization.
 *
 * \tparam T The element type.
 * \tparam N The number of elements in the array.
 *
 * \note [SWS_CORE_01201]
 */
template <typename T, std::size_t N>
struct ArrayStorage<T, N, true> {
protected:
    /*! \brief Actual storage for \c N elements of type \c T. */
    alignas(T) T data_[N];

public:
    /*!
     * \brief Variadic constructor to initialize \c data_ with up to \c N arguments using brace‑initialization.
     *
     * The constructor is constrained to accept at most \c N arguments, all of which must be convertible to \c T,
     * and such that brace‑initialization does not cause narrowing conversions.
     *
     * \tparam Args The types of the constructor arguments.
     * \param args The arguments to initialize the array.
     *
     * \note [SWS_CORE_01201], [SWS_CORE_01214], [SWS_CORE_01215], [SWS_CORE_01241]
     */
    template <typename... Args,
              typename = std::enable_if_t<
                  // Condition #1: Must not exceed N arguments
                  (sizeof...(Args) <= N) &&
                  // Condition #2: Each argument must be convertible to T
                  (std::conjunction_v<std::is_convertible<Args, T>...>) &&
                  // Condition #3: Brace-initialization does not cause narrowing
                  (detail::is_brace_initializable_array_v<T, N, Args...>) &&
                  // Condition #4: Prevent constructor from being selected when Args... is exactly Array<T, N>
                  (!detail::is_single_same_array_v<Args...>) &&
                  // Condition #5: Not empty parameter pack (use default constructor instead)
                  (sizeof...(Args) > 0)
              >>
    constexpr ArrayStorage(Args&&... args)
#ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
        noexcept(std::conjunction_v<std::is_nothrow_constructible<T, Args&&>...>)
#else
        noexcept
#endif
        : data_{std::forward<Args>(args)...} 
    {
#ifndef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
        static_assert(std::conjunction_v<std::is_nothrow_constructible<T, Args&&>...>,
            "\n[ERROR] in ara::core::Array: The type T and args must be noexcept.\n");
#endif  
    }

    // -----------------------------------------------------------------------------------
    //  REJECTING CONSTRUCTOR (TOO MANY OR WRONG TYPES) [SWS_CORE_01241]
    // -----------------------------------------------------------------------------------
    /*!
     * \brief Overload constructor that catches calls violating the above constraints.
     *
     * \tparam Args  Parameter pack that either exceeds N or has arguments not convertible to T.
     * \note         This never actually constructs anything; it only fires `static_assert` errors.
     *
     * \note  [SWS_CORE_01241]
     */
    template <
        typename... Args,
        // Condition: either too many arguments OR not all convertible OR narrowing
        typename = std::enable_if_t<
            (!detail::is_single_same_array_v<Args...>) && 
            ( (sizeof...(Args) > N) || 
              (!std::conjunction_v<std::is_convertible<Args, T>...>) ||
              (!detail::is_brace_initializable_array_v<T, N, Args...>) )
        >,
        int = 0
    >
    constexpr explicit ArrayStorage(Args&&...) noexcept
    {
        static_assert(sizeof...(Args) <= N,
            "\n[ERROR] Too many arguments passed to Array<T,N> constructor!\n"
            "        Up to N elements are allowed.\n");

        static_assert(std::conjunction_v<std::is_convertible<Args, T>...>,
            "\n[ERROR] One or more arguments cannot be converted to T.\n");

        static_assert(detail::is_brace_initializable_array_v<T, N, Args...>,
            "\n[ERROR] Brace-initialization would cause narrowing conversions.\n");
    }

    /*!
     * \brief Default constructor for the storage.
     *
     * Zero‑initializes \c data_.
     */
    constexpr ArrayStorage() noexcept = default;

    /*!
     * \brief Defaulted copy constructor.
     */
    constexpr ArrayStorage(const ArrayStorage&) noexcept = default;

    /*!
     * \brief Defaulted move constructor.
     */
    constexpr ArrayStorage(ArrayStorage&&) noexcept = default;

    /*!
     * \brief Defaulted copy assignment operator.
     */
    constexpr ArrayStorage& operator=(const ArrayStorage&) noexcept = default;

    /*!
     * \brief Defaulted move assignment operator.
     */
    constexpr ArrayStorage& operator=(ArrayStorage&&) noexcept = default;
};

/*!
 * \brief Partial specialization for Array storage when \c N == 0.
 *
 * No actual storage is allocated for zero‑sized arrays.
 *
 * \tparam T The element type.
 * \tparam N The (zero) number of elements in the array.
 *
 * \note [SWS_CORE_01201]
 */
template <typename T, std::size_t N>
struct ArrayStorage<T, N, false> {
public:
    /*!
     * \brief Variadic constructor for \c N == 0.
     *
     * This constructor is enabled only when no arguments are provided.
     *
     * \tparam Args The types of constructor arguments (must be empty).
     */
    template <typename... Args,
              typename = std::enable_if_t<(sizeof...(Args) == 0)>>
    constexpr explicit ArrayStorage(Args&&...) noexcept { /* Do Nothing */ }

    /*!
     * \brief Default constructor for zero‑sized storage.
     */
    constexpr ArrayStorage() noexcept = default;

    /*!
     * \brief Defaulted copy constructor.
     */
    constexpr ArrayStorage(const ArrayStorage&) noexcept = default;

    /*!
     * \brief Defaulted move constructor.
     */
    constexpr ArrayStorage(ArrayStorage&&) noexcept = default;

    /*!
     * \brief Defaulted copy assignment operator.
     */
    constexpr ArrayStorage& operator=(const ArrayStorage&) noexcept = default;

    /*!
     * \brief Defaulted move assignment operator.
     */
    constexpr ArrayStorage& operator=(ArrayStorage&&) noexcept = default;
};

/***********************************************************************************************************************
 *  SPAN: STORAGE BASE CLASS
 ***********************************************************************************************************************/
/*!
 * \brief  Storage optimization for static vs dynamic extent
 */
template<typename ElementType, std::size_t Extent>
class span_storage_base {
protected:
    ElementType* data_ = nullptr;
    
    constexpr span_storage_base() noexcept = default;
    constexpr span_storage_base(ElementType* data, std::size_t) noexcept : data_(data) {}
    
    [[nodiscard]] constexpr std::size_t size() const noexcept { return Extent; }
    constexpr void set_size(std::size_t) noexcept {}
};

template<typename ElementType>
class span_storage_base<ElementType, dynamic_extent> {
protected:
    ElementType* data_ = nullptr;
    std::size_t size_ = 0;
    
    constexpr span_storage_base() noexcept = default;
    constexpr span_storage_base(ElementType* data, std::size_t size) noexcept 
        : data_(data), size_(size) {}
    
    [[nodiscard]] constexpr std::size_t size() const noexcept { return size_; }
    constexpr void set_size(std::size_t size) noexcept { size_ = size; }
};

/*!
 * \brief  Calculate subspan extent at compile time
 */
template<std::size_t Extent, std::size_t Offset, std::size_t Count>
struct subspan_extent {
    static constexpr std::size_t value = 
        Count != dynamic_extent ? Count :
        (Extent != dynamic_extent ? Extent - Offset : dynamic_extent);
};

} // namespace detail
} // namespace core
} // namespace ara

#endif // ARA_CORE_INTERNAL_UTILITY_H_