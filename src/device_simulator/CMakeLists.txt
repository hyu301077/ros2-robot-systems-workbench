cmake_minimum_required(VERSION 3.22)
project(device_simulator)

# Humble 与 Lyrical 共用 C++17，显式固定标准可以避免两台终端使用不同默认值。
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find_package 读取依赖 package 导出的 CMake 配置；名称必须与 package.xml 一致。
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(robot_device_interfaces REQUIRED)

# add_executable 决定最终生成的可执行文件名，也是 ros2 run 使用的程序名。
add_executable(device_simulator_node
  src/device_simulator_node.cpp
)
target_compile_features(device_simulator_node PUBLIC cxx_std_17)

# 使用导入目标链接 ROS 2 和自定义消息的头文件、类型支持及传递依赖。
# 这种现代 CMake 写法已经在 Humble 与 Lyrical CI 中实际构建通过。
target_link_libraries(device_simulator_node
  PRIVATE
    rclcpp::rclcpp
    ${robot_device_interfaces_TARGETS}
)

# ROS 2 约定节点可执行文件安装到 lib/<package>，ros2 run 才能按 package 找到它。
install(
  TARGETS device_simulator_node
  DESTINATION lib/${PROJECT_NAME}
)

# Launch 文件也必须进入 install 空间，ros2 launch 才能在安装后的 package 中找到它。
install(
  DIRECTORY launch
  DESTINATION share/${PROJECT_NAME}
)

# 汇总并导出当前 package 的 ament 元数据，通常是文件中的最后一条命令。
ament_package()
