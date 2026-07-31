#include <cstdlib>
#include <iostream>
#include <string>

#include "device_monitor/alert_evaluator.hpp"

namespace
{

// 这是一个最小测试程序：每个失败条件累加，最后统一返回非零退出码给 CTest。
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
  // 正常状态：两个告警都不应触发。
  {
    const auto result = device_monitor::evaluate_alerts(80.0F, 25.0F);
    expect(!result.low_battery, "normal battery must not trigger a warning");
    expect(!result.high_temperature, "normal temperature must not trigger an error");
  }

  // 阈值以下与恰好等于阈值分别测试，防止把“低于 20”误写成“小于等于 20”。
  {
    const auto result = device_monitor::evaluate_alerts(19.9F, 25.0F);
    expect(result.low_battery, "battery below 20 must trigger a warning");
    expect(!result.high_temperature, "low battery alone must not trigger temperature error");
  }

  {
    const auto result = device_monitor::evaluate_alerts(20.0F, 25.0F);
    expect(!result.low_battery, "battery equal to 20 must not trigger a warning");
  }

  // 同样验证高温规则使用“高于 70”，而不是“大于等于 70”。
  {
    const auto result = device_monitor::evaluate_alerts(80.0F, 70.1F);
    expect(result.high_temperature, "temperature above 70 must trigger an error");
    expect(!result.low_battery, "high temperature alone must not trigger battery warning");
  }

  {
    const auto result = device_monitor::evaluate_alerts(80.0F, 70.0F);
    expect(!result.high_temperature, "temperature equal to 70 must not trigger an error");
  }

  // 两种异常同时出现时，两项结果都必须保留，不能因一个告警覆盖另一个。
  {
    const auto result = device_monitor::evaluate_alerts(10.0F, 80.0F);
    expect(result.low_battery, "low battery must still trigger when temperature is high");
    expect(result.high_temperature, "high temperature must still trigger when battery is low");
  }

  // CTest 根据进程退出码判断通过或失败，输出文字只用于帮助开发者定位问题。
  if (failures != 0) {
    std::cerr << failures << " alert evaluator assertion(s) failed\n";
    return EXIT_FAILURE;
  }

  std::cout << "All alert evaluator tests passed\n";
  return EXIT_SUCCESS;
}
