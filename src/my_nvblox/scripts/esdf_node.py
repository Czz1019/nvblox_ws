#!/usr/bin/env python3
import rclpy
from rclpy.node import Node
from sensor_msgs.msg import Image, CameraInfo, PointCloud2, PointField
import message_filters
from cv_bridge import CvBridge
import tf2_ros
from rclpy.time import Time
from sensor_msgs_py import point_cloud2
from std_msgs.msg import Header

import numpy as np
import torch
import struct
import matplotlib

# nvblox_torch 核心库
from nvblox_torch.mapper import Mapper
from nvblox_torch.constants import constants

class LiveNvbloxNode(Node):
    def __init__(self):
        super().__init__('esdf_node')
        
        # 1. 初始化 GPU Mapper (体素大小 0.05 米)
        self.voxel_size = 0.05
        self.mapper = Mapper(self.voxel_size, torch.device('cuda:0'))
        self.bridge = CvBridge()

        # 2. 初始化 TF 监听器
        self.tf_buffer = tf2_ros.Buffer()
        self.tf_listener = tf2_ros.TransformListener(self.tf_buffer, self)

        # 3. 订阅 D435i 数据并做时间同步 (允许 0.1 秒的误差)
        self.sub_depth = message_filters.Subscriber(self, Image, '/camera/aligned_depth_to_color/image_raw')
        self.sub_color = message_filters.Subscriber(self, Image, '/camera/color/image_raw')
        self.sub_info = message_filters.Subscriber(self, CameraInfo, '/camera/aligned_depth_to_color/camera_info')
        
        self.ts = message_filters.ApproximateTimeSynchronizer(
            [self.sub_depth, self.sub_color, self.sub_info], queue_size=10, slop=0.1)
        self.ts.registerCallback(self.image_callback)

        # 4. 初始化发布者
        self.esdf_pub = self.create_publisher(PointCloud2, '~/esdf_pointcloud', 10)
        self.get_logger().info("ESDF Python Node Started! 正在等待 D435i 数据流...")

    def transform_to_matrix(self, trans):
        """辅助函数：将 ROS TF 转换为 4x4 齐次变换矩阵"""
        t = trans.transform.translation
        q = trans.transform.rotation
        x, y, z, w = q.x, q.y, q.z, q.w
        
        R = np.array([
            [1 - 2*(y**2 + z**2), 2*(x*y - w*z), 2*(x*z + w*y)],
            [2*(x*y + w*z), 1 - 2*(x**2 + z**2), 2*(y*z - w*x)],
            [2*(x*z - w*y), 2*(y*z + w*x), 1 - 2*(x**2 + y**2)]
        ])
        T = np.eye(4)
        T[:3, :3] = R
        T[0, 3] = t.x
        T[1, 3] = t.y
        T[2, 3] = t.z
        return torch.tensor(T, device='cuda', dtype=torch.float32)

    def image_callback(self, depth_msg, color_msg, info_msg):
        # --- A. 获取相机位姿 ---
        try:
            trans = self.tf_buffer.lookup_transform(
                'odom', depth_msg.header.frame_id, depth_msg.header.stamp, rclpy.duration.Duration(seconds=0.1))
        except tf2_ros.TransformException as e:
            self.get_logger().warn(f"TF 树断裂或未就绪: {e}")
            return

        pose_matrix_tensor = self.transform_to_matrix(trans)

        # --- B. 准备内参和图像 Tensor ---
        intrinsics = torch.tensor([
            [info_msg.k[0], 0, info_msg.k[2]],
            [0, info_msg.k[4], info_msg.k[5]],
            [0, 0, 1]
        ], device='cuda', dtype=torch.float32)

        # D435i 深度图通常是 16UC1，需要转为浮点数的米
        cv_depth = self.bridge.imgmsg_to_cv2(depth_msg, desired_encoding='16UC1')
        depth_tensor = torch.tensor(cv_depth.astype(np.float32) / 1000.0, device='cuda')

        cv_color = self.bridge.imgmsg_to_cv2(color_msg, desired_encoding='rgb8')
        color_tensor = torch.tensor(cv_color, device='cuda', dtype=torch.uint8)

        # --- C. 送入 GPU 进行建图融合 ---
        self.mapper.integrate_depth(depth_tensor, pose_matrix_tensor, intrinsics)
        self.mapper.integrate_color(color_tensor, pose_matrix_tensor, intrinsics)

        # --- D. 更新 ESDF 并提取可视化切片 ---
        self.mapper.update_esdf()
        
        # 1. 生成查询网格 (在机器人当前位置前方生成 4x4 米的网格，高度 Z=0.5m)
        grid_range = 2.0
        x = np.arange(-grid_range, grid_range, self.voxel_size)
        y = np.arange(-grid_range, grid_range, self.voxel_size)
        xx, yy = np.meshgrid(x, y)
        zz = np.full_like(xx, 0.5) 
        
        query_pts_np = np.stack([xx, yy, zz], axis=-1).reshape(-1, 3)
        query_grid_xyz_m = torch.tensor(query_pts_np, device='cuda', dtype=torch.float32)
        
        # 2. 从 GPU 查询距离数据
        sdf_values = self.mapper.query_differentiable_layer(constants.QueryType.ESDF, query_grid_xyz_m)
        
        # 3. 过滤出有效数据
        valid_mask = torch.logical_not(sdf_values == constants.esdf_unknown_distance())
        valid_xyz = query_grid_xyz_m[valid_mask].cpu().numpy()
        valid_sdf = sdf_values[valid_mask].cpu().numpy()
        
        # 4. 打包为 ROS 2 点云发布
        if len(valid_xyz) > 0:
            cmap = matplotlib.colormaps.get_cmap('plasma')
            sdf_normalized = np.clip(valid_sdf, 0.0, 1.0) 
            colors = cmap(sdf_normalized)[:, :3] * 255.0
            
            points_for_ros = []
            for i in range(len(valid_xyz)):
                r, g, b = int(colors[i][0]), int(colors[i][1]), int(colors[i][2])
                rgb = struct.unpack('I', struct.pack('BBBB', b, g, r, 255))[0]
                points_for_ros.append([valid_xyz[i][0], valid_xyz[i][1], valid_xyz[i][2], rgb])
            
            header = Header()
            header.stamp = depth_msg.header.stamp
            header.frame_id = 'odom'
            
            fields = [
                PointField(name='x', offset=0, datatype=PointField.FLOAT32, count=1),
                PointField(name='y', offset=4, datatype=PointField.FLOAT32, count=1),
                PointField(name='z', offset=8, datatype=PointField.FLOAT32, count=1),
                PointField(name='rgb', offset=12, datatype=PointField.UINT32, count=1),
            ]
            
            pc2_msg = point_cloud2.create_cloud(header, fields, points_for_ros)
            self.esdf_pub.publish(pc2_msg)
            # self.get_logger().info(f"ESDF 切片已发布: 包含 {len(valid_xyz)} 个点。")

def main(args=None):
    rclpy.init(args=args)
    node = LiveNvbloxNode()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()

if __name__ == '__main__':
    main()