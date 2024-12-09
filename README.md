# vulkan-sample

A bare minimum vulkan application that can be compiled and executed on Linux.

## Build and execute

Clone the repository with submodules

```git clone --recurse-submodules https://github.com/jparimaa/vulkan-sample```

Install the required libraries

```sudo apt install libvulkan-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev```

Run CMake

```cmake . -Bbuild && cd build && make```

Execute

```./VulkanSample```
