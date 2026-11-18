#!/usr/bin/env bash
set -e
robot_project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
source "$robot_project_dir/scripts/ros-env.sh"
cd "$robot_project_dir"
colcon build --base-paths . --symlink-install --cmake-args -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
