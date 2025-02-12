/**********************************************************************************************************************
 *  PROJECT
 *  -------------------------------------------------------------------------------------------------------------------
 *  \verbatim
 *  OpenAA: Open Source Adaptive AUTOSAR Project (CXX_STANDARD 17)
 *  \endverbatim
 *  -------------------------------------------------------------------------------------------------------------------
 *  FILE DESCRIPTION
 *  -------------------------------------------------------------------------------------------------------------------
 *  \file       ara/os/interface/process/process_factory.cpp
 *  \brief      Implementation of the ProcessFactory.
 *
 *  \details
 *              This file implements the CreateInstance method which returns a statically allocated,
 *              thread-safe instance of the platform-specific ProcessInteraction implementation.
 *              Dynamic memory allocation is avoided to reduce overhead and meet ASIL-D requirements.
 **********************************************************************************************************************/

#include "ara/os/interface/process/process_factory.h"
#if defined(__linux__)
    #include "ara/os/linux/process/process.h" // Linux-specific implementation
#elif defined(__QNXNTO__)
    #include "ara/os/qnx/process/process.h"   // QNX-specific implementation
#else
    #error "Unsupported platform. The ProcessFactory cannot create a ProcessInteraction instance."
#endif

namespace ara {
namespace os {
namespace interface {
namespace process {

auto ProcessFactory::CreateInstance() noexcept -> const ProcessInteraction& {
#if defined(__linux__)
    // Return the Linux-specific ProcessInteraction instance.
    return linux::process::CreateProcessInteractionInstance();
#elif defined(__QNXNTO__)
    // Return the QNX-specific ProcessInteraction instance.
    return qnx::process::CreateProcessInteractionInstance();
#endif
}

} // namespace process
} // namespace interface
} // namespace os
} // namespace ara