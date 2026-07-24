# 兼容性策略 / Compatibility policy

## 目标环境矩阵 / Target matrix

| 角色 / Role | 操作系统 / Operating system | ROS 2 | 支持定位 / Support policy |
|---|---|---|---|
| 终端1 / Terminal 1 | Ubuntu 22.04 | Humble | 当前兼容目标 / Compatibility target |
| 终端2 / Terminal 2 | Ubuntu 26.04 | Lyrical | 长期主目标 / Long-term primary target |

本仓库提供的是**源码兼容性**，不是可在两个系统间直接复制的二进制文件。每台终端都应使用
自己的 ROS 2 安装，在本机独立构建并运行完整系统。

This repository provides **source compatibility**, not portable binaries. Each
terminal builds and runs the complete system locally with its matching ROS 2
installation.

跨设备 ROS 通信，以及 Humble 与 Lyrical 混合组成同一个运行时 ROS 图，明确不在本项目范围内。
GitHub 只同步源代码。

Cross-device ROS communication and mixed Humble/Lyrical runtime graphs are
explicitly out of scope. GitHub synchronizes source code only.

## 实现规则 / Implementation rules

- C++17 是两个目标环境共同的语言基线。
  C++17 is the common language baseline.
- 优先使用两个发行版中都存在且稳定的 `rclcpp`、ROS 接口生成、参数、Topic、Launch 和
  `ament_cmake` API。
  Prefer stable `rclcpp`, ROS interface generation, parameters, topics, launch,
  and `ament_cmake` APIs present in both distributions.
- 保持一份实现和一个 `main` 分支。
  Keep a single implementation and a single `main` branch.
- 只有真实构建证明存在 API 差异时，才增加小而隔离的兼容层；不要把发行版判断散落在业务代码中。
  Add a small, isolated compatibility adapter only after a real build proves an
  API difference; do not scatter distribution checks through business logic.
- 在接受新的 ROS 依赖前，必须在两个发行版上验证。
  Verify every new ROS dependency on both distributions before accepting it.
- 两个目标环境的接口定义必须保持一致。
  Keep interface definitions identical on both targets.

## 构建系统兼容性 / Build-system compatibility

两个 C++ 节点 package 使用现代导入目标 `rclcpp::rclcpp`，并使用
`${robot_device_interfaces_TARGETS}` 导出的生成目标列表。这种写法已经通过 Humble 和
Lyrical CI，同时避免在 C++ 源码中加入发行版判断。

The two C++ node packages link with the modern imported target
`rclcpp::rclcpp` and the generated target list exported through
`${robot_device_interfaces_TARGETS}`. This form has passed both Humble and
Lyrical CI and keeps ROS distribution checks out of the C++ source.

项目有意不再使用旧的 `ament_target_dependencies()` 辅助宏。它在 Humble 的
`find_package(ament_cmake)` 后可用，但当前 Lyrical 配置不会通过相同的 package 查找暴露
该宏。仓库的静态审计会拒绝它，避免兼容性问题再次出现。

The older `ament_target_dependencies()` helper is intentionally not used.
Although it is available after `find_package(ament_cmake)` in Humble, the
current Lyrical configuration does not expose that helper through the same
package lookup. The repository's static audit rejects it to prevent this
compatibility regression from returning.

## 生命周期 / Lifecycle

ROS 2 Humble 面向 Ubuntu 22.04，生命周期在 2027 年 5 月结束；ROS 2 Lyrical 面向
Ubuntu 26.04，生命周期在 2031 年 5 月结束。因此终端1需要在未来迁移，终端2则是长期基线。

ROS 2 Humble targets Ubuntu 22.04 and reaches end of life in May 2027. ROS 2
Lyrical targets Ubuntu 26.04 and reaches end of life in May 2031. Terminal 1
therefore has a planned future migration, while Terminal 2 is the long-term
baseline.

官方参考 / Official references:

- <https://docs.ros.org/en/humble/Releases/Release-Humble-Hawksbill.html>
- <https://docs.ros.org/en/lyrical/Releases/Release-Lyrical-Luth.html>

## 当前验证状态 / Current verification status

- 终端1操作系统识别已完成：Ubuntu 22.04.5。
  Terminal 1 OS detection: completed; Ubuntu 22.04.5 confirmed.
- 终端1 ROS 2 Humble 构建、单元测试和有界运行时冒烟测试已通过；详见
  [validation_terminal1.md](validation_terminal1.md)。
  Terminal 1 ROS 2 Humble build, unit test and bounded runtime smoke test:
  passed on Ubuntu 22.04.5; see
  [validation_terminal1.md](validation_terminal1.md).
- GitHub Actions 运行 `30076049851` 已通过：Humble/Ubuntu 22.04 与
  Lyrical/Ubuntu 26.04 的依赖检查、构建、单元测试和有界冒烟测试均成功。
  GitHub Actions run `30076049851`: Humble/Ubuntu 22.04 and Lyrical/Ubuntu
  26.04 dependency checks, builds, unit tests and bounded smoke tests passed.
- 终端2实机上的 ROS 2 Lyrical 构建尚未在本工作区执行。
  Terminal 2 ROS 2 Lyrical build: not run in this workspace.
- 跨设备通信刻意不适用。
  Cross-device communication: intentionally not applicable.
