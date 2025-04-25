/**********************************************************************************************************************
 *  PROJECT
 *  -------------------------------------------------------------------------------------------------------------------
 *  \verbatim
 *  OpenAA: Open Source Adaptive AUTOSAR Project (CXX_STANDARD 17)
 *  \endverbatim
 *  -------------------------------------------------------------------------------------------------------------------
 *  FILE DESCRIPTION
 *  -------------------------------------------------------------------------------------------------------------------
 *  \file       ara/os/qnx/process/process.h
 *  \brief      QNX-specific implementation of ProcessInteraction.
 *
 *  \details
 *              Declares the QNX-specific ProcessQnxImpl class which retrieves the process name
 *              via QNX procfs devctl(DCMD_PROC_INFO). Inherits from the CRTP base
 *              ara::os::interface::process::ProcessInteraction<ProcessQnxImpl> for static dispatch.
 *              The implementation is thread-safe, avoids heap allocation.
 **********************************************************************************************************************/

#ifndef ARA_OS_QNX_PROCESS_PROCESS_H
#define ARA_OS_QNX_PROCESS_PROCESS_H

#include "ara/os/interface/process/process_interaction.h"
#include <cstddef>  // For std::size_t

namespace ara {
namespace os {
namespace qnx {
namespace process {

/**
 * \brief QNX-specific implementation of the ProcessInteraction interface.
 *
 * \details
 * Provides process name retrieval by opening "/proc/self" and using devctl(DCMD_PROC_INFO)
 * to obtain procfs_info.cmd. Inherits from ProcessInteraction<ProcessQnxImpl> to avoid virtual calls.
 */
class ProcessQnxImpl final
    : public ara::os::interface::process::ProcessInteraction<ProcessQnxImpl>
{
public:
    /**
     * \brief Retrieves the process name (command name) via devctl on "/proc/self".
     *
     * \param[out] buffer      Pointer to the output buffer.
     * \param[in]  bufferSize  Size of the output buffer in bytes.
     *
     * \return ErrorCode indicating:
     *         - Success if the name was retrieved.
     *         - NullBuffer if `buffer` is null.
     *         - BufferTooSmall if `bufferSize` is zero or insufficient.
     *         - RetrievalFailed for any devctl or parse error.
     *
     * \note This method is noexcept, thread-safe, and uses only stack-based buffers.
     */
    auto GetProcessNameImpl(char* buffer,
                            std::size_t bufferSize) const noexcept
        -> ara::os::interface::process::ErrorCode;

    ~ProcessQnxImpl() = default;
};

} // namespace process
} // namespace qnx
} // namespace os
} // namespace ara

#endif // ARA_OS_QNX_PROCESS_PROCESS_H