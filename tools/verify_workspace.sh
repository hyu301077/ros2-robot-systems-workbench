#!/usr/bin/env bash

set -euo pipefail

workspace_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

if [[ ! -r /etc/os-release ]]; then
  echo "ERROR: /etc/os-release is not readable."
  exit 1
fi

# shellcheck disable=SC1091
source /etc/os-release

case "${VERSION_ID:-unknown}" in
  "22.04")
    expected_ros="humble"
    ;;
  "26.04")
    expected_ros="lyrical"
    ;;
  *)
    echo "ERROR: Ubuntu ${VERSION_ID:-unknown} is outside the supported matrix."
    exit 1
    ;;
esac

ros_setup="/opt/ros/${expected_ros}/setup.bash"
if [[ ! -r "${ros_setup}" ]]; then
  echo "ERROR: missing ${ros_setup}; install the matching ROS 2 environment first."
  exit 1
fi

# Humble's generated setup script reads optional variables before expanding
# defaults, so temporarily disable nounset while sourcing ROS environments.
set +u
# shellcheck disable=SC1090
source "${ros_setup}"
set -u
"${workspace_root}/tools/doctor.sh"

cd "${workspace_root}"

echo "==> Building workspace for ROS 2 ${expected_ros}"
colcon build --symlink-install

set +u
# shellcheck disable=SC1091
source "${workspace_root}/install/setup.bash"
set -u

echo "==> Running workspace tests"
colcon test

echo "==> Reporting test results"
colcon test-result --verbose

echo "==> Running bounded integration smoke test"
"${workspace_root}/tools/smoke_test.sh"
