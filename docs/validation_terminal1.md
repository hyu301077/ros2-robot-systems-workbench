# 终端1验证记录

本页记录真实执行结果，不把静态检查或“理论兼容”写成运行通过。

## 验证范围

- 日期：2026-07-24
- 操作系统：Ubuntu 22.04.5 LTS（Jammy）
- ROS 2：Humble
- 运行范围：终端1本机内的完整双节点系统
- 源码标准：C++17

终端2的 Ubuntu 26.04 / ROS 2 Lyrical 验证不在本页范围内。

## 环境命令

| 命令 | 实际结果 |
|---|---|
| `cat /etc/os-release` | Ubuntu 22.04.5 LTS，Jammy |
| `echo $ROS_DISTRO` | source后为`humble` |
| `ros2 --help` | 退出码0 |
| `colcon --version-check` | 本机CLI不接受该选项形式，退出码2 |
| `colcon version-check` | 正确的本机子命令形式，联网检查退出码0 |
| `g++ --version` | GCC 11.4.0 |
| `cmake --version` | CMake 3.22.1 |
| `apt-cache policy ros-humble-ros-base` | 已安装且候选版本一致 |
| `apt-cache policy ros-humble-desktop` | 有官方候选版本，但本项目未安装桌面套件 |

`colcon version-check`显示大多数扩展为最新版本；
`colcon-parallel-executor`有更新版本，但当前APT安装版本能够完成本项目构建，因此没有绕过
系统包管理器单独升级。

## 依赖核对

以下依赖均能在本机官方Humble安装前缀`/opt/ros/humble`中找到：

- `ament_cmake`
- `rclcpp`
- `rosidl_default_generators`
- `rosidl_default_runtime`
- `launch`
- `launch_ros`
- `ros2launch`
- colcon公共扩展

`rosdep update`已成功更新用户缓存，随后
`rosdep check --from-paths src --ignore-src`返回“所有系统依赖均已满足”。构建所需的
声明依赖也通过APT包和ROS package前缀逐项核对。

## 项目验证

| 验证 | 命令或入口 | 实际结果 |
|---|---|---|
| 静态一致性 | `tools/static_audit.py` | 通过 |
| 依赖检查 | `rosdep check --from-paths src --ignore-src` | 所有系统依赖均已满足 |
| 工作空间构建 | `colcon build --symlink-install` | 3个package通过 |
| 自定义消息生成 | `ros2 interface show robot_device_interfaces/msg/DeviceStatus` | 通过，5个字段正确 |
| 可执行文件发现 | `ros2 pkg executables` | 两个节点均可发现 |
| 单元测试 | `colcon test` | 通过 |
| 测试明细 | `colcon test-result --verbose` | 1个测试，0错误，0失败，0跳过 |
| 节点与Topic发现 | `tools/smoke_test.sh` | 两个节点和`/device/status`均发现 |
| Topic消息 | 冒烟测试中的一次有界echo | 收到`DeviceStatus` |
| 低电量告警 | 以19%初始电量启动 | 观察到WARN |
| 高温告警 | 注入75℃测试消息 | 观察到ERROR |
| 进程清理 | 冒烟测试退出检查 | 未留下节点或Launch进程 |
| 一键复验 | `tools/verify_workspace.sh` | 通过 |

## 验证中修复的问题

1. Humble生成的`setup.bash`会读取可选环境变量。验证脚本现在只在source期间临时关闭
   Bash nounset检查，source完成后立即恢复。
2. ROS Launch默认向`~/.ros`写日志。冒烟测试现在使用独立的`ROS_HOME`和
   `ROS_LOG_DIR`，日志被限制在当次测试目录中。
3. 受限环境中的ROS CLI daemon不能可靠查询图。冒烟测试使用Humble实际支持的
   `--no-daemon`直接查询DDS图；普通用户终端仍可使用README中的标准命令。

## 尚未验证

- 终端2 Ubuntu 26.04 / ROS 2 Lyrical构建与运行。
- GitHub Actions中的Humble和Lyrical两个job。
- GitHub上传、第二终端拉取和AI增量交接流程。
