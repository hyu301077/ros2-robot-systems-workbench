#ifndef DEVICE_MONITOR__ALERT_EVALUATOR_HPP_
#define DEVICE_MONITOR__ALERT_EVALUATOR_HPP_

namespace device_monitor
{

// 阈值集中定义在普通 C++ 头文件中，节点日志和测试使用同一份数值，避免规则漂移。
inline constexpr float kLowBatteryThreshold = 20.0F;
inline constexpr float kHighTemperatureThreshold = 70.0F;

// 纯数据结果不包含 ROS 类型，调用方只需要把结果映射为相应的日志级别。
struct AlertResult
{
  bool low_battery;
  bool high_temperature;
};

// 这个函数刻意不依赖 ROS。订阅回调只负责从消息取值并调用它，
// 因而无需启动节点或 DDS，就能把告警边界作为普通 C++ 逻辑进行单元测试。
[[nodiscard]] AlertResult evaluate_alerts(
  float battery_level,
  float temperature_celsius) noexcept;

}  // namespace device_monitor

#endif  // DEVICE_MONITOR__ALERT_EVALUATOR_HPP_
