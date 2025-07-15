/**********************************************************************************************************************
 *  PROJECT
 *  -------------------------------------------------------------------------------------------------------------------
 *  \verbatim
 *  OpenAA: Open Source Adaptive AUTOSAR Project (CXX_STANDARD 17)
 *  \endverbatim
 *  -------------------------------------------------------------------------------------------------------------------
 *  FILE DESCRIPTION
 *  -------------------------------------------------------------------------------------------------------------------
 *  \file       demo/manager/demo_manager.cpp
 *  \brief      Implementation of the demo::manager::DemoManager class.
 *
 *  \details    This file implements the demo::manager::DemoManager class, ensuring controlled access to a unique
 *              instance without any heap overhead.
 *********************************************************************************************************************/

/**********************************************************************************************************************
 *  INCLUDES
 *********************************************************************************************************************/
#include <algorithm>                        // For std::all_of
#include <atomic>                           // For std::atomic, memory_order
#include <cstdint>                          // For the std types
#include <iostream>                         // For std::cout / std::cerr
#include <csignal>                          // For signal block / sigwait
#include <cstring>                          // For std::strerror
#include <thread>                           // For std::thread
#include <chrono>                           // For steady_clock timing

#include "ara/core/array.h"                 // For platform core Array class
#include "ara/core/abort.h"
#include "demo/manager/demo_manager.h"      // For the manager class

namespace demo {
namespace manager {

/** -------------------------------------------------------------------------------------------------------------------
 *  @brief      Static member initialization.
 *
 *  Initializes the running-cycle constant, the singleton-instance flag and the guarding mutex.
 */
constexpr std::uint32_t kRunningCycle{5000U};
bool                    DemoManager::instanceCreated_{false};
std::mutex              DemoManager::mutex_{};          // used only for singleton & wait-loop

/** -------------------------------------------------------------------------------------------------------------------
 *  @brief      Retrieves the unique instance of DemoManager.
 *
 *  This static method initializes the instance upon the first call in a thread-safe manner and returns an optional
 *  reference to it. Only the first caller can successfully obtain the instance. Subsequent calls will return
 *  an empty optional, ensuring that only the creator has access.
 *
 *  @return     std::optional<std::reference_wrapper<DemoManager>> Optional containing the instance reference or
 *              an empty optional if the instance has already been created.
 */
auto DemoManager::StartManager() noexcept -> std::optional<std::reference_wrapper<DemoManager>>
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (instanceCreated_) {                      // somebody already grabbed it
        return std::nullopt;
    }
    static DemoManager instance;                 // static-lifetime, no heap
    instanceCreated_ = true;
    return std::ref(instance);
}

/** -------------------------------------------------------------------------------------------------------------------
 *  @brief      Private constructor implementation.
 */
DemoManager::DemoManager() noexcept
  : graceful_shutdown_handler_thread_{},
    shutdown_notifier_{},
    turn_off_requested_{false}
{
    InitializeDemoManager();
}

DemoManager::~DemoManager() noexcept
{
    std::cout << "[demo mngr][INFO] Demo Manager demolished." << std::endl;
}

/** -------------------------------------------------------------------------------------------------------------------
 *  @brief      Private init method implementation.
 */
auto DemoManager::InitializeDemoManager() noexcept -> void
{
    /* Spawn the dedicated signal-handling thread */
    graceful_shutdown_handler_thread_ = std::thread(&DemoManager::GracefulShutdownHandler, this);
    if (!graceful_shutdown_handler_thread_.joinable()) {
        ara::core::Abort("[demo mngr][FATAL] Graceful shutdown handler thread creation failed.");
    }

    std::cout << "[demo mngr][INFO] Demo Manager initialized successfully." << std::endl;
}

/** -------------------------------------------------------------------------------------------------------------------
 *  @brief      Signal-handler thread: waits for SIGTERM/SIGINT and requests shutdown.
 */
auto DemoManager::GracefulShutdownHandler() noexcept -> void
{
    /* Set thread name for debugging */
    pthread_setname_np(pthread_self(), "demo_sig");

    sigset_t signals{};
    constexpr ara::core::Array<int, 2> kShutdownSigs{SIGTERM, SIGINT};

    if (sigemptyset(&signals) != 0 ||
        std::any_of(kShutdownSigs.cbegin(), kShutdownSigs.cend(),
                    [&signals](int s) { return sigaddset(&signals, s) != 0; }) ||
        pthread_sigmask(SIG_BLOCK, &signals, nullptr) != 0)
    {
        ara::core::Abort("[demo mngr][FATAL] Initialise shutdown signal handling failed.");
    }

    int received{};
    if (sigwait(&signals, &received) != 0) {
        ara::core::Abort("[demo mngr][FATAL] sigwait() failed.");
    }

    /* Log which signal we caught (no mutex required for console output) */
    switch (received) {
        case SIGTERM: std::cout << "[demo mngr][INFO] Demo Manager caught a SIGTERM." << std::endl; break;
        case SIGINT:  std::cout << "[demo mngr][INFO] Demo Manager caught a SIGINT."  << std::endl; break;
        default:      std::cout << "[demo mngr][WARN] Demo Manager caught an unknown signal." << std::endl; break;
    }

    /* --- Acquire/Release publication edge ---------------------------------- */
    turn_off_requested_.store(true, std::memory_order_release);   // publish flag
    shutdown_notifier_.notify_all();                              // wake manager loop
}

/** -------------------------------------------------------------------------------------------------------------------
 *  @brief      Runs the manager and returns an exit code.
 */
auto DemoManager::RunManager() noexcept -> std::uint8_t
{
    std::uint8_t exit_code{EXIT_SUCCESS};
    bool         thread_running{true};

    pthread_t   native_handle = pthread_self();
    sched_param param{};
    int         current_policy{-1};

    /* Preserve current scheduling parameters */
    if (pthread_getschedparam(native_handle, &current_policy, &param) != 0) {
        ara::core::Abort("[demo mngr][FATAL] Failed to get current scheduling parameters: ",
                         std::strerror(errno));
    }

    std::cout << "[demo mngr][INFO] Manager is in Running state." << std::endl;

    /* Unique_lock needed only for the condition-variable */
    std::unique_lock<std::mutex> lock(mutex_);

    do {
        auto start_time = std::chrono::steady_clock::now();

        /* Periodic debug print */
        std::cout << "[demo mngr][INFO] Current scheduling policy: ";
        switch (current_policy) {
            case SCHED_FIFO:  std::cout << "SCHED_FIFO";  break;
            case SCHED_RR:    std::cout << "SCHED_RR";    break;
            case SCHED_OTHER: std::cout << "SCHED_OTHER"; break;
            default:          std::cout << "UNKNOWN";
        }
        std::cout << ", priority: " << param.sched_priority << std::endl;

        /* Compute remaining time in the cycle */
        auto elapsed_time   = std::chrono::steady_clock::now() - start_time;
        using namespace std::chrono;
        using namespace std::chrono_literals; 
        
        constexpr auto kRunningPeriod = kRunningCycle * 1ms;
        auto           remaining_time = kRunningPeriod - elapsed_time;

        if (remaining_time > std::chrono::milliseconds::zero()) {
            /* Wait until next tick *or* until the shutdown flag is published */
            thread_running = !shutdown_notifier_.wait_for(
                lock,
                remaining_time,
                [this] {
                    return turn_off_requested_.load(std::memory_order_acquire);   // acquire edge
                });
        } else {
            std::cout << "[demo mngr][WARN] Manager over-ran the configured "
                      << kRunningCycle << " ms cycle (took "
                      << std::chrono::duration_cast<std::chrono::milliseconds>(elapsed_time).count()
                      << " ms)." << std::endl;

            thread_running = !turn_off_requested_.load(std::memory_order_acquire);
        }
    } while (thread_running);

    TerminateDemoManager();
    return exit_code;   // EXIT_SUCCESS
}

/** -------------------------------------------------------------------------------------------------------------------
 *  @brief      Private tear-down sequence.
 */
auto DemoManager::TerminateDemoManager() noexcept -> void
{
    if (graceful_shutdown_handler_thread_.joinable()) {
        graceful_shutdown_handler_thread_.join();
    }
}

} // namespace manager
} // namespace demo
