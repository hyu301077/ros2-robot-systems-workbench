#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <functional>
#include <memory>
#include <random>
#include <stdexcept>
#include <string>

#include "rclcpp/rclcpp.hpp"
#include "robot_device_interfaces/msg/device_status.hpp"

namespace device_simulator
{

// 继承 rclcpp::Node 后，这个 C++ 对象就拥有 ROS 2 节点能力。
// V0 中一个 executable 只创建一个 Node，也就是“一进程一节点”的直观模型。
class DeviceSimulatorNode final : public rclcpp::Node
{
public:
  DeviceSimulatorNode()
  : Node("device_simulator_node"),
    random_engine_(std::random_device{}()),
    temperature_step_(-0.25, 0.25)
  {
    // Parameter 让同一个可执行程序可以通过命令行或 Launch 配置，
    // 无需为了更换设备编号、初始电量等配置而重新编译代码。
    device_id_ = declare_parameter<std::string>("device_id", "robot-001");
    battery_level_ = declare_parameter<double>("initial_battery_level", 100.0);
    software_version_ = declare_parameter<std::string>("software_version", "1.0.0");
    publish_period_ms_ = declare_parameter<std::int64_t>("publish_period_ms", 1000);

    // 在创建 Timer 前检查参数，避免无效周期等配置进入节点主循环。
    validate_parameters();

    // Publisher 的模板参数就是 Topic 的消息类型。
    // QoS(10) 表示发送端保留最近 10 条尚待处理的数据，适合这个低频状态流。
    publisher_ = create_publisher<robot_device_interfaces::msg::DeviceStatus>(
      "/device/status", rclcpp::QoS(10));

    // wall timer 使用操作系统时间，每到周期就调用一次 publish_status()。
    // 它不依赖 ROS 模拟时钟，符合当前“真实时间下模拟设备上报”的需求。
    timer_ = create_wall_timer(
      std::chrono::milliseconds(publish_period_ms_),
      std::bind(&DeviceSimulatorNode::publish_status, this));

    RCLCPP_INFO(
      get_logger(),
      "Simulator ready: device_id=%s, initial_battery=%.1f%%, version=%s, period=%ld ms",
      device_id_.c_str(), battery_level_, software_version_.c_str(),
      static_cast<long>(publish_period_ms_));
  }

private:
  void validate_parameters()
  {
    // 对必须有内容的字符串采用失败即退出；启动失败比带着错误身份继续发布更容易诊断。
    if (device_id_.empty()) {
      throw std::invalid_argument("parameter 'device_id' must not be empty");
    }
    if (software_version_.empty()) {
      throw std::invalid_argument("parameter 'software_version' must not be empty");
    }
    if (publish_period_ms_ <= 0) {
      throw std::invalid_argument("parameter 'publish_period_ms' must be greater than zero");
    }
    if (!std::isfinite(battery_level_)) {
      throw std::invalid_argument("parameter 'initial_battery_level' must be finite");
    }

    // 电量越界时采用夹紧而不是退出，使输入仍可恢复，同时用 WARN 明确提示配置问题。
    const double requested_battery = battery_level_;
    battery_level_ = std::clamp(battery_level_, 0.0, 100.0);
    if (requested_battery != battery_level_) {
      RCLCPP_WARN(
        get_logger(),
        "initial_battery_level %.1f is outside [0, 100]; clamped to %.1f",
        requested_battery, battery_level_);
    }
  }

  void publish_status()
  {
    // 每个周期加入一个很小的随机温度变化，并限制在正常模拟范围内。
    temperature_celsius_ = std::clamp(
      temperature_celsius_ + temperature_step_(random_engine_), 20.0, 40.0);

    // .msg 文件在编译时生成了下面这个 C++ 类型。
    // 发布者和订阅者共享这一数据契约，但不需要直接调用对方的代码。
    robot_device_interfaces::msg::DeviceStatus message;
    message.device_id = device_id_;
    message.battery_level = static_cast<float>(battery_level_);
    message.temperature = static_cast<float>(temperature_celsius_);
    message.online = true;
    message.software_version = software_version_;

    // publish() 将消息交给 ROS 2 中间件；节点不需要知道订阅者位于哪个进程。
    publisher_->publish(message);

    RCLCPP_INFO(
      get_logger(),
      "Published /device/status: id=%s battery=%.1f%% temperature=%.1f C online=true version=%s",
      message.device_id.c_str(), message.battery_level, message.temperature,
      message.software_version.c_str());

    // 发布后再扣电量，因此第一条消息会准确携带配置的初始值（默认 100%）。
    battery_level_ = std::max(0.0, battery_level_ - kBatteryDropPerMessage);
  }

  static constexpr double kBatteryDropPerMessage = 0.1;

  // 这些成员保存节点配置和下一次发布所需的模拟状态。
  std::string device_id_;
  double battery_level_{100.0};
  double temperature_celsius_{25.0};
  std::string software_version_;
  std::int64_t publish_period_ms_{1000};

  std::mt19937 random_engine_;
  std::uniform_real_distribution<double> temperature_step_;

  // ROS 实体必须在节点运行期间保持存活，因此用 SharedPtr 保存为成员变量。
  rclcpp::Publisher<robot_device_interfaces::msg::DeviceStatus>::SharedPtr publisher_;
  rclcpp::TimerBase::SharedPtr timer_;
};

}  // namespace device_simulator

int main(int argc, char * argv[])
{
  // init() 初始化 ROS 2 客户端库，并解析 --ros-args 等 ROS 专用命令行参数。
  rclcpp::init(argc, argv);

  try {
    // spin() 进入事件循环；Timer 到期时，执行器会调用 publish_status()。
    rclcpp::spin(std::make_shared<device_simulator::DeviceSimulatorNode>());
  } catch (const std::exception & exception) {
    RCLCPP_FATAL(
      rclcpp::get_logger("device_simulator_node"),
      "Failed to start simulator: %s", exception.what());
    rclcpp::shutdown();
    return 1;
  }

  // spin() 因 Ctrl+C 等原因退出后，释放 ROS 2 全局资源。
  rclcpp::shutdown();
  return 0;
}
