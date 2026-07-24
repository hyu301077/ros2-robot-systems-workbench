from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue


def generate_launch_description():
    device_id = LaunchConfiguration("device_id")
    initial_battery_level = LaunchConfiguration("initial_battery_level")
    software_version = LaunchConfiguration("software_version")
    publish_period_ms = LaunchConfiguration("publish_period_ms")

    return LaunchDescription(
        [
            DeclareLaunchArgument("device_id", default_value="robot-001"),
            DeclareLaunchArgument("initial_battery_level", default_value="100.0"),
            DeclareLaunchArgument("software_version", default_value="1.0.0"),
            DeclareLaunchArgument("publish_period_ms", default_value="1000"),
            Node(
                package="device_simulator",
                executable="device_simulator_node",
                name="device_simulator_node",
                output="screen",
                parameters=[
                    {
                        "device_id": device_id,
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
            Node(
                package="device_monitor",
                executable="device_monitor_node",
                name="device_monitor_node",
                output="screen",
            ),
        ]
    )
