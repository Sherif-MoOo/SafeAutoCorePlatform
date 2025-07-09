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
#include <cstring>                                  // For std::memcpy, std::memmove, std::memcmp
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
[[nodiscard]] inline constexpr auto is_constant_evaluated() noexcept -> bool {
    #if defined(__cpp_lib_is_constant_evaluated) \
        && (__cpp_lib_is_constant_evaluated >= 202002L)
        return std::is_constant_evaluated();              // C++20 standard API
    #elif defined(__has_builtin) && __has_builtin(__builtin_is_constant_evaluated)
        return __builtin_is_constant_evaluated();         // GCC/Clang builtin in C++17
    #elif defined(_MSC_VER)
        return __is_constant_evaluated();                 // MSVC intrinsic
    #elif defined(__GNUC__)
        return __builtin_constant_p(0);                   // legacy GCC / QNX 7.x fallback
    #else
        return false;                                     // fallback: always run-time
    #endif
}

template<typename Ptr>
[[nodiscard]] inline constexpr auto to_address(const Ptr& p) noexcept
    -> decltype(std::pointer_traits<Ptr>::to_address(p));

template<typename Raw>
[[nodiscard]] inline constexpr auto to_address(Raw* p) noexcept -> Raw*;

// definitions
template<typename Raw>
[[nodiscard]] inline constexpr auto to_address(Raw* p) noexcept -> Raw* {
    return p;
}

template<typename Ptr>
[[nodiscard]] inline constexpr auto to_address(const Ptr& p) noexcept
    -> decltype(std::pointer_traits<Ptr>::to_address(p))
{
#if defined(__cpp_lib_to_address) && (__cpp_lib_to_address >= 201711L)
    return std::to_address(p);
#else
    return std::pointer_traits<Ptr>::to_address(p);
#endif
}

template<typename Ptr>
[[nodiscard]] inline constexpr auto to_address(const Ptr& p) noexcept {
    return ara::core::detail::to_address(p.operator->());
}

/***********************************************************************************************************************
 *  BRANCH PREDICTION HINTS
 ***********************************************************************************************************************/
[[nodiscard]] inline constexpr auto likely(bool x) noexcept -> bool{
#if defined(__has_builtin) && __has_builtin(__builtin_expect)
    return __builtin_expect(!!x, 1);
#else
    return x;
#endif
}

[[nodiscard]] inline constexpr bool unlikely(bool x) noexcept {
#if defined(__has_builtin) && __has_builtin(__builtin_expect)
    return __builtin_expect(!!x, 0);
#else
    return x;
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

template<class CharT, class Traits>
inline constexpr bool is_default_char_traits_v =
    std::is_same_v<Traits, typename default_traits<CharT>::type>;

template<typename T>
using is_real_pointer = std::bool_constant<
    std::is_pointer_v<std::remove_reference_t<T>>>;

template<typename T>
inline constexpr bool is_real_pointer_v = is_real_pointer<T>::value;

/*!
 * \brief SFINAE helper to detect if a type is a basic_string (including rvalues)
 */
template<typename T>
struct is_basic_string : std::false_type {};

template<typename CharT, typename Traits, typename Alloc>
struct is_basic_string<std::basic_string<CharT, Traits, Alloc>> : std::true_type {};

template<typename T>
inline constexpr bool is_basic_string_v = is_basic_string<std::decay_t<T>>::value;

/**********************************************************************************************************************
 *  SPAN: IMPLEMENTATION DETAILS - TYPE TRAITS AND SFINAE HELPERS
 *********************************************************************************************************************/
 
/*!
 * \brief  void_t helper for detection idiom
 */
template<typename...>
using void_t = void;


#if __cplusplus >= 202002L
template<typename T> using remove_cvref_t = std::remove_cvref_t<T>;
#else
template<typename T>
using remove_cvref_t =
    typename std::remove_cv<typename std::remove_reference<T>::type>::type;
#endif
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
 * \brief  Detect if type satisfies iterator requirements.
 * \tparam T Type to test.
 */
template<typename T>
struct is_iterator_impl {
    template<typename U>
    static auto test(int) -> decltype(
        typename std::iterator_traits<U>::iterator_category{},
        ++std::declval<U&>(),
        *std::declval<U>(),
        std::declval<U>() != std::declval<U>(),
        std::true_type{}
    );
    
    template<typename>
    static auto test(...) -> std::false_type;
    
    using type = decltype(test<T>(0));
};

template<typename T>
using is_iterator = typename is_iterator_impl<T>::type;

template<typename T>
inline constexpr bool is_iterator_v = is_iterator<T>::value;

template<typename It>
using iterator_category_t = typename std::iterator_traits<It>::iterator_category;

template<typename It>
using iterator_value_t = typename std::iterator_traits<It>::value_type;

template<typename It>
using iterator_reference_t = typename std::iterator_traits<It>::reference;

template<typename It>
constexpr bool is_input_iterator_v = std::is_base_of_v<std::input_iterator_tag, iterator_category_t<It>>;

template<typename It>
constexpr bool is_random_access_iterator_v = std::is_base_of_v<std::random_access_iterator_tag, iterator_category_t<It>>;

// Check if two iterators have compatible value types for comparison
template<typename It1, typename It2>
constexpr bool has_compatible_value_types_v = std::is_same_v<
    std::decay_t<iterator_value_t<It1>>,
    std::decay_t<iterator_value_t<It2>>
>;

/*!
 * \brief  SFINAE-friendly iterator category detection.
 * \tparam T Type to test for iterator category.
 */
template<typename T, typename = void>
struct has_iterator_category : std::false_type {};

template<typename T>
struct has_iterator_category<T, std::void_t<
    typename std::iterator_traits<T>::iterator_category
>> : std::true_type {};

template<typename T>
inline constexpr bool has_iterator_category_v = has_iterator_category<T>::value;

} // namespace detail
} // namespace core
} // namespace ara

#endif // ARA_CORE_INTERNAL_UTILITY_H_