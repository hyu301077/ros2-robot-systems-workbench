#!/usr/bin/env bash

set -u

if [[ ! -r /etc/os-release ]]; then
  echo "ERROR: /etc/os-release is not readable."
  exit 1
fi

# shellcheck disable=SC1091
source /etc/os-release

case "${VERSION_ID:-unknown}" in
  "22.04")
    expected_ros="humble"
    terminal_name="terminal_1"
    ;;
  "26.04")
    expected_ros="lyrical"
    terminal_name="terminal_2"
    ;;
  *)
    expected_ros="unsupported"
    terminal_name="unknown"
    ;;
esac

echo "terminal=${terminal_name}"
echo "ubuntu=${VERSION_ID:-unknown} (${VERSION_CODENAME:-unknown})"
echo "expected_ros=${expected_ros}"
echo "active_ros=${ROS_DISTRO:-not_sourced}"

missing=0

for command_name in git g++ cmake colcon ros2; do
  if command -v "${command_name}" >/dev/null 2>&1; then
    command_path="$(command -v "${command_name}")"
    echo "${command_name}=found:${command_path}"
  else
    echo "${command_name}=missing"
    missing=1
  fi
done

if command -v gh >/dev/null 2>&1; then
  echo "gh=found:$(command -v gh)"
else
  echo "gh=missing (required only for GitHub publishing)"
fi

if [[ "${expected_ros}" == "unsupported" ]]; then
  echo "ERROR: this Ubuntu version is outside the supported project matrix."
  exit 1
fi

expected_setup="/opt/ros/${expected_ros}/setup.bash"
if [[ -r "${expected_setup}" ]]; then
  echo "ros_setup=found:${expected_setup}"
else
  echo "ros_setup=missing:${expected_setup}"
  missing=1
fi

if [[ "${ROS_DISTRO:-}" != "${expected_ros}" ]]; then
  echo "ros_environment=not_ready"
  echo "hint=source ${expected_setup}"
  missing=1
else
  echo "ros_environment=ready"
fi

if ((missing != 0)); then
  echo "result=NOT_READY"
  exit 1
fi

echo "result=READY"
