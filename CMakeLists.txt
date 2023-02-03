cmake_minimum_required(VERSION 3.18)
project(vulkan_sample)

set(_target "VulkanSample")
set(_src_dir "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(_exe_source_list
   ${_src_dir}/main.cpp
)

add_executable(${_target} ${_exe_source_list})

find_package(Vulkan REQUIRED)
find_package(X11 REQUIRED)

message(STATUS "X11_FOUND = ${X11_FOUND}")
message(STATUS "X11_INCLUDE_DIR = ${X11_INCLUDE_DIR}")
message(STATUS "X11_LIBRARIES = ${X11_LIBRARIES}")

target_include_directories(${_target} PRIVATE ${Vulkan_INCLUDE_DIRS} ${X11_INCLUDE_DIR})
target_link_libraries(${_target} PRIVATE ${Vulkan_LIBRARIES} ${X11_LIBRARIES})
