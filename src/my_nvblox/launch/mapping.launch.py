# from launch import LaunchDescription
# from launch_ros.actions import Node

# def generate_launch_description():
#     return LaunchDescription([
#         # 1. 启动 Intel RealSense D435i
#         # 参数 align_depth.enable 必须为 True，保证深度和彩色画面像素对齐
#         Node(
#             package='realsense2_camera',
#             executable='realsense2_camera_node',
#             name='camera',
#             parameters=[{'align_depth.enable': True}]
#         ),
        
#         # 2. 静态 TF 模拟器 (用于测试连通性)
#         # 注意：实际机器人系统中，这里应当由你的 VSLAM (如 rtabmap) 或里程计节点来提供真实的动态位姿。
#         Node(
#             package='tf2_ros',
#             executable='static_transform_publisher',
#             arguments=['0', '0', '0', '0', '0', '0', 'odom', 'camera_depth_optical_frame']
#         ),

#         # 3. 启动刚才我们写的 nvblox 节点
#         Node(
#             package='my_nvblox',
#             executable='mapper_node',
#             name='mapper_node',
#             output='screen'
#         )
#     ])
from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():
    return LaunchDescription([
        # 1. 启动 Intel RealSense D435i
        Node(
            package='realsense2_camera',
            namespace='camera', # 保持为空，防止话题名变成 /camera/camera/
            executable='realsense2_camera_node',
            name='camera',
            parameters=[{
                'align_depth.enable': True,
                'enable_sync': True
            }]
        ),
        
        # 2. 静态 TF (ROS 2 Humble 标准写法)
        Node(
            package='tf2_ros',
            executable='static_transform_publisher',
            # 明确指定把 odom 连到 camera_link 上
            arguments=['--x', '0', '--y', '0', '--z', '0', '--qx', '0', '--qy', '0', '--qz', '0', '--qw', '1', '--frame-id', 'odom', '--child-frame-id', 'camera_link']
        ),

        # 3. 启动 nvblox 建图节点
        Node(
            package='my_nvblox',
            executable='mapper_node',
            name='mapper_node',
            output='screen'
        )
    ])