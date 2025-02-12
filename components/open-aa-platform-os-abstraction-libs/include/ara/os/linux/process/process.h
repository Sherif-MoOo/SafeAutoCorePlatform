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
 *              Declares the Linux-specific ProcessInteractionImpl class which retrieves the process name
 *              from the /proc filesystem. The implementation is thread-safe, avoids heap allocation,
 *              and meets ASIL-D safety and security guidelines.
 **********************************************************************************************************************/

 #ifndef ARA_OS_LINUX_PROCESS_PROCESS_H
 #define ARA_OS_LINUX_PROCESS_PROCESS_H
  
 namespace ara {
 namespace os {
 namespace linux {
 namespace process {
 
/*!
 * \brief Linux-specific implementation of the ProcessInteraction interface.
 *
 * \details
 * Implements process name retrieval using /proc/<pid>/comm.
 * All operations are done using safe file and string operations to meet ASIL-D standards.
 */
class ProcessInteractionImpl final : public ara::os::interface::process::ProcessInteraction {
public:
    /*!
     * \brief Retrieves the process name from /proc/<pid>/comm.
     *
     * \param[out] buffer      Pointer to the output buffer.
     * \param[in]  bufferSize  Size of the output buffer.
     *
     * \return An ErrorCode indicating success or failure.
     *
     * \note The method is thread-safe and avoids exceptions.
     */
    auto GetProcessName(char* buffer, std::size_t bufferSize) const noexcept 
        -> ara::os::interface::process::ErrorCode override;

    ~ProcessInteractionImpl() override = default;
};

/*!
 * \brief Factory function to obtain a Linux-specific ProcessInteraction instance.
 *
 * \return A const reference to a statically allocated ProcessInteractionImpl instance.
 *
 * \note No dynamic memory allocation is performed. The instance is constructed
 *       on first use in a thread-safe manner.
 */
const ara::os::interface::process::ProcessInteraction& CreateProcessInteractionInstance();

} // namespace process
} // namespace linux
} // namespace os
} // namespace ara

#endif // ARA_OS_LINUX_PROCESS_PROCESS_H
