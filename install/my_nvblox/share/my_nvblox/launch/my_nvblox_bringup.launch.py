from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():
    return LaunchDescription([
        # 1. 传感器输入层：启动 RealSense D435i
        # 作用：开启深度与彩色对齐，开启时间戳同步，确保图像质量
        Node(
            package='realsense2_camera',
            executable='realsense2_camera_node',
            name='camera',
            namespace='',
            parameters=[{
                'align_depth.enable': True,
                'enable_sync': True,
            }],
            output='screen'
        ),

        # 2. 空间定位层：发布静态 TF (odom -> camera_link)
        # 作用：在真实 SLAM 接入前提供一个原点不变的坐标系，避免底层建图引擎因缺少位姿而崩溃
        Node(
            package='tf2_ros',
            executable='static_transform_publisher',
            name='static_transform_publisher',
            # 参数依次为：x y z yaw pitch roll frame_id child_frame_id
            arguments=['0', '0', '0', '0', '0', '0', 'odom', 'camera_link'],
            output='screen'
        ),

        # 3. 3D 语义建图层：启动自定义的 nvblox 核心节点
        # 作用：接收图像与 TF，送入 GPU 进行体素建图
        Node(
            package='my_nvblox',
            executable='nvblox_node',
            name='nvblox_node',
            output='screen'
        )
    ])