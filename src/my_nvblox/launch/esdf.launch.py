from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():
    return LaunchDescription([
        Node(
            package='my_nvblox',
            executable='esdf_node',
            name='my_nvblox_esdf_node',
            output='screen',
            # 如果你的相机话题不一样，可以在这里重映射
            # remappings=[
            #     ('/camera/depth/image_raw', '/your/actual/depth_topic'),
            #     ('/camera/depth/camera_info', '/your/actual/info_topic'),
            # ]
        )
    ])