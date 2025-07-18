/***********************************************************************************************************************
 *  PROJECT
 *  --------------------------------------------------------------------------------------------------------------------
 *  \verbatim
 *  OpenAA: Open Source Adaptive AUTOSAR Project (CXX_STANDARD 17)
 *  \endverbatim
 *  --------------------------------------------------------------------------------------------------------------------
 *  FILE DESCRIPTION
 *  --------------------------------------------------------------------------------------------------------------------
 *  \file       ara/core/abort.h
 *  \brief      Definition and implementation of the ara::core::Abort API.
 *
 *  \details    This header provides a header-only implementation of the abort mechanism for the Adaptive
 *              AUTOSAR platform. It defines the API for explicitly aborting an operation due to a
 *              standardized or non-standardized violation. In accordance with [SWS_CORE_00090], [SWS_CORE_00003],
 *              [SWS_CORE_12403]–[SWS_CORE_12409], the implementation logs a message to the standard error stream,
 *              calls any installed abort handlers in reverse order, and finally terminates the process via std::abort.
 *
 *  \note       The arguments passed to Abort() must be convertible to ara::core::StringView.
**********************************************************************************************************************/
#ifndef OPEN_AA_ADAPTIVE_AUTOSAR_LIBS_INCLUDE_ARA_CORE_ABORT_H_
#define OPEN_AA_ADAPTIVE_AUTOSAR_LIBS_INCLUDE_ARA_CORE_ABORT_H_

/***********************************************************************************************************************
 *  INCLUDES
 ***********************************************************************************************************************/
#include <unistd.h>      // ::write, STDERR_FILENO
#include <cerrno>        // errno, EINTR
#include <climits>       // SSIZE_MAX
#include <cstdlib>       /* For std::abort */
#include <string_view>   /* For std::string_view */
#include <iostream>      /* For std::cerr */

#include "ara/core/string_view.h"                   // For ara::core::StringView
#include "ara/core/internal/utility.h"              // For utility functions and traits

/***********************************************************************************************************************
 *  NAMESPACE: ara::core
***********************************************************************************************************************/
namespace ara {
namespace core {

using namespace literals::string_view_literals;

namespace detail {

inline constexpr ara::core::StringView kNull{"<null>"};
inline constexpr ara::core::StringView kTrunc{"...[truncated]"};

/* Core safe write of a *validated* StringView payload; void, best effort. */
template <std::size_t MaxBytes>
inline void SafeWriteFd(int fd, ara::core::StringView sv) noexcept
{
    const char* data = sv.data();
    std::size_t len  = sv.size();

    if (data == nullptr) {
        [[maybe_unused]] auto _ = ::write(fd, kNull.data(), kNull.size());
        return;
    }

    if (len > MaxBytes) {
        len = MaxBytes;
    }

    const char*     p         = data;
    std::size_t     remaining = len;

    while (remaining > 0) {
        const std::size_t chunk = std::min<std::size_t>(remaining,
                                      static_cast<std::size_t>(SSIZE_MAX));
        const ssize_t n = ::write(fd, p, chunk);
        if (n > 0) {
            p         += n;
            remaining -= static_cast<std::size_t>(n);
            continue;
        }
        if (n == -1 && errno == EINTR) {
            continue;  // retry
        }
        break;          // unrecoverable error; give up
    }

    if (sv.size() > MaxBytes) {
        [[maybe_unused]] auto _ = ::write(fd, kTrunc.data(), kTrunc.size());
    }
}

template <std::size_t MaxBytes = 4096U, typename... SVs>
inline void AbortWriteLine(int fd,
                           ara::core::StringView header_sv,
                           SVs&&... svs) noexcept
{
    static_assert((std::is_same_v<std::decay_t<SVs>, ara::core::StringView> && ...),
                  "\n[ERROR] ara::core::AbortWriteLine(): all args must be ara::core::StringView.\n");

    detail::SafeWriteFd<MaxBytes>(fd, header_sv);

    bool first = true;
    auto write_one = [&](ara::core::StringView sv) noexcept {
        if (!first) {
            static constexpr char sp = ' ';
            [[maybe_unused]] auto _ = ::write(fd, &sp, 1);
        }
        first = false;
        detail::SafeWriteFd<MaxBytes>(fd, sv);
    };

    (write_one(std::forward<SVs>(svs)), ...);

    static constexpr char nl = '\n';
    [[maybe_unused]] auto _ = ::write(fd, &nl, 1);
}

} // namespace detail_abort_write

/***********************************************************************************************************************
 *  ABORT HANDLER DEFINITION
***********************************************************************************************************************/
/* Type alias for an abort handler.
 * Handlers must conform to a function pointer type with the noexcept specifier.
 *
 * [SWS_CORE_00050]
 */
using AbortHandler = auto (*)() noexcept -> void;

/***********************************************************************************************************************
 *  API FUNCTION: Abort
 ***********************************************************************************************************************/
/*! Aborts the current operation.
 *
 * This function outputs a log message (to the standard error stream and via DLT logging if implemented)
 * containing the provided message arguments, calls all installed AbortHandlers in reverse order,
 * and terminates the process by calling std::abort.
 *
 * Template Parameter:
 *    Args - Types of the arguments provided. All must be convertible to ara::core::StringView.
 * Parameter:
 *    args - Custom texts to be added in the log message.
 *
 * [SWS_CORE_00052], [SWS_CORE_12403], [SWS_CORE_12408]
 *
 * \warning This function does not return.
 *
 * Note:
 *    Although the function is marked inline, this does not guarantee that the compiler will optimize away
 *    the function call. The inline specifier here is used to allow multiple definitions in different TUs
 *    and to potentially enable compiler inlining.
 */
template <typename... Args>
[[noreturn]] auto Abort(const Args &... args) noexcept -> void
{
    /* Ensure all arguments are convertible to ara::core::StringView */
    static_assert((std::is_convertible_v<Args, StringView> && ...),
        "\n[ERROR] All arguments to ara::core::Abort() must be convertible to ara::core::StringView.\n");
    
    /* Lock the mutex to ensure thread-safety and block other calls while abort is in progress */
    std::lock_guard<std::mutex> lock(detail::g_abortMutex);

    /* Directly log each argument to std::cerr using the stream interface.
     * This avoids the need for any temporary buffers.
     */
    detail::AbortWriteLine(
        STDERR_FILENO,
        "Process aborted via ara::core::Abort: "_sv,
        ara::core::StringView(args)...);
    /* [Optional] Insert DLT logging here as a fatal log with the appropriate context id.
     * For example: DltLogFatal(...);
     * (Implementation-specific; omitted for brevity.)
     */

    /* Call installed AbortHandlers in reverse order */
    for (std::size_t i = detail::g_abortHandlerCount; i > 0; --i)
    {
        if (detail::g_abortHandlers[i - 1] != nullptr)
        {
            detail::g_abortHandlers[i - 1]();
        }
    }

    /* If no AbortHandler terminated the process, force abnormal termination */
    std::abort();
}

/***********************************************************************************************************************
 *  API FUNCTION: AddAbortHandler
 ***********************************************************************************************************************/
/* Adds a custom AbortHandler.
 *
 * Parameter:
 *    handler - A pointer to a custom AbortHandler.
 *
 * Returns:
 *    true if the handler was successfully installed; false otherwise (e.g. if the maximum
 *    number of handlers is exceeded or if a nullptr is provided).
 *
 * [SWS_CORE_00054]
 * This function is thread-safe.
 */
inline auto AddAbortHandler(AbortHandler handler) noexcept -> bool
{
    if (handler == nullptr)
    {
        return false;
    }
    std::lock_guard<std::mutex> lock(detail::g_abortMutex);
    if (detail::g_abortHandlerCount < detail::kMaxAbortHandlers)
    {
        detail::g_abortHandlers[detail::g_abortHandlerCount++] = handler;
        return true;
    }
    return false;
}

/***********************************************************************************************************************
 *  API FUNCTION: SetAbortHandler
 ***********************************************************************************************************************/
/* Sets (or resets) the custom AbortHandler.
 *
 * Parameter:
 *    handler - A pointer to a custom AbortHandler, or nullptr to remove all previously installed
 *              handlers and restore the default behavior.
 *
 * Returns:
 *    The most recently installed AbortHandler prior to this call, or nullptr if none was installed.
 *
 * [SWS_CORE_00051]
 * This function is thread-safe. If a nullptr is passed, all previously installed handlers are removed.
 */
inline auto SetAbortHandler(AbortHandler handler) noexcept -> AbortHandler
{
    std::lock_guard<std::mutex> lock(detail::g_abortMutex);
    AbortHandler previous = (detail::g_abortHandlerCount > 0
                                  ? detail::g_abortHandlers[detail::g_abortHandlerCount - 1]
                                  : nullptr);

    /* If a new handler is provided, install it */
    if (handler != nullptr)
    {
        detail::g_abortHandlers[0] = handler;
        detail::g_abortHandlerCount = 1U;
    } else {

        /* Remove all existing handlers */
        detail::g_abortHandlerCount = 0U;

    }

    return previous;
}

} /* namespace core */
} /* namespace ara */

#endif /* OPEN_AA_ADAPTIVE_AUTOSAR_LIBS_INCLUDE_ARA_CORE_ABORT_H_ */
