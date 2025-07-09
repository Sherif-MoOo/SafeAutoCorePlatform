/***********************************************************************************************************************
 *  PROJECT
 *  --------------------------------------------------------------------------------------------------------------------
 *  \verbatim
 *  OpenAA: Open Source Adaptive AUTOSAR Project (CXX_STANDARD 17)
 *  Author: Sherif Mohamed
 *  \endverbatim
 *  --------------------------------------------------------------------------------------------------------------------
 *  FILE DESCRIPTION
 *
 *  \file       ara/core/internal/array_storage.h
 *  \brief      Internal storage for array types.
 *
 *  \details    This file provides the implementation details for array storage,
 *              including the ArrayStorage class template.
 ***********************************************************************************************************************/

#ifndef ARA_CORE_INTERNAL_STORAGE_ARRAY_STORAGE_H_
#define ARA_CORE_INTERNAL_STORAGE_ARRAY_STORAGE_H_

#include "ara/core/internal/utility.h"  // For utility functions and traits

namespace ara::core::detail {

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
                  (is_brace_initializable_array_v<T, N, Args...>) &&
                  // Condition #4: Prevent constructor from being selected when Args... is exactly Array<T, N>
                  (!is_single_same_array_v<Args...>) &&
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
            (!is_single_same_array_v<Args...>) && 
            ( (sizeof...(Args) > N) || 
              (!std::conjunction_v<std::is_convertible<Args, T>...>) ||
              (!is_brace_initializable_array_v<T, N, Args...>) )
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
     */
    constexpr ArrayStorage()
     #ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
        noexcept(std::is_nothrow_default_constructible_v<T>)
     #else
        noexcept
     #endif
        = default;

    /*!
     * \brief Defaulted copy constructor.
     */
    constexpr ArrayStorage(const ArrayStorage&)
        #ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
            noexcept(std::is_nothrow_copy_constructible_v<T>)
        #else
            noexcept
        #endif
            = default;

    /*!
     * \brief Defaulted move constructor.
     */
    constexpr ArrayStorage(ArrayStorage&&)
        #ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
            noexcept(std::is_nothrow_move_constructible_v<T>)
        #else
            noexcept
        #endif
            = default;

    /*!
     * \brief Defaulted copy assignment operator.
     */
    constexpr ArrayStorage& operator=(const ArrayStorage&)
        #ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
            noexcept(std::is_nothrow_copy_assignable_v<T>)
        #else
            noexcept
        #endif
            = default;

    /*!
     * \brief Defaulted move assignment operator.
     */
    constexpr ArrayStorage& operator=(ArrayStorage&&)
        #ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
            noexcept(std::is_nothrow_move_assignable_v<T>)
        #else
            noexcept
        #endif
            = default;

    ~ArrayStorage()
        #ifdef ENABLE_PLATFORM_CONDITIONAL_EXCEPTION
            noexcept(std::is_nothrow_destructible_v<T>)
        #else
            noexcept
        #endif
        = default;
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

    /*!
     * \brief Defaulted destructor.
     */
    ~ArrayStorage() noexcept = default;
};

} // namespace ara::core::detail

#endif // ARA_CORE_INTERNAL_STORAGE_ARRAY_STORAGE_H_