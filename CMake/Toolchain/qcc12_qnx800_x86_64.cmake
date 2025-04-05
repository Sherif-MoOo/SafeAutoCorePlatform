# CMake/Toolchain/qcc12_qnx8_x86_64.cmake

#[======================================================================
# CMake Toolchain File for QNX 8.0 x86_64
#]======================================================================]

# Ensure QNX environment variables are set
if((NOT DEFINED ENV{QNX_HOST}) OR (NOT DEFINED ENV{QNX_TARGET}))
    message(FATAL_ERROR "Environment variables QNX_HOST and QNX_TARGET must be set.
    $ source /path/to/qnxsdp-env.sh\n")
endif()

# Set system properties
set(CMAKE_SYSTEM_VERSION   8.0.0)
set(CMAKE_SYSTEM_NAME      QNX)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Specify paths for finding programs, libraries, and includes
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Specify the compilers
set(CMAKE_C_COMPILER   qcc)
set(CMAKE_CXX_COMPILER q++)

# Define compiler targets
set(CMAKE_C_COMPILER_TARGET   "12.2.0,gcc_ntox86_64")
set(CMAKE_CXX_COMPILER_TARGET "12.2.0,gcc_ntox86_64_cxx")

# Disable response files for object files if CMake version >= 3.23
if(CMAKE_VERSION GREATER_EQUAL 3.23)
    set(CMAKE_C_USE_RESPONSE_FILE_FOR_OBJECTS FALSE)
    set(CMAKE_CXX_USE_RESPONSE_FILE_FOR_OBJECTS FALSE)
endif()

# Set the system root to QNX target
set(CMAKE_SYSROOT $ENV{QNX_TARGET})
