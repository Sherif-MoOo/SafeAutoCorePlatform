# OpenAA: Adaptive AUTOSAR CXX_STANDARD 17 Project

This is a **modular** and **scalable** open‑source Adaptive AUTOSAR demo using **CXX_STANDARD 17**.
The project leverages **CMake** for build configuration, enabling straightforward integration, testing, d future expansion.

---

## Table of Contents
1. [Key Features](#key-features)
2. [Repository Structure](#repository-structure)
3. [Components Overview](#components-overview)
   - [open-aa-platform-os-abstraction-libs](#1-open-aa-platform-os-abstraction-libs)
   - [open-aa-std-adaptive-autosar-libs](#2-open-aa-std-adaptive-autosar-libs)
   - [open-aa-example-apps](#3-open-aa-example-apps)
4. [Tests Overview](#tests-overview)
5. [Prerequisites](#prerequisites)
   - [Installing Dependencies on Ubuntu](#installing-dependencies-on-ubuntu)
   - [Installing QNX SDP](#installing-qnx-sdp-for-qnx-builds)
6. [Building the Project](#building-the-project)
   - [Usage](#usage)
   - [Options](#options)
   - [Example Commands](#example-commands)
7. [Build Targets](#build-targets)
8. [Testing the Project](#testing-the-project)
9. [Running the Examples](#running-the-examples)
10. [Advanced Configuration](#advanced-configuration)
    - [Adding a New Build Target](#adding-a-new-build-target)
    - [Integrating Additional Components](#integrating-additional-components)
11. [Troubleshooting](#troubleshooting)
12. [Contributing](#contributing)
13. [License](#license)

---

## Key Features

- **Modular Architecture**: Easily add or remove components as needed.
- **Scalable Design**: Suitable for both small-scale applications and large automotive systems.
- **Comprehensive Testing**: Extensive tests ensure reliability and correctness, including oss‑translation unit sharing.
- **Cross‑Platform Support**: Build and run on both Linux and QNX with various architectures.
- **Modern CXX_STANDARD 17 Implementation**: Leverages inline variables, constexpr where possible, and robust error handling and others.

---

## Repository Structure

The repository is organized as follows:

```
.
├── CMake                                       // CMake configuration and toolchain files
├── components                                  // Source code for components:
│   ├── open-aa-std-adaptive-autosar-libs       // Core Adaptive AUTOSAR libraries (e.g., array, abort)
│   ├── open-aa-platform-os-abstraction-libs    // OS abstraction layers for Linux and QNX
│   └── open-aa-example-apps                    // Example applications demonstrating library usage
├── open-aa-tests                               // Test applications for core platform components
├── build.sh                                    // Build script
├── CMakeLists.txt                              // Top-level CMake configuration
├── LICENSE
└── README.md
```
---

## Components Overview

### 1. open-aa-platform-os-abstraction-libs
Provides OS abstraction layers to facilitate cross‑platform development.

- **Interface Layer**: Abstract interfaces for process management (e.g., `process_factory.h`, process_interaction.h`).
- **Linux Implementation**: Concrete implementations for Linux (`process.cpp` under the Linux folder).
- **QNX Implementation**: Concrete implementations for QNX (`process.cpp` under the QNX folder).

### 2. open-aa-std-adaptive-autosar-libs
Contains core utilities and internal mechanisms essential for Adaptive AUTOSAR:

- **ara::core::Array**: A fixed‑size array container with enhanced functionality.
- **ara::core::Abort**: API for explicitly aborting operations when violations occur.
- **Internal Utilities**: Helpers for location handling and violation management (e.g., `location_utils., `violation_handler.h`).

### 3. open-aa-example-apps
Demonstrates how to use the Adaptive AUTOSAR libraries via sample applications.

- **demo/app**: A sample application illustrating integration through a `demo_manager`.

---

## Tests Overview

The `open-aa-tests` directory contains test applications for the core platform:
- **core_platform tests**: Validate components such as `ara::core::Array` and `ara::core::Abort`.
- The tests include both positive scenarios and negative (compile‑time or runtime) tests (the latter are commented out).

---

## Prerequisites

**Operating System**: Linux (tested on Ubuntu 22.04) or QNX.

**C++ Compiler**:
 - **GCC**: Version 11.x.x or later smaller versions (for Linux builds)
 - **QNX QCC**: Version 12 (for QNX builds)

**CMake**: Version 3.27 or later  
**Bash** and **GNU Make**: For building targets.

### Installing Dependencies on Ubuntu
```bash
sudo apt-get update
sudo apt-get install -y software-properties-common lsb-release
sudo apt-get remove --purge cmake
sudo apt-get install -y apt-transport-https ca-certificates gnupg software-properties-common wget
wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc | sudo apt-key add -
sudo apt-add-repository 'deb https://apt.kitware.com/ubuntu/ focal main'
sudo apt-get update
sudo apt-get install cmake
sudo apt-get install -y build-essential cmake gcc-11 g++-11
```

### Installing QNX SDP (for QNX Builds)
Follow QNX's official documentation to install the QNX Software Development Platform.

---

## Building the Project

The project is built using CMake and the provided `build.sh` script.

### Usage
```bash
./build.sh [OPTIONS]
```

### Options
- **`-h, --help`**: Show help message and exit.
- **`-c, --clean`**: Remove existing build and install directories.
- **`-t, --build-type`**: Build type (`Debug` or `Release`). Default: `Release`.
- **`-b, --build-target`**: Build target (e.g., `gcc11_linux_x86_64`, `gcc11_linux_aarch64`, cc12_qnx800_aarch64`, `qcc12_qnx800_x86_64`).
- **`-s, --sdp-path`**: Path to `qnxsdp-env.sh` for QNX builds.
- **`-j, --jobs`**: Number of parallel jobs.
- **`-e, --exception-safety`**: Choose exception safety mode:
    - `conditional` (default): Defines `ARA_CORE_ARRAY_ENABLE_CONDITIONAL_EXCEPTIONS`
    - `safe`: Does not define the macro (safe mode)

### Example Commands

**1. Clean and Build for GCC 11 Linux x86_64 (Release):**
```bash
./build.sh --clean -b gcc11_linux_x86_64 -t Release -j 8
```

**2. Build for QNX aarch64 (Debug) with safe exception mode:**
```bash
./build.sh -b qcc12_qnx800_aarch64 -t Debug -s /path/to/qnxsdp-env.sh -e safe -j 4
```

**3. Build for GCC 11 Linux aarch64 (Release) with conditional exceptions:**
```bash
./build.sh --clean -b gcc11_linux_aarch64 -t Release -e conditional
```

---

## Build Targets

| Build Target              | Compiler | Platform | Architecture | Build Types    |
|---------------------------|----------|----------|--------------|----------------|
| `gcc11_linux_x86_64`      | GCC 11   | Linux    | x86_64       | Debug/Release  |
| `gcc11_linux_aarch64`     | GCC 11   | Linux    | aarch64le    | Debug/Release  |
| `qcc12_qnx800_aarch64`    | QCC 12   | QNX      | aarch64le    | Debug/Release  |
| `qcc12_qnx800_x86_64`     | QCC 12   | QNX      | x86_64       | Debug/Release  |

*Note: “Debug” or “Release” is appended internally based on the build type.*

---

## Testing the Project

After building, test executables are located in:
```bash
cd install/<build-target>/
./platform_core_test/core_array_test/bin/core_array_test [OPTION]
./platform_core_test/core_abort_test/bin/core_abort_test [OPTION]
```
The repository includes comprehensive tests for core platform components. In particular, the 
`ara::core::Array` test suite (located in `tests/core_platform/ara_core_array_test`) covers a wide range 
of scenarios to ensure reliability and correctness. The test executable accepts a test number as a 
command-line argument to run specific tests:

- **1**: Element Access and Iterators  
    Tests both checked access via `at()` and unchecked access using `operator[]`, as well as forward iteration.
- **2**: get<I>() Functionality  
    Validates the compile-time and runtime behavior of the helper function `get<I>()`.
- **3**: Swap and Fill  
    Verifies that arrays can be swapped correctly and that the `fill()` method assigns the specified 
    value to all elements. Includes constexpr tests.
- **4**: Comparison Operators  
    Checks equality (`==`), inequality (`!=`), and lexicographical comparisons (`<`, `<=`, `>`, `>=`).
- **5**: Usage with User-Defined Class  
    Demonstrates array usage with a custom class (`SafeTestClass`) that implements copy/move semantics.
- **6**: Usage with User-Defined Struct  
    Tests array behavior using a custom struct (`SafeTestStruct`).
- **7**: Copy and Move Semantics  
    Validates correct behavior of copy and move constructors and assignments.
- **8**: Const Correctness  
    Ensures that const arrays support proper element access and iteration, with compile-time checks.
- **9**: Violation Handling (Out-of-Range)  
    Confirms that accessing an out-of-range index via `at()` triggers a violation and terminates the process.
- **10**: Zero-Sized Arrays  
    Checks that zero-sized arrays behave correctly (e.g., `size()`, `empty()`, and `data()` return expected results).
- **11**: Reverse Iterators  
    Tests reverse iteration capabilities using `rbegin()`, `rend()`, `crbegin()`, and `crend()`.
- **12**: Partial Initialization  
    Verifies that arrays can be partially initialized, with unspecified elements defaulting as expected.
- **13**: Negative Scenarios  
    Contains (commented-out) tests demonstrating expected compile-time or run-time errors when misusing the API.
- **14**: Two-Dimensional Arrays  
    Demonstrates usage of nested arrays and validates operations on multi-dimensional arrays.

To run the array tests, navigate to the corresponding test executable (e.g., `ara_core_array_test`)
in the install directory and pass the desired test number as a command-line argument.

The test executable `ara_core_abort_test` accepts a test number as a command-line argument:
- **1**: Inline tests (e.g., AddAbortHandler, SetAbortHandler)
- **2**: Cross‑Translation Unit Sharing test (set handler from another file)
- **3**: Termination test (calls Abort() and terminates the process)

---

## Running the Examples

The **open-aa-example-apps** component contains demo applications to illustrate library usage.

1. **Build** the project:
   ```bash
   ./build.sh --clean -b gcc11_linux_x86_64 -t Release
   ```
2. **Navigate** to the installed directory for your target:
   ```bash
   cd install/<build-target>/adaptive_platform/opt/demo_app/bin
   ```
3. **Run** the demo application:
   ```bash
   ./demo_app
   ```

---

## Advanced Configuration

### Adding a New Build Target
1. Create a new CMake configuration file in `CMake/CMakeConfig/` (for example, by copying an existing one).
2. Define the new target in the build script (`build.sh`) and the ToolChain Folder.
3. Update `CMakePresets.json` with a new preset for the target.

### Integrating Additional Components
1. Add a new component directory under `components/`.
2. Create a CMakeLists.txt file for the component with the required includes, sources, and dependencies.
3. Integrate the component in the root CMakeLists.txt using `add_subdirectory()`.

---

## Troubleshooting

1. **CMake Not Found**  
   - **Error**: `cmake: command not found`
   - **Solution**: Install CMake and ensure it’s in your PATH:
     ```bash
     cmake --version
     ```

2. **QNX Environment Variables Not Set**  
   - **Error**: `Error: QNX_HOST and QNX_TARGET environment variables must be set.`
   - **Solution**: Source `qnxsdp-env.sh` or specify the path via the `-s` option.

3. **Toolchain File Not Found**  
   - **Error**: `Error: Toolchain file not found: ...`
   - **Solution**: Verify the file exists in `CMake/CMakeConfig/` and that your build target is correct.

4. **Compilation Errors**  
   - **Cause**: Mismatched compiler versions or missing dependencies.
   - **Solution**: Use the correct compiler and install any missing dependencies.

---

## Contributing

Contributions are welcome! Follow these steps:
1. **Fork the Repository**.
2. **Create a Feature Branch**:
   ```bash
   git checkout -b feature/my-new-feature
   ```
3. **Commit Your Changes** with clear messages.
4. **Push to Your Fork**:
   ```bash
   git push origin feature/my-new-feature
   ```
5. **Open a Pull Request** targeting the `master_integration` branch.

---

## License

This project is licensed under the **MIT License**. See the [LICENSE](LICENSE) file for details.

---

Thank you for using and contributing to OpenAA: Adaptive AUTOSAR CXX_STANDARD 17 Project!
For more information, visit the [GitHub repository](https://github.com/Sherif-MoOo/AdaptiveAutosAR-Cpp17).