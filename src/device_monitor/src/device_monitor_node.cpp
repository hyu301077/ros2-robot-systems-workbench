#include <functional>
#include <memory>

#include "device_monitor/alert_evaluator.hpp"
#include "rclcpp/rclcpp.hpp"
#include "robot_device_interfaces/msg/device_status.hpp"

namespace device_monitor
{

class DeviceMonitorNode final : public rclcpp::Node
{
public:
  DeviceMonitorNode()
  : Node("device_monitor_node")
  {
    subscription_ = create_subscription<robot_device_interfaces::msg::DeviceStatus>(
      "/device/status",
      rclcpp::QoS(10),
      std::bind(&DeviceMonitorNode::handle_status, this, std::placeholders::_1));

    RCLCPP_INFO(get_logger(), "Monitoring /device/status");
  }

private:
  void handle_status(
    const robot_device_interfaces::msg::DeviceStatus::SharedPtr message) const
  {
    RCLCPP_INFO(
      get_logger(),
      "Received /device/status: id=%s battery=%.1f%% temperature=%.1f C online=%s version=%s",
      message->device_id.c_str(),
      message->battery_level,
      message->temperature,
      message->online ? "true" : "false",
      message->software_version.c_str());

    const AlertResult alerts =
      evaluate_alerts(message->battery_level, message->temperature);

    if (alerts.low_battery) {
      RCLCPP_WARN(
        get_logger(),
        "Low battery: device=%s battery=%.1f%% (threshold: below %.1f%%)",
        message->device_id.c_str(),
        message->battery_level,
        kLowBatteryThreshold);
    }

    if (alerts.high_temperature) {
      RCLCPP_ERROR(
        get_logger(),
        "High temperature: device=%s temperature=%.1f C (threshold: above %.1f C)",
        message->device_id.c_str(),
        message->temperature,
        kHighTemperatureThreshold);
    }
  }

  rclcpp::Subscription<robot_device_interfaces::msg::DeviceStatus>::SharedPtr subscription_;
};

}  // namespace device_monitor

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<device_monitor::DeviceMonitorNode>());
  rclcpp::shutdown();
  return 0;
}
