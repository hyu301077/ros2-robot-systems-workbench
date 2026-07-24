# ROS 2学习笔记：从车载C++应用到机器人节点

## ROS 2是什么

ROS 2不是Linux发行版，也不是实时操作系统内核。它是一套用于构建分布式机器人软件的：

- 通信模型和客户端库；
- 消息与接口定义机制；
- 节点发现和中间件抽象；
- 构建、启动、测试和调试工具；
- 可复用package生态。

你仍然使用普通C++、CMake、Linux进程和日志。ROS 2增加的是一套统一的通信与工程约定。

## Package、executable和node

### Package

package是源码与依赖的交付单元，类似一个有明确manifest和构建规则的组件目录。
`package.xml`描述依赖与元数据，`CMakeLists.txt`描述怎样构建和安装。

package不等于运行中的应用。例如`robot_device_interfaces`只有消息定义，没有进程。

### Executable

executable是编译产物，可类比车载系统中的一个应用程序文件。本项目产生：

- `device_simulator_node`
- `device_monitor_node`

### Node

node是加入ROS图的逻辑参与者。它可以创建Publisher、Subscriber、Timer和Parameter。
本项目为了容易理解，让一个executable只创建一个node。

以后可以把多个node组合进一个进程以减少进程数量或通信开销，但这不是V0内容。

## Publisher和Subscriber

Publisher负责向Topic写消息：

```text
device_simulator_node
  → publish(DeviceStatus)
```

Subscriber声明关注某个Topic。收到匹配消息后，ROS 2调用它注册的回调：

```text
/device/status有新消息
  → DeviceMonitorNode::handle_status()
```

发布者不直接调用订阅者函数，也不需要知道订阅者进程地址。这与应用之间保存socket连接
或直接调用RPC服务不同。

## Topic是什么

Topic是有名字、有消息类型的数据通道。本项目的通道是：

```text
名称：/device/status
类型：robot_device_interfaces/msg/DeviceStatus
```

Topic适合持续状态流和广播语义。Publisher与Subscriber必须在Topic名称和消息类型上匹配。

这里使用以`/`开头的绝对Topic名，确保两个节点无论位于什么namespace都使用同一通道。
大型系统中通常会更多地利用namespace和相对名称，但V0先保持固定。

## DDS承担什么职责

ROS 2的C++ API位于`rclcpp`。它下面依次经过ROS客户端库公共层、RMW抽象层，最终由
DDS实现负责网络通信。

DDS主要承担：

- 发现Publisher和Subscriber；
- 根据Topic和类型匹配端点；
- 序列化和反序列化消息；
- 在进程间或主机间传输；
- 执行可靠性、历史深度等QoS规则。

因此本项目不需要自己写一个“中央消息转发节点”。DDS不是你的业务node；它是ROS 2
通信基础设施的一部分。

本项目的Publisher和Subscriber都使用深度为10的默认可靠QoS。它表示为每个端点保留
有限历史，并使用ROS 2默认可靠性设置。V0不进一步比较QoS策略。

## 自定义消息如何工作

`DeviceStatus.msg`是语言无关的接口描述：

```text
string device_id
float32 battery_level
float32 temperature
bool online
string software_version
```

构建接口package时，`rosidl_default_generators`根据它生成C++头文件和类型支持代码。
C++节点随后包含：

```cpp
#include "robot_device_interfaces/msg/device_status.hpp"
```

所以生成的头文件不存在于源码目录；必须先构建接口package。

独立接口package类似集中管理的IDL或公共协议组件，但生成流程、类型注册和依赖关系由
ROS 2工具链统一处理。

## Parameter是什么

Parameter是node的命名配置值。它不是Topic消息，也不是C++全局变量。

模拟器在构造时声明：

- `device_id`
- `initial_battery_level`
- `software_version`
- `publish_period_ms`

Launch在node启动前提供覆盖值。节点启动后也可以使用`ros2 param`查看这些配置。

ROS参数支持的浮点标量是float64，因此初始电量用`double`接收，再转换为消息的
`float32`。

## CMake、ament_cmake和colcon分别是什么

### CMake

CMake描述单个C/C++项目的目标、源文件、编译特性、链接关系和安装规则。例如：

- `add_executable()`创建可执行程序；
- `target_compile_features()`要求C++17；
- `install()`决定colcon安装到哪里。

### ament_cmake

ament_cmake是在CMake之上的ROS 2扩展。它提供：

- ROS package注册；
- 依赖连接；
- 接口生成集成；
- 测试和安装约定。

`ament_package()`会把当前目录注册成可被ROS工具发现的package。

### colcon

colcon面向整个workspace。它读取各package的依赖关系，按顺序调用它们自己的构建系统。

在本项目中，colcon必须先生成`robot_device_interfaces`，才能编译两个包含消息头文件的
C++节点。

构建后常见目录：

```text
build/    每个package的中间文件
install/  可运行的overlay与setup脚本
log/      构建和测试日志
```

这些都是生成物，不进入Git。

## `source install/setup.bash`做了什么

系统安装的ROS 2是underlay，本项目构建结果是overlay。

```bash
source /opt/ros/humble/setup.bash
source install/setup.bash
```

第一条让shell找到Humble的命令和package；第二条把当前项目放到其上层，让ROS工具找到
刚构建的消息、可执行程序和Launch文件。

`source`只修改当前shell的环境。新开终端后必须重新执行。终端1和终端2不能在同一个
shell中混合source Humble与Lyrical。

## 常用观察命令

### `ros2 node list`

列出当前ROS图发现的node。本项目运行后应包含：

```text
/device_simulator_node
/device_monitor_node
```

### `ros2 topic list`

列出当前发现的Topic，应包含`/device/status`。ROS 2还可能显示参数事件和日志等内部
Topic。

### `ros2 topic echo /device/status`

临时创建一个Subscriber并打印收到的每条消息。它很适合确认：

- Publisher是否真的在发布；
- Topic名称和类型是否正确；
- 字段值是否符合预期。

它默认持续等待，按`Ctrl+C`停止。自动测试使用`--once`和外部超时避免永久运行。

### 其他有用命令

```bash
ros2 node info /device_simulator_node
ros2 topic info /device/status
ros2 interface show robot_device_interfaces/msg/DeviceStatus
ros2 param list /device_simulator_node
```

## Launch是什么

Launch文件描述如何一起启动和配置多个进程。本项目的Launch：

- 启动模拟器；
- 启动监控器；
- 把四个Launch参数传给模拟器；
- 把两个进程的日志输出到终端。

它类似启动脚本或简化的服务编排，但它理解ROS package、node和Parameter。

## 与多应用车载系统的相似点

| 车载系统经验 | 本项目中的对应概念 |
|---|---|
| 独立应用进程 | node executable及其进程 |
| 公共IPC数据结构/IDL | `.msg`接口package |
| 状态广播 | Topic Publisher |
| 接收广播的应用 | Topic Subscriber |
| 应用启动配置 | ROS Parameter |
| 系统启动脚本 | Launch |
| 多模块统一构建入口 | colcon workspace |
| 应用诊断日志 | `RCLCPP_INFO/WARN/ERROR` |

## 主要区别

1. ROS 2节点通过图发现，不需要在业务代码中写死对端地址。
2. Topic是数据流和广播，不是请求/响应RPC。
3. DDS在底层统一处理发现、序列化、传输和QoS。
4. 自定义消息由工具生成语言绑定，而不是手工维护多份序列化代码。
5. package、executable、进程和node是不同概念。
6. ROS工作空间可以统一构建许多package，同时保留清楚的依赖图。
7. ROS 2发行版通常与特定Ubuntu LTS绑定，同一源码需要在目标组合上分别验证。

## 怎样调试本项目

建议按数据流逐层检查：

1. `ros2 node list`：两个node是否存在；
2. `ros2 topic list`：Topic是否存在；
3. `ros2 topic info /device/status`：是否有一个Publisher和一个Subscriber；
4. `ros2 topic echo /device/status`：数据是否到达；
5. 查看模拟器INFO：参数和发布值是否正确；
6. 查看监控器INFO/WARN/ERROR：回调和规则是否执行；
7. `colcon test-result --verbose`：单元测试失败原因。

这与排查车载IPC链路很相似：先确认进程，再确认通道，再确认协议和内容，最后确认业务
判断。
