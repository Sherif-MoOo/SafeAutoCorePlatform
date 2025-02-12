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
 *              Declares the QNX-specific ProcessInteractionImpl class.
 *              This implementation uses QNX’s procfs interface with devctl() to retrieve
 *              the process name from the process information.
 *
 *              This class is designed to be thread-safe, avoids dynamic memory allocation,
 *              and meets ASIL-D safety and security requirements.
 **********************************************************************************************************************/

#ifndef ARA_OS_QNX_PROCESS_PROCESS_H
#define ARA_OS_QNX_PROCESS_PROCESS_H

namespace ara {
namespace os {
namespace qnx {
namespace process {

/*!
 * \brief QNX-specific implementation of the ProcessInteraction interface.
 *
 * \details
 * Provides a concrete implementation for retrieving the process name (the command name)
 * by using QNX’s procfs interface and the devctl() command DCMD_PROC_INFO.
 */
class ProcessInteractionImpl final : public ara::os::interface::process::ProcessInteraction {
public:
    /*!
     * \brief Retrieves the process name.
     *
     * \param[out] buffer      Pointer to the output buffer.
     * \param[in]  bufferSize  Size of the output buffer.
     *
     * \return An ErrorCode indicating the result.
     *
     * \note This implementation opens "/proc/self" and issues a devctl() call (DCMD_PROC_INFO)
     *       to obtain a procfs_info structure that includes the process command name (typically in the
     *       field "cmd"). The command name is then copied into the provided buffer using safe operations.
     */
    auto GetProcessName(char* buffer, std::size_t bufferSize) const noexcept 
        -> ara::os::interface::process::ErrorCode override;

    ~ProcessInteractionImpl() override = default;
};

/*!
 * \brief Factory function to obtain a QNX-specific ProcessInteraction instance.
 *
 * \return A const reference to a statically allocated ProcessInteractionImpl instance.
 *
 * \note This function avoids dynamic memory allocation and meets ASIL-D guidelines.
 */
const ara::os::interface::process::ProcessInteraction& CreateProcessInteractionInstance();

} // namespace process
} // namespace qnx
} // namespace os
} // namespace ara

#endif // ARA_OS_QNX_PROCESS_PROCESS_H
 