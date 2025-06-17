# OpenAA: Adaptive AUTOSAR CXX_STANDARD 17 Project

This is a **modular** and **scalable** open‑source Adaptive AUTOSAR demo using **CXX_STANDARD 17**.
The project leverages **CMake** for build configuration, enabling straightforward integration, testing, and future expansion.
 
 ---
 
 ## Table of Contents
 1. [Key Features](#key-features)
 2. [Repository Structure](#repository-structure)
 3. [Components Overview](#components-overview)
    - [open-aa-platform-os-abstraction-libs](#1-open-aa-platform-os-abstraction-libs)
    - [open-aa-std-adaptive-autosar-libs](#2-open-aa-std-adaptive-autosar-libs)
    - [open-aa-example-apps](#3-open-aa-example-apps)
 4. [Tests Overview](#tests-overview)
 5. [Prerequisites and Environment Setup](#prerequisites-and-environment-setup)
    - [Ubuntu (GCC 11 & GCC 13)](#ubuntu-gcc-11--gcc-13)
    - [Cross Compilation for AArch64 (GCC 11/13)](#cross-compilation-for-aarch64)
    - [QNX Environment](#qnx-environment)
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
- **Comprehensive Testing**: Extensive tests ensure reliability and correctness.
- **Cross‑Platform Support**: Build and run on Linux (Ubuntu 22.04, Ubuntu 24.04) and QNX with various architectures.
- **Modern CXX_STANDARD 17 Implementation**: Leverages inline variables, constexpr where possible, robust error handling, and other modern features.
- **Multiple Compiler Support**: Use GCC 11, GCC 13 (including cross-compilation for AArch64), and QNX QCC.

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

- **Interface Layer**: Abstract interfaces for process management (e.g., `process_factory.h`, `process_interaction.h`).
- **Linux Implementation**: Concrete implementations for Linux (`process.cpp` under the Linux folder).
- **QNX Implementation**: Concrete implementations for QNX (`process.cpp` under the QNX folder).

### 2. open-aa-std-adaptive-autosar-libs
Contains core utilities and internal mechanisms essential for Adaptive AUTOSAR:

- **ara::core::Array**: A fixed‑size array container with enhanced functionality.
- **ara::core::Abort**: API for explicitly aborting operations when violations occur.
- **Internal Utilities**: Helpers for location handling and violation management (e.g., `location_utils.h`, `violation_handler.h`).

### 3. open-aa-example-apps
Demonstrates how to use the Adaptive AUTOSAR libraries via sample applications.

- **demo/app**: A sample application illustrating integration through a `demo_manager`.

---

## Tests Overview

The `open-aa-tests` directory contains test applications for the core platform:
- **core_platform tests**: Validate components such as `ara::core::Array` and `ara::core::Abort`.
- Tests include both positive scenarios and negative (compile‑time or runtime) tests (the latter are commented out).

---

## Prerequisites and Environment Setup

### Ubuntu (GCC 11 || GCC 13 || Clang 21)

**Operating System**: Ubuntu 22.04 or Ubuntu 24.04

**Required Packages:**

```bash
$ sudo apt-get update
$ sudo apt-get install -y software-properties-common lsb-release
$ sudo apt-get remove --purge cmake
$ sudo apt-get install -y apt-transport-https ca-certificates gnupg wget
$ wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc | sudo apt-key add -
$ sudo apt-add-repository 'deb https://apt.kitware.com/ubuntu/ focal main'
$ sudo apt-get update
$ sudo apt-get install -y cmake build-essential

# For GCC 11 and GCC 13 (Ubuntu 24.04 supports both)
$ sudo apt-get install -y gcc-11 g++-11 gcc-13 g++-13

# For LLVM support if wanted In Ubuntu
$ echo "deb [arch=amd64] http://apt.llvm.org/noble llvm-toolchain-noble main" \
  | sudo tee /etc/apt/sources.list.d/llvm-noble.list

$ sudo apt update
$ sudo apt install clang-21 lld-21 lldb-21
$ sudo update-alternatives --install /usr/bin/clang   clang   /usr/bin/clang-21   100
$ sudo update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-21 100
$ sudo update-alternatives --config clang
$ sudo update-alternatives --config clang++
```

### Cross Compilation for AArch64

**Operating System**: Ubuntu (22.04 or 24.04)

**Required Packages:**

```bash
$ sudo apt-get update
$ sudo apt-get install -y gcc-11-aarch64-linux-gnu g++-11-aarch64-linux-gnu gcc-13-aarch64-linux-gnu g++-13-aarch64-linux-gnu
```

**Note:** Ensure that the toolchain file (e.g., `CMake/Toolchain/*.cmake`) exists for your desired env
### QNX Environment

**Operating System:** QNX or Linux for QNX cross-compilation

**Requirements:**

- QNX Software Development Platform (SDP) version 7.1 or later (refer to [QNX documentation](http://www.qnx.com/download/) for installation instructions).
- QNX QCC (typically version 12) must be installed and properly configured.

**Setup:**

1. Install QNX SDP as per QNX documentation.
2. Source the QNX environment script before building or use the build script in the repo with --sdp-path /path/to/qnxsdp-env.sh

```bash
$ source /path/to/qnxsdp-env.sh
```

3. In the build command, specify the QNX toolchain target (e.g., `qcc12_qnx800_aarch64` or `qcc12_qnx800_x86_64`).

---

## Building the Project

The project is built using CMake and the provided `build.sh` script.

### Usage

```bash
$ ./build.sh [OPTIONS]
```

### Options
- **`-h, --help`**: Show help message and exit.
- **`-c, --clean`**: Remove existing build and install directories.
- **`-t, --build-type`**: Build type (`Debug` or `Release`). Default: `Release`.
- **`-b, --build-target`**: Build target (e.g., `gcc11_linux_x86_64`, `gcc11_linux_aarch64`, `gcc13_linux_x86_64`, `gcc13_linux_aarch64`, `qcc12_qnx800_aarch64`, `qcc12_qnx800_x86_64`).
- **`-s, --sdp-path`**: Path to `qnxsdp-env.sh` for QNX builds.
- **`-j, --jobs`**: Number of parallel jobs.
- **`-e, --exception-safety`**: Choose exception safety mode:
    - `conditional` (default): Defines `ENABLE_PLATFORM_CONDITIONAL_EXCEPTION`
    - `safe`: Does not define the macro (safe mode)

### Example Commands

**1. Clean and Build for GCC 11 Linux x86_64 (Release):**
```bash
$ ./build.sh --clean -b gcc11_linux_x86_64 -t Release -j 8
```

**2. Build for QNX aarch64 (Debug) with safe exception mode:**
```bash
$ ./build.sh -b qcc12_qnx800_aarch64 -t Debug -s /path/to/qnxsdp-env.sh -e safe -j 4
```

**3. Build for GCC 11 Linux aarch64 (Release) with conditional exceptions:**
```bash
$ ./build.sh --clean -b gcc11_linux_aarch64 -t Release -e conditional
```

**4. Build for GCC 13 Linux x86_64 (Release) on Ubuntu 24.04:**
```bash
$ ./build.sh --clean -b gcc13_linux_x86_64 -t Release -j 8
```

**5. Build for GCC 13 Linux aarch64 (Release) on Ubuntu 24.04:**
```bash
$ ./build.sh --clean -b gcc13_linux_aarch64 -t Release -j 1
```

---

## Build Targets

| Build Target              | Compiler  | Platform | Architecture | Build Types    |
|---------------------------|-----------|----------|--------------|----------------|
| `gcc11_linux_x86_64`      | GCC   11  | Linux    | x86_64       | Debug/Release  |
| `gcc11_linux_aarch64`     | GCC   11  | Linux    | aarch64      | Debug/Release  |
| `gcc13_linux_x86_64`      | GCC   13  | Linux    | x86_64       | Debug/Release  |
| `gcc13_linux_aarch64`     | GCC   13  | Linux    | aarch64      | Debug/Release  |
| `clang21_linux_x86_64`    | CLANG 21  | Linux    | x86_64       | Debug/Release  |
| `clang21_linux_aarch64`   | CLANG 21  | Linux    | aarch64      | Debug/Release  |
| `qcc12_qnx800_aarch64`    | QCC   12  | QNX 8.0  | aarch64le    | Debug/Release  |
| `qcc12_qnx800_x86_64`     | QCC   12  | QNX 8.0  | x86_64       | Debug/Release  |

*Note: “Debug” or “Release” is appended internally based on the build type.*

---

## Testing the Project

After building, test executables are located in:
```bash
$ cd install/<build-target>/
$ ./platform_core_test/core_array_test/bin/core_array_test [OPTION]
$ ./platform_core_test/core_abort_test/bin/core_abort_test [OPTION]
```
The repository includes comprehensive tests for core platform components. In particular, the 
`ara::core::Array` test suite (located in `tests/core_platform/ara_core_array_test`) covers a wide range 
of scenarios to ensure reliability and correctness. The test executable accepts a test number as a 
command-line argument to run specific tests.

---

## Running the Examples

The **open-aa-example-apps** component contains demo applications to illustrate library usage.

1. **Build** the project:
   ```bash
   $ ./build.sh --clean -b gcc11_linux_x86_64 -t Release
   ```
2. **Navigate** to the installed directory for your target:
   ```bash
   $ cd install/<build-target>/adaptive_platform/opt/demo_app/bin
   ```
3. **Run** the demo application:
   ```bash
   $ ./demo_app
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
     $ cmake --version
     ```

2. **Compiler Not Found (GCC / Cross-Compiler)**  
   - **Error**: `... is not a full path and was not found in the PATH.`
   - **Solution**: Verify the compiler installation and update the PATH or specify full paths in the toolchain file.

3. **QNX Environment Variables Not Set**  
   - **Error**: `Error: QNX_HOST and QNX_TARGET environment variables must be set.`
   - **Solution**: Source `qnxsdp-env.sh` or specify the path via the `-s` option.

4. **Toolchain File Not Found**  
   - **Error**: `Error: Toolchain file not found: ...`
   - **Solution**: Verify the file exists in `CMake/CMakeConfig/` and that your build target is correct.

5. **Compilation Errors**  
   - **Cause**: Mismatched compiler versions or missing dependencies.
   - **Solution**: Use the correct compiler and install any missing dependencies.

---

## Contributing

Contributions are welcome! Follow these steps:
1. **Fork the Repository**.
2. **Create a Feature Branch**:
   ```bash
   $ git checkout -b feature/my-new-feature
   ```
3. **Commit Your Changes** with clear messages.
4. **Push to Your Fork**:
   ```bash
   $ git push origin feature/my-new-feature
   ```
5. **Open a Pull Request** targeting the `master_integration` branch.

---

## License

This project is licensed under the **MIT License**. See the [LICENSE](LICENSE) file for details.

---

Thank you for using and contributing to OpenAA: Adaptive AUTOSAR CXX_STANDARD 17 Project!
For more information, visit the [GitHub repository](https://github.com/Sherif-MoOo/AdaptiveAutosAR-Cpp17).