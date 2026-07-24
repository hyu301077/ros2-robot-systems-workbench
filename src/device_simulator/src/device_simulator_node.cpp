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

class DeviceSimulatorNode final : public rclcpp::Node
{
public:
  DeviceSimulatorNode()
  : Node("device_simulator_node"),
    random_engine_(std::random_device{}()),
    temperature_step_(-0.25, 0.25)
  {
    device_id_ = declare_parameter<std::string>("device_id", "robot-001");
    battery_level_ = declare_parameter<double>("initial_battery_level", 100.0);
    software_version_ = declare_parameter<std::string>("software_version", "1.0.0");
    publish_period_ms_ = declare_parameter<std::int64_t>("publish_period_ms", 1000);

    validate_parameters();

    publisher_ = create_publisher<robot_device_interfaces::msg::DeviceStatus>(
      "/device/status", rclcpp::QoS(10));

    // A wall timer matches this simulator's real-time reporting behavior and
    // stays independent from a future simulated ROS clock.
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
    temperature_celsius_ = std::clamp(
      temperature_celsius_ + temperature_step_(random_engine_), 20.0, 40.0);

    robot_device_interfaces::msg::DeviceStatus message;
    message.device_id = device_id_;
    message.battery_level = static_cast<float>(battery_level_);
    message.temperature = static_cast<float>(temperature_celsius_);
    message.online = true;
    message.software_version = software_version_;

    publisher_->publish(message);

    RCLCPP_INFO(
      get_logger(),
      "Published /device/status: id=%s battery=%.1f%% temperature=%.1f C online=true version=%s",
      message.device_id.c_str(), message.battery_level, message.temperature,
      message.software_version.c_str());

    // Update after publishing so the first message reports the configured
    // initial value (100% by default), then decreases slowly once per cycle.
    battery_level_ = std::max(0.0, battery_level_ - kBatteryDropPerMessage);
  }

  static constexpr double kBatteryDropPerMessage = 0.1;

  std::string device_id_;
  double battery_level_{100.0};
  double temperature_celsius_{25.0};
  std::string software_version_;
  std::int64_t publish_period_ms_{1000};

  std::mt19937 random_engine_;
  std::uniform_real_distribution<double> temperature_step_;

  rclcpp::Publisher<robot_device_interfaces::msg::DeviceStatus>::SharedPtr publisher_;
  rclcpp::TimerBase::SharedPtr timer_;
};

}  // namespace device_simulator

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);

  try {
    rclcpp::spin(std::make_shared<device_simulator::DeviceSimulatorNode>());
  } catch (const std::exception & exception) {
    RCLCPP_FATAL(
      rclcpp::get_logger("device_simulator_node"),
      "Failed to start simulator: %s", exception.what());
    rclcpp::shutdown();
    return 1;
  }

  rclcpp::shutdown();
  return 0;
}
