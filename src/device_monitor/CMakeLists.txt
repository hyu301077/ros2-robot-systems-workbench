cmake_minimum_required(VERSION 3.22)
project(device_monitor)

# 两个目标发行版共同使用 C++17，防止不同编译器默认标准造成行为差异。
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(robot_device_interfaces REQUIRED)

# 告警判断做成不链接 ROS 的静态库，节点和单元测试都复用同一份业务规则。
add_library(device_alert_evaluator STATIC
  src/alert_evaluator.cpp
)
target_compile_features(device_alert_evaluator PUBLIC cxx_std_17)

# PUBLIC 表示使用这个库的目标也需要看到 include 目录中的公开头文件。
target_include_directories(device_alert_evaluator
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# 这个 executable 是真正由 ros2 run 或 Launch 启动的监控节点进程。
add_executable(device_monitor_node
  src/device_monitor_node.cpp
)
target_compile_features(device_monitor_node PUBLIC cxx_std_17)

# 节点同时链接纯告警库、rclcpp 以及自定义消息生成的类型支持目标。
target_link_libraries(device_monitor_node
  PRIVATE
    device_alert_evaluator
    rclcpp::rclcpp
    ${robot_device_interfaces_TARGETS}
)

# 安装到 ROS 2 约定位置后，ros2 run 才能通过 package 名称定位 executable。
install(
  TARGETS device_monitor_node
  DESTINATION lib/${PROJECT_NAME}
)

# V0 使用 CTest 和一个轻量级断言程序，避免为了少量纯函数测试增加额外框架依赖。
include(CTest)
if(BUILD_TESTING)
  add_executable(test_alert_evaluator
    test/test_alert_evaluator.cpp
  )
  target_compile_features(test_alert_evaluator PUBLIC cxx_std_17)
  target_link_libraries(test_alert_evaluator
    device_alert_evaluator
  )
  add_test(
    NAME alert_evaluator_unit_tests
    COMMAND test_alert_evaluator
  )
endif()

# 生成并导出 ament package 元数据。
ament_package()
