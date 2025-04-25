/**********************************************************************************************************************
 *  PROJECT
 *  -------------------------------------------------------------------------------------------------------------------
 *  \verbatim
 *  OpenAA: Open Source Adaptive AUTOSAR Project (CXX_STANDARD 17)
 *  \endverbatim
 *  -------------------------------------------------------------------------------------------------------------------
 *  FILE DESCRIPTION
 *  -------------------------------------------------------------------------------------------------------------------
 *  \file       ara/os/interface/process/process_interaction.cpp
 *  \brief      Out-of-line CRTP dispatch and free-function definition for ProcessInteraction.
 *
 *  \details
 *              This file provides:
 *               - The out-of-line implementation of
 *                   ProcessInteraction<Derived>::GetProcessName(), forwarding to
 *                   Derived::GetProcessNameImpl().
 *               - The free-function definition of GetProcessName() (non-inline),
 *                 allowing LTO to inline it across translation units.
 *               - Explicit template instantiation for each platform-specific Derived.
 **********************************************************************************************************************/

#include "ara/os/interface/process/process_interaction.h"

#if defined(__linux__)
#  include "ara/os/linux/process/process.h"
   namespace ara::os::interface::process {
       using DefaultProcessImpl = ::ara::os::linux::process::ProcessLinuxImpl;
   }
#elif defined(__QNXNTO__)
#  include "ara/os/qnx/process/process.h"
   namespace ara::os::interface::process {
       using DefaultProcessImpl = ::ara::os::qnx::process::ProcessQnxImpl;
   }
#else
#  error "Unsupported platform for CRTP instantiation"
#endif

namespace ara {
namespace os {
namespace interface {
namespace process {

//------------------------------------------------------------------------------
// Out-of-line CRTP dispatch
//------------------------------------------------------------------------------
template <typename Derived>
auto ProcessInteraction<Derived>::GetProcessName(char* buffer,
                                                 std::size_t bufferSize) const noexcept
    -> ErrorCode
{
    return static_cast<const Derived*>(this)
        ->GetProcessNameImpl(buffer, bufferSize);
}

//------------------------------------------------------------------------------
// Explicit template instantiations
//------------------------------------------------------------------------------
#if defined(__linux__)
template class ProcessInteraction<linux::process::ProcessLinuxImpl>;
#elif defined(__QNXNTO__)
template class ProcessInteraction<qnx::process::ProcessQnxImpl>;
#endif

//------------------------------------------------------------------------------
// Free-function definition (non-inline)
//------------------------------------------------------------------------------
auto GetProcessName(char* buffer, std::size_t bufferSize) noexcept -> ErrorCode
{
    return DefaultProcessImpl{}.GetProcessName(buffer, bufferSize);
}

} // namespace process
} // namespace interface
} // namespace os
} // namespace ara
