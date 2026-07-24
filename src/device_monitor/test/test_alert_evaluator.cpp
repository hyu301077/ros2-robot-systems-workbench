#include <cstdlib>
#include <iostream>
#include <string>

#include "device_monitor/alert_evaluator.hpp"

namespace
{

int failures = 0;

void expect(const bool condition, const std::string & description)
{
  if (!condition) {
    std::cerr << "FAILED: " << description << '\n';
    ++failures;
  }
}

}  // namespace

int main()
{
  {
    const auto result = device_monitor::evaluate_alerts(80.0F, 25.0F);
    expect(!result.low_battery, "normal battery must not trigger a warning");
    expect(!result.high_temperature, "normal temperature must not trigger an error");
  }

  {
    const auto result = device_monitor::evaluate_alerts(19.9F, 25.0F);
    expect(result.low_battery, "battery below 20 must trigger a warning");
    expect(!result.high_temperature, "low battery alone must not trigger temperature error");
  }

  {
    const auto result = device_monitor::evaluate_alerts(20.0F, 25.0F);
    expect(!result.low_battery, "battery equal to 20 must not trigger a warning");
  }

  {
    const auto result = device_monitor::evaluate_alerts(80.0F, 70.1F);
    expect(result.high_temperature, "temperature above 70 must trigger an error");
    expect(!result.low_battery, "high temperature alone must not trigger battery warning");
  }

  {
    const auto result = device_monitor::evaluate_alerts(80.0F, 70.0F);
    expect(!result.high_temperature, "temperature equal to 70 must not trigger an error");
  }

  {
    const auto result = device_monitor::evaluate_alerts(10.0F, 80.0F);
    expect(result.low_battery, "low battery must still trigger when temperature is high");
    expect(result.high_temperature, "high temperature must still trigger when battery is low");
  }

  if (failures != 0) {
    std::cerr << failures << " alert evaluator assertion(s) failed\n";
    return EXIT_FAILURE;
  }

  std::cout << "All alert evaluator tests passed\n";
  return EXIT_SUCCESS;
}
