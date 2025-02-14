#[=======================================================================[
Author: Sherif Mohamed

File description:
-----------------
CMake configuration for the core_platform tests.
#]=======================================================================]

#****************************************************************************************************
# Test Executable Definition
#****************************************************************************************************
add_executable(core_abort_test
    ara_core_abort.cpp
    main.cpp
)

# If you want a compile definition, do so for the test, not the core lib:
target_compile_definitions(core_abort_test
    PRIVATE
        PROCESS_IDENTIFIER="TestAbort"
)

#****************************************************************************************************
# Linking Configuration
#****************************************************************************************************
target_link_libraries(core_abort_test
    PRIVATE
        ara::core::abort
)

#****************************************************************************************************
# Installation Configuration
#****************************************************************************************************
install(TARGETS core_abort_test
    DESTINATION platform_core_test/core_abort_test/bin
)

#****************************************************************************************************
# Testing Configuration
#****************************************************************************************************
enable_testing()
add_test(NAME AraCoreArrayTest
    COMMAND ara_core_abort_test
)
