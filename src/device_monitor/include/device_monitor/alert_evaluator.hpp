#ifndef DEVICE_MONITOR__ALERT_EVALUATOR_HPP_
#define DEVICE_MONITOR__ALERT_EVALUATOR_HPP_

namespace device_monitor
{

inline constexpr float kLowBatteryThreshold = 20.0F;
inline constexpr float kHighTemperatureThreshold = 70.0F;

struct AlertResult
{
  bool low_battery;
  bool high_temperature;
};

// This function deliberately has no ROS dependency. The subscription callback
// only adapts a message into these values, so threshold behavior can be tested
// as ordinary C++ without spinning a node.
[[nodiscard]] AlertResult evaluate_alerts(
  float battery_level,
  float temperature_celsius) noexcept;

}  // namespace device_monitor

#endif  // DEVICE_MONITOR__ALERT_EVALUATOR_HPP_
