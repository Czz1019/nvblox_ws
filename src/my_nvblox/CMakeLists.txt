# # cmake_minimum_required(VERSION 3.8)
# # project(my_nvblox)

# # if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
# #   add_compile_options(-Wall -Wextra -Wpedantic)
# # endif()

# # # find dependencies
# # find_package(ament_cmake REQUIRED)
# # find_package(rclcpp REQUIRED)
# # find_package(sensor_msgs REQUIRED)
# # find_package(geometry_msgs REQUIRED)
# # find_package(tf2 REQUIRED)
# # find_package(tf2_ros REQUIRED)
# # find_package(tf2_geometry_msgs REQUIRED)
# # find_package(cv_bridge REQUIRED)
# # find_package(message_filters REQUIRED)
# # find_package(std_msgs REQUIRED)

# # if(BUILD_TESTING)
# #   find_package(ament_lint_auto REQUIRED)
# #   # the following line skips the linter which checks for copyrights
# #   # comment the line when a copyright and license is added to all source files
# #   set(ament_cmake_copyright_FOUND TRUE)
# #   # the following line skips cpplint (only works in a git repo)
# #   # comment the line when this package is in a git repo and when
# #   # a copyright and license is added to all source files
# #   set(ament_cmake_cpplint_FOUND TRUE)
# #   ament_lint_auto_find_test_dependencies()
# # endif()

# # ament_package()
# cmake_minimum_required(VERSION 3.8)
# project(my_nvblox LANGUAGES CXX C CUDA) # 启用 CUDA 语言支持

# if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
#   add_compile_options(-Wall -Wextra -Wpedantic)
# endif()

# # 1. 寻找 ROS 2 相关的依赖
# find_package(ament_cmake REQUIRED)
# find_package(rclcpp REQUIRED)
# find_package(sensor_msgs REQUIRED)
# find_package(geometry_msgs REQUIRED)
# find_package(tf2 REQUIRED)
# find_package(tf2_ros REQUIRED)
# find_package(tf2_geometry_msgs REQUIRED)
# find_package(cv_bridge REQUIRED)
# find_package(message_filters REQUIRED)
# find_package(visualization_msgs REQUIRED)

# # --- 新增：寻找 nvblox 的底层依赖库 ---
# find_package(CUDAToolkit REQUIRED)
# find_package(glog REQUIRED)
# find_package(gflags REQUIRED)

# # 2. 寻找系统中的 nvblox 核心库
# find_package(nvblox REQUIRED)

# # 3. 编译我们的 C++ 节点
# add_executable(mapper_node src/mapper_node.cpp)
# add_executable(nvblox_node 
#   src/nvblox_node.cpp
#   src/mesh_publisher.cpp
#   src/esdf_publisher.cpp
# )

# target_include_directories(mapper_node PUBLIC
#   $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
#   $<INSTALL_INTERFACE:include>
#   ${CUDAToolkit_INCLUDE_DIRS}
# )

# ament_target_dependencies(mapper_node
#   rclcpp sensor_msgs geometry_msgs tf2 tf2_ros tf2_geometry_msgs cv_bridge message_filters visualization_msgs
# )

# # 4. 核心：将 nvblox 的库文件链接到我们的可执行文件中
# # target_link_libraries(mapper_node nvblox_lib CUDA::cudart)
# # 链接 nvblox_lib、CUDA 运行时以及底层日志库
# target_link_libraries(mapper_node nvblox_lib CUDA::cudart glog::glog gflags)

# # 5. 安装规则
# install(TARGETS mapper_node
#   DESTINATION lib/${PROJECT_NAME})

# install(DIRECTORY launch
#   DESTINATION share/${PROJECT_NAME})

# install(PROGRAMS
#   scripts/esdf_node.py
#   DESTINATION lib/${PROJECT_NAME}
# )

# install(DIRECTORY
#   launch
#   DESTINATION share/${PROJECT_NAME}
# )

# ament_package()

cmake_minimum_required(VERSION 3.8)
project(my_nvblox)

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(cv_bridge REQUIRED)
find_package(message_filters REQUIRED)
find_package(tf2_ros REQUIRED)
find_package(OpenCV REQUIRED)
find_package(nvblox REQUIRED) # 链接您源码编译的底层库

add_executable(nvblox_node src/nvblox_node.cpp)

target_include_directories(nvblox_node PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)

# 链接核心库与 CUDA 相关库
target_link_libraries(nvblox_node
  nvblox::nvblox_lib
)

ament_target_dependencies(nvblox_node
  rclcpp sensor_msgs cv_bridge message_filters tf2_ros
)

install(TARGETS nvblox_node DESTINATION lib/${PROJECT_NAME})
ament_package()