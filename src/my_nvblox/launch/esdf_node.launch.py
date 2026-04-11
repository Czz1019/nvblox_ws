from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():
    return LaunchDescription([
        Node(
            package='realsense2_camera',
            namespace='', 
            executable='realsense2_camera_node',
            name='camera',
            parameters=[{'align_depth.enable': True, 'enable_sync': True}]
        ),
        
        Node(
            package='tf2_ros',
            executable='static_transform_publisher',
            arguments=['--x', '0', '--y', '0', '--z', '0', 
                       '--qx', '0', '--qy', '0', '--qz', '0', '--qw', '1', 
                       '--frame-id', 'odom', '--child-frame-id', 'camera_link']
        ),

        Node(
            package='my_nvblox',
            executable='esdf_node.py',  # 指向修改后的脚本名
            name='esdf_node',
            output='screen'
        )
    ])