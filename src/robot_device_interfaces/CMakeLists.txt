cmake_minimum_required(VERSION 3.22)
project(robot_device_interfaces)

find_package(ament_cmake REQUIRED)
find_package(rosidl_default_generators REQUIRED)

# Keep messages in a package without executable code. Producers and consumers
# can then depend on the communication contract without pulling in either node.
rosidl_generate_interfaces(${PROJECT_NAME}
  "msg/DeviceStatus.msg"
)

ament_export_dependencies(rosidl_default_runtime)

ament_package()
