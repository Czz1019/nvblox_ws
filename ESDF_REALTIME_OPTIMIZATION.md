# ESDF 实时性问题分析与优化记录

## 1. 问题现象

工程运行时 ESDF 更新频率较低，通常卡在约 4 Hz。调整查询范围、分辨率等参数后改善不明显，单次“ESDF 完整流程”耗时约 200～250 ms。

本次检查的主要代码路径为：

```text
RealSense 深度/颜色同步
  -> TSDF 与颜色积分
  -> Mesh 更新及发布
  -> ESDF Slice 更新
  -> ESDF Slice 查询
  -> PointCloud2 构造及发布
```

## 2. 根因分析

### 2.1 Mesh 与 ESDF 共用同一个定时回调

原来的 `publish_timer_callback()` 顺序执行：

```cpp
mapper_->updateColorMesh(...);
mesh_publisher_->publish(*mapper_);
mapper_->updateEsdfSlice(...);
esdf_publisher_->publish(*mapper_);
```

整个流程持有同一个 `mapper_mutex_`，并运行在 ROS 2 单线程 executor 中。因此测得的“ESDF 完整耗时”实际还包含：

- Mesh GPU 更新；
- Mesh Device→Host 序列化；
- CUDA stream 同步；
- 大体积 Mesh ROS 消息构造及发布。

这也是调整 ESDF 查询参数后总耗时变化不明显的主要原因之一。

### 2.2 定时器周期限制更新频率

原 launch 文件配置：

```python
'publish_period_ms': 500,
```

该配置理论上最多只能达到 2 Hz。即使外部将周期缩短，当一次回调耗时达到 200～250 ms 时，实际频率仍会被限制在约 4～5 Hz。

### 2.3 CPU 直接访问 Device 内存中的 ESDF block

当前 nvblox 的 `BlockMemoryPoolParams` 默认使用：

```cpp
MemoryType::kDevice
```

原 ESDF 查询代码在 CPU 循环中执行：

```cpp
auto block = esdf_layer.getBlockAtIndex(block_idx);
const auto & voxel = block->voxels[x][y][z];
```

但当前 nvblox 的 `unified_ptr::operator->()` 明确不允许 CPU 解引用 `kDevice` 指针。这条链路既不符合当前内存接口，也无法形成高效的批量 Device→Host 数据传输。

### 2.4 ESDF 输出高度与查询高度不一致

`updateEsdfSlice()` 的默认输出高度为 1.0 m，而原发布器默认查询：

```text
esdf_slice_height = 0.5 m
```

原代码只设置了最大 ESDF 距离，没有将 integrator 的输出高度同步为查询高度。这可能造成查询错误的 block 或得到大量无效数据，也会让部分查询参数调整看起来不敏感。

### 2.5 无新数据时仍可能执行不必要的可视化工作

原实现没有区分：

- ESDF 是否已有新深度帧需要处理；
- Mesh 或 ESDF PointCloud 是否存在订阅者。

因此可能重复更新未变化的地图，或在没有消费者时继续执行 GPU 序列化和 ROS 消息构造。

## 3. 本次修改

### 3.1 拆分 Mesh 和 ESDF 定时器

将原来的统一发布定时器拆分为：

- `esdf_timer_`：负责 ESDF 更新和 ESDF PointCloud 发布；
- `mesh_timer_`：负责 Mesh 更新和发布。

新增参数：

| 参数 | 默认值 | 作用 |
| --- | ---: | --- |
| `esdf_update_period_ms` | 100 ms | ESDF 目标更新周期，理论目标 10 Hz |
| `mesh_update_period_ms` | 1000 ms | Mesh 更新周期，降低其对 ESDF 的干扰 |

旧参数 `publish_period_ms` 暂时保留，用作 `mesh_update_period_ms` 未配置时的兼容默认值。

### 3.2 只在有新深度帧时更新 ESDF

增加 `last_esdf_integrated_frame_count_`，当 TSDF 没有集成新深度帧时直接跳过本次 ESDF 更新：

```cpp
if (integrated_frame_count_ == last_esdf_integrated_frame_count_) {
  return;
}
```

这样可避免对未变化地图重复执行 ESDF 更新。

### 3.3 ESDF 查询改为批量 Device→Host 快照

新查询流程如下：

```text
获得 ESDF block 索引
  -> 筛选与 XY 查询范围及 Z Slice 相交的 blocks
  -> EsdfLayerSerializerGpu 批量复制到 Host
  -> 建立 block index 到序列化数据的哈希索引
  -> 在 Host 快照上完成规则网格查询
  -> 构造 PointCloud2
```

主要收益：

- 不再从 CPU 解引用 Device block；
- 每轮只发生一次批量 GPU 数据传输和同步；
- 只复制查询区域涉及的 blocks，而不是整个 ESDF layer；
- 查询阶段变为普通 Host 内存访问。

### 3.4 对齐 ESDF 输出层和查询层高度

初始化 Mapper 后显式设置：

```cpp
mapper_->esdf_integrator().esdf_slice_height(esdf_slice_height_);
mapper_->esdf_integrator().esdf_slice_min_height(esdf_slice_min_height_);
mapper_->esdf_integrator().esdf_slice_max_height(esdf_slice_max_height_);
```

新增并显式配置的参数：

| 参数 | 默认值 | 作用 |
| --- | ---: | --- |
| `esdf_slice_height` | 0.5 m | ESDF 输出和 PointCloud 查询所在高度 |
| `esdf_slice_min_height` | 0.0 m | 折叠到二维 ESDF 的最低障碍物高度 |
| `esdf_slice_max_height` | 1.0 m | 折叠到二维 ESDF 的最高障碍物高度 |

### 3.5 根据订阅状态跳过可视化开销

Mesh 没有订阅者时，跳过 Mesh 更新、序列化和发布。

ESDF 没有 PointCloud 订阅者时，仍更新内部 ESDF layer，但跳过 ESDF Device→Host 快照、PointCloud2 构造和发布。这样规划模块仍可使用最新 ESDF，同时避免不必要的可视化开销。

### 3.6 增加分段耗时日志

ESDF 主流程日志：

```text
ESDF timing: lock=... ms update=... ms snapshot+publish=... ms total=... ms
```

ESDF 查询和发布细分日志：

```text
ESDF publish timing: device_snapshot+query=... ms cloud=... ms ros_publish=... ms
```

Mesh 日志：

```text
Mesh update+publish: ... ms
```

这些日志均进行了节流，不会每帧刷屏。

## 4. 修改文件

- `src/my_nvblox/src/nvblox_node.cpp`
- `src/my_nvblox/src/esdf_publisher.cpp`
- `src/my_nvblox/src/mesh_publisher.cpp`
- `src/my_nvblox/include/my_nvblox/nvblox_node.hpp`
- `src/my_nvblox/include/my_nvblox/esdf_publisher.hpp`
- `src/my_nvblox/include/my_nvblox/mesh_publisher.hpp`
- `src/my_nvblox/launch/my_nvblox_bringup.launch.py`

## 5. 当前启动参数

launch 文件中的主要配置为：

```python
'voxel_size': 0.05,
'esdf_update_period_ms': 100,
'mesh_update_period_ms': 1000,
'esdf_slice_height': 0.5,
'esdf_slice_min_height': 0.0,
'esdf_slice_max_height': 1.0,
```

ESDF 查询范围和采样分辨率仍使用节点默认值：

```text
esdf_xy_min      = -5.0 m
esdf_xy_max      =  5.0 m
esdf_resolution  =  0.1 m
```

对应约 101 × 101 个查询点。

## 6. 编译与运行

在安装了 ROS 2 Humble、CUDA 和 nvblox 的目标机器上执行：

```bash
cd ~/nvblox_ws
source /opt/ros/humble/setup.bash
colcon build --packages-select my_nvblox --symlink-install
source install/setup.bash
ros2 launch my_nvblox my_nvblox_bringup.launch.py
```

检查 ESDF PointCloud 实际发布频率：

```bash
ros2 topic hz /my_nvblox/static_esdf_pointcloud
```

如果节点命名空间发生变化，可先查找实际 topic：

```bash
ros2 topic list | grep esdf
```

## 7. 如何判断剩余瓶颈

### `update` 耗时较高

说明主要瓶颈在 ESDF GPU 增量计算。可继续检查：

- 地图是否无限增长；
- `voxel_size` 是否过小；
- 最大积分距离是否过大；
- ESDF 最大距离是否超过实际规划需要；
- 是否需要以机器人为中心清理远处 blocks，改为局部地图。

### `device_snapshot+query` 耗时较高

说明查询区域或 Device→Host 数据量仍然过大。可尝试：

- 缩小 `esdf_xy_min` 和 `esdf_xy_max`；
- 降低 PointCloud 可视化频率；
- 适当增大 `esdf_resolution`；
- 后续实现只提取单个 Z voxel plane 的 CUDA kernel，避免复制 block 内其余 Z voxels；
- 如果规划模块运行在 GPU 上，直接保留 GPU ESDF 查询结果，避免回传 Host。

### `cloud` 或 `ros_publish` 耗时较高

说明 PointCloud 构造或 DDS 传输成为瓶颈。可尝试：

- 只发布 `observed` 点；
- 降低可视化发布频率，但保持内部 ESDF 高频更新；
- 使用更紧凑的二维栅格消息；
- 使用进程内通信或共享内存传输。

### 每隔一段时间出现明显抖动

如果抖动周期与 Mesh 更新周期一致，说明 Mesh 仍在竞争 Mapper 锁或 GPU。可以：

- 在 RViz 中关闭 Mesh 显示，使节点自动跳过 Mesh 更新；
- 将 `mesh_update_period_ms` 提高到 2000～5000 ms；
- 将 Mesh 更新移到独立 Mapper 快照或独立进程。

### 分段耗时都很低，但 topic 频率仍然低

这种情况通常说明 ESDF timer 被其他 ROS 回调延迟，重点检查：

- 深度与颜色 ApproximateTime 同步；
- 每帧颜色图像的 CPU 格式转换；
- 深度和颜色 Host→Device 拷贝；
- TSDF 和颜色积分耗时；
- 单线程 executor 的回调排队情况。

后续可考虑将深度积分、颜色/Mesh 和 ESDF 可视化拆到不同 callback group 或工作线程，但 Mapper 写操作仍需保持正确的串行同步。

## 8. 验证状态

本次已完成：

- 代码差异检查：`git diff --check`；
- launch 文件 Python 语法检查；
- nvblox 内存类型、序列化接口和 ESDF Slice 参数的静态核对。

当前检查环境缺少 ROS 2 Humble 开发库，因此尚未在此环境中完成 ROS 目标编译和 RealSense 实机频率测试。最终性能需要在目标机器上根据新增的分段耗时日志和 `ros2 topic hz` 结果确认。

## 9. 预期效果

此次修改消除了 ESDF 更新链路中确定存在的 Mesh 串行开销、错误 Device 指针访问和切片高度不一致问题，并将配置目标调整为 10 Hz。

实际能否稳定达到 10 Hz 取决于：

- 当前地图规模；
- GPU 型号和负载；
- 深度/颜色积分耗时；
- RViz 是否订阅 Mesh；
- ESDF 查询区域大小；
- ROS executor 的调度延迟。

建议首次实机测试时同时记录三类 timing 日志和 ESDF topic 频率，再根据第 7 节判断下一阶段优化方向。
