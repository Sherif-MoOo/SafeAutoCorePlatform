/**********************************************************************************************************************
 *  PROJECT
 *  -------------------------------------------------------------------------------------------------------------------
 *  \verbatim
 *  OpenAA: Open Source Adaptive AUTOSAR Project (CXX_STANDARD 17)
 *  \endverbatim
 *  -------------------------------------------------------------------------------------------------------------------
 *  FILE DESCRIPTION
 *  -------------------------------------------------------------------------------------------------------------------
 *  \file       ara/os/interface/process/process_factory.h
 *  \brief      Declaration of the ara::os::interface::process::ProcessFactory.
 *
 *  \details
 *              This file declares the ProcessFactory class responsible for providing a singleton instance
 *              of a platform-specific implementation of the ProcessInteraction interface.
 *              The implementation is stateless and thread-safe, and by using a function-local static object,
 *              dynamic memory allocation is avoided. This design meets safety (ASIL-D) and security requirements.
 **********************************************************************************************************************/

#ifndef OPEN_AA_ADAPTIVE_AUTOSAR_LIBS_INCLUDE_ARA_OS_INTERFACE_PROCESS_PROCESS_FACTORY_H_
#define OPEN_AA_ADAPTIVE_AUTOSAR_LIBS_INCLUDE_ARA_OS_INTERFACE_PROCESS_PROCESS_FACTORY_H_

#include "ara/os/interface/process/process_interaction.h"

namespace ara {
namespace os {
namespace interface {
namespace process {

/*!
 * \brief Factory class for retrieving platform-specific ProcessInteraction instances.
 *
 * \details
 * This class provides a single access point to a statically allocated, thread-safe instance of a platform-specific
 * implementation of the ProcessInteraction interface. By avoiding heap allocation, we reduce memory overhead
 * and meet stringent ASIL-D guidelines.
 *
 * \note The returned instance is valid for the entire lifetime of the program.
 */
class ProcessFactory {
public:
    /*!
     * \brief Retrieves the platform-specific ProcessInteraction instance.
     *
     * \return A const reference to an instance of ara::os::interface::process::ProcessInteraction.
     *
     * \note This method performs no dynamic memory allocation.
     *       The returned instance is statically allocated in a thread-safe manner.
     */
    static auto CreateInstance() noexcept -> const ProcessInteraction&;
};

} // namespace process
} // namespace interface
} // namespace os
} // namespace ara

#endif // OPEN_AA_ADAPTIVE_AUTOSAR_LIBS_INCLUDE_ARA_OS_INTERFACE_PROCESS_PROCESS_FACTORY_H_ 