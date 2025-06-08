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

/**********************************************************************************************************************
 *  SECTION: Forward Declaration
 *********************************************************************************************************************/
namespace ara::core {
/*!
 * \brief  Forward declaration of the Array class template.
 */
template <typename T, std::size_t N>
class Array;    

/*!
 * \brief  Forward declaration of the get function template.
 */
template <std::size_t I, typename T, std::size_t N>
constexpr auto get(ara::core::Array<T,N>&) noexcept -> T&;

/*!
 * \brief  Forward declaration of the get function template.
 */
template <std::size_t I, typename T, std::size_t N>
constexpr auto get(const ara::core::Array<T,N>&) noexcept -> const T&;

/*!
 * \brief  Forward declaration of the get function template.
 */
template <std::size_t I, typename T, std::size_t N>
constexpr auto get(ara::core::Array<T,N>&&) noexcept -> T&&;

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
 * This trait checks if the type T has a valid std::tuple_size specialization,
 * indicating that it behaves like a tuple.
 *
 * \tparam T The type to check.
 */
template<typename T, typename = void>
struct is_tuple_like : std::false_type {};

template<typename T>
struct is_tuple_like<T,
        std::void_t< decltype(std::tuple_size<std::remove_cv_t<T>>::value) >>
      : std::true_type {};

template<typename T>
inline constexpr bool is_tuple_like_v = is_tuple_like<T>::value;


/*!
 * \brief  Converts a tuple-like type to a std::tuple.
 *
 * This function template converts a tuple-like type (e.g., ara::core::Array)
 * into a std::tuple, preserving the types of its elements.
 *
 * \tparam Src The source tuple-like type to convert.
 * \param src The source tuple-like object to convert.
 * \return A std::tuple containing the elements of the source object.
 */
template<typename Src, std::size_t... I>
constexpr auto to_std_tuple_impl(Src&& src, std::index_sequence<I...>)
{
    /* unqualified get -> ADL; NO ‘using std::get;’ */
    return std::tuple< std::decay_t<decltype(get<I>(src))>... >{
               get<I>(std::forward<Src>(src))... };
}

/*!
 * \brief  Converts a tuple-like type to a std::tuple.
 *
 * This function template converts a tuple-like type (e.g., ara::core::Array)
 * into a std::tuple, preserving the types of its elements.
 *
 * \tparam Src The source tuple-like type to convert.
 * \param src The source tuple-like object to convert.
 * \return A std::tuple containing the elements of the source object.
 */
template<typename Src>
constexpr auto to_std_tuple(Src&& src)
{
    constexpr std::size_t N =
        std::tuple_size_v<std::remove_cv_t<std::remove_reference_t<Src>>>;
    return to_std_tuple_impl(std::forward<Src>(src),
                             std::make_index_sequence<N>{});
}

/*!
 * \brief  Helper function to apply a callable to the elements of a tuple-like object.
 *
 * This function forwards the callable and the tuple-like object, expanding the
 * tuple-like object's elements as arguments to the callable.
 *
 * \tparam F     The type of the callable (function, lambda, etc.).
 * \tparam Tuple The type of the tuple-like object.
 * \tparam I     The index sequence for unpacking the tuple-like object's elements.
 *
 * \param f   The callable to apply.
 * \param tup The tuple-like object containing the elements to pass to the callable.
 * \return    The result of calling f with the unpacked elements of tup.
 */
template<typename F, typename Tuple, std::size_t... I>
constexpr decltype(auto) apply_impl(F&& f,
                                    Tuple&& tup,
                                    std::index_sequence<I...>)
{
    /* again: unqualified get – ADL only */
    return std::forward<F>(f)( get<I>(std::forward<Tuple>(tup))... );
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
    static auto test(int) -> std::true_type;

    /* Fallback if substitution in the above fails */
    template <typename...>
    static auto test(...) -> std::false_type;

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
template <typename InputIt1, typename InputIt2>
constexpr auto lex_compare(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2) noexcept -> bool {
    for (; first1 != last1 && first2 != last2; ++first1, ++first2) {
        if (*first1 < *first2)
            return true;
        if (*first2 < *first1)
            return false;
    }
    return (first1 == last1) && (first2 != last2);
}

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
    T data_[N]{};

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
              typename = std::enable_if_t<(sizeof...(Args) > 0)>>
    constexpr ArrayStorage(Args&&... args)
#ifdef ARA_CORE_ARRAY_ENABLE_CONDITIONAL_EXCEPTIONS
        noexcept(std::conjunction_v<std::is_nothrow_constructible<T, Args&&>...>)
#else
        noexcept
#endif
        : data_{std::forward<Args>(args)...} 
    {
#ifndef ARA_CORE_ARRAY_ENABLE_CONDITIONAL_EXCEPTIONS
        static_assert(std::conjunction_v<std::is_nothrow_constructible<T, Args&&>...>,
            "\n[ERROR] in ara::core::Array: The type T and args must be noexcept.\n");
#endif  
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
protected:
    /*!
     * \brief Variadic constructor for \c N == 0.
     *
     * This constructor is enabled only when no arguments are provided.
     *
     * \tparam Args The types of constructor arguments (must be empty).
     */
    template <typename... Args,
              typename = std::enable_if_t<(sizeof...(Args) == 0)>>
    constexpr ArrayStorage(Args&&...) noexcept { /* Do Nothing */ }

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

} // namespace detail
} // namespace core
} // namespace ara

#endif // ARA_CORE_INTERNAL_UTILITY_H_