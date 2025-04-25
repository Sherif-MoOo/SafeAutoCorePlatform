/**********************************************************************************************************************
 *  PROJECT
 *  -------------------------------------------------------------------------------------------------------------------
 *  \verbatim
 *  OpenAA: Open Source Adaptive AUTOSAR Project (CXX_STANDARD 17)
 *  \endverbatim
 *  -------------------------------------------------------------------------------------------------------------------
 *  FILE DESCRIPTION
 *  -------------------------------------------------------------------------------------------------------------------
 *  \file       ara/os/interface/process/process_interaction.h
 *  \brief      CRTP-based interface and free-function declaration for ProcessInteraction.
 *
 *  \details
 *              This header defines:
 *               - The ErrorCode enum for process operations.
 *               - A CRTP base template ProcessInteraction<Derived> with an out-of-line dispatch.
 *               - A declaration of GetProcessName() for client code.
 *
 *              No virtual dispatch is used, ensuring zero run-time overhead.
 **********************************************************************************************************************/

#ifndef OPEN_AA_ADAPTIVE_AUTOSAR_LIBS_INCLUDE_ARA_OS_INTERFACE_PROCESS_PROCESS_INTERACTION_H_
#define OPEN_AA_ADAPTIVE_AUTOSAR_LIBS_INCLUDE_ARA_OS_INTERFACE_PROCESS_PROCESS_INTERACTION_H_

/**********************************************************************************************************************
 *  INCLUDES
 **********************************************************************************************************************/
#include <cstddef>      // For std::size_t
#include <cstdint>      // For uint8_t

namespace ara {
namespace os {
namespace interface {
namespace process {

/*!
 * \brief  Error codes for process operations.
 */
enum class ErrorCode : uint8_t {
    Success = 0,      /*!< Operation completed successfully */
    BufferTooSmall,   /*!< The provided buffer is insufficient */
    RetrievalFailed,  /*!< Failed to retrieve the process name */
    NullBuffer,       /*!< The provided buffer pointer is null */
    UnknownError      /*!< An unspecified error occurred */
};

/*!
 * \brief  CRTP base for platform-specific ProcessInteraction.
 *
 * \tparam Derived  The concrete implementation class providing GetProcessNameImpl().
 */
template <typename Derived>
class ProcessInteraction {
public:
    /*!
     * \brief  Dispatch to Derived::GetProcessNameImpl().
     *
     * \param[out] buffer      Buffer to receive the process name.
     * \param[in]  bufferSize  Size of the buffer in bytes.
     *
     * \return ErrorCode indicating the result.
     */
    auto GetProcessName(char* buffer, std::size_t bufferSize) const noexcept -> ErrorCode;
};

/*!
 * \brief  Retrieves the name of the current process.
 *
 * \param[out] buffer      Buffer to receive the process name.
 * \param[in]  bufferSize  Size of the buffer in bytes.
 *
 * \return ErrorCode indicating the result.
 *
 * \note   Defined in the .cpp so that LTO can inline it across TUs.
 */
auto GetProcessName(char* buffer, std::size_t bufferSize) noexcept -> ErrorCode;

} // namespace process
} // namespace interface
} // namespace os
} // namespace ara

#endif // OPEN_AA_ADAPTIVE_AUTOSAR_LIBS_INCLUDE_ARA_OS_INTERFACE_PROCESS_PROCESS_INTERACTION_H_
