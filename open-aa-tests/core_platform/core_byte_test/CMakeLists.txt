#[=======================================================================[
Author: Sherif Mohamed

File description:
-----------------
CMake configuration for the core_platform tests.
#]=======================================================================]

#****************************************************************************************************
# Test Executable Definition
#****************************************************************************************************
add_executable(core_byte_test
    ara_core_byte.cpp
)

# If you want a compile definition, do so for the test, not the core lib:
target_compile_definitions(core_byte_test
    PRIVATE
        PROCESS_IDENTIFIER="TestByte"
)

#****************************************************************************************************
# Linking Configuration
#****************************************************************************************************
target_link_libraries(core_byte_test
    PRIVATE
        ara::core::byte
)

#****************************************************************************************************
# Installation Configuration
#****************************************************************************************************
install(TARGETS core_byte_test
    DESTINATION platform_core_test/core_byte_test/bin
)

#****************************************************************************************************
# Testing Configuration
#****************************************************************************************************
enable_testing()
add_test(NAME AraCoreByteTest
    COMMAND ara_core_byte_test
)
