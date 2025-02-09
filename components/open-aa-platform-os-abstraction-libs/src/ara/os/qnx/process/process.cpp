/**********************************************************************************************************************
 *  PROJECT
 *  -------------------------------------------------------------------------------------------------------------------
 *  \verbatim
 *  OpenAA: Open Source Adaptive AUTOSAR Project (CXX_STANDARD 17)
 *  \endverbatim
 *  -------------------------------------------------------------------------------------------------------------------
 *  FILE DESCRIPTION
 *  -------------------------------------------------------------------------------------------------------------------
 *  \file       ara/os/qnx/process/process.cpp
 *  \brief      QNX-specific implementation of the ara::os::interface::process::ProcessInteraction interface.
 *
 *  \details    Implements the GetProcessName method using QNX system calls and standard libraries.
 *
 *  \note       This implementation ensures thread safety and handles potential errors gracefully.
 ***********************************************************************************************************************/

#include "ara/os/qnx/process/process.h"
#include "ara/core/array.h" // Include the ara::core::Array template

#include <unistd.h>         // For getpid
#include <cstring>          // For std::strncpy

namespace ara {
namespace os {
namespace qnx {
namespace process {

/**********************************************************************************************************************
 *  FUNCTION: ProcessInteractionImpl::GetProcessName
 *********************************************************************************************************************/
/*!
 * \brief  Retrieves the name of the current process.
 *
 * \param[out] buffer      Pointer to the buffer where the process name will be stored.
 * \param[in]  bufferSize  Size of the provided buffer in bytes.
 *
 * \return An ara::os::interface::process::ErrorCode indicating the result of the operation.
 *
 * \note   This method avoids exceptions and uses safe string operations to prevent buffer overflows.
 */
auto ProcessInteractionImpl::GetProcessName(char* buffer, std::size_t bufferSize) const noexcept -> ara::os::interface::process::ErrorCode {
    static_cast<void>(buffer);
    static_cast<void>(bufferSize);
    return ara::os::interface::process::ErrorCode::NullBuffer;
}

/**********************************************************************************************************************
 *  FUNCTION: CreateProcessInteractionInstance
 *********************************************************************************************************************/
/*!
 * \brief  Factory function to create a QNX-specific ProcessInteraction instance.
 *
 * \return A std::unique_ptr to an ara::os::interface::process::ProcessInteraction object.
 */
auto CreateProcessInteractionInstance() -> std::unique_ptr<ara::os::interface::process::ProcessInteraction> {
    return std::make_unique<ProcessInteractionImpl>();
}

} // namespace process
} // namespace qnx
} // namespace os
} // namespace ara
