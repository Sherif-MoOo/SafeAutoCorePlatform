// ========================================================================
// Project: OpenAA: Open Source Adaptive AUTOSAR Project (CXX_STANDARD 17)
// File:    main.cpp
// Brief:   Comprehensive test application for the ara::core::Abort API.
// Details: This program tests all possible scenarios for the Abort API:
//          1) Inline tests for AddAbortHandler() and SetAbortHandler().
//          2) Cross-translation unit sharing: setting a handler in another file and retrieving it here.
//          3) Termination test: calling Abort() (which terminates the process).
//          Negative test cases (expected compile-time errors) are provided as commented-out code.
// Note:    All comments use the double-slash style.
// ========================================================================
#if defined(__linux__)
#include <sys/prctl.h>
#endif
#include <cstdint>
#include <iostream>    
#include <csignal>     
#include <algorithm>   
#include <optional>    
#include <functional>
#include <cassert>
#include <string>  
#include "ara/core/abort.h"

static constexpr std::string_view   kProcessNameView{"CoreAbortTest"};
static constexpr std::uint8_t       kMaxProcessName{15};

// Forward declaration of a function defined in another translation unit.
ara::core::AbortHandler SetHandlerFromOtherTU() noexcept;

namespace core_abort_test {
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
        ara::core::Abort("[FATAL] Initialize signal handling failed.");
    }
}

} // namespace sighandle
} // namespace core_abort_test

// ------------------------------------------------------------------------
// Test 1: Inline Functions
// ------------------------------------------------------------------------
void TestInlineFunctions()
{
    std::cout << "=== Test 1: Inline Functions ===" << std::endl;

    // Test 1a: Reset handlers when none are installed.
    auto prevHandler = ara::core::SetAbortHandler(nullptr);
    std::cout << "prevHandler: " 
              << (prevHandler == nullptr ? "nullptr" : "non-null") << std::endl;
    assert(prevHandler == nullptr);

    // Test 1b: Install a dummy handler using AddAbortHandler.
    auto DummyHandler = []() noexcept {
        std::cout << "[DummyHandler] Called." << std::endl;
    };
    bool added = ara::core::AddAbortHandler(DummyHandler);
    std::cout << "DummyHandler added: " << (added ? "true" : "false") << std::endl;
    assert(added);

    // Test 1c: Replace the installed handler using SetAbortHandler.
    auto oldHandler = ara::core::SetAbortHandler(DummyHandler);
    std::cout << "Old handler from SetAbortHandler: " 
              << (oldHandler == DummyHandler ? "DummyHandler" : "other") << std::endl;
    assert(oldHandler == DummyHandler);

    // Test 1d: Reset the handler by passing nullptr.
    oldHandler = ara::core::SetAbortHandler(nullptr);
    std::cout << "Old handler after resetting: " 
              << (oldHandler == DummyHandler ? "DummyHandler" : "other") << std::endl;
    assert(oldHandler == DummyHandler);

    // Compile-time negative test (commented out):
    // The following would fail because an int is not convertible to ara::core::StringView.
    // ara::core::Abort(42);

    // Negative test (commented out): Installing a nullptr should return false.
    /*
    bool addNull = ara::core::AddAbortHandler(nullptr);
    assert(addNull == false);
    */

    std::cout << "Inline tests passed." << std::endl;
}

// ------------------------------------------------------------------------
// Test 2: Cross-Translation Unit Sharing
// ------------------------------------------------------------------------
void TestCrossTU()
{
    std::cout << "=== Test 2: Cross-Translation Unit Sharing ===" << std::endl;
    
    // Call the function from cross_tu.cpp to set a unique handler.
    // That function sets the global handler to CrossTUHandler and returns the previous handler.
    // Since no handler was installed before, it should return nullptr.
    SetHandlerFromOtherTU();
    
    // Now, in main.cpp, retrieve the current handler by calling SetAbortHandler(nullptr).
    // This call will remove the currently installed handler and return it.
    ara::core::AbortHandler currentHandler = ara::core::SetAbortHandler(nullptr);
    
    // We expect currentHandler to be non-null (i.e. the handler set in cross_tu.cpp).
    std::cout << "Current handler retrieved in main.cpp: " 
              << reinterpret_cast<const void*>(currentHandler) << std::endl;
    assert(currentHandler != nullptr);
    
    // Optionally, we could call the handler to verify its behavior.
    currentHandler();
    
    std::cout << "Cross-TU test passed: A non-null handler from cross_tu.cpp is visible in main.cpp." << std::endl;
}

// ------------------------------------------------------------------------
// Test 3: Termination Test
// ------------------------------------------------------------------------
// WARNING: This test calls Abort() and terminates the process. Run it in isolation.
void TestTerminateFunction()
{
    std::cout << "=== Test 3: Termination Test ===" << std::endl;
    
    // Define a termination handler that prints a message.
    auto TerminateHandler = []() noexcept {
        std::cout << "[TerminateHandler] Executing before termination." << std::endl;
    };
    ara::core::SetAbortHandler(TerminateHandler);

    std::cout << "Calling Abort() now. The process should log a message, invoke TerminateHandler, and then abort." << std::endl;
    ara::core::Abort("Test termination via Abort().");

    // No code after this call is executed.
}

// ------------------------------------------------------------------------
// Print usage instructions
// ------------------------------------------------------------------------
void PrintUsage(const char* prog)
{
    std::cout << "Usage: " << prog << " [test_number]\n"
              << "Available Tests:\n"
              << "  1 - Inline tests (AddAbortHandler, SetAbortHandler, etc.)\n"
              << "  2 - Cross-TU Sharing test\n"
              << "  3 - Termination test (calls Abort() and terminates the process)\n";
}
// ------------------------------------------------------------------------
// Main entry point.
// ------------------------------------------------------------------------
int main(int argc, char* argv[])
{   
    static_assert(kProcessNameView.size() <= kMaxProcessName,
    "\n[ERROR] Process name is too long!!\n");

#if defined(__linux__)
    prctl(PR_SET_NAME, kProcessNameView.data(), 0, 0, 0);
#elif defined(__QNXNTO__)
    // On QNX, use pthread_setname_np() to set the name of the calling thread (main thread).
    // This name will be used as the process name.
    pthread_setname_np(pthread_self(), kProcessNameView.data());
#endif

    core_abort_test::sighandle::InitilizeSigHandlerMask();

    if (argc != 2) {
        PrintUsage(argv[0]);
        return 1;
    }
    std::string choice = argv[1];
    
    if (choice == "1")
    {
        TestInlineFunctions();
    }
    else if (choice == "2")
    {
        TestCrossTU();
    }
    else if (choice == "3")
    {
        TestTerminateFunction();
    }
    else {
        PrintUsage(argv[0]);
        return 1;
    }
    return 0;
}