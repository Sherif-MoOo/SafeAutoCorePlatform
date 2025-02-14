#[=======================================================================[
Author: Sherif Mohamed

File description:
-----------------
CMake configuration for the core_platform tests.
#]=======================================================================]

#****************************************************************************************************
# Test Executable Definition
#****************************************************************************************************
add_executable(core_array_test
    ara_core_array.cpp
)

# If you want a compile definition, do so for the test, not the core lib:
target_compile_definitions(core_array_test
    PRIVATE
        PROCESS_IDENTIFIER="TestArray"
)

#****************************************************************************************************
# Linking Configuration
#****************************************************************************************************
target_link_libraries(core_array_test
    PRIVATE
        ara::core::array
)

#****************************************************************************************************
# Installation Configuration
#****************************************************************************************************
install(TARGETS core_array_test
    DESTINATION platform_core_test/core_array_test/bin
)

#****************************************************************************************************
# Testing Configuration
#****************************************************************************************************
enable_testing()
add_test(NAME AraCoreArrayTest
    COMMAND ara_core_array_test
)
