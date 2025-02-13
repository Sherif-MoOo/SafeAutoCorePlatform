/**********************************************************************************************************************
 *  PROJECT
 *  -------------------------------------------------------------------------------------------------------------------
 *  \verbatim
 *  OpenAA: Open Source Adaptive AUTOSAR Project (CXX_STANDARD 17)
 *  \endverbatim
 *  -------------------------------------------------------------------------------------------------------------------
 *  FILE DESCRIPTION
 *  -------------------------------------------------------------------------------------------------------------------
 *  \file       main.cpp
 *  \brief      The entry file for the demo App...
 *
 *  \details    This application just a demo simple application for the adaptive autosar platform.
 *
 *  \note       Most likely your application shall have the same flow.
 ***********************************************************************************************************************/

/**********************************************************************************************************************
 *  INCLUDES
 *********************************************************************************************************************/

#include <iostream>                     // For std::cout and std::cerr
#include <csignal>                      // For signal mask
#include <algorithm>                    // For std::all_of
#include <optional>                     // For std::optional
#include <functional>                   // For std::reference_wrapper

#include "ara/core/array.h"             // For platform core Array class
#include "ara/core/abort.h"
#include "demo/manager/demo_manager.h"  // For manager class

namespace demo {
namespace sighandle {

/*!
 * \brief Block all signals except the SIGABRT, SIGBUS, SIGFPE, SIGILL, SIGSEGV signals.
 */
static auto InitilizeSigHandlerMask() noexcept -> void {

    bool success{false};

    sigset_t signals{};

    /** -------------------------------------------------------------------------------------------------------------------
     *  @brief  Private init method implementation.
     *
     *  @brief Configures the signal mask for the process to ensure proper signal handling.
     *
     *  This configuration must be applied as early as possible in the program's execution to guarantee that any newly
     *  spawned thread inherits this signal mask and blocks signals accordingly. By doing so, we ensure that only the
     *  dedicated signal handler thread is responsible for handling specific signals.
     *
     *  @details
     *  All signals are blocked except for the following critical ones: `SIGABRT`, `SIGBUS`, `SIGFPE`, `SIGILL`, and
     *  `SIGSEGV`. Blocking these signals could lead to undefined behavior, as their default handling typically involves
     *  terminating the process and generating a core dump. Preserving their default actions ensures that fatal errors are
     *  handled appropriately, adhering to the underlying POSIX environment's standards.
     *
     *  **Signal Mask Inheritance:**
     *  - The configured signal mask will be inherited by all subsequent threads. This inheritance guarantees that the
     *    blocking configuration is consistent across all threads, preventing unintended signal interruptions outside the
     *    dedicated handler thread.
     *
     *  **Critical Signals Preserved:**
     *  - The following signals are **not** blocked and are exclusively:
     *
     *    - **`SIGABRT` (Abort Signal):**
     *      - **Purpose:** Indicates that the process has aborted, typically invoked by the `abort()` function.
     *      - **Default Action:** Terminates the process and generates a core dump for debugging.
     *      -  @note    In addition to the ara::core::AbortHandler, or alternatively to it, the application
     *                  can also influence this mechanism by installing a signal handler for SIGABRT.
     *                  The signal handler for SIGABRT has the same choices of actions as the ara::core::
     *                  AbortHandler: it can terminate the process, return from the function call, 
     *                  defer function return by entering an infinite loop, or perform a non-local goto operation. The
     *                  same caveats as for the ara::core::AbortHandler apply here: non-local goto
     *                  operations and infinite loops should be avoided.
     *                  If the SIGABRT handler does not return, it should in general terminate abnormally
     *                  with SIGABRT. To do this without entering an infinite loop, it should restore the default
     *                  disposition of SIGABRT with std::signal(SIGABRT, SIG_DFL) and then re-raise
     *                  SIGABRT with e.g. std::raise(SIGABRT).
     *                  This “second step” of influence that the SIGABRT handler provides allows applications
     *                  that are already handling other synchronous signals such as SIGSEGV or SIGFPE to
     *                  treat SIGABRT the same way.
     *
     *    - **`SIGBUS` (Bus Error Signal):**
     *      - **Purpose:** Signifies an invalid memory access, such as misaligned memory access or accessing non-existent memory regions.
     *      - **Default Action:** Terminates the process and produces a core dump.
     *
     *    - **`SIGFPE` (Floating-Point Exception Signal):**
     *      - **Purpose:** Raised due to erroneous arithmetic operations, including division by zero, integer overflow, or invalid floating-point operations.
     *      - **Default Action:** Terminates the process and generates a core dump.
     *
     *    - **`SIGILL` (Illegal Instruction Signal):**
     *      - **Purpose:** Occurs when the process attempts to execute an invalid, undefined, or privileged machine instruction.
     *      - **Default Action:** Terminates the process and produces a core dump.
     *
     *    - **`SIGSEGV` (Segmentation Fault Signal):**
     *      - **Purpose:** Indicates an invalid memory reference, such as dereferencing a null or dangling pointer.
     *      - **Default Action:** Terminates the process and generates a core dump.
     *
     *  @note    A small subset of ara::core types and functions shall be usable independently of
     *           initialization with ara::core::Initialize and de-initialization with ara::core::
     *           Deinitialize. These are:
     *           • ara::core::ErrorCode and all its member functions and supporting constructs (including non-member operators)
     *           • ara::core::StringView and all its member functions and supporting constructs (including non-member operators)
     *           • ara::core::Result and all its member functions and supporting constructs, except for ara::core::Result::ValueOrThrow
     *           • ara::core::ErrorDomain and all its member functions and its sub-classes, as long as they adhere to [SWS_CORE_10400], but excluding <Prefix>ErrorDomain::ThrowAsException
     *           • ara::core::Initialize
     *           • ara::core::Abort
     *           • ara::core::SetAbortHandler
     *           • ara::core::AddAbortHandler
     *          - **POSIX Compliance:** This setup adheres to POSIX standards, ensuring compatibility and predictable behavior across
     *            Unix-like operating systems.
     *          - **Core Dumps:** Preserving the default actions for critical signals facilitates the generation of core dumps,
     *            which are essential for post-mortem debugging and diagnosing the root causes of fatal errors.
     */

    /* Fill the 'signals' variable with all possible signals */
    if (sigfillset(&signals) == 0) {

        /**  @note due to the above [SWS_CORE_15002] -> note that we can't use ara::core::Array<T,n> here */
        /* Define a C-style array containing the critical signals that should NOT be blocked */
        /**  @note  In Unix (and POSIX) systems, signal numbers such as SIGABRT, SIGBUS, SIGILL, SIGFPE, and SIGSEGV 
         *          are defined as macros that expand to integer constants. There is no separate type alias 
         *          specifically for these signal numbers—they are simply of type int. 
         */
        constexpr int kCriticalSigs[5] = { SIGABRT, SIGBUS, SIGILL, SIGFPE, SIGSEGV };

        /* Lambda to remove (unblock) each critical signal from the signal set */
        auto const dlt_sig_status = [&signals](int sig) noexcept -> bool {
            return (sigdelset(&signals, sig) == 0);
        };

        /* Check if all critical signals were successfully removed from the signal set */
        if (std::all_of(std::begin(kCriticalSigs), std::end(kCriticalSigs), dlt_sig_status)) {

            /* Set the process's signal mask to block all signals except the critical ones */
            if (pthread_sigmask(SIG_SETMASK, &signals, nullptr) == 0) {
                success = true;
            }
        }
    }

    /* If signal mask configuration fails, log an error message and abort the process */
    if (!success) {
        ara::core::Abort("[demo main][FATAL] Initialize signal handling failed.");
    }
}

} // namespace sighandle
} // namespace demo
    


int main() {

    /*Set main thread name for debugging*/
    pthread_setname_np(pthread_self(), "demo_main");

    demo::sighandle::InitilizeSigHandlerMask();

    // ara::core::Initialize

    std::cout << "[demo main][INFO] main thread started." << std::endl;

    std::uint8_t exit_code{EXIT_FAILURE};
    {
        std::optional<std::reference_wrapper<demo::manager::DemoManager>> managerOpt = demo::manager::DemoManager::StartManager();


        if (managerOpt.has_value()) {
            demo::manager::DemoManager& manager = managerOpt.value().get();

            exit_code = manager.RunManager();

            std::cout << "[demo main][INFO] Manager exited with code: " << static_cast<int>(exit_code) << std::endl;
        }
        else {
            std::cerr << "[demo main][ERROR] Failed to start DemoManager: Instance already created and exclusively owned." << std::endl;
        }
    }

    // ara::core::Deinitialize

    std::cout << "[demo main][INFO] main thread finished." << std::endl;

    return exit_code;
}