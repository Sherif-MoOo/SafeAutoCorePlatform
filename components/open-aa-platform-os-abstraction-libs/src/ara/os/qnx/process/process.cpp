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
 *  \brief      QNX-specific implementation of ProcessInteraction.
 *
 *  \details
 *              Implements ProcessQnxImpl::GetProcessNameImpl() which retrieves the process name
 *              by reading the symbolic link "/proc/self/exefile" and extracting the basename.
 *              This translation unit contains all heavy I/O logic in one place, ensuring CRTP dispatch
 *              remains minimal. The implementation is thread-safe and avoids dynamic memory allocation.
 **********************************************************************************************************************/

#include "ara/os/qnx/process/process.h"
#include <unistd.h>     // For readlink()
#include <cstring>      // For std::strncpy, std::strlen, std::strrchr
#include <cstddef>      // For std::size_t

namespace ara {
namespace os {
namespace qnx {
namespace process {

auto ProcessQnxImpl::GetProcessNameImpl(char* buffer,
                                        std::size_t bufferSize) const noexcept
    -> ara::os::interface::process::ErrorCode
{
    using ErrorCode = ara::os::interface::process::ErrorCode;

    // 1. Validate input parameters.
    if (buffer == nullptr) {
        return ErrorCode::NullBuffer;
    }
    if (bufferSize == 0) {
        return ErrorCode::BufferTooSmall;
    }

    // 2. Read the symbolic link "/proc/self/exefile".
    constexpr std::size_t tempBufferSize = 1024;
    char tempBuffer[tempBufferSize] = {0};
    ssize_t len = readlink("/proc/self/exefile", tempBuffer, tempBufferSize - 1);
    if (len < 0) {
        return ErrorCode::RetrievalFailed;
    }
    tempBuffer[len] = '\0';  // Ensure null termination.

    // 3. Extract the basename from the full path.
    const char* baseName = std::strrchr(tempBuffer, '/');
    baseName = (baseName != nullptr) ? (baseName + 1) : tempBuffer;

    // 4. Determine the length of the basename.
    std::size_t nameLength = std::strlen(baseName);
    if (nameLength + 1 > bufferSize) {
        return ErrorCode::BufferTooSmall;
    }

    // 5. Copy the basename safely into the provided buffer.
    std::strncpy(buffer, baseName, bufferSize - 1);
    buffer[bufferSize - 1] = '\0';  // Guarantee null termination.

    return ErrorCode::Success;
}

} // namespace process
} // namespace qnx
} // namespace os
} // namespace ara
