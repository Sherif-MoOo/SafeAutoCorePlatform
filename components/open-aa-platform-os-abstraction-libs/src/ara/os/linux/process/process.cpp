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
 *              Implements the GetProcessName method which reads the process name from
 *              /proc/<pid>/comm using safe file and string operations.
 *              This implementation avoids dynamic memory allocation and is thread-safe,
 *              fulfilling ASIL-D requirements.
 **********************************************************************************************************************/

 #include "ara/os/interface/process/process_interaction.h"
 #include "ara/os/linux/process/process.h"
 #include "ara/core/array.h" // For ara::core::Array
 
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
 
auto ProcessInteractionImpl::GetProcessName(char* buffer, std::size_t bufferSize) const noexcept
    -> ara::os::interface::process::ErrorCode
{
    using ErrorCode = ara::os::interface::process::ErrorCode;

    // 1. Validate input parameters.
    if (buffer == nullptr) {
        return ErrorCode::NullBuffer;
    }
    if (bufferSize == 0) {
        return ErrorCode::BufferTooSmall; // Not enough space even for the null terminator.
    }

    // 2. Build the file path for /proc/<pid>/comm in a safe manner.
    pid_t pid = getpid();
    constexpr std::size_t maxPathLength = 256;
    ara::core::Array<char, maxPathLength> procPath{};
    int snprintfResult = std::snprintf(procPath.data(), maxPathLength, "/proc/%d/comm", pid);
    if (snprintfResult < 0 || static_cast<std::size_t>(snprintfResult) >= maxPathLength) {
        return ErrorCode::RetrievalFailed;
    }

    // 3. Open the /proc/<pid>/comm file.
    std::ifstream commFile(procPath.data(), std::ios::in);
    if (!commFile.is_open()) {
        return ErrorCode::RetrievalFailed;
    }

    // 4. Read the process name from the file.
    std::string processName;
    std::getline(commFile, processName);
    commFile.close();

    // 5. Verify that a process name was retrieved.
    if (processName.empty()) {
        return ErrorCode::RetrievalFailed;
    }

    // 6. Ensure the provided buffer can hold the process name and null terminator.
    if (processName.length() + 1 > bufferSize) {
        return ErrorCode::BufferTooSmall;
    }

    // 7. Copy the process name safely into the output buffer.
    std::strncpy(buffer, processName.c_str(), bufferSize - 1);
    buffer[bufferSize - 1] = '\0'; // Guarantee null termination.

    return ErrorCode::Success;
}

const ara::os::interface::process::ProcessInteraction& CreateProcessInteractionInstance() {
    static const ProcessInteractionImpl instance{};
    return instance;
}

} // namespace process
} // namespace linux
} // namespace os
} // namespace ara
