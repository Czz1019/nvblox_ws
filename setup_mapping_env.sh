#!/usr/bin/env bash

# 1. ROS 2
source /opt/ros/humble/setup.bash

# 2. 当前ROS 2工作空间
if [ -f "$HOME/nvblox_ws/install/setup.bash" ]; then
    source "$HOME/nvblox_ws/install/setup.bash"
fi

export PATH="$HOME/.local/bin:$PATH"

USER_SITE="$HOME/.local/lib/python3.10/site-packages"
export PYTHONPATH="$USER_SITE:${PYTHONPATH:-}"

# --------------------------------------------------
# cuVSLAM wheel
# --------------------------------------------------
export CUVSLAM_DIR="$USER_SITE/cuvslam"

# --------------------------------------------------
# nvblox_torch wheel
# --------------------------------------------------
export NVBLOX_TORCH_DIR="$USER_SITE/nvblox_torch"

NVBLOX_WHEEL_LIBS="$(
    find "$NVBLOX_TORCH_DIR/lib" \
        \( -type f -o -type l \) \
        -name '*.so*' \
        -printf '%h\n' \
        2>/dev/null \
        | sort -u \
        | paste -sd: -
)"

# --------------------------------------------------
# PyTorch动态库
# --------------------------------------------------
TORCH_LIB_DIR="$(
    /usr/bin/python3 - <<'PY'
import os
import torch

print(
    os.path.join(
        os.path.dirname(torch.__file__),
        "lib",
    )
)
PY
)"

# --------------------------------------------------
# 删除旧Isaac ROS和旧nvblox动态库路径
# --------------------------------------------------
FILTERED_LD_LIBRARY_PATH="$(
    printf '%s' "${LD_LIBRARY_PATH:-}" \
        | tr ':' '\n' \
        | grep -v -E \
          'isaac_ros-dev|isaac_ros_visual_slam|/usr/local/lib$|nvblox_ws/build|nvblox_ws/install/nvblox|nvblox_ws/src/nvblox/build' \
        | awk 'NF && !seen[$0]++' \
        | paste -sd: -
)"

# wheel自带库必须排在最前面
export LD_LIBRARY_PATH="$CUVSLAM_DIR:$NVBLOX_WHEEL_LIBS:$TORCH_LIB_DIR:/usr/local/cuda/lib64${FILTERED_LD_LIBRARY_PATH:+:$FILTERED_LD_LIBRARY_PATH}"

echo "Python: $(which python3)"
echo "cuVSLAM: $CUVSLAM_DIR"
echo "nvblox_torch: $NVBLOX_TORCH_DIR"
echo "PyTorch libs: $TORCH_LIB_DIR"
