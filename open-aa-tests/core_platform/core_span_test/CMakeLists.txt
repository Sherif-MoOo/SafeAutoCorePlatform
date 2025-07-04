#[=======================================================================[
Author: Sherif Mohamed

File description:
-----------------
CMake configuration for the core_platform tests.
#]=======================================================================]

#****************************************************************************************************
# Test Executable Definition
#****************************************************************************************************
if (CMAKE_SYSTEM_NAME STREQUAL "QNX"
    AND CMAKE_SYSTEM_VERSION VERSION_GREATER_EQUAL "7.1.0")

else()
    set(CMAKE_CXX_STANDARD 20)
endif()

add_executable(core_span_test
    ara_core_span.cpp
)

# If you want a compile definition, do so for the test, not the core lib:
target_compile_definitions(core_span_test
    PRIVATE
        PROCESS_IDENTIFIER="TestSpan"
)

#****************************************************************************************************
# Linking Configuration
#****************************************************************************************************
target_link_libraries(core_span_test
    PRIVATE
        ara::core::span
)

#****************************************************************************************************
# Installation Configuration
#****************************************************************************************************
install(TARGETS core_span_test
    DESTINATION platform_core_test/core_span_test/bin
)

#****************************************************************************************************
# Testing Configuration
#****************************************************************************************************
enable_testing()
add_test(NAME AraCoreSpanTest
    COMMAND ara_core_span_test
)
