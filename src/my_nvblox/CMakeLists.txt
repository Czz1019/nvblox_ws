cmake_minimum_required(VERSION 3.22.1)
project(my_nvblox)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(MY_NVBLOX_PACKAGE_NAME ${PROJECT_NAME})
set(NVBLOX_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../nvblox")

if(NOT DEFINED CMAKE_CUDA_ARCHITECTURES)
  set(CMAKE_CUDA_ARCHITECTURES "72;75;80;86;87;89" CACHE STRING
      "CUDA architectures to build nvblox for when GPU auto-detection is unavailable" FORCE)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(tf2 REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(tf2_ros REQUIRED)
find_package(tf2_eigen REQUIRED)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  # the following line skips the linter which checks for copyrights
  # comment the line when a copyright and license is added to all source files
  set(ament_cmake_copyright_FOUND TRUE)
  # the following line skips cpplint (only works in a git repo)
  # comment the line when this package is in a git repo and when
  # a copyright and license is added to all source files
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

if(EXISTS "${NVBLOX_SOURCE_DIR}/CMakeLists.txt")
  message(STATUS "Building vendored nvblox with CUDA architectures: ${CMAKE_CUDA_ARCHITECTURES}")
  set(BUILD_EXPERIMENTS OFF CACHE BOOL "Disable nvblox experiments" FORCE)
  set(BUILD_PYTORCH_WRAPPER OFF CACHE BOOL "Disable nvblox torch bindings" FORCE)
  set(BUILD_TESTING OFF CACHE BOOL "Disable nvblox tests" FORCE)
  set(USE_SYSTEM_EIGEN ON CACHE BOOL "Use system Eigen for vendored nvblox" FORCE)
  set(USE_SYSTEM_GFLAGS ON CACHE BOOL "Use system gflags for vendored nvblox" FORCE)
  set(USE_SYSTEM_GLOG ON CACHE BOOL "Use system glog for vendored nvblox" FORCE)
  add_subdirectory("${NVBLOX_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/nvblox")
else()
  message(FATAL_ERROR "Expected vendored nvblox source tree at ${NVBLOX_SOURCE_DIR}")
endif()

add_executable(esdf_node
  src/esdf_builder.cpp
  src/esdf_ros2_node.cpp
)

target_include_directories(esdf_node
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

ament_target_dependencies(esdf_node
  geometry_msgs
  rclcpp
  sensor_msgs
  tf2
  tf2_eigen
  tf2_ros
)

target_link_libraries(esdf_node
  PRIVATE
    nvblox_lib
    nvblox_gpu_hash
    nvblox_stdgpu
)

install(TARGETS esdf_node
  DESTINATION lib/${MY_NVBLOX_PACKAGE_NAME}
)

install(DIRECTORY include/
  DESTINATION include
)

install(DIRECTORY launch
  DESTINATION share/${MY_NVBLOX_PACKAGE_NAME}
)

ament_package()
