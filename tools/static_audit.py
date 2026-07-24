#!/usr/bin/env python3

"""Fast, ROS-independent consistency checks for this learning workspace."""

from __future__ import annotations

import ast
import os
import re
import subprocess
import sys
import xml.etree.ElementTree as ET
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
ERRORS: list[str] = []


def require(condition: bool, message: str) -> None:
    if not condition:
        ERRORS.append(message)


def read(relative_path: str) -> str:
    path = ROOT / relative_path
    require(path.is_file(), f"missing file: {relative_path}")
    if not path.is_file():
        return ""
    return path.read_text(encoding="utf-8")


def audit_packages() -> None:
    expected_packages = {
        "robot_device_interfaces",
        "device_simulator",
        "device_monitor",
    }
    package_files = sorted((ROOT / "src").glob("*/package.xml"))
    actual_packages = {path.parent.name for path in package_files}
    require(
        actual_packages == expected_packages,
        f"package set mismatch: expected {sorted(expected_packages)}, "
        f"found {sorted(actual_packages)}",
    )

    for package_file in package_files:
        package_name = package_file.parent.name
        try:
            root = ET.parse(package_file).getroot()
        except ET.ParseError as error:
            ERRORS.append(f"invalid XML in {package_file.relative_to(ROOT)}: {error}")
            continue

        require(root.attrib.get("format") == "3", f"{package_name}: package format must be 3")
        require(root.findtext("name") == package_name, f"{package_name}: XML name mismatch")
        require(root.findtext("version") == "0.1.0", f"{package_name}: version must be 0.1.0")
        require(root.findtext("license") == "Apache-2.0", f"{package_name}: license mismatch")

        cmake_path = package_file.parent / "CMakeLists.txt"
        cmake = read(str(cmake_path.relative_to(ROOT)))
        require(
            "cmake_minimum_required(VERSION 3.22)" in cmake,
            f"{package_name}: CMake minimum must be 3.22",
        )
        require(f"project({package_name})" in cmake, f"{package_name}: CMake project mismatch")
        require("ament_package()" in cmake, f"{package_name}: missing ament_package()")

    interface_root = ET.parse(
        ROOT / "src/robot_device_interfaces/package.xml"
    ).getroot()
    interface_buildtools = {item.text for item in interface_root.findall("buildtool_depend")}
    require(
        {"ament_cmake", "rosidl_default_generators"} <= interface_buildtools,
        "robot_device_interfaces: generator build dependencies are incomplete",
    )
    require(
        interface_root.findtext("exec_depend") == "rosidl_default_runtime",
        "robot_device_interfaces: missing rosidl_default_runtime",
    )
    require(
        interface_root.findtext("member_of_group") == "rosidl_interface_packages",
        "robot_device_interfaces: missing rosidl_interface_packages membership",
    )

    simulator_root = ET.parse(ROOT / "src/device_simulator/package.xml").getroot()
    simulator_exec = {item.text for item in simulator_root.findall("exec_depend")}
    require(
        {"device_monitor", "launch", "launch_ros", "ros2launch"} <= simulator_exec,
        "device_simulator: launch runtime dependencies are incomplete",
    )

    node_targets = {
        "device_simulator": "device_simulator_node",
        "device_monitor": "device_monitor_node",
    }
    for package_name, node_target in node_targets.items():
        cmake = read(f"src/{package_name}/CMakeLists.txt")
        cmake_without_comments = re.sub(r"#[^\n]*", "", cmake)
        require(
            "ament_target_dependencies" not in cmake_without_comments,
            f"{package_name}: ament_target_dependencies violates the modern CMake policy",
        )
        link_block = re.search(
            rf"target_link_libraries\s*\(\s*{re.escape(node_target)}\s+"
            r"PRIVATE(?P<body>.*?)\)",
            cmake_without_comments,
            flags=re.DOTALL,
        )
        require(link_block is not None, f"{package_name}: missing PRIVATE link block")
        link_body = link_block.group("body") if link_block is not None else ""
        require(
            "rclcpp::rclcpp" in link_body,
            f"{package_name}: node link block is missing rclcpp::rclcpp",
        )
        require(
            "${robot_device_interfaces_TARGETS}" in link_body,
            f"{package_name}: node link block is missing generated interface targets",
        )


def audit_message_contract() -> None:
    message = read("src/robot_device_interfaces/msg/DeviceStatus.msg")
    fields = []
    for line in message.splitlines():
        content = line.split("#", maxsplit=1)[0].strip()
        if content:
            fields.append(content)

    expected_fields = [
        "string device_id",
        "float32 battery_level",
        "float32 temperature",
        "bool online",
        "string software_version",
    ]
    require(fields == expected_fields, f"DeviceStatus fields changed: {fields}")


def audit_cpp_contracts() -> None:
    simulator = read("src/device_simulator/src/device_simulator_node.cpp")
    for fragment in [
        'Node("device_simulator_node")',
        '"/device/status"',
        '"device_id", "robot-001"',
        '"initial_battery_level", 100.0',
        '"software_version", "1.0.0"',
        '"publish_period_ms", 1000',
        "message.online = true",
        "battery_level_ - kBatteryDropPerMessage",
    ]:
        require(fragment in simulator, f"simulator contract missing: {fragment}")

    monitor = read("src/device_monitor/src/device_monitor_node.cpp")
    for fragment in [
        'Node("device_monitor_node")',
        '"/device/status"',
        "RCLCPP_INFO",
        "RCLCPP_WARN",
        "RCLCPP_ERROR",
        "evaluate_alerts",
    ]:
        require(fragment in monitor, f"monitor contract missing: {fragment}")

    evaluator = read("src/device_monitor/src/alert_evaluator.cpp")
    require(
        "battery_level < kLowBatteryThreshold" in evaluator,
        "low-battery comparison must remain strict",
    )
    require(
        "temperature_celsius > kHighTemperatureThreshold" in evaluator,
        "high-temperature comparison must remain strict",
    )

    test_source = read("src/device_monitor/test/test_alert_evaluator.cpp")
    for boundary in ["19.9F", "20.0F", "70.1F", "70.0F", "10.0F, 80.0F"]:
        require(boundary in test_source, f"alert test boundary missing: {boundary}")


def audit_launch() -> None:
    relative_path = "src/device_simulator/launch/device_system.launch.py"
    launch_source = read(relative_path)
    try:
        ast.parse(launch_source, filename=relative_path)
    except SyntaxError as error:
        ERRORS.append(f"invalid launch Python: {error}")

    for fragment in [
        'package="device_simulator"',
        'executable="device_simulator_node"',
        'package="device_monitor"',
        'executable="device_monitor_node"',
        'DeclareLaunchArgument("device_id"',
        'DeclareLaunchArgument("initial_battery_level"',
        'DeclareLaunchArgument("software_version"',
        'DeclareLaunchArgument("publish_period_ms"',
    ]:
        require(fragment in launch_source, f"launch contract missing: {fragment}")

    simulator_cmake = read("src/device_simulator/CMakeLists.txt")
    require("DIRECTORY launch" in simulator_cmake, "launch directory is not installed")


def audit_scripts() -> None:
    scripts = [
        "tools/doctor.sh",
        "tools/ai_context.sh",
        "tools/smoke_test.sh",
        "tools/verify_workspace.sh",
    ]
    for relative_path in scripts:
        path = ROOT / relative_path
        require(path.is_file(), f"missing script: {relative_path}")
        if not path.is_file():
            continue
        require(os.access(path, os.X_OK), f"script is not executable: {relative_path}")
        result = subprocess.run(
            ["bash", "-n", str(path)],
            cwd=ROOT,
            capture_output=True,
            text=True,
            check=False,
        )
        require(
            result.returncode == 0,
            f"bash syntax failed for {relative_path}: {result.stderr.strip()}",
        )

    smoke = read("tools/smoke_test.sh")
    for fragment in [
        "timeout 10s ros2 topic echo",
        "timeout 10s ros2 topic pub --once",
        "Low battery:",
        "High temperature:",
        "kill -INT",
        "kill -TERM",
        "kill -KILL",
    ]:
        require(fragment in smoke, f"smoke test safety/coverage missing: {fragment}")


def audit_docs_and_ci() -> None:
    required_docs = [
        "README.md",
        "docs/architecture.md",
        "docs/learning_notes.md",
        "docs/interview_questions.md",
        "docs/compatibility.md",
        "docs/system_manifest.yaml",
        "LICENSE",
    ]
    for relative_path in required_docs:
        read(relative_path)

    readme = read("README.md")
    for heading in [
        "## 支持环境",
        "## 系统架构",
        "## 三个 package",
        "## `DeviceStatus` 消息",
        "## 编译",
        "## 运行",
        "## 当前实际验证状态",
        "## 常见错误与排查",
    ]:
        require(heading in readme, f"README section missing: {heading}")

    for link in re.findall(r"\[[^\]]+\]\(([^)]+)\)", readme):
        if link.startswith(("http://", "https://", "#")):
            continue
        require((ROOT / link).exists(), f"README relative link is broken: {link}")

    interview = read("docs/interview_questions.md")
    question_count = len(re.findall(r"^## \d+\.", interview, flags=re.MULTILINE))
    require(question_count >= 15, f"only {question_count} interview questions found")

    workflow = read(".github/workflows/ci.yml")
    for fragment in [
        "ubuntu-22.04",
        "ros_distro: humble",
        "ubuntu-26.04",
        "ros_distro: lyrical",
        "rosdep update",
        "tools/verify_workspace.sh",
    ]:
        require(fragment in workflow, f"CI matrix/command missing: {fragment}")

    for action in [
        "actions/checkout",
        "ros-tooling/setup-ros",
        "actions/upload-artifact",
    ]:
        require(
            re.search(
                rf"uses:\s*{re.escape(action)}@[0-9a-f]{{40}}",
                workflow,
            )
            is not None,
            f"CI action is not pinned to a full commit SHA: {action}",
        )


def audit_git_whitespace() -> None:
    ignored_directories = {
        ".git",
        "build",
        "install",
        "log",
        "__pycache__",
    }
    for path in ROOT.rglob("*"):
        if not path.is_file() or any(part in ignored_directories for part in path.parts):
            continue
        try:
            content = path.read_text(encoding="utf-8")
        except UnicodeDecodeError:
            continue
        for line_number, line in enumerate(content.splitlines(), start=1):
            require(
                line == line.rstrip(" \t"),
                f"trailing whitespace: {path.relative_to(ROOT)}:{line_number}",
            )
        require(
            not content or content.endswith("\n"),
            f"missing final newline: {path.relative_to(ROOT)}",
        )

    result = subprocess.run(
        ["git", "diff", "--check"],
        cwd=ROOT,
        capture_output=True,
        text=True,
        check=False,
    )
    require(result.returncode == 0, f"git diff --check failed: {result.stdout.strip()}")


def main() -> int:
    audit_packages()
    audit_message_contract()
    audit_cpp_contracts()
    audit_launch()
    audit_scripts()
    audit_docs_and_ci()
    audit_git_whitespace()

    if ERRORS:
        for error in ERRORS:
            print(f"FAIL: {error}", file=sys.stderr)
        print(f"Static audit failed with {len(ERRORS)} error(s).", file=sys.stderr)
        return 1

    print("PASS: project structure and cross-file contracts are consistent")
    print("NOTE: this static audit does not replace colcon build/test or ROS smoke testing")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
