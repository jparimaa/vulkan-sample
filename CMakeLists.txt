cmake_minimum_required(VERSION 3.18)
project(vulkan_sample)

set(_target "VulkanSample")
set(_src_dir "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(_exe_source_list
   ${_src_dir}/main.cpp
)

add_executable(${_target} ${_exe_source_list})

find_package(Vulkan REQUIRED)
add_subdirectory(glfw)
target_include_directories(${_target} PRIVATE ${Vulkan_INCLUDE_DIRS})
target_link_libraries(${_target} PRIVATE glfw ${Vulkan_LIBRARIES})
