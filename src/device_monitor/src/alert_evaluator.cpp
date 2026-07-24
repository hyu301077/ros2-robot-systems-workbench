#include "device_monitor/alert_evaluator.hpp"

namespace device_monitor
{

AlertResult evaluate_alerts(
  const float battery_level,
  const float temperature_celsius) noexcept
{
  return AlertResult{
    battery_level < kLowBatteryThreshold,
    temperature_celsius > kHighTemperatureThreshold};
}

}  // namespace device_monitor
