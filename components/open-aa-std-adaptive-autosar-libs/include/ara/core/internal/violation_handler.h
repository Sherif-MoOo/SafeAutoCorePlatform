/**********************************************************************************************************************
 *  PROJECT
 *  -------------------------------------------------------------------------------------------------------------------
 *  \verbatim
 *  OpenAA: Open Source Adaptive AUTOSAR Project (CXX_STANDARD 17)
 *  \endverbatim
 *  -------------------------------------------------------------------------------------------------------------------
 *  FILE DESCRIPTION
 *  -------------------------------------------------------------------------------------------------------------------
 *  \file       ara/core/internal/ViolationHandler.h
 *  \brief      Definition of the ara::core::internal::ViolationHandler class.
 *
 *  \details    This file declares the ara::core::internal::ViolationHandler class, which is responsible for handling
 *              violations as per AUTOSAR Adaptive Platform requirements. It ensures that violations are logged
 *              appropriately and that the process is terminated in a controlled manner.
 *
 *              The ViolationHandler follows the Singleton design pattern to provide a single point of violation
 *              management across the system. It is designed to be thread-safe and noexcept where applicable,
 *              aligning with the safety and reliability standards of automotive systems.
 *
 *  \note       Based on the Adaptive AUTOSAR SWS (e.g., R24-11) requirements, especially:
 *              - [SWS_CORE_00040] (Errors originating from C++ standard classes)
 *              - [SWS_CORE_13017] (ViolationMessage ArrayAccessOutOfRangeViolation)
 *              - [SWS_CORE_00090] (Handling of Standardized Violations)
 *********************************************************************************************************************/
#ifndef ARA_CORE_INTERNAL_VIOLATION_HANDLER_H_
#define ARA_CORE_INTERNAL_VIOLATION_HANDLER_H_
#include <limits>                                   // For numeric_limits

/**********************************************************************************************************************
 *  NAMESPACE: ara::core::internal
 *********************************************************************************************************************/
namespace ara {
namespace core {
/*!
 * \brief  Forward declaration of the Array class template.
 */
template <typename T, std::size_t N>
class Array;

class Byte;

template<typename ElementType, std::size_t Extent>
class Span;

template<typename CharT, typename Traits>
class BasicStringView;

namespace internal {

/**********************************************************************************************************************
 *  CLASS: ViolationHandler
 *********************************************************************************************************************/
/*!
 * \brief  Singleton class responsible for handling violations within the ara::core::Array class.
 *
 * \details
 * The ViolationHandler class manages the logging and termination processes when violations occur.
 * It adheres to AUTOSAR requirements by ensuring that violations are logged using the Diagnostic Log and
 * Trace (DLT) mechanism and that the process is terminated in a controlled manner upon detecting a violation.
 *
 * This class cannot be instantiated directly. Instead, the singleton instance is accessed via the
 * Instance() method. The class is designed to be thread-safe and noexcept where applicable to meet
 * the stringent safety and reliability standards of automotive systems.
 *
 * \note   [SWS_CORE_00090], [SWS_CORE_13017]
 */
class ViolationHandler final {
public:

    struct ArrayKey{
        constexpr ArrayKey(const ArrayKey&) noexcept = delete;
        constexpr ArrayKey(ArrayKey&&) noexcept = delete;
        constexpr auto operator=(const ArrayKey&) noexcept = delete;
        constexpr auto operator=(ArrayKey&&) noexcept = delete;
        ~ArrayKey() = default;

        private:
            /*!
             * \brief  Private constructor to prevent instantiation.
             *
             * \details
             * This constructor is private to ensure that the ArrayKey cannot be instantiated outside of this class.
             */
            constexpr ArrayKey() noexcept {
                // This constructor is intentionally left empty.
            };
            /*!
             * \brief  Grants friendship to the ara::core::Array class to allow exclusive access.
             *
             * \tparam T  The type of elements in the Array.
             * \tparam N  The number of elements in the Array.
             *
             * \note   Ensures that ara::core::Array allowed trigger violations.
             */
            template <typename T, std::size_t N>
            friend class ara::core::Array;
    };

    struct ByteKey{
        constexpr ByteKey(const ByteKey&) noexcept = delete;
        constexpr ByteKey(ByteKey&&) noexcept = delete;
        constexpr auto operator=(const ByteKey&) noexcept = delete;
        constexpr auto operator=(ByteKey&&) noexcept = delete;
        ~ByteKey() = default;

        private:
            /*!
             * \brief  Private constructor to prevent instantiation.
             *
             * \details
             * This constructor is private to ensure that the ByteKey cannot be instantiated outside of this class.
             */
            constexpr ByteKey() noexcept {
                // This constructor is intentionally left empty.
            };
            /*!
             * \brief  Grants friendship to the ara::core::Byte class to allow exclusive access.
             *
             * \note   Ensures that ara::core::Byte allowed trigger violations.
             */
            friend class ara::core::Byte;
    };

    
    struct SpanKey{
        constexpr SpanKey(const SpanKey&) noexcept = delete;
        constexpr SpanKey(SpanKey&&) noexcept = delete;
        constexpr auto operator=(const SpanKey&) noexcept = delete;
        constexpr auto operator=(SpanKey&&) noexcept = delete;
        ~SpanKey() = default;

        private:
            /*!
             * \brief  Private constructor to prevent instantiation.
             *
             * \details
             * This constructor is private to ensure that the SpanKey cannot be instantiated outside of this class.
             */
            constexpr SpanKey() noexcept {
                // This constructor is intentionally left empty.
            };
            /*!
             * \brief  Grants friendship to the ara::core::Span class to allow exclusive access.
             *
             * \note   Ensures that ara::core::Span allowed trigger violations.
             */
            template<typename ElementType, std::size_t Extent>
            friend class ara::core::Span;
    };

    /*!
     * \brief  Key class for StringView violations.
     *
     * \details
     * This class is used to prevent instantiation and ensure that only the ara::core::StringView class can trigger
     * violations related to string views.
     *
     * \note   Ensures that ara::core::StringView allowed trigger violations.
     */
    struct StringViewKey{
        constexpr StringViewKey(const StringViewKey&) noexcept = delete;
        constexpr StringViewKey(StringViewKey&&) noexcept = delete;
        constexpr auto operator=(const StringViewKey&) noexcept = delete;
        constexpr auto operator=(StringViewKey&&) noexcept = delete;
        ~StringViewKey() = default;

        private:
            /*!
             * \brief  Private constructor to prevent instantiation.
             *
             * \details
             * This constructor is private to ensure that the StringViewKey cannot be instantiated outside of this class.
             */
            constexpr StringViewKey() noexcept {
                // This constructor is intentionally left empty.
            };

            /*!
             * \brief  Grants friendship to the ara::core::StringView class to allow exclusive access.
             *
             * \note   Ensures that ara::core::StringView allowed trigger violations.
             */
            template<typename CharT, typename Traits>
            friend class ara::core::BasicStringView;
    };

    /*!
     * \brief  Retrieves the singleton instance of ViolationHandler.
     *
     * \return Reference to the ViolationHandler instance.
     *
     * \note   Ensures that only one instance exists throughout the application lifecycle.
     */
    static auto Instance() noexcept -> ViolationHandler&;

    /*!
     * \brief  Triggers an ArrayAccessOutOfRangeViolation.
     *
     * \param  location     An implementation-defined identifier of the location where the violation was detected
     *                      (e.g., "file.cpp:123").
     * \param  indexValue   The index that was out of range.
     * \param  arraySize    The size of the array.
     *
     * \details
     * Logs a violation message following the format specified by [SWS_CORE_13017] and terminates the process
     * abnormally as per [SWS_CORE_00090]. This method is noexcept and does not throw exceptions.
     *
     * \note   [SWS_CORE_13017], [SWS_CORE_00090]
     */
    [[noreturn]] auto TriggerArrayAccessOutOfRangeViolation(ArrayKey&& /*unused*/,
                                               std::string_view location,
                                               std::size_t indexValue,
                                               std::size_t arraySize) noexcept -> void;

    /*!
     * \brief  Triggers a ByteRangeViolation.
     *
     * \param  location   An implementation-defined identifier of the location where the violation was detected
     *                    (e.g., "file.cpp:123").
     * \param  value      The invalid value that caused the violation.
     *
     * \details
     * Logs a violation message and terminates the process abnormally as per [SWS_CORE_00090]. This method is noexcept
     * and does not throw exceptions.
     *
     * \note   [SWS_CORE_00090]
     */
    [[noreturn]] auto TriggerByteRangeViolation(ByteKey&& /*unused*/,
                                                std::string_view location,
                                                long long value) noexcept -> void;

    
    /*!
     * \brief  Triggers a SpanSizeViolation.
     *
     * \param  location   An implementation-defined identifier of the location where the violation was detected
     *                    (e.g., "file.cpp:123").
     * \param  actual     The actual size of the span.
     * \param  expected   The expected size of the span.
     *
     * \details
     * Logs a violation message and terminates the process abnormally as per [SWS_CORE_00090]. This method is noexcept
     * and does not throw exceptions.
     *
     */                                            
    [[noreturn]] auto TriggerSpanSizeViolation(SpanKey&& /*unused*/,
                                               std::string_view location,
                                               std::size_t actual,
                                               std::size_t expected) noexcept -> void;
    
    /*!
     * \brief  Triggers a SpanBoundsViolation.
     *
     * \param  location   An implementation-defined identifier of the location where the violation was detected
     *                    (e.g., "file.cpp:123").
     * \param  index      The index that was out of bounds.
     * \param  size       The size of the span.
     *
     * \details
     * Logs a violation message and terminates the process abnormally as per [SWS_CORE_00090]. This method is noexcept
     * and does not throw exceptions.
     *
     */                                
    [[noreturn]] auto TriggerSpanBoundsViolation(SpanKey&& /*unused*/,
                                                 std::string_view location,
                                                 std::size_t index,
                                                 std::size_t size) noexcept -> void;

    /*!
     * \brief  Triggers a StringViewBoundsViolation.
     *
     * \param location An implementation-defined identifier of the location where the violation was detected
     *                 (e.g., "file.cpp:123").
     * \param index   The index that was out of bounds.
     * \param size    The size of the string view.
     *
     * \details
     * Logs a violation message and terminates the process abnormally as per [SWS_CORE_00090]. This method is noexcept
     * and does not throw exceptions.
     *
     * \note   [SWS_CORE_00090]
     */
    [[noreturn]] auto TriggerStringViewBoundsViolation(StringViewKey&& /*unused*/,
                                                       std::string_view location,
                                                       std::size_t index,
                                                       std::size_t size) noexcept -> void;


    /*!
     * \brief Triggers a TriggerStringViewPosViolation.
     *
     * \param location An implementation-defined identifier of the location where the violation was detected
     *                 (e.g., "file.cpp:123").
     * \param pos      The position that was out of range.
     * \param size     The size of the string view.
     *
     * \details
     * Logs a violation message and terminates the process abnormally.
    */
    [[noreturn]] auto TriggerStringViewPosViolation(StringViewKey&& /*unused*/,
                                                    std::string_view location,
                                                    std::size_t pos,
                                                    std::size_t size) noexcept -> void;


    /*!
     * \brief  Triggers a StringViewNullptrViolation.
     *
     * \param  location   An implementation-defined identifier of the location where the violation was detected
     *                    (e.g., "file.cpp:123").
     *
     * \details
     * Logs a violation message and terminates the process abnormally as per [SWS_CORE_00090]. This method is noexcept
     * and does not throw exceptions.
     *
     */
    [[noreturn]] auto TriggerStringViewNullptrViolation(StringViewKey&& /*unused*/,
                                                        std::string_view location) noexcept -> void;
    
    /*!
     * \brief  Triggers a StringViewEmptyViolation.
     *
     * \param  location   An implementation-defined identifier of the location where the violation was detected
     *                    (e.g., "file.cpp:123").
     * \param  operation  The operation that caused the violation (e.g., "access", "substring").
     *
     * \details
     * Logs a violation message and terminates the process abnormally as per [SWS_CORE_00090]. This method is noexcept
     * and does not throw exceptions.
     *
     */
    [[noreturn]] auto TriggerStringViewEmptyViolation(StringViewKey&& /*unused*/,
                                                      std::string_view location,
                                                      const char* operation) noexcept -> void;

    /*!
     * \brief  Triggers a StringViewIndexViolation.
     *
     * \param  location   An implementation-defined identifier of the location where the violation was detected
     *                    (e.g., "file.cpp:123").
     * \param  index      The index that was out of bounds.
     * \param  size       The size of the string view.
     *
     * \details
     * Logs a violation message and terminates the process abnormally as per [SWS_CORE_00090]. This method is noexcept
     * and does not throw exceptions.
     *
     */
    [[noreturn]] auto TriggerStringViewIndexViolation(StringViewKey&& /*unused*/,
                                                      std::string_view location,
                                                      std::size_t index,
                                                      std::size_t size) noexcept -> void;
    
                                                
    /*!
     * \brief  Triggers a StringViewRemoveViolation.
     *
     * \param  location   An implementation-defined identifier of the location where the violation was detected
     *                    (e.g., "file.cpp:123").
     * \param  n          The number of characters to remove.
     * \param  size       The size of the string view.
     * \param  operation  The operation that caused the violation (e.g., "remove", "erase").
     *
     * \details
     * Logs a violation message and terminates the process abnormally as per [SWS_CORE_00090]. This method is noexcept
     * and does not throw exceptions.
     *
     */
    [[noreturn]] auto TriggerStringViewRemoveViolation(StringViewKey&& /*unused*/,
                                                       std::string_view location,
                                                       std::size_t n,
                                                       std::size_t size,
                                                       const char* operation) noexcept -> void;
private:
    /*!
     * \brief  Private constructor to enforce the Singleton pattern.
     *
     * \details
     * Prevents external instantiation of the ViolationHandler class. Only the Instance() method
     * can access this constructor to create the singleton instance.
     */
    constexpr ViolationHandler() noexcept = default;

    /*!
     * \brief  Deletes the copy constructor.
     *
     * \details
     * Prevents copying of the singleton instance.
     */
    constexpr ViolationHandler(const ViolationHandler&) = delete;

    /*!
     * \brief  Deletes the move constructor.
     *
     * \details
     * Prevents moving of the singleton instance.
     */
    constexpr ViolationHandler(ViolationHandler&&) = delete;

    /*!
     * \brief  Deletes the copy assignment operator.
     *
     * \details
     * Prevents copy assignment of the singleton instance.
     */
    constexpr auto operator=(const ViolationHandler&) noexcept -> ViolationHandler& = delete;

    /*!
     * \brief  Deletes the move assignment operator.
     *
     * \details
     * Prevents move assignment of the singleton instance.
     */
    constexpr auto operator=(ViolationHandler&&) noexcept -> ViolationHandler& = delete;
                                               

    /*!
     * \brief  Retrieves the identifier of the current process.
     *
     * \return The process identifier as a std::string.
     *
     * \details
     * Interacts with the OS Abstraction Layer to obtain the name of the current process. If the process name
     * cannot be retrieved, defaults to "UnknownProcess" or "UnsupportedPlatform" based on the context.
     *
     * \note   [SWS_CORE_00090]
     */
    auto GetProcessIdentifier() noexcept -> std::string;

};

} // namespace internal
} // namespace core
} // namespace ara

#endif // ARA_CORE_INTERNAL_VIOLATION_HANDLER_H_
