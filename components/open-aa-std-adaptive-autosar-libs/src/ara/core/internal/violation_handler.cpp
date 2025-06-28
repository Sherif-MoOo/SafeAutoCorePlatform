/**********************************************************************************************************************
 *  PROJECT
 *  -------------------------------------------------------------------------------------------------------------------
 *  \verbatim
 *  OpenAA: Open Source Adaptive AUTOSAR Project (CXX_STANDARD 17)
 *  \endverbatim
 *  -------------------------------------------------------------------------------------------------------------------
 *  FILE DESCRIPTION
 *  -------------------------------------------------------------------------------------------------------------------
 *  \file       ara/core/internal/ViolationHandler.cpp
 *  \brief      Implementation of the ara::core::internal::ViolationHandler class.
 *
 *  \details    This file provides the implementation of the ara::core::internal::ViolationHandler class, which
 *              handles violations by logging diagnostic messages and terminating the process as required
 *              by AUTOSAR specifications.
 *
 *              The class ensures that violation messages are formatted according to [SWS_CORE_13017] and that
 *              the process is terminated in a controlled and standardized manner.
 *
 *  \note       Based on the Adaptive AUTOSAR SWS (e.g., R24-11) requirements, especially:
 *              - [SWS_CORE_00040] (Errors originating from C++ standard classes)
 *              - [SWS_CORE_13017] (ViolationMessage ArrayAccessOutOfRangeViolation)
 *              - [SWS_CORE_00090] (Handling of Standardized Violations)
 *********************************************************************************************************************/
/**********************************************************************************************************************
 *  INCLUDES
 *********************************************************************************************************************/
#include <cstddef>       // For std::size_t
#include <string>        // For std::string
#include <iostream>      // For std::cerr
#include <cstdlib>       // For std::terminate
#include <cstring>       // For std::strncpy
#include <charconv>     /* For std::to_chars */

// Include OS Abstraction Layer headers for ProcessInteraction
#include "ara/os/interface/process/process_interaction.h"       // For ProcessFactory

#include <string_view>                                      // Required for std::string_view
#include "ara/core/internal/violation_handler.h"
#include "ara/core/abort.h"

namespace ara {
namespace core {
namespace internal {

/**********************************************************************************************************************
 *  FUNCTION: ViolationHandler::Instance
 *********************************************************************************************************************/
/*!
 * \brief  Retrieves the singleton instance of ViolationHandler.
 *
 * \return Reference to the ViolationHandler instance.
 *
 * \details
 * Utilizes a local static variable to ensure thread-safe initialization as per C++11 and later standards.
 *
 * \note   [SWS_CORE_00090]
 */
auto ViolationHandler::Instance() noexcept -> ViolationHandler&
{
    static ViolationHandler instance;
    return instance;
}

/**********************************************************************************************************************
 *  FUNCTION: ViolationHandler::TriggerArrayAccessOutOfRangeViolation
 *********************************************************************************************************************/
/*!
 * \brief  Triggers an ArrayAccessOutOfRangeViolation.
 *
 * \param  location     An implementation-defined identifier of the location where the violation was detected
 *                      (e.g., "file.cpp:123").
 * \param  indexValue   The index that was out of range.
 * \param  arraySize    The size of the array.
 *
 * \details
 * Constructs the violation message as per [SWS_CORE_13017], logs it, and terminates the process.
 *
 * \note   [SWS_CORE_13017], [SWS_CORE_00090]
 */

/* This function is part of the ViolationHandler class and is called when an array access
   violation is detected. It logs an error message and terminates the process according
   to AUTOSAR requirements. */
[[noreturn]] auto ViolationHandler::TriggerArrayAccessOutOfRangeViolation(ArrayKey&& /*unused*/,
                                                                          std::string_view location,
                                                                          std::size_t indexValue,
                                                                          std::size_t arraySize) noexcept -> void
{
    /* Allocate fixed-size buffers for converting numeric values.
       Thirty-two characters is sufficient for representing a std::size_t value. */
    char idxBuffer[32]{0};
    char sizeBuffer[32]{0};

    /* Convert 'indexValue' to characters using std::to_chars.
       The conversion writes the numeric value into the provided buffer. */
    auto [idxPtr, idxEc] = std::to_chars(idxBuffer, idxBuffer + sizeof(idxBuffer), indexValue);

    /* Convert 'arraySize' to characters using std::to_chars. */
    auto [sizePtr, sizeEc] = std::to_chars(sizeBuffer, sizeBuffer + sizeof(sizeBuffer), arraySize);

    /* Create string_view objects from the conversion results.
       If the conversion fails, an empty string_view is used instead. */
    std::string_view idxStr = (idxEc == std::errc{}) 
                              ? std::string_view(idxBuffer, static_cast<std::size_t>(idxPtr - idxBuffer)) 
                              : "";
    std::string_view sizeStr = (sizeEc == std::errc{}) 
                               ? std::string_view(sizeBuffer, static_cast<std::size_t>(sizePtr - sizeBuffer)) 
                               : "";

    /* Terminate the process by calling the Abort API.
       The Abort function is passed each part of the message as separate arguments,
       ensuring that no dynamic memory allocation is required. */
    ara::core::Abort(
        "[App vlt][FATAL]: Violation detected in ",
        GetProcessIdentifier(),
        " at ",
        location,
        ": Array access out of range: Tried to access ",
        idxStr,
        " in array of size ",
        sizeStr,
        "."
    );
}

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
[[noreturn]] auto ViolationHandler::TriggerByteRangeViolation(ByteKey&& /*unused*/,
                                                    std::string_view location,
                                                    long long value) noexcept -> void
{
    // Allocate a buffer for the numeric value.
    char valueBuffer[32]{0};

    // Convert the value to characters using std::to_chars.
    auto [valuePtr, ec] = std::to_chars(valueBuffer, valueBuffer + sizeof(valueBuffer), value);

    // Create a string_view from the conversion result.
    std::string_view valueStr = (ec == std::errc{}) 
                                ? std::string_view(valueBuffer, static_cast<std::size_t>(valuePtr - valueBuffer)) 
                                : "";

    // Terminate the process by calling the Abort API.
    ara::core::Abort(
        "[App vlt][FATAL]: Violation detected in ",
        GetProcessIdentifier(),
        " Byte range violation at ",
        location,
        ": Byte range violation: Invalid byte value ",
        valueStr,
        "."
    );
}

/*!
 * \brief  Triggers a ShiftRangeViolation.
 *
 * \param  location   An implementation-defined identifier of the location where the violation was detected
 *                    (e.g., "file.cpp:123").
 * \param  shift      The invalid shift amount that caused the violation.
 *
 * \details
 * Logs a violation message and terminates the process abnormally as per [SWS_CORE_00090]. This method is noexcept
 * and does not throw exceptions.
 *
 * \note   [SWS_CORE_00090]
 */
[[noreturn]] auto ViolationHandler::TriggerShiftRangeViolation(ByteKey&& /*unused*/,
                                                    std::string_view location,
                                                    long long shift) noexcept -> void
{
    // Allocate a buffer for the numeric value.
    char shiftBuffer[32]{0};

    // Convert the shift value to characters using std::to_chars.
    auto [shiftPtr, ec] = std::to_chars(shiftBuffer, shiftBuffer + sizeof(shiftBuffer), shift);

    // Create a string_view from the conversion result.
    std::string_view shiftStr = (ec == std::errc{}) 
                                ? std::string_view(shiftBuffer, static_cast<std::size_t>(shiftPtr - shiftBuffer)) 
                                : "";

    // Terminate the process by calling the Abort API.
    ara::core::Abort(
        "[App vlt][FATAL]: Violation detected in ",
        GetProcessIdentifier(),
        " at ",
        location,
        ": Shift range violation: Invalid shift amount ",
        shiftStr,
        "."
    );
}

/*!
 * \brief  Triggers a BitPositionViolation.
 *
 * \param  location   An implementation-defined identifier of the location where the violation was detected
 *                    (e.g., "file.cpp:123").
 * \param  pos        The invalid bit position that caused the violation.
 *
 * \details
 * Logs a violation message and terminates the process abnormally as per [SWS_CORE_00090]. This method is noexcept
 * and does not throw exceptions.
 *
 * \note   [SWS_CORE_00090]
 */
[[noreturn]] auto ViolationHandler::TriggerBitPositionViolation(ByteKey&& /*unused*/,
                                                    std::string_view location,
                                                    std::size_t pos) noexcept -> void
{
    // Allocate a buffer for the numeric value.
    char posBuffer[32]{0};

    // Convert the position to characters using std::to_chars.
    auto [posPtr, ec] = std::to_chars(posBuffer, posBuffer + sizeof(posBuffer), pos);

    // Create a string_view from the conversion result.
    std::string_view posStr = (ec == std::errc{}) 
                              ? std::string_view(posBuffer, static_cast<std::size_t>(posPtr - posBuffer)) 
                              : "";

    // Terminate the process by calling the Abort API.
    ara::core::Abort(
        "[App vlt][FATAL]: Violation detected in ",
        GetProcessIdentifier(),
        " at ",
        location,
        ": Bit position violation: Invalid bit position ",
        posStr,
        "."
    );
}

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
[[noreturn]] auto ViolationHandler::TriggerSpanSizeViolation(SpanKey&& /*unused*/,
                                                    std::string_view location,
                                                    std::size_t actual,
                                                    std::size_t expected) noexcept -> void
{
    // Allocate buffers for the numeric values.
    char actualBuffer[32]{0};
    char expectedBuffer[32]{0};

    // Convert 'actual' and 'expected' to characters using std::to_chars.
    auto [actualPtr, actualEc] = std::to_chars(actualBuffer, actualBuffer + sizeof(actualBuffer), actual);
    auto [expectedPtr, expectedEc] = std::to_chars(expectedBuffer, expectedBuffer + sizeof(expectedBuffer), expected);

    // Create string_view objects from the conversion results.
    std::string_view actualStr = (actualEc == std::errc{}) 
                                 ? std::string_view(actualBuffer, static_cast<std::size_t>(actualPtr - actualBuffer)) 
                                 : "";
    std::string_view expectedStr = (expectedEc == std::errc{}) 
                                   ? std::string_view(expectedBuffer, static_cast<std::size_t>(expectedPtr - expectedBuffer)) 
                                   : "";

    // Terminate the process by calling the Abort API.
    ara::core::Abort(
        "[App vlt][FATAL]: Violation detected in ",
        GetProcessIdentifier(),
        " at ",
        location,
        ": Span size violation: Actual size ",
        actualStr,
        " does not match expected size ",
        expectedStr,
        "."
    );
}

/*! 
 * \brief  Triggers a SpanNullPointerViolation.
 *
 * \param  location   An implementation-defined identifier of the location where the violation was detected
 *                    (e.g., "file.cpp:123").
 *
 * \details
 * Logs a violation message and terminates the process abnormally as per [SWS_CORE_00090]. This method is noexcept
 * and does not throw exceptions.
 *
 */
[[noreturn]] auto ViolationHandler::TriggerSpanNullPointerViolation(SpanKey&& /*unused*/,
                                                    std::string_view location) noexcept -> void
{
    // Terminate the process by calling the Abort API.
    ara::core::Abort(
        "[App vlt][FATAL]: Violation detected in ",
        GetProcessIdentifier(),
        " at ",
        location,
        ": Span Size shall be zero if the pointer is null."
    );
}

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
[[noreturn]] auto ViolationHandler::TriggerSpanBoundsViolation(SpanKey&& /*unused*/,
                                                    std::string_view location,
                                                    std::size_t index,
                                                    std::size_t size) noexcept -> void
{
    // Allocate buffers for the numeric values.
    char indexBuffer[32]{0};
    char sizeBuffer[32]{0};

    // Convert 'index' and 'size' to characters using std::to_chars.
    auto [indexPtr, indexEc] = std::to_chars(indexBuffer, indexBuffer + sizeof(indexBuffer), index);
    auto [sizePtr, sizeEc] = std::to_chars(sizeBuffer, sizeBuffer + sizeof(sizeBuffer), size);

    // Create string_view objects from the conversion results.
    std::string_view indexStr = (indexEc == std::errc{}) 
                                ? std::string_view(indexBuffer, static_cast<std::size_t>(indexPtr - indexBuffer)) 
                                : "";
    std::string_view sizeStr = (sizeEc == std::errc{}) 
                               ? std::string_view(sizeBuffer, static_cast<std::size_t>(sizePtr - sizeBuffer)) 
                               : "";

    // Terminate the process by calling the Abort API.
    ara::core::Abort(
        "[App vlt][FATAL]: Violation detected in ",
        GetProcessIdentifier(),
        " at ",
        location,
        ": Span bounds violation: Index ",
        indexStr,
        " is out of bounds for span of size ",
        sizeStr,
        "."
    );
}

/*!
 * \brief  Triggers a StringViewPosViolation.
 *
 * \param  location   An implementation-defined identifier of the location where the violation was detected
 *                    (e.g., "file.cpp:123").
 * \param  pos        The position that was out of range.
 * \param  size       The size of the string view.
 *
 * \details
 * Logs a violation message and terminates the process abnormally as per [SWS_CORE_00090]. This method is noexcept
 * and does not throw exceptions.
 *
 */
[[noreturn]] auto ViolationHandler::TriggerStringViewPosViolation(StringViewKey&& /*unused*/,
                                                    std::string_view location,
                                                    std::size_t pos,
                                                    std::size_t size) noexcept -> void
{
    // Allocate buffers for the numeric values.
    char posBuffer[32]{0};
    char sizeBuffer[32]{0};

    // Convert 'pos' and 'size' to characters using std::to_chars.
    auto [posPtr, posEc] = std::to_chars(posBuffer, posBuffer + sizeof(posBuffer), pos);
    auto [sizePtr, sizeEc] = std::to_chars(sizeBuffer, sizeBuffer + sizeof(sizeBuffer), size);

    // Create string_view objects from the conversion results.
    std::string_view posStr = (posEc == std::errc{}) 
                              ? std::string_view(posBuffer, static_cast<std::size_t>(posPtr - posBuffer)) 
                              : "";
    std::string_view sizeStr = (sizeEc == std::errc{}) 
                               ? std::string_view(sizeBuffer, static_cast<std::size_t>(sizePtr - sizeBuffer)) 
                               : "";

    // Terminate the process by calling the Abort API.
    ara::core::Abort(
        "[App vlt][FATAL]: Violation detected in ",
        GetProcessIdentifier(),
        " at ",
        location,
        ": String view position violation: Position ",
        posStr,
        " is out of range for string view of size ",
        sizeStr,
        "."
    );
}

/*! 
 * \brief  Triggers a StringViewBoundsViolation.
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
[[noreturn]] auto ViolationHandler::TriggerStringViewBoundsViolation(StringViewKey&& /*unused*/,
                                                    std::string_view location,
                                                    std::size_t index,
                                                    std::size_t size) noexcept -> void
{
    // Allocate buffers for the numeric values.
    char indexBuffer[32]{0};
    char sizeBuffer[32]{0};

    // Convert 'index' and 'size' to characters using std::to_chars.
    auto [indexPtr, indexEc] = std::to_chars(indexBuffer, indexBuffer + sizeof(indexBuffer), index);
    auto [sizePtr, sizeEc] = std::to_chars(sizeBuffer, sizeBuffer + sizeof(sizeBuffer), size);

    // Create string_view objects from the conversion results.
    std::string_view indexStr = (indexEc == std::errc{}) 
                                ? std::string_view(indexBuffer, static_cast<std::size_t>(indexPtr - indexBuffer)) 
                                : "";
    std::string_view sizeStr = (sizeEc == std::errc{}) 
                               ? std::string_view(sizeBuffer, static_cast<std::size_t>(sizePtr - sizeBuffer)) 
                               : "";

    // Terminate the process by calling the Abort API.
    ara::core::Abort(
        "[App vlt][FATAL]: Violation detected in ",
        GetProcessIdentifier(),
        " at ",
        location,
        ": String view bounds violation: Index ",
        indexStr,
        " is out of bounds for string view of size ",
        sizeStr,
        "."
    );
}

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
[[noreturn]] auto ViolationHandler::TriggerStringViewNullptrViolation(StringViewKey&& /*unused*/,
                                                    std::string_view location) noexcept -> void
{
    // Terminate the process by calling the Abort API.
    ara::core::Abort(
        "[App vlt][FATAL]: Violation detected in ",
        GetProcessIdentifier(),
        " at ",
        location,
        ": String view nullptr violation."
    );
}

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
[[noreturn]] auto ViolationHandler::TriggerStringViewEmptyViolation(StringViewKey&& /*unused*/,
                                                    std::string_view location,
                                                    const char* operation) noexcept -> void
{
    // Terminate the process by calling the Abort API.
    ara::core::Abort(
        "[App vlt][FATAL]: Violation detected in ",
        GetProcessIdentifier(),
        " at ",
        location,
        ": String view empty violation during ",
        operation,
        "."
    );
}

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
[[noreturn]] auto ViolationHandler::TriggerStringViewIndexViolation(StringViewKey&& /*unused*/,
                                                    std::string_view location,
                                                    std::size_t index,
                                                    std::size_t size) noexcept -> void
{
    // Allocate buffers for the numeric values.
    char indexBuffer[32]{0};
    char sizeBuffer[32]{0};

    // Convert 'index' and 'size' to characters using std::to_chars.
    auto [indexPtr, indexEc] = std::to_chars(indexBuffer, indexBuffer + sizeof(indexBuffer), index);
    auto [sizePtr, sizeEc] = std::to_chars(sizeBuffer, sizeBuffer + sizeof(sizeBuffer), size);

    // Create string_view objects from the conversion results.
    std::string_view indexStr = (indexEc == std::errc{}) 
                                ? std::string_view(indexBuffer, static_cast<std::size_t>(indexPtr - indexBuffer)) 
                                : "";
    std::string_view sizeStr = (sizeEc == std::errc{}) 
                               ? std::string_view(sizeBuffer, static_cast<std::size_t>(sizePtr - sizeBuffer)) 
                               : "";

    // Terminate the process by calling the Abort API.
    ara::core::Abort(
        "[App vlt][FATAL]: Violation detected in ",
        GetProcessIdentifier(),
        " at ",
        location,
        ": String view index violation: Index ",
        indexStr,
        " is out of bounds for string view of size ",
        sizeStr,
        "."
    );
}

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
[[noreturn]] auto ViolationHandler::TriggerStringViewRemoveViolation(StringViewKey&& /*unused*/,
                                                    std::string_view location,
                                                    std::size_t n,
                                                    std::size_t size,
                                                    const char* operation) noexcept -> void
{
    // Allocate buffers for the numeric values.
    char nBuffer[32]{0};
    char sizeBuffer[32]{0};

    // Convert 'n' and 'size' to characters using std::to_chars.
    auto [nPtr, nEc] = std::to_chars(nBuffer, nBuffer + sizeof(nBuffer), n);
    auto [sizePtr, sizeEc] = std::to_chars(sizeBuffer, sizeBuffer + sizeof(sizeBuffer), size);

    // Create string_view objects from the conversion results.
    std::string_view nStr = (nEc == std::errc{}) 
                            ? std::string_view(nBuffer, static_cast<std::size_t>(nPtr - nBuffer)) 
                            : "";
    std::string_view sizeStr = (sizeEc == std::errc{}) 
                               ? std::string_view(sizeBuffer, static_cast<std::size_t>(sizePtr - sizeBuffer)) 
                               : "";

    // Terminate the process by calling the Abort API.
    ara::core::Abort(
        "[App vlt][FATAL]: Violation detected in ",
        GetProcessIdentifier(),
        " at ",
        location,
        ": String view remove violation during ",
        operation,
        ": Attempted to remove ",
        nStr,
        " characters from string view of size ",
        sizeStr,
        "."
    );
}


/**********************************************************************************************************************
 *  FUNCTION: ViolationHandler::GetProcessIdentifier
 **********************************************************************************************************************/
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
 auto ViolationHandler::GetProcessIdentifier() noexcept -> std::string {
   // Define a buffer size that is sufficiently large to hold the process name.
   constexpr std::size_t processNameBufferSize = 256;
   char processNameBuffer[processNameBufferSize] = {0};

   // Attempt to retrieve the process name via the CRTP-based dispatch function.
   auto error = ara::os::interface::process::GetProcessName(
       processNameBuffer, processNameBufferSize);

   // If the process name could not be retrieved, use a default/fallback name.
   if (error != ara::os::interface::process::ErrorCode::Success) {
       std::strncpy(processNameBuffer, "UnknownProcess", processNameBufferSize - 1);
       processNameBuffer[processNameBufferSize - 1] = '\0';
   }

   // Construct and return a std::string from the buffer.
   return std::string(processNameBuffer);
}

} // namespace internal
} // namespace core
} // namespace ara
