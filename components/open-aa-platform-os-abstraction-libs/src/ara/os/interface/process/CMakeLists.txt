#[======================================================================
# OpenAA: Open Source Adaptive AUTOSAR Project
# Author: Sherif Mohamed
#
# File description:
# -----------------
# CMake configuration for the ara::os::process interface.
# Defines the interface and adds source files.
#]=======================================================================]

#****************************************************************************************************
# Library Definition
#****************************************************************************************************

# Define the ara_os_process_interface library as an OBJECT library.
add_library(ara_os_process_interface OBJECT
    process_interaction.cpp
)

#****************************************************************************************************
# Include Directories
#****************************************************************************************************

# Specify the include directories as PRIVATE to prevent exposure in export sets.
target_include_directories(ara_os_process_interface
    PRIVATE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/components/open-aa-platform-os-abstraction-libs/include>
        $<INSTALL_INTERFACE:include>
)

#****************************************************************************************************
# Compiler Definitions
#****************************************************************************************************

# Define any necessary compile definitions for the interface.
# Example: Define a macro if needed.
# target_compile_definitions(ara_os_process_interface PRIVATE SOME_MACRO=1)

#****************************************************************************************************
# Compiler Settings
#****************************************************************************************************

# Set properties specific to the interface library if needed.
# Example: Position-independent code for shared libraries.
# set_target_properties(ara_os_process_interface PROPERTIES POSITION_INDEPENDENT_CODE ON)

#****************************************************************************************************
# Link Dependencies
#****************************************************************************************************

# Link against other libraries if required.
# Example:
# target_link_libraries(ara_os_process_interface PRIVATE some_other_library)
