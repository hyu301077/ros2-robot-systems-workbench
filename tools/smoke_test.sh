#!/usr/bin/env bash

set -euo pipefail

workspace_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
setup_file="${workspace_root}/install/setup.bash"

if [[ ! -r "${setup_file}" ]]; then
  echo "ERROR: ${setup_file} is missing; build the workspace first."
  exit 1
fi

# ROS-generated setup scripts may read optional variables that are unset.
set +u
# shellcheck disable=SC1090
source "${setup_file}"
set -u

for command_name in ros2 timeout setsid; do
  if ! command -v "${command_name}" >/dev/null 2>&1; then
    echo "ERROR: required command is missing: ${command_name}"
    exit 1
  fi
done

if [[ -z "${ROS_DOMAIN_ID:-}" ]]; then
  export ROS_DOMAIN_ID="$((100 + ($$ % 100)))"
fi
export ROS_LOCALHOST_ONLY="${ROS_LOCALHOST_ONLY:-1}"
export RCUTILS_COLORIZED_OUTPUT=0

mkdir -p "${workspace_root}/log"
smoke_directory="$(mktemp -d "${workspace_root}/log/smoke_test.XXXXXX")"
launch_log="${smoke_directory}/launch.log"
echo_log="${smoke_directory}/topic_echo.log"
publish_log="${smoke_directory}/topic_publish.log"
launch_pid=""

# Keep ROS CLI state and launch logs inside this bounded test artifact directory.
# This avoids polluting ~/.ros and also works in restricted CI/sandbox homes.
export ROS_HOME="${smoke_directory}/ros_home"
export ROS_LOG_DIR="${smoke_directory}/ros_logs"
mkdir -p "${ROS_HOME}" "${ROS_LOG_DIR}"

launch_group_alive()
{
  [[ -n "${launch_pid}" ]] && kill -0 -- "-${launch_pid}" 2>/dev/null
}

stop_launch()
{
  if launch_group_alive; then
    kill -INT -- "-${launch_pid}" 2>/dev/null || kill -INT "${launch_pid}" 2>/dev/null || true

    for _ in {1..20}; do
      if ! launch_group_alive; then
        break
      fi
      sleep 0.25
    done

    if launch_group_alive; then
      kill -TERM -- "-${launch_pid}" 2>/dev/null ||
        kill -TERM "${launch_pid}" 2>/dev/null ||
        true

      for _ in {1..20}; do
        if ! launch_group_alive; then
          break
        fi
        sleep 0.25
      done
    fi

    if launch_group_alive; then
      kill -KILL -- "-${launch_pid}" 2>/dev/null ||
        kill -KILL "${launch_pid}" 2>/dev/null ||
        true
    fi
  fi

  if [[ -n "${launch_pid}" ]]; then
    wait "${launch_pid}" 2>/dev/null || true
  fi
  timeout 3s ros2 daemon stop >/dev/null 2>&1 || true
  launch_pid=""
}

show_failure_context()
{
  echo "---- launch output (last 80 lines) ----"
  tail -n 80 "${launch_log}" 2>/dev/null || true
  echo "smoke_artifacts=${smoke_directory}"
}

trap stop_launch EXIT
trap 'exit 130' INT
trap 'exit 143' TERM

setsid ros2 launch device_simulator device_system.launch.py \
  initial_battery_level:=19.0 \
  publish_period_ms:=200 \
  >"${launch_log}" 2>&1 &
launch_pid=$!

discovery_deadline=$((SECONDS + 15))
graph_ready=false
while ((SECONDS < discovery_deadline)); do
  if ! kill -0 "${launch_pid}" 2>/dev/null; then
    echo "ERROR: launch exited before graph discovery completed."
    show_failure_context
    exit 1
  fi

  # Query the graph directly so stale or unavailable ros2 daemon state cannot
  # hide nodes that are already communicating over DDS.
  topic_list="$(
    timeout 3s ros2 topic list --no-daemon --spin-time 1 2>/dev/null || true
  )"
  node_list="$(
    timeout 3s ros2 node list --no-daemon --spin-time 1 2>/dev/null || true
  )"
  if grep -Fxq "/device/status" <<<"${topic_list}" &&
    grep -Fxq "/device_simulator_node" <<<"${node_list}" &&
    grep -Fxq "/device_monitor_node" <<<"${node_list}"
  then
    graph_ready=true
    break
  fi
  sleep 0.25
done

if [[ "${graph_ready}" != "true" ]]; then
  echo "ERROR: expected nodes and /device/status were not discovered in 15 seconds."
  show_failure_context
  exit 1
fi

if ! timeout 10s ros2 topic echo \
  --no-daemon --spin-time 1 \
  /device/status robot_device_interfaces/msg/DeviceStatus --once \
  >"${echo_log}" 2>&1
then
  echo "ERROR: timed out waiting for one /device/status message."
  show_failure_context
  exit 1
fi

if ! grep -Fq "device_id:" "${echo_log}" ||
  ! grep -Fq "battery_level:" "${echo_log}" ||
  ! grep -Fq "temperature:" "${echo_log}"
then
  echo "ERROR: echoed message did not contain the expected DeviceStatus fields."
  show_failure_context
  exit 1
fi

if ! timeout 10s ros2 topic pub --once \
  /device/status robot_device_interfaces/msg/DeviceStatus \
  "{device_id: smoke-hot, battery_level: 80.0, temperature: 75.0, online: true, software_version: smoke-test}" \
  >"${publish_log}" 2>&1
then
  echo "ERROR: failed to inject the high-temperature test message."
  show_failure_context
  exit 1
fi

alert_deadline=$((SECONDS + 10))
alerts_ready=false
while ((SECONDS < alert_deadline)); do
  if grep -Eq "\\[WARN\\].*Low battery:" "${launch_log}" &&
    grep -Eq "\\[ERROR\\].*High temperature:" "${launch_log}"
  then
    alerts_ready=true
    break
  fi
  sleep 0.25
done

if [[ "${alerts_ready}" != "true" ]]; then
  echo "ERROR: expected WARN and ERROR log records were not observed."
  show_failure_context
  exit 1
fi

stop_launch
trap - EXIT INT TERM

echo "PASS: discovered both nodes and /device/status"
echo "PASS: received one DeviceStatus message"
echo "PASS: observed low-battery WARN"
echo "PASS: observed high-temperature ERROR"
echo "smoke_artifacts=${smoke_directory}"
