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
#include <cstdint>                          // For the std types
#include <iostream>                         // For the std::cout and cerr
#include <csignal>                          // For signal Block to handle shutdown
#include <cstring>                          // For std::strerror

#include "ara/core/array.h"                 // For platform core Array class
#include "ara/core/abort.h"
#include "demo/manager/demo_manager.h"      // For the manager class

namespace demo {
namespace manager {

/** -------------------------------------------------------------------------------------------------------------------
 *  @brief      Static member initialization.
 *
 *  Initializes The Running cycle, the static flag and mutex.
 */
constexpr std::uint32_t kRunningCycle{5000U};
bool DemoManager::instanceCreated_{false};
std::mutex DemoManager::mutex_{};

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
auto DemoManager::StartManager() noexcept -> std::optional<std::reference_wrapper<DemoManager>> {
    std::lock_guard<std::mutex> lock(mutex_);
    if (instanceCreated_) {
        return std::nullopt;
    }
    static DemoManager instance;
    instanceCreated_ = true;
    return std::ref(instance);
}

/** -------------------------------------------------------------------------------------------------------------------
 *  @brief      Private constructor implementation.
 *
 *  Initializes the DemoManager instance. Any required initialization code can be placed here.
 */
DemoManager::DemoManager() noexcept
    : graceful_shutdown_handler_thread_{},
      shutdown_notifier_{},
      turn_off_requested_{false}
{
    InitializeDemoManager();
}

DemoManager::~DemoManager() noexcept {

    std::cout << "[demo mngr][INFO] Demo Manager demolished."<< std::endl;
}

/** -------------------------------------------------------------------------------------------------------------------
 *  @brief      Private init method implementation.
 *
 *   the instance init sequence.
 */
auto DemoManager::InitializeDemoManager() noexcept -> void {

    /*Start the signal handler thread and check for errors in thread creation*/
    graceful_shutdown_handler_thread_ = std::thread(&DemoManager::GracefulShutdownHandler, this);
    if (!graceful_shutdown_handler_thread_.joinable()) {

        ara::core::Abort("[demo mngr][FATAL] Graceful shutdown handler thread creation failed.");
    }


    std::cout << "[demo mngr][INFO] Demo Manager initialized successfuly."<< std::endl;
}

/** -------------------------------------------------------------------------------------------------------------------
 *  @brief      Private init method implementation.
 *
 *   the instance init sequence.
 */
auto DemoManager::GracefulShutdownHandler() noexcept -> void {
    
    /*Set shutdown thread name for debugging*/
    pthread_setname_np(pthread_self(), "demo_sig");

    bool success{false};
    
    sigset_t singals{};

    int def_sig{-1};

    constexpr const ara::core::Array<int,2> kShutdownSigs{SIGTERM, SIGINT};

    auto const add_sig_status = [&singals](int const& sig)  noexcept -> bool {

        return (sigaddset(&singals, sig) == 0);
    };

    /*Empty the unitialized signal set*/
    if(sigemptyset(&singals) == 0) {
        
        /*add the SIGTERM, and SIGINT to the singal set*/
        if(std::all_of(kShutdownSigs.cbegin(), kShutdownSigs.cend(), add_sig_status)) {

            /*Block these signals in this thread so they can be caught by sigwait*/
            if (pthread_sigmask(SIG_BLOCK, &singals, nullptr) == 0) {

                /*The thread is blocked and Wait for a signal to be received*/
                if(sigwait(&singals, &def_sig) == 0) {
                    
                    std::lock_guard<std::mutex> lock(mutex_);

                    switch(def_sig) {

                        case kShutdownSigs[0]:

                            std::cout << "[demo mngr][INFO] Demo Manager caught a SIGTERM." << std::endl;
                            break;
                        
                        case kShutdownSigs[1]:
                            std::cout << "[demo mngr][INFO] Demo Manager caught a SIGINT." << std::endl;
                            break;

                    }

                    turn_off_requested_.store(true);
                    shutdown_notifier_.notify_all();
                    success = true;

                }
            }
        }
    }

    if(!success) {

        ara::core::Abort("[demo mngr][FATAL] Initialize shutdown signal handling failed.");
    }




}

/** -------------------------------------------------------------------------------------------------------------------
 *  @brief      Runs the manager and returns an exit code.
 *
 *  Executes the primary functionality of the manager and returns a success exit code.
 *
 *  @return     std::uint8_t Exit code indicating success.
 */
auto DemoManager::RunManager() noexcept -> std::uint8_t {
    
    std::uint8_t exit_code{EXIT_SUCCESS};

    bool thread_running{true};

    /* Retrieve the native pthread handle */
    pthread_t native_handle = pthread_self();

    /* Define scheduling parameters */
    sched_param param{};

    int current_policy{-1};

    /* Use unique_lock for compatibility with condition_variable */
    std::unique_lock<std::mutex> lock(mutex_);

    /* Get current scheduling parameters to preserve existing priority */ 
    if (pthread_getschedparam(native_handle, &current_policy, &param) != 0) {

        ara::core::Abort("[demo mngr][FATAL] Failed to get current scheduling parameters: ",
                          std::strerror(errno));
    }

    std::cout << "[demo mngr][INFO] Manager Is on Running State" << std::endl;

    do {
        auto start_time = std::chrono::steady_clock::now();

            std::cout << "[demo mngr][INFO] Current Scheduling Policy: ";
            switch (current_policy) {
                case SCHED_FIFO:
                    std::cout << "SCHED_FIFO";
                    break;
                case SCHED_RR:
                    std::cout << "SCHED_RR";
                    break;
                case SCHED_OTHER:
                    std::cout << "SCHED_OTHER";
                    break;
                default:
                    std::cout << "UNKNOWN";
            }
            std::cout << ", Priority: " << param.sched_priority << std::endl;

        /* Calculate the time taken and adjust the sleep duration accordingly */
        auto elapsed_time   = std::chrono::steady_clock::now() - start_time;
        auto remaining_time = std::chrono::milliseconds(kRunningCycle) - elapsed_time;

        if (remaining_time > std::chrono::milliseconds(0)) {

            thread_running = !shutdown_notifier_.wait_for(lock, remaining_time, [this]() { 
                return turn_off_requested_.load(); 
            });

        } else {

            std::cout << "[demo mngr][WARN] Manager took more than the configured time: "
                            << kRunningCycle
                            << " ms"
                            << " and the execution,"
                            << " time taken is: "
                            << std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed_time).count())
                            << " ms.";

            thread_running = !turn_off_requested_.load();

        }

        
    } while (thread_running);

    TerminateDemoManager();
    
    return exit_code; // 0 typically indicates success
}


/** -------------------------------------------------------------------------------------------------------------------
 *  @brief      Private end of class method implementation.
 *
 *   the instance demolishing sequence.
 */
auto DemoManager::TerminateDemoManager() noexcept -> void {

    if (graceful_shutdown_handler_thread_.joinable()) {

        graceful_shutdown_handler_thread_.join();
    }

}

} // namespace manager
} // namespace demo
