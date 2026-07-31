# 设备的逻辑编号。它描述“是哪台设备”，不等同于运行节点的电脑主机名。
string device_id

# 剩余电量百分比，约定范围为 [0.0, 100.0]。
float32 battery_level

# 设备温度，单位为摄氏度。
float32 temperature

# 设备当前是否在线；V0 模拟器始终发布 true。
bool online

# 设备上报的软件版本，例如 "1.0.0"。
string software_version
