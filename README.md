# ROS 2 Robot Systems Workbench

一个面向车载嵌入式 C++ 开发者的最小 ROS 2 学习系统。项目把“多个独立车载应用”
映射为“多个 ROS 2 节点”，通过一个真实可生成的自定义消息和一条 Topic 展示节点、
消息、参数、Launch、编译、测试与调试的基本工作流。

V0 只包含状态模拟与监控，不包含真实硬件、Service、Action、OTA、数据库、云端、
Gazebo、Web 界面或复杂线程模型。

## 支持环境

| 终端 | 操作系统 | ROS 2 | 定位 |
|---|---|---|---|
| 终端1 | Ubuntu 22.04 | Humble | 当前兼容目标 |
| 终端2 | Ubuntu 26.04 | Lyrical | 长期主目标 |

两个终端使用同一份 C++17 源码，但分别编译、分别运行完整节点系统，不共享二进制，
也不组成跨设备或混合发行版 ROS 图。

依赖仅限：

- C++17 编译器与 CMake 3.22 或更高版本；
- ROS 2 对应发行版；
- `ament_cmake`、`rclcpp`；
- `rosidl_default_generators`、`rosidl_default_runtime`；
- `launch`、`launch_ros`、`ros2launch`；
- `colcon`。

Humble 官方支持至 2027 年 5 月，终端1需要在此之前规划升级。安装 ROS 2 和开发工具
应遵循对应发行版的官方文档；本项目脚本不会执行 `sudo` 或自动安装软件。

## 系统架构

```mermaid
flowchart LR
    I[robot_device_interfaces<br/>DeviceStatus.msg]
    S[device_simulator_node<br/>进程 1]
    T[/device/status<br/>DeviceStatus Topic]
    M[device_monitor_node<br/>进程 2]

    I -. 生成 C++ 消息类型 .-> S
    I -. 生成 C++ 消息类型 .-> M
    S -- 每个周期发布 --> T
    T -- DDS 分发 --> M
```

V0 刻意采用“一个 executable = 一个进程 = 一个 node”，方便从车载独立应用模型迁移。
这是项目约束，不是 ROS 2 的强制限制。

## 三个 package

### `robot_device_interfaces`

只定义通信契约，不产生应用进程。把消息放在独立 package 中有三个原因：

1. 发布者和订阅者依赖同一个数据契约；
2. 使用消息的节点不必互相依赖业务代码；
3. 修改接口时，colcon 和 CI 可以明确找出所有受影响的消费者。

### `device_simulator`

生成 `device_simulator_node`，模拟硬件或 MCU：

- 向 `/device/status` 发布状态；
- 默认周期 1000 ms；
- 电量首次发布为初始值，之后每次下降 0.1；
- 温度从 25℃ 开始，在 20℃～40℃间轻微随机变化；
- 始终报告 `online=true`；
- 提供设备编号、初始电量、软件版本和发布周期参数；
- 包含启动整个 V0 系统的 Launch 文件。

### `device_monitor`

生成 `device_monitor_node`：

- 订阅 `/device/status`；
- 输出所有 `DeviceStatus` 字段；
- 电量 `< 20` 时记录 WARN；
- 温度 `> 70℃` 时记录 ERROR；
- 告警判断是无 ROS 依赖的普通 C++ 函数，可独立单元测试。

## `DeviceStatus` 消息

| 字段 | ROS类型 | 含义 |
|---|---|---|
| `device_id` | `string` | 稳定的设备逻辑编号 |
| `battery_level` | `float32` | 剩余电量百分比，期望范围 0～100 |
| `temperature` | `float32` | 摄氏温度 |
| `online` | `bool` | 设备是否在线 |
| `software_version` | `string` | 设备软件版本 |

接口源码位于
[`DeviceStatus.msg`](src/robot_device_interfaces/msg/DeviceStatus.msg)。

## 目录结构

```text
.
├── src/
│   ├── robot_device_interfaces/
│   │   └── msg/DeviceStatus.msg
│   ├── device_simulator/
│   │   ├── launch/device_system.launch.py
│   │   └── src/device_simulator_node.cpp
│   └── device_monitor/
│       ├── include/device_monitor/alert_evaluator.hpp
│       ├── src/
│       └── test/test_alert_evaluator.cpp
├── docs/
│   ├── architecture.md
│   ├── learning_notes.md
│   ├── interview_questions.md
│   ├── compatibility.md
│   ├── validation_terminal1.md
│   └── system_manifest.yaml
├── tools/
│   ├── doctor.sh
│   ├── ai_context.sh
│   ├── smoke_test.sh
│   └── verify_workspace.sh
└── .github/
```

`build/`、`install/` 和 `log/` 由 colcon 生成，不提交到 Git。

## 编译

打开一个没有 source 其他 ROS 发行版的新终端：

```bash
cd /path/to/ros2-robot-systems-workbench

# 终端1执行这一条
source /opt/ros/humble/setup.bash

# 终端2执行这一条，不要与上面一条在同一shell中执行
source /opt/ros/lyrical/setup.bash

tools/doctor.sh
rosdep check --from-paths src --ignore-src
colcon build --symlink-install
source install/setup.bash
```

如果 `rosdep check` 报缺少依赖，先审阅其输出，再由使用者决定是否安装；项目不会自动
调用 `sudo`。

## 运行

一次启动两个节点：

```bash
# 终端1
source /opt/ros/humble/setup.bash

# 终端2改用：source /opt/ros/lyrical/setup.bash
source install/setup.bash
ros2 launch device_simulator device_system.launch.py
```

覆盖模拟器参数：

```bash
ros2 launch device_simulator device_system.launch.py \
  device_id:=robot-002 \
  initial_battery_level:=75.0 \
  software_version:=1.1.0 \
  publish_period_ms:=500
```

也可以分别启动：

```bash
ros2 run device_monitor device_monitor_node
ros2 run device_simulator device_simulator_node
```

## 查看节点、Topic与参数

在另一个已经 source ROS 与本工作空间的终端执行：

```bash
ros2 node list
ros2 node info /device_simulator_node
ros2 node info /device_monitor_node

ros2 topic list
ros2 topic info /device/status
ros2 topic echo /device/status

ros2 interface show robot_device_interfaces/msg/DeviceStatus

ros2 param list /device_simulator_node
ros2 param get /device_simulator_node device_id
ros2 param get /device_simulator_node publish_period_ms
```

## 预期输出

以下是代码定义的预期形态，不是当前机器已经采集的运行日志：

```text
[device_simulator_node]: Published /device/status: id=robot-001 battery=100.0% temperature=25.1 C online=true version=1.0.0
[device_monitor_node]: Received /device/status: id=robot-001 battery=100.0% temperature=25.1 C online=true version=1.0.0
```

使用低初始电量时：

```text
[WARN] [device_monitor_node]: Low battery: device=robot-001 battery=19.0% (threshold: below 20.0%)
```

向 Topic 注入 75℃ 状态时：

```text
[ERROR] [device_monitor_node]: High temperature: device=smoke-hot temperature=75.0 C (threshold: above 70.0 C)
```

## 测试与完整验证

标准命令：

```bash
colcon build --symlink-install
source install/setup.bash
colcon test
colcon test-result --verbose
```

完整自动验证入口：

```bash
tools/verify_workspace.sh
```

它会依次执行环境检查、构建、CTest单元测试、测试结果检查和有界冒烟测试。冒烟测试：

- 在独立 `ROS_DOMAIN_ID` 中启动两个节点；
- 验证两个节点和 `/device/status` 被发现；
- 执行一次 `ros2 topic echo --once`；
- 以 19% 初始电量验证真实 WARN 日志；
- 注入一条 75℃ 消息验证真实 ERROR 日志；
- 使用超时和进程组清理，避免残留节点。

只运行冒烟测试：

```bash
# 终端1
source /opt/ros/humble/setup.bash

# 终端2改用：source /opt/ros/lyrical/setup.bash
source install/setup.bash
tools/smoke_test.sh
```

## 当前实际验证状态

截至 2026-07-24：

| 项目 | 结果 |
|---|---|
| 终端1操作系统识别 | 通过：Ubuntu 22.04.5 |
| Bash脚本语法 | 通过 |
| package XML解析 | 通过 |
| Launch Python语法 | 通过 |
| `DeviceStatus`字段契约检查 | 通过 |
| 告警纯C++编译 | 通过：GCC 11.4，C++17，启用严格警告 |
| 低电量逻辑单元测试 | 通过 |
| 高温逻辑单元测试 | 通过 |
| ROS 2与colcon环境 | 通过：Humble，`ros-humble-ros-base`，`ros-dev-tools` |
| `colcon build --symlink-install` | 通过：3个package |
| `colcon test` | 通过 |
| `colcon test-result --verbose` | 通过：1个测试，0错误，0失败，0跳过 |
| 两节点Topic通信 | 通过：`/device/status` |
| WARN/ERROR真实ROS日志 | 通过：19%低电量WARN、75℃高温ERROR |
| 有界Launch冒烟测试 | 通过，且退出后未留下节点进程 |
| `rosdep check --from-paths src --ignore-src` | 通过：所有系统依赖均已满足 |
| GitHub Actions Humble验证 | 通过：Ubuntu 22.04 runner完成依赖检查、构建、测试和冒烟测试 |
| GitHub Actions Lyrical验证 | 通过：Ubuntu 26.04 runner完成依赖检查、构建、测试和冒烟测试 |
| 终端2实机 Lyrical验证 | 未执行 |
| GitHub上传 | 已完成：公开仓库 |

完整的终端1命令、版本和测试记录见
[终端1验证记录](docs/validation_terminal1.md)。

## GitHub Actions

仓库包含两个独立CI job：

- `ubuntu-22.04` + Humble；
- `ubuntu-26.04` + Lyrical。

CI使用固定到完整提交SHA的`ros-tooling/setup-ros`（对应`v0.7`）在临时GitHub runner
中安装ROS开发环境，然后执行依赖检查和`tools/verify_workspace.sh`。CI中的安装只影响
一次性runner，不会修改终端1或终端2。`ubuntu-26.04`目前仍是GitHub Public Preview
runner，所以它是辅助证据，不能取代终端2的真实验证。

提交`6c90c60`对应的[双发行版CI运行](https://github.com/hyu301077/ros2-robot-systems-workbench/actions/runs/30076049851)
已全部通过。项目源码位于公开仓库
[ros2-robot-systems-workbench](https://github.com/hyu301077/ros2-robot-systems-workbench)。

## 常见错误与排查

### `ros2: command not found`

ROS 2尚未安装，或者当前shell没有source：

```bash
source /opt/ros/humble/setup.bash
# 或
source /opt/ros/lyrical/setup.bash
```

不要在同一个shell中source两个发行版。

### `colcon: command not found`

终端缺少colcon开发工具。根据对应ROS官方安装文档安装；本项目不会自动安装。

### `package 'device_simulator' not found`

确认已经成功构建，并在运行终端执行：

```bash
source install/setup.bash
```

### 找不到生成的`device_status.hpp`

先检查接口package是否构建成功：

```bash
colcon build --packages-select robot_device_interfaces
```

然后构建其消费者：

```bash
colcon build --packages-up-to device_simulator device_monitor
```

### `ros2 topic echo`一直等待

这是正常的订阅行为：它会等待下一条消息。检查模拟器是否运行、Topic拼写是否为绝对名称
`/device/status`，并查看：

```bash
ros2 node list
ros2 topic info /device/status
```

### 修改代码后行为没有变化

确认重新构建并重新source `install/setup.bash`。不要在已source旧overlay的shell中进行构建。

### 没有看到低电量或高温告警

正常模拟器很久才会降到20%以下，而且温度被限制在20℃～40℃。使用
`tools/smoke_test.sh`进行确定性验证，不要仅等待正常运行自然触发。

### AI在另一终端不知道改了什么

先保留本地修改，再执行：

```bash
git status --short --branch
tools/ai_context.sh --fetch
```

确认工作树干净且只有落后提交后再运行`git pull --ff-only`。AI应优先读取新增提交摘要和
受影响文件，而不是重新加载整个仓库。

## 延伸阅读

- [系统架构](docs/architecture.md)
- [面向车载C++开发者的学习笔记](docs/learning_notes.md)
- [项目面试题与参考答案](docs/interview_questions.md)
- [双发行版兼容策略](docs/compatibility.md)
- [终端1验证记录](docs/validation_terminal1.md)
- [紧凑系统清单](docs/system_manifest.yaml)

官方资料：

- <https://docs.ros.org/en/humble/>
- <https://docs.ros.org/en/lyrical/>
