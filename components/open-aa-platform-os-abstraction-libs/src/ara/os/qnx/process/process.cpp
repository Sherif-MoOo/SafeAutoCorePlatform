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
 *              This file implements the QNX-specific ProcessInteraction interface.
 *              It retrieves the process name by reading the symbolic link "/proc/self/exefile"
 *              to obtain the full executable path and then extracting the basename.
 *
 *              This solution is based on the QNX 7.1 documentation (and works in QNX 8.0) and meets ASIL-D
 *              requirements by avoiding dynamic memory allocation and using robust error handling.
 **********************************************************************************************************************/

 #include "ara/os/interface/process/process_interaction.h"
 #include "ara/os/qnx/process/process.h"
 #include "ara/core/array.h" // Included for consistency if needed
 
 #include <unistd.h>     // For readlink()
 #include <cstring>      // For std::strncpy, std::strlen, std::strrchr
 #include <cstddef>      // For std::size_t
 
 namespace ara {
 namespace os {
 namespace qnx {
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
         return ErrorCode::BufferTooSmall; // Buffer too small for null terminator.
     }
 
     // 2. Read the symbolic link "/proc/self/exefile" which points to the executable.
     //    We'll use a temporary buffer to hold the full path.
     constexpr std::size_t tempBufferSize = 1024;
     char tempBuffer[tempBufferSize] = {0};
     ssize_t len = readlink("/proc/self/exefile", tempBuffer, tempBufferSize - 1);
     if (len < 0) {
         return ErrorCode::RetrievalFailed;
     }
     tempBuffer[len] = '\0'; // Ensure null termination.
 
     // 3. Extract the basename from the full path.
     //    Find the last occurrence of '/'.
     const char* baseName = std::strrchr(tempBuffer, '/');
     if (baseName != nullptr) {
         baseName++;  // Skip the '/' character.
     } else {
         baseName = tempBuffer;  // No '/' found; use the entire string.
     }
 
     // 4. Determine the length of the basename.
     std::size_t nameLength = std::strlen(baseName);
     if (nameLength + 1 > bufferSize) {
         return ErrorCode::BufferTooSmall;
     }
 
     // 5. Copy the basename safely into the provided buffer.
     std::strncpy(buffer, baseName, bufferSize - 1);
     buffer[bufferSize - 1] = '\0'; // Guarantee null termination.
 
     return ErrorCode::Success;
 }
 
 const ara::os::interface::process::ProcessInteraction& CreateProcessInteractionInstance() {
     static const ProcessInteractionImpl instance{};
     return instance;
 }
 
 } // namespace process
 } // namespace qnx
 } // namespace os
 } // namespace ara
 