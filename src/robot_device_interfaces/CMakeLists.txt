cmake_minimum_required(VERSION 3.22)
project(robot_device_interfaces)

# ament_cmake 提供 ROS 2 package 的 CMake 组织方式；
# rosidl_default_generators 会把 .msg 文件生成可供 C++ 等语言使用的类型代码。
find_package(ament_cmake REQUIRED)
find_package(rosidl_default_generators REQUIRED)

# 自定义消息放在独立 package 中，相当于车载多应用系统中的“公共接口定义库”。
# 发布者和订阅者只依赖通信契约，不需要依赖对方的业务实现。
rosidl_generate_interfaces(${PROJECT_NAME}
  "msg/DeviceStatus.msg"
)

# 下游 package 在运行时需要 rosidl 生成的类型支持库，因此将依赖导出给它们。
ament_export_dependencies(rosidl_default_runtime)

# 读取前面的声明并生成 ament package 元数据；通常放在 CMakeLists.txt 最后。
ament_package()
