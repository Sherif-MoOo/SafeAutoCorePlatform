# OpenAA: Adaptive AUTOSAR C++17 Project

This is a **modular** and **scalable** open-source Adaptive AUTOSAR demo using **C++17**.
The project leverages **CMake** for build configuration, enabling straightforward
integration, testing, and future expansion.

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
- **Scalable Design**: Suitable for small-scale applications and large automotive systems.
- **Comprehensive Testing**: Includes tests to ensure reliability and correctness.
- **Cross-Platform Support**: Build and run on both Linux and QNX platforms with various architectures.

---

## Repository Structure

.
├── CMake
│ ├── CMakeConfig
│ │ ├── gcc11_linux_aarch64_debug.cmake
│ │ ├── gcc11_linux_aarch64_release.cmake
│ │ ├── gcc11_linux_x86_64_debug.cmake
│ │ ├── gcc11_linux_x86_64_release.cmake
│ │ ├── qcc12_qnx800_aarch64_debug.cmake
│ │ ├── qcc12_qnx800_aarch64_release.cmake
│ │ ├── qcc12_qnx800_x86_64_debug.cmake
│ │ └── qcc12_qnx800_x86_64_release.cmake
│ └── Toolchain
│     ├── CMakeLogging
│     │ └── tool_chain_log_config.cmake
│     ├── gcc11_linux_aarch64_debug.cmake
│     ├── gcc11_linux_aarch64_release.cmake
│     ├── gcc11_linux_x86_64_debug.cmake
│     ├── gcc11_linux_x86_64_release.cmake
│     ├── qcc12_qnx800_aarch64_debug.cmake
│     ├── qcc12_qnx800_aarch64_release.cmake
│     ├── qcc12_qnx800_x86_64_debug.cmake
│     └── qcc12_qnx800_x86_64_release.cmake
├── CMakeLists.txt
├── CMakePresets.json
├── LICENSE
├── README.md
├── build.sh
├── components
│ ├── open-aa-platform-os-abstraction-libs
│ │ ├── CMakeLists.txt
│ │ ├── include
│ │ │ └── ara
│ │ │     └── os
│ │ │         ├── interface
│ │ │         │ └── process
│ │ │         │     ├── process_factory.h
│ │ │         │     └── process_interaction.h
│ │ │         ├── linux
│ │ │         │ └── process
│ │ │         │     └── process.h
│ │ │         └── qnx
│ │ │             └── process
│ │ │                 └── process.h
│ │ └── src
│ │     ├── CMakeLists.txt
│ │     └── ara
│ │         └── os
│ │             ├── interface
│ │             │ └── process
│ │             │     ├── CMakeLists.txt
│ │             │     └── process_factory.cpp
│ │             ├── linux
│ │             │ └── process
│ │             │     ├── CMakeLists.txt
│ │             │     └── process.cpp
│ │             └── qnx
│ │                 └── process
│ │                     ├── CMakeLists.txt
│ │                     └── process.cpp
│ ├ open-aa-std-adaptive-autosar-libs
│ │ ├── CMakeLists.txt
│ │ ├── include
│ │ │ └── ara
│ │ │     └── core
│ │ │         ├── array.h
│ │ │         └── internal
│ │ │             ├── location_utils.h
│ │ │             └── violation_handler.h
│ │ └── src
│ │     └── ara
│ │         └── core
│ │             └── internal
│ │                 └── violation_handler.cpp
│ └── open-aa-example-apps
│     ├── CMakeLists.txt
│     └── demo
│         ├── CMakeLists.txt
│         └── app
│             ├── CMakeLists.txt
│             ├── include
│             │ └── demo
│             │     └── manager
│             │         └── demo_manager.h
│             └── src
│                 ├── demo
│                 │ └── manager
│                 │     └── demo_manager.cpp
│                 └── main.cpp
└── tests
    └── core_platform
        ├── CMakeLists.txt
        └── ara_core_array.cpp

---

## Components Overview

### 1. **open-aa-platform-os-abstraction-libs**
This component provides OS abstraction layers, facilitating cross-platform
support for different operating systems and architectures. It includes:

- **Interface Layer**: Abstract interfaces for process interactions
  (e.g., `process_factory.h`, `process_interaction.h`).
- **Linux Implementation**: Concrete implementations for Linux platforms
  (`process.cpp` under `linux/process`).
- **QNX Implementation**: Concrete implementations for QNX platforms
  (`process.cpp` under `qnx/process`).

### 2. **open-aa-std-adaptive-autosar-libs**
Encompasses standard Adaptive AUTOSAR libraries, including core utilities
and internal mechanisms essential for the project's functionality.

- **Core Utilities**: Implements functionalities such as the
  `ara::core::Array` class (`array.h`).
- **Internal Utilities**: Includes helpers for location handling and
  violation management (`location_utils.h`, `violation_handler.h`).

### 3. **open-aa-example-apps**
Showcases example applications demonstrating how to use the Adaptive AUTOSAR
libraries. Includes:

- **`demo/app`**: A sample application illustrating how to integrate and
  interact with the libraries via a `demo_manager`.

---

## Tests Overview

The `tests/core_platform` directory contains test applications to validate
the core platform components. These tests ensure reliability and correctness.

- **`ara_core_array.cpp`**: Test cases for the `ara::core::Array` class.

---

## Prerequisites

Before building, ensure your system meets the following requirements:

- **Operating System**: Linux (tested on Ubuntu 22.04)
- **C++ Compiler**:
  - **GCC**: Version 11.4.0 or later
  - **QNX QCC**: Version 12 (for QNX builds)
- **CMake**: Version 3.27 or later
- **Bash**: Version 4.0 or later
- **GNU Make**: For building targets

### Installing Dependencies on Ubuntu
```bash
sudo apt update
sudo apt install -y build-essential cmake gcc-11 g++-11
```

### Installing QNX SDP (for QNX Builds)

To build for QNX platforms, you need to install the QNX Software Development
Platform (SDP). Please refer to QNX's official documentation for instructions.

---

## Building the Project

The project is built via the `build.sh` script, which supports multiple
configurations, toolchains, architectures, and **exception safety modes**.

### Usage
```bash
./build.sh [OPTIONS]
```

### Options

- **`-h` / `--help`**: Show help message and exit.
- **`-c` / `--clean`**: Remove existing build and install directories for a clean build.
- **`-t` / `--build-type`**: Build type (`Debug` or `Release`). Default: `Release`.
- **`-b` / `--build-target`**: Build target:
  - `gcc11_linux_x86_64`
  - `gcc11_linux_aarch64`
  - `qcc12_qnx800_aarch64`
  - `qcc12_qnx800_x86_64`
- **`-s` / `--sdp-path`**: Path to `qnxsdp-env.sh` for QNX builds.
- **`-j` / `--jobs`**: Number of parallel jobs (defaults to number of CPU cores).
- **`-e` / `--exception-safety`**: **New**: Choose exception safety mode:
  - `conditional` (default): Defines `ARA_CORE_ARRAY_ENABLE_CONDITIONAL_EXCEPTIONS`
  - `safe`: Does **not** define that macro (i.e., “safe” mode)

---

### Example Commands

**1. Clean and Build for GCC 11 Linux x86_64 (Release)**
```bash
./build.sh --clean -b gcc11_linux_x86_64 -t Release -j 8
```

**2. Build for QNX aarch64 (Debug) with _safe_ exception mode**
```bash
./build.sh -b qcc12_qnx800_aarch64 -t Debug -s /path/to/qnxsdp-env.sh -e safe -j 4
```

**3. Build for GCC 11 Linux aarch64 (Release) with _conditional_ exceptions**
```bash
./build.sh --clean -b gcc11_linux_aarch64 -t Release -e conditional
```

**4. Clean and Build for QNX x86_64 (Release)**
```bash
./build.sh --clean -b qcc12_qnx800_x86_64 -t Release -s /path/to/qnxsdp-env.sh -j 4
```

---

## Build Targets

| Build Target              | Compiler | Platform | Architecture | Build Types    |
|---------------------------|----------|----------|--------------|----------------|
| `gcc11_linux_x86_64`     | GCC 11   | Linux    | x86_64       | Debug/Release  |
| `gcc11_linux_aarch64`    | GCC 11   | Linux    | aarch64le    | Debug/Release  |
| `qcc12_qnx800_aarch64`   | QCC 12   | QNX      | aarch64le    | Debug/Release  |
| `qcc12_qnx800_x86_64`    | QCC 12   | QNX      | x86_64       | Debug/Release  |

*Note: “Debug” or “Release” is appended internally, based on `--build-type`.*

---

## Testing the Project

After building, you can run the test executables as follows:
```bash
cd install/<build-target>/
./platform_core_test/bin/ara_core_array_test [OPTION]
```

---

## Running the Examples

The **open-aa-example-apps** component contains demo applications to illustrate
how to use the Adaptive AUTOSAR libraries:

1. **Build** the project:
   ```bash
   ./build.sh --clean -b gcc11_linux_x86_64 -t Release
   ```
2. **Navigate** to the installed directory for your target:
   ```bash
   cd install/<build-target>/adaptive_platform/opt/demo_app/bin
   ```
3. **Run** the example binary (e.g., `demo_app`, etc.):
   ```bash
   ./demo_app
   ```

Inspect the source in `components/open-aa-example-apps/demo/app/src` to
understand how the example is structured.

---

## Advanced Configuration

### Adding a New Build Target

1. **Create a New CMake Configuration File**:
   - In `CMake/CMakeConfig/`, add a `.cmake` file (e.g., copy an existing one).
2. **Define the Build Target in `build.sh`**:
   - Extend the script’s logic to map your new target to its config file.
3. **Update `CMakePresets.json`**:
   - Add a new preset referencing the new target.

### Integrating Additional Components

1. **Add a New Component Directory**:
   - Under `components/`, create a folder for your new component.
2. **Define `CMakeLists.txt`**:
   - Set up includes, sources, and dependencies.
3. **Reference in Root `CMakeLists.txt`**:
   - Use `add_subdirectory(components/your-new-component)` to integrate it.

### Getting Help

If you encounter issues not covered in this section, feel free to open an issue
on the [GitHub repository](https://github.com/Sherif-MoOo/AdaptiveAutosAR-Cpp17/issues).

---

## Troubleshooting

1. **CMake Not Found**  
   - **Error**: `cmake: command not found`
   - **Solution**: Install CMake and ensure it’s in your `PATH`.
     ```bash
     sudo apt install -y cmake
     ```

2. **QNX Environment Variables Not Set**  
   - **Error**: `Error: QNX_HOST and QNX_TARGET environment variables must be set.`
   - **Solution**: Source `qnxsdp-env.sh` or specify via `-s /path/to/qnxsdp-env.sh`.

3. **Toolchain File Not Found**  
   - **Error**: `Error: Toolchain file not found: ...`
   - **Solution**: Verify the file exists in `CMake/CMakeConfig/` and
     that your build target is correct.

4. **Compilation Errors**  
   - **Cause**: Mismatched compiler versions or missing dependencies.
   - **Solution**: Ensure correct compiler usage and install any missing deps.

---

## Contributing

Contributions are welcome! Please:

1. **Fork the Repository**: Create your personal fork.
2. **Create a Feature Branch**:
   ```bash
   git checkout -b feature/my-new-feature
   ```
3. **Commit Your Changes**:
   ```bash
   git commit -m "Add new feature XYZ"
   ```
4. **Push to Your Fork**:
   ```bash
   git push origin feature/my-new-feature
   ```
5. **Open a Pull Request**: Target the `master_integration` branch of this repo.

---

## License

This project is licensed under the **MIT License**.
See the [LICENSE](LICENSE) file for more details.

---

**Thank you for using and contributing to OpenAA: Adaptive AUTOSAR C++17 Project!**
For more information, visit the [GitHub repository](https://github.com/Sherif-MoOo/AdaptiveAutosAR-Cpp17).