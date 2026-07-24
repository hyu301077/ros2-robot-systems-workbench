cmake_minimum_required(VERSION 3.22)
project(device_monitor)

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

add_library(device_alert_evaluator STATIC
  src/alert_evaluator.cpp
)
target_compile_features(device_alert_evaluator PUBLIC cxx_std_17)
target_include_directories(device_alert_evaluator
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(device_monitor_node
  src/device_monitor_node.cpp
)
target_compile_features(device_monitor_node PUBLIC cxx_std_17)
target_link_libraries(device_monitor_node
  PRIVATE
    device_alert_evaluator
    rclcpp::rclcpp
    ${robot_device_interfaces_TARGETS}
)

install(
  TARGETS device_monitor_node
  DESTINATION lib/${PROJECT_NAME}
)

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

ament_package()
