# nvblox 实时建图与 ROS 2 可视化项目说明

> 适用环境：Ubuntu 22.04、ROS 2 Humble、Python 3.10、NVIDIA CUDA 12、RealSense D435i、cuVSLAM、nvblox_torch。
>
> 本说明以当前规划的 `nvblox_realtime_py` ROS 2 Python 包结构为准。实际代码发生变化时，应同步更新本文件。

## 文档目录

1. 项目概述与目标
2. 系统环境与依赖
3. 项目目录结构
4. 文件职责与模块接口
5. 数据流与线程关系
6. ROS 2 话题、服务与 RViz 显示
7. 首次部署与环境检查
8. 构建、启动与日常操作
9. 可复用命令脚本
10. 参数配置说明
11. 地图保存与结果查看
12. 逐层验证方法
13. 常见故障与解决方向
14. 开发与维护建议
15. 最短运行清单

## 1. 项目概述与目标

本项目面向 Ubuntu 22.04、ROS 2 Humble 和 NVIDIA 独立显卡平台，使用 RealSense D435i 获取双目红外图像与深度图，使用 cuVSLAM 估计相机位姿，并将深度与位姿输入 nvblox_torch 完成 TSDF、Mesh 和 ESDF 构建。ROS 2 包负责将中间结果和地图结果转换为 RViz 可直接显示的标准消息。

```text
RealSense D435i
  -> 左右红外图像 + 深度图
  -> cuVSLAM 双目视觉里程计
  -> 相机位姿 T_world_camera
  -> nvblox_torch TSDF 融合
  -> Mesh / ESDF
  -> ROS 2 Image、Odometry、Path、TF、Marker、PointCloud2
  -> RViz 可视化与地图文件保存
```

### 1.1 预期功能

- 实时获取 D435i 左右红外图像和深度图。
- 通过 cuVSLAM 输出尺度一致的相机六自由度位姿。
- 将深度图和位姿融合进 nvblox TSDF 地图。
- 定期生成三角网格并发布至 RViz。
- 更新 ESDF，并在指定高度查询二维距离切片。
- 发布 Image、CameraInfo、Odometry、Path、TF、PointCloud2 和 Marker。
- 通过服务保存 `.nvblox` 地图和 `.ply` 网格。

## 2. 系统环境与依赖

| 项目 | 当前建议/已验证值 | 说明 |
|---|---|---|
| 操作系统 | Ubuntu 22.04 | ROS 2 Humble 的常用平台 |
| ROS 2 | Humble | 通过 `/opt/ros/humble/setup.bash` 加载 |
| Python | 3.10 / `/usr/bin/python3` | ROS 2 节点入口与 wheel 必须使用同一解释器 |
| GPU | RTX 4060 Laptop GPU | 用于 cuVSLAM、PyTorch 与 nvblox CUDA |
| CUDA | CUDA 12 系列 | cuVSLAM cu12、nvblox_torch cu12 |
| cuVSLAM | 15.0.0+cu12 | wheel 位于 `~/.local/lib/python3.10/site-packages` |
| nvblox_torch | 0.0.10.dev1+cu12ubuntu22 | 负责 TSDF、Mesh 与 ESDF |
| PyTorch | 2.9.1+cu128 | `torch.cuda.is_available()` 应为 True |
| 相机 | Intel RealSense D435i | 建议 USB 3.1/3.2 |

关键原则：ROS 入口脚本、cuVSLAM wheel、nvblox_torch wheel 必须由同一个 Python 3.10 解释器加载；`LD_LIBRARY_PATH` 必须优先指向 wheel 自带的匹配动态库。

## 3. 项目目录结构

```text
~/nvblox_ws/
├── setup_mapping_env.sh
├── nvblox_project_commands.sh
├── output/
│   ├── realtime_map.nvblox
│   └── realtime_mesh.ply
├── src/
│   ├── nvblox/
│   └── nvblox_realtime_py/
│       ├── package.xml
│       ├── setup.py
│       ├── setup.cfg
│       ├── resource/nvblox_realtime_py
│       ├── config/realtime.yaml
│       ├── launch/realtime_mapping.launch.py
│       ├── rviz/nvblox_realtime.rviz
│       └── nvblox_realtime_py/
│           ├── __init__.py
│           ├── realsense_source.py
│           ├── cuvslam_tracker.py
│           ├── fusion_engine.py
│           ├── realtime_pipeline.py
│           ├── ros_mapper_node.py
│           └── standalone_mapper.py
├── build/
├── install/
└── log/
```

建议：源码只放在 `src` 中；`build`、`install` 和 `log` 由 colcon 生成；运行结果统一放在 `output`；环境与日常命令脚本放在工作空间根目录。

## 4. 文件职责与模块接口

| 文件 | 职责 | 主要输入/输出 |
|---|---|---|
| `setup_mapping_env.sh` | 加载 ROS、工作空间、Python 路径和动态库 | 输出当前 shell 的运行环境 |
| `realsense_source.py` | 打开 D435i 并同步采集 | IR-L、IR-R、depth、intrinsics、extrinsics |
| `cuvslam_tracker.py` | 构造双目 Rig 并调用 Tracker | 图像 + 时间戳 -> 4×4 位姿 |
| `fusion_engine.py` | 封装 nvblox Mapper | 深度 + 位姿 -> TSDF/Mesh/ESDF |
| `realtime_pipeline.py` | 组织采集、跟踪和融合 | 保存状态与 `VisualizationSnapshot` |
| `ros_mapper_node.py` | ROS 2 可视化包装器 | 发布图像、TF、Path、Mesh、ESDF |
| `standalone_mapper.py` | 无 RViz 的命令行入口 | 适合性能和保存测试 |
| `realtime.yaml` | 参数集中配置 | 体素、距离、频率、切片范围 |
| `realtime_mapping.launch.py` | 统一启动节点和 RViz | 可用 `use_rviz:=false` |
| `nvblox_realtime.rviz` | RViz 显示预设 | Fixed Frame、Topic、样式 |
| `nvblox_project_commands.sh` | 可重复调用命令集合 | build/run/check/save/stop 等 |




## 5. 首次部署与环境检查

### 5.1 安装可复用命令脚本

```bash
cp nvblox_project_commands.sh ~/nvblox_ws/
chmod +x ~/nvblox_ws/nvblox_project_commands.sh
```

### 5.2 核心依赖检查

```bash
bash ~/nvblox_ws/nvblox_project_commands.sh check
```

必须满足：

- `CUDA available` 为 True。
- cuVSLAM 的 `OdometryConfig` 存在。
- `from nvblox_torch.mapper import Mapper` 成功。
- `libcuvslam.so` 指向当前 wheel 的 cuvslam 目录，而不是旧 Isaac ROS 工作空间。
- RealSense USB Type Descriptor 建议为 3.1 或 3.2。

### 5.3 手动导入测试

```bash
source ~/nvblox_ws/setup_mapping_env.sh
python3 - <<'PY'
import torch
import cuvslam
import nvblox_torch
import pyrealsense2
from nvblox_torch.mapper import Mapper
print("CUDA:", torch.cuda.is_available())
print("GPU:", torch.cuda.get_device_name(0))
print("cuVSLAM:", cuvslam.__file__)
print("nvblox_torch:", nvblox_torch.__file__)
print("Mapper:", Mapper)
PY
```

## 6. 构建、启动与日常操作

### 6.1 一键构建

```bash
bash ~/nvblox_ws/nvblox_project_commands.sh build
```

### 6.2 启动建图与 RViz

```bash
bash ~/nvblox_ws/nvblox_project_commands.sh run
```

### 6.3 无 RViz 启动

```bash
bash ~/nvblox_ws/nvblox_project_commands.sh run-headless
```


## 7. 参数配置说明

| 参数 | 初值 | 作用 | 调整建议 |
|---|---:|---|---|
| `voxel_size_m` | 0.05 | 体素边长 | 更小更精细但显存和计算增加 |
| `max_integration_distance_m` | 5.0 | 最大融合距离 | 室内常用 3~5 m |
| `warmup_ir_frames` | 30 | 开始融合前有效位姿数量 | 跟踪不稳可增加 |
| `esdf_update_hz` | 2.0 | 内部 ESDF 更新频率 | 优先保证 TSDF 实时性 |
| `mesh_publish_hz` | 0.5 | Mesh 发布频率 | 不建议初期超过 1 Hz |
| `esdf_publish_hz` | 1.0 | ESDF 可视化频率 | 查询区域大时降低 |
| `max_mesh_triangles` | 8000 | RViz Marker 三角形上限 | 卡顿时降低至 3000~5000 |
| `esdf_slice_height_m` | 0.5 | ESDF 切片世界高度 | 应落在已建区域 |
| `esdf_side_length_m` | 10.0 | 查询边长 | 调小可降低查询量 |
| `esdf_resolution_m` | 0.1 | 查询点间隔 | 越小越密、越耗时 |
| `depth_cloud_stride` | 8 | 深度点云降采样 | 卡顿时增大 |
| `emitter_enabled` | false | IR 投影器开关 | 双目跟踪阶段优先关闭 |

## 8. 地图保存与结果查看

### 8.1 保存

```bash
bash ~/nvblox_ws/nvblox_project_commands.sh save-map
bash ~/nvblox_ws/nvblox_project_commands.sh save-mesh
bash ~/nvblox_ws/nvblox_project_commands.sh list-output
```

### 8.2 CloudCompare查看 Mesh

CloudCompare /home/czz/nvblox_ws/output/realtime_mesh.ply

输出文件含义：`.nvblox` 是可重新加载的 nvblox 地图格式；`.ply` 是通用三角网格，可用 Open3D、MeshLab 或 CloudCompare 查看。


xiaoyu
