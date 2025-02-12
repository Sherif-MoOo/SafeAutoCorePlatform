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

// Include OS Abstraction Layer headers for ProcessInteraction
#include "ara/os/interface/process/process_factory.h"       // For ProcessFactory

#include <string_view>                                      // Required for std::string_view
#include "ara/core/internal/violation_handler.h"

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
[[noreturn]] auto ViolationHandler::TriggerArrayAccessOutOfRangeViolation(std::string_view location,
                                                              std::size_t indexValue,
                                                              std::size_t arraySize) noexcept -> void
{
    std::cerr << "[App vlt][FATAL]: Violation detected in " << GetProcessIdentifier()
              << " at " << location
              << ": Array access out of range: Tried to access "
              << indexValue << " in array of size " << arraySize << ".\n" << std::endl;

    // Terminate the process as per AUTOSAR requirements
    Abort();
}


/**********************************************************************************************************************
 *  FUNCTION: ViolationHandler::Abort
 *********************************************************************************************************************/
/*!
 * \brief  Handles the termination of the process upon violation detection.
 *
 * \details
 * Logs a fatal error message to std::cerr and calls std::terminate() to abort the process.
 *
 * \note   [SWS_CORE_00090]
 */
[[noreturn]] auto ViolationHandler::Abort() noexcept -> void
{
    std::cerr << "FATAL: Process aborted due to a critical violation in ara::core::Array.\n";
    std::terminate();
}

/**********************************************************************************************************************
 *  FUNCTION: ViolationHandler::GetProcessIdentifier
 *********************************************************************************************************************/
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
    // The size should accommodate the expected maximum length plus the null terminator.
    constexpr std::size_t processNameBufferSize = 256;
    char processNameBuffer[processNameBufferSize] = {0};

    // Retrieve the singleton instance of the platform-specific ProcessInteraction.
    // The factory returns a reference so no pointer-check is needed.
    const auto& processInteraction = ara::os::interface::process::ProcessFactory::CreateInstance();

    // Attempt to retrieve the process name into the local buffer.
    auto error = processInteraction.GetProcessName(processNameBuffer, processNameBufferSize);

    // If the process name could not be retrieved, use a default/fallback name.
    if (error != ara::os::interface::process::ErrorCode::Success) {
        // Copy a default string into the buffer ensuring no overflow.
        std::strncpy(processNameBuffer, "UnknownProcess", processNameBufferSize - 1);
        // Guarantee null termination.
        processNameBuffer[processNameBufferSize - 1] = '\0';
    }

    // Construct and return a std::string from the buffer.
    // Note: While std::string may allocate memory, the critical path avoids dynamic allocations.
    return std::string(processNameBuffer);
}


} // namespace internal
} // namespace core
} // namespace ara
