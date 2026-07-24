cmake_minimum_required(VERSION 3.22)
project(device_simulator)

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

add_executable(device_simulator_node
  src/device_simulator_node.cpp
)
target_compile_features(device_simulator_node PUBLIC cxx_std_17)
target_link_libraries(device_simulator_node
  PRIVATE
    rclcpp::rclcpp
    ${robot_device_interfaces_TARGETS}
)

install(
  TARGETS device_simulator_node
  DESTINATION lib/${PROJECT_NAME}
)

install(
  DIRECTORY launch
  DESTINATION share/${PROJECT_NAME}
)

ament_package()
