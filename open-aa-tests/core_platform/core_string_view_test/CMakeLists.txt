#[=======================================================================[
Author: Sherif Mohamed

File description:
-----------------
CMake configuration for the core_platform tests.
#]=======================================================================]

#****************************************************************************************************
# Test Executable Definition
#****************************************************************************************************
set(CMAKE_CXX_STANDARD 20)

add_executable(core_string_view_test
    ara_core_string_view.cpp
)

# If you want a compile definition, do so for the test, not the core lib:
target_compile_definitions(core_string_view_test
    PRIVATE
        PROCESS_IDENTIFIER="TestStringView"
)

#****************************************************************************************************
# Linking Configuration
#****************************************************************************************************
target_link_libraries(core_string_view_test
    PRIVATE
        ara::core::string_view
)

#****************************************************************************************************
# Installation Configuration
#****************************************************************************************************
install(TARGETS core_string_view_test
    DESTINATION platform_core_test/core_string_view_test/bin
)

#****************************************************************************************************
# Testing Configuration
#****************************************************************************************************
enable_testing()
add_test(NAME AraCoreStringViewTest
    COMMAND ara_core_string_view_test
)
