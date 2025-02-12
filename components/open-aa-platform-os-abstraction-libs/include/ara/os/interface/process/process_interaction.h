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
 *  \brief      Definition of the ara::os::interface::process::ProcessInteraction interface.
 *
 *  \details
 *              This file defines the abstract interface for process-related operations.
 *              Implementations must be thread-safe, use safe string and file operations, and meet ASIL-D standards.
 **********************************************************************************************************************/

#ifndef OPEN_AA_ADAPTIVE_AUTOSAR_LIBS_INCLUDE_ARA_OS_INTERFACE_PROCESS_PROCESS_INTERACTION_H_
#define OPEN_AA_ADAPTIVE_AUTOSAR_LIBS_INCLUDE_ARA_OS_INTERFACE_PROCESS_PROCESS_INTERACTION_H_

/**********************************************************************************************************************
 *  INCLUDES
 *********************************************************************************************************************/
/*!
 * \brief  Includes necessary standard headers for process operations.
 *
 */
#include <cstddef>      // For std::size_t
#include <cstdint>      // For fixed-width integer types
#include <cstring>      // For std::strlen, std::strncpy

namespace ara {
namespace os {
namespace interface {
namespace process {

/*!
 * \brief Enumeration of error codes for ProcessInteraction operations.
 *
 * \details
 * This enum provides clear, unambiguous error codes for process-related operations.
 * These codes support ASIL-D requirements by enabling precise error handling.
 */
enum class ErrorCode : uint8_t {
    Success = 0,         /*!< Operation completed successfully */
    BufferTooSmall,      /*!< The provided buffer is insufficient */
    RetrievalFailed,     /*!< Failed to retrieve the process name */
    NullBuffer,          /*!< The provided buffer pointer is null */
    UnknownError         /*!< An unspecified error occurred */
};

/*!
 * \brief Abstract interface for platform-specific process interaction.
 *
 * \details
 * This interface abstracts away the operating system specifics for retrieving process information.
 * Implementations must be thread-safe and adhere to safe programming practices (e.g., avoid buffer overflows).
 *
 * \note This interface is designed to be implemented without dynamic memory allocation within the methods.
 */
class ProcessInteraction {
public:
    virtual ~ProcessInteraction() = default;

    /*!
     * \brief Retrieves the name of the current process.
     *
     * \param[out] buffer      Pointer to the buffer where the process name will be stored.
     * \param[in]  bufferSize  Size of the provided buffer in bytes.
     *
     * \return An ErrorCode indicating the result:
     *         - Success if the process name was successfully retrieved.
     *         - NullBuffer if the output pointer is null.
     *         - BufferTooSmall if the buffer is too small.
     *         - RetrievalFailed if an unexpected error occurred.
     *
     * \note The returned process name is null-terminated.
     *       Implementations must use safe string operations and be thread-safe.
     */
    virtual auto GetProcessName(char* buffer, std::size_t bufferSize) const noexcept -> ErrorCode = 0;
};

} // namespace process
} // namespace interface
} // namespace os
} // namespace ara

#endif // OPEN_AA_ADAPTIVE_AUTOSAR_LIBS_INCLUDE_ARA_OS_INTERFACE_PROCESS_PROCESS_INTERACTION_H_
