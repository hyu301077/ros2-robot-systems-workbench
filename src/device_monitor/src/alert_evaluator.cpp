#include "device_monitor/alert_evaluator.hpp"

namespace device_monitor
{

AlertResult evaluate_alerts(
  const float battery_level,
  const float temperature_celsius) noexcept
{
  // 需求使用严格比较：电量恰好 20% 和温度恰好 70℃ 都不触发告警。
  // 两个判断彼此独立，因此低电量和高温可以在同一条状态消息中同时成立。
  return AlertResult{
    battery_level < kLowBatteryThreshold,
    temperature_celsius > kHighTemperatureThreshold};
}

}  // namespace device_monitor
