from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue


def generate_launch_description():
    # LaunchConfiguration 是“延迟求值”的占位符：真正启动时才读取 Launch 参数。
    device_id = LaunchConfiguration("device_id")
    initial_battery_level = LaunchConfiguration("initial_battery_level")
    software_version = LaunchConfiguration("software_version")
    publish_period_ms = LaunchConfiguration("publish_period_ms")

    # LaunchDescription 描述一次启动要执行的动作，而不是在这里直接创建 C++ 对象。
    return LaunchDescription(
        [
            # DeclareLaunchArgument 让用户可以在 ros2 launch 命令末尾覆盖默认值。
            DeclareLaunchArgument("device_id", default_value="robot-001"),
            DeclareLaunchArgument("initial_battery_level", default_value="100.0"),
            DeclareLaunchArgument("software_version", default_value="1.0.0"),
            DeclareLaunchArgument("publish_period_ms", default_value="1000"),
            # 第一个 Node 动作启动模拟器 executable，并把 Launch 参数传给 ROS Parameter。
            Node(
                package="device_simulator",
                executable="device_simulator_node",
                name="device_simulator_node",
                output="screen",
                parameters=[
                    {
                        "device_id": device_id,
                        # Launch 参数最初都是字符串，ParameterValue 显式恢复数值类型。
                        "initial_battery_level": ParameterValue(
                            initial_battery_level, value_type=float
                        ),
                        "software_version": software_version,
                        "publish_period_ms": ParameterValue(
                            publish_period_ms, value_type=int
                        ),
                    }
                ],
            ),
            # 第二个 Node 动作启动监控进程。两个节点通过 Topic 通信，不需要中央转发节点。
            Node(
                package="device_monitor",
                executable="device_monitor_node",
                name="device_monitor_node",
                output="screen",
            ),
        ]
    )
