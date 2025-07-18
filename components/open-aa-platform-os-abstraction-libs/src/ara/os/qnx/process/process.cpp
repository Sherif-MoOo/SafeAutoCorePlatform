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
#include <fcntl.h>
#include <sys/param.h>   // PATH_MAX on QNX

namespace ara {
namespace os {
namespace qnx {
namespace process {

auto ProcessQnxImpl::GetProcessNameImpl(char*             buffer,
                                        std::size_t       bufferSize) const noexcept
        -> ara::os::interface::process::ErrorCode
{
    using ErrorCode = ara::os::interface::process::ErrorCode;

    if (buffer == nullptr)      { return ErrorCode::NullBuffer;      }
    if (bufferSize == 0U)       { return ErrorCode::BufferTooSmall;  }

    constexpr char kExeFilePath[] = "/proc/self/exefile";

    constexpr std::size_t kTmpSize = PATH_MAX;
    char                  tmp[kTmpSize]   = {0};

    int fd = ::open(kExeFilePath, O_RDONLY);
    if (fd == -1)                 { return ErrorCode::RetrievalFailed; }

    ssize_t n = ::read(fd, tmp, kTmpSize - 1);
    ::close(fd);

    if (n <= 0)                   { return ErrorCode::RetrievalFailed; }
    tmp[static_cast<std::size_t>(n)] = '\0';

    const char* base = std::strrchr(tmp, '/');
    base = (base ? base + 1 : tmp);

    std::size_t nameLen = std::strlen(base);
    if (nameLen + 1U > bufferSize) {
        return ErrorCode::BufferTooSmall;
    }

    std::memcpy(buffer, base, nameLen);
    buffer[nameLen] = '\0';

    return ErrorCode::Success;
}

} // namespace process
} // namespace qnx
} // namespace os
} // namespace ara
