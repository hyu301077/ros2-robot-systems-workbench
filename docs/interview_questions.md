# 围绕V0项目的面试题与参考答案

以下问题只覆盖本仓库V0已经写入源码的设计，不假设存在真实硬件、OTA、Service、
Action、Gazebo或跨设备ROS通信。

## 1. ROS 2在这个项目中解决了什么问题？

它提供节点发现、Topic发布订阅、自定义消息生成、参数、Launch以及统一构建测试工具。
业务代码不再自行实现应用发现、消息序列化和IPC分发。

## 2. 为什么三个package不是三个运行中的应用？

`robot_device_interfaces`只定义`DeviceStatus`消息，不产生可执行程序。
`device_simulator`和`device_monitor`各产生一个可执行程序，所以V0运行时是两个应用
进程、两个node。

## 3. Package、executable、进程和node有什么区别？

package组织源码与依赖；executable是编译产物；进程是操作系统对executable的运行实例；
node是进程中加入ROS图的逻辑参与者。V0让后三者保持1:1，但ROS 2不强制这样做。

## 4. 为什么把`DeviceStatus`放在独立package？

消息是生产者和消费者的公共契约。独立后，两个业务package只依赖接口而不互相依赖，
也能让构建工具按依赖顺序先生成消息代码。

## 5. `DeviceStatus`包含哪些字段？

设备编号`device_id`、电量`battery_level`、温度`temperature`、在线状态`online`和软件
版本`software_version`。电量与温度使用`float32`。

## 6. `/device/status`的数据流是什么？

模拟器的wall timer周期触发回调，构造`DeviceStatus`并由Publisher发布。DDS发现匹配的
Subscriber并分发消息，监控节点的回调随后打印状态和判断告警。

## 7. 为什么Topic名称写成`/device/status`？

开头的`/`使它成为绝对名称，不受node namespace影响。V0用固定绝对名称降低入门复杂度；
大型系统中可通过namespace和remap实现多设备复用。

## 8. 为什么不需要中央消息转发node？

ROS 2下层的DDS已经负责发现、端点匹配、序列化和分发。额外的业务转发node会重复功能，
增加延迟和故障点。它与可选的DDS发现基础设施也不是同一概念。

## 9. Publisher和Subscriber是否直接互相调用？

不是。Publisher只面向Topic和消息类型，Subscriber也只声明Topic、类型和回调。双方通过
ROS 2/RMW/DDS解耦。

## 10. 为什么使用深度为10的QoS？

`rclcpp::QoS(10)`是简单明确的入门设置，提供有限历史深度并沿用默认可靠性。V0没有网络
丢包场景，因此不引入更复杂QoS策略。

## 11. 模拟器有哪些Parameter，类型是什么？

`device_id`和`software_version`是string；`initial_battery_level`在ROS参数层是float64，
C++使用`double`；`publish_period_ms`是int64。写入`float32`消息前显式转换初始电量。

## 12. 为什么第一条消息仍然是100%电量？

模拟器先构造并发布当前值，再执行0.1的下降。这样“初始电量”确实表示第一帧，而不是
第一次定时器触发前就被修改。

## 13. 模拟温度怎样保持合理？

温度从25℃开始，每个周期增加一个-0.25到0.25之间的随机步长，并限制在20℃到40℃。
因此正常模拟不会自然触发70℃高温告警。

## 14. 告警逻辑为什么与ROS回调分开？

`evaluate_alerts()`只接收两个float并返回两个bool，不依赖node、executor或DDS。这样能用
普通C++快速验证阈值；ROS回调只负责消息适配和日志级别映射。

## 15. 电量和温度的边界条件是什么？

电量严格小于20触发，等于20不触发；温度严格大于70触发，等于70不触发。测试同时覆盖
正常值、边界、越界和两种告警同时出现。

## 16. 单元测试通过能否证明WARN和ERROR日志正确？

不能。单元测试只证明`AlertResult`布尔值。还需要启动两个node，捕获
`RCLCPP_WARN`和`RCLCPP_ERROR`输出，才能证明回调与日志级别集成正确。

## 17. Launch文件做了什么？

它声明四个可覆盖参数，启动`device_simulator_node`与`device_monitor_node`，并把日志输出
到屏幕。它没有把两个node放进同一个进程。

## 18. 为什么使用wall timer而不是ROS time timer？

这是一个边缘设备状态模拟器，发布周期应按真实经过时间执行。V0没有仿真时钟，
wall timer语义更直接，也不会因`/clock`暂停而停止。

## 19. `colcon`、`ament_cmake`和CMake如何配合？

CMake描述每个C++ package的target；ament_cmake增加ROS package、依赖和消息生成约定；
colcon遍历整个workspace并按package依赖顺序调用CMake，再统一执行测试。

## 20. 为什么构建后还要`source install/setup.bash`？

构建产物位于workspace overlay。source会更新当前shell的package索引和运行路径，使
`ros2 run`、`ros2 launch`和接口查询找到刚生成的内容。

## 21. 同一份源码怎样支持Humble和Lyrical？

代码使用C++17和两个发行版共同存在的稳定API，保持单一`main`，然后在
Ubuntu 22.04/Humble和Ubuntu 26.04/Lyrical上分别构建测试。二进制不跨系统复用。

## 22. 为什么不测试Humble节点与Lyrical节点跨电脑通信？

项目需求是每台电脑独立运行完整系统。混合发行版ROS图会引入DDS版本、类型支持和工具
兼容风险，不属于V0验收范围。

## 23. 冒烟测试怎样避免永久运行？

脚本使用独立ROS domain、外部超时、后台进程组和退出trap。无论成功还是失败都会先发送
INT，再按需发送TERM和KILL，并等待Launch进程结束。

## 24. 冒烟测试怎样快速触发两个告警？

Launch时把初始电量设为19%，让模拟器立即产生低电量WARN；然后使用
`ros2 topic pub --once`注入75℃的`DeviceStatus`，验证高温ERROR。这样不需要等待正常
模拟自然到达阈值。
