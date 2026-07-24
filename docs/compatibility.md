# Compatibility policy

## Target matrix

| Role | Operating system | ROS 2 | Support policy |
|---|---|---|---|
| Terminal 1 | Ubuntu 22.04 | Humble | Compatibility target |
| Terminal 2 | Ubuntu 26.04 | Lyrical | Long-term primary target |

The repository provides source compatibility, not portable binaries. Each
terminal builds and runs the complete system locally with its matching ROS 2
installation.

Cross-device ROS communication and mixed Humble/Lyrical runtime graphs are
explicitly out of scope. GitHub synchronizes source code only.

## Implementation rules

- C++17 is the common language baseline.
- Prefer stable `rclcpp`, ROS interface generation, parameters, topics, launch,
  and `ament_cmake` APIs present in both distributions.
- Keep a single implementation and a single `main` branch.
- Add a small, isolated compatibility adapter only after a real build proves an
  API difference; do not scatter distribution checks through business logic.
- Verify every new ROS dependency on both distributions before accepting it.
- Keep interface definitions identical on both targets.

## Build-system compatibility

The two C++ node packages link with the modern imported target
`rclcpp::rclcpp` and the generated target list exported through
`${robot_device_interfaces_TARGETS}`. This form has passed both Humble and
Lyrical CI and keeps ROS distribution checks out of the C++ source.

The older `ament_target_dependencies()` helper is intentionally not used.
Although it is available after `find_package(ament_cmake)` in Humble, the
current Lyrical configuration does not expose that helper through the same
package lookup. The repository's static audit rejects it to prevent this
compatibility regression from returning.

## Lifecycle

ROS 2 Humble targets Ubuntu 22.04 and reaches end of life in May 2027. ROS 2
Lyrical targets Ubuntu 26.04 and reaches end of life in May 2031. Terminal 1
therefore has a planned future migration, while Terminal 2 is the long-term
baseline.

Official references:

- <https://docs.ros.org/en/humble/Releases/Release-Humble-Hawksbill.html>
- <https://docs.ros.org/en/lyrical/Releases/Release-Lyrical-Luth.html>

## Current verification status

- Terminal 1 OS detection: completed; Ubuntu 22.04.5 confirmed.
- Terminal 1 ROS 2 Humble build, unit test and bounded runtime smoke test:
  passed on Ubuntu 22.04.5; see
  [validation_terminal1.md](validation_terminal1.md).
- GitHub Actions run `30076049851`: Humble/Ubuntu 22.04 and Lyrical/Ubuntu
  26.04 dependency checks, builds, unit tests and bounded smoke tests passed.
- Terminal 2 ROS 2 Lyrical build: not run in this workspace.
- Cross-device communication: intentionally not applicable.
