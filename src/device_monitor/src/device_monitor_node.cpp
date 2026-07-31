#include <functional>
#include <memory>

#include "device_monitor/alert_evaluator.hpp"
#include "rclcpp/rclcpp.hpp"
#include "robot_device_interfaces/msg/device_status.hpp"

namespace device_monitor
{

// 监控节点只负责 ROS 通信和日志映射，具体阈值判断放在独立的普通 C++ 函数中。
class DeviceMonitorNode final : public rclcpp::Node
{
public:
  DeviceMonitorNode()
  : Node("device_monitor_node")
  {
    // 消息类型、Topic 名称和 QoS 必须与发布者兼容，DDS 才能完成双方发现和数据传输。
    // 回调中的 _1 代表订阅到的第一项参数，也就是 DeviceStatus 消息。
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
    // Executor 收到一条 Topic 数据后调用本函数；SharedPtr 管理消息对象的生命周期。
    RCLCPP_INFO(
      get_logger(),
      "Received /device/status: id=%s battery=%.1f%% temperature=%.1f C online=%s version=%s",
      message->device_id.c_str(),
      message->battery_level,
      message->temperature,
      message->online ? "true" : "false",
      message->software_version.c_str());

    // ROS 回调只做适配：从消息中取出普通数值，再交给可独立测试的告警函数。
    const AlertResult alerts =
      evaluate_alerts(message->battery_level, message->temperature);

    // WARN 用于需要关注但系统仍能继续运行的低电量状态。
    if (alerts.low_battery) {
      RCLCPP_WARN(
        get_logger(),
        "Low battery: device=%s battery=%.1f%% (threshold: below %.1f%%)",
        message->device_id.c_str(),
        message->battery_level,
        kLowBatteryThreshold);
    }

    // ERROR 表示更严重的高温状态；这里不用 else，允许两种告警同时输出。
    if (alerts.high_temperature) {
      RCLCPP_ERROR(
        get_logger(),
        "High temperature: device=%s temperature=%.1f C (threshold: above %.1f C)",
        message->device_id.c_str(),
        message->temperature,
        kHighTemperatureThreshold);
    }
  }

  // 保存为成员变量可以保证 Subscription 在节点的整个运行期内持续有效。
  rclcpp::Subscription<robot_device_interfaces::msg::DeviceStatus>::SharedPtr subscription_;
};

}  // namespace device_monitor

int main(int argc, char * argv[])
{
  // init -> 创建节点 -> spin 处理回调 -> shutdown，是最小 ROS 2 C++ 节点的标准生命周期。
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<device_monitor::DeviceMonitorNode>());
  rclcpp::shutdown();
  return 0;
}
