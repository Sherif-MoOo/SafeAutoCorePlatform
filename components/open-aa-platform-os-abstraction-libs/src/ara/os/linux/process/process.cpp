/**********************************************************************************************************************
 *  PROJECT
 *  -------------------------------------------------------------------------------------------------------------------
 *  \verbatim
 *  OpenAA: Open Source Adaptive AUTOSAR Project (CXX_STANDARD 17)
 *  \endverbatim
 *  -------------------------------------------------------------------------------------------------------------------
 *  FILE DESCRIPTION
 *  -------------------------------------------------------------------------------------------------------------------
 *  \file       ara/os/linux/process/process.cpp
 *  \brief      Linux-specific implementation of ProcessInteraction.
 *
 *  \details
 *              Implements the GetProcessNameImpl method which reads the process name from
 *              /proc/<pid>/comm using safe file and string operations. This translation unit
 *              contains all heavy I/O logic in one place, ensuring CRTP dispatch remains minimal.
 *              The implementation avoids dynamic memory allocation and is thread-safe.
 **********************************************************************************************************************/

#include "ara/os/linux/process/process.h"
#include "ara/core/array.h"  // For ara::core::Array

#include <unistd.h>     // For getpid()
#include <sys/types.h>  // For pid_t
#include <fstream>      // For std::ifstream
#include <cstring>      // For std::strncpy
#include <cstdio>       // For std::snprintf
#include <string>

namespace ara {
namespace os {
namespace linux {
namespace process {

auto ProcessLinuxImpl::GetProcessNameImpl(char* buffer,
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

    // 2. Build the file path for /proc/<pid>/comm.
    pid_t pid = getpid();
    constexpr std::size_t maxPathLength = 256;
    ara::core::Array<char, maxPathLength> procPath{};
    int len = std::snprintf(procPath.data(), maxPathLength, "/proc/%d/comm", pid);
    if (len < 0 || static_cast<std::size_t>(len) >= maxPathLength) {
        return ErrorCode::RetrievalFailed;
    }

    // 3. Open and read the process name.
    std::ifstream commFile(procPath.data());
    if (!commFile.is_open()) {
        return ErrorCode::RetrievalFailed;
    }
    std::string name;
    std::getline(commFile, name);
    commFile.close();

    // 4. Verify retrieved name.
    if (name.empty()) {
        return ErrorCode::RetrievalFailed;
    }
    if (name.size() + 1 > bufferSize) {
        return ErrorCode::BufferTooSmall;
    }

    // 5. Copy into user buffer safely.
    std::strncpy(buffer, name.c_str(), bufferSize - 1);
    buffer[bufferSize - 1] = '\0';

    return ErrorCode::Success;
}

} // namespace process
} // namespace linux
} // namespace os
} // namespace ara
