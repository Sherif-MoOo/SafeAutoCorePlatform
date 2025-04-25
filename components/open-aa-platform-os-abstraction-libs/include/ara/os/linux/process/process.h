/**********************************************************************************************************************
 *  PROJECT
 *  -------------------------------------------------------------------------------------------------------------------
 *  \verbatim
 *  OpenAA: Open Source Adaptive AUTOSAR Project (CXX_STANDARD 17)
 *  \endverbatim
 *  -------------------------------------------------------------------------------------------------------------------
 *  FILE DESCRIPTION
 *  -------------------------------------------------------------------------------------------------------------------
 *  \file       ara/os/linux/process/process.h
 *  \brief      Linux-specific implementation of ProcessInteraction.
 *
 *  \details
 *              Declares the Linux-specific ProcessLinuxImpl class which retrieves the process name
 *              from the /proc filesystem. Inherits from the CRTP base
 *              ara::os::interface::process::ProcessInteraction<ProcessLinuxImpl> to enable
 *              compile-time dispatch without virtual calls. The implementation is thread-safe,
 *              avoids heap allocation.
 **********************************************************************************************************************/

#ifndef ARA_OS_LINUX_PROCESS_PROCESS_H
#define ARA_OS_LINUX_PROCESS_PROCESS_H

#include "ara/os/interface/process/process_interaction.h"
#include <cstddef>  // For std::size_t

namespace ara {
namespace os {
namespace linux {
namespace process {

/*!
 * \brief Linux-specific implementation of the ProcessInteraction interface.
 *
 * \details
 * Implements process name retrieval using /proc/<pid>/comm.
 * Inherits from ProcessInteraction<ProcessLinuxImpl> for static dispatch,
 * avoiding any v-table overhead.
 */
class ProcessLinuxImpl final
    : public ara::os::interface::process::ProcessInteraction<ProcessLinuxImpl>
{
public:
    /*!
     * \brief Retrieves the process name from /proc/<pid>/comm.
     *
     * \param[out] buffer      Pointer to the output buffer.
     * \param[in]  bufferSize  Size of the output buffer in bytes.
     *
     * \return ErrorCode indicating:
     *         - Success if the name was retrieved.
     *         - NullBuffer if `buffer` is null.
     *         - BufferTooSmall if `bufferSize` is zero or insufficient.
     *         - RetrievalFailed for any I/O or parse error.
     *
     * \note This method is noexcept, thread-safe, and avoids dynamic allocation.
     */
    auto GetProcessNameImpl(char* buffer,
                            std::size_t bufferSize) const noexcept
        -> ara::os::interface::process::ErrorCode;

    ~ProcessLinuxImpl() = default;
};

} // namespace process
} // namespace linux
} // namespace os
} // namespace ara

#endif // ARA_OS_LINUX_PROCESS_PROCESS_H
