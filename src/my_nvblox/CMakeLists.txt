cmake_minimum_required(VERSION 3.22)
project(my_nvblox)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(visualization_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(std_msgs REQUIRED)
find_package(tf2 REQUIRED)
find_package(tf2_ros REQUIRED)
find_package(tf2_eigen REQUIRED)
find_package(message_filters REQUIRED)
find_package(cv_bridge REQUIRED)
find_package(image_transport REQUIRED)

find_package(gflags REQUIRED)
find_package(glog REQUIRED)
find_package(nvblox REQUIRED)

find_package(OpenCV REQUIRED)
find_package(CUDAToolkit REQUIRED)
find_package(Eigen3 REQUIRED)

include_directories(
  include
  ${OpenCV_INCLUDE_DIRS}
  ${EIGEN3_INCLUDE_DIRS}
)

add_executable(nvblox_node
  src/nvblox_node.cpp
  src/mesh_publisher.cpp
  src/esdf_publisher.cpp
)

ament_target_dependencies(nvblox_node
  rclcpp
  sensor_msgs
  visualization_msgs
  geometry_msgs
  std_msgs
  tf2
  tf2_ros
  tf2_eigen
  message_filters
  cv_bridge
  image_transport
)

target_link_libraries(nvblox_node
  nvblox_lib
  glog::glog
  gflags_nothreads_static
  CUDA::cudart
  ${OpenCV_LIBRARIES}
)

target_include_directories(nvblox_node PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

install(TARGETS nvblox_node
  DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY include/
  DESTINATION include
)

install(DIRECTORY launch/
  DESTINATION share/${PROJECT_NAME}
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  set(ament_cmake_cpplint_FOUND TRUE)
  set(ament_cmake_copyright_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()