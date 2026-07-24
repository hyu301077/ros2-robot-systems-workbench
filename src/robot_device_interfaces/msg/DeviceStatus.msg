# A stable logical identifier, independent of the computer running the node.
string device_id

# Remaining battery in percent, expected in the range [0.0, 100.0].
float32 battery_level

# Device temperature in degrees Celsius.
float32 temperature

# Whether the simulated device is currently available.
bool online

# Software version reported by the device, for example "1.0.0".
string software_version
