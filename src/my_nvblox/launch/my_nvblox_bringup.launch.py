# from launch import LaunchDescription
# from launch_ros.actions import Node

# def generate_launch_description():
#     realsense_node = Node(
#         package='realsense2_camera',
#         executable='realsense2_camera_node',
#         output='screen',
#         namespace='',
#         parameters=[{
#             'enable_color': True,
#             'enable_depth': True,
#             'align_depth.enable': True,
#             'rgb_camera.profile': '1280x720x15',
#             'depth_module.profile': '1280x720x15',
#             'pointcloud.enable': False,
#             'enable_infra1': False,
#             'enable_infra2': False,
#         }]
#     )

#     static_tf_node = Node(
#         package='tf2_ros',
#         executable='static_transform_publisher',
#         name='odom_to_camera_link_broadcaster',
#         arguments=[
#             '0', '0', '0',
#             '0', '0', '0',
#             'odom', 'camera_link'
#         ],
#         output='screen'
#     )

#     my_nvblox_node = Node(
#         package='my_nvblox',
#         executable='nvblox_node',
#         name='my_nvblox',
#         output='screen',
#         parameters=[{
#             'global_frame': 'odom',
#             'camera_frame': 'camera_color_optical_frame',
#             'depth_topic': '/camera/aligned_depth_to_color/image_raw',
#             'color_topic': '/camera/color/image_raw',
#             'camera_info_topic': '/camera/color/camera_info',
#             'voxel_size': 0.05,
#             'publish_period_ms': 500,
#             'esdf_slice_height': 0.5,
#             'esdf_xy_min': -5.0,
#             'esdf_xy_max': 5.0,
#             'esdf_resolution': 0.1,
#         }]
#     )

#     return LaunchDescription([
#         realsense_node,
#         static_tf_node,
#         my_nvblox_node
#     ])

# from launch import LaunchDescription
# from launch_ros.actions import Node

# def generate_launch_description():
#     realsense_node = Node(
#         package='realsense2_camera',
#         executable='realsense2_camera_node',
#         namespace='',
#         output='screen',
#         parameters=[{
#             'enable_color': True,
#             'enable_depth': True,
#             'align_depth.enable': True,

#             # 第一阶段先固定 profile，优先稳定
#             'rgb_camera.profile': '1280x720x15',
#             'depth_module.profile': '1280x720x15',

#             'pointcloud.enable': False,
#             'enable_infra1': False,
#             'enable_infra2': False,
#         }]
#     )

#     static_tf_node = Node(
#         package='tf2_ros',
#         executable='static_transform_publisher',
#         name='odom_to_camera_link_broadcaster',
#         arguments=[
#             '0', '0', '0',
#             '0', '0', '0',
#             'odom', 'camera_link'
#         ],
#         output='screen'
#     )

#     my_nvblox_node = Node(
#         package='my_nvblox',
#         executable='nvblox_node',
#         name='my_nvblox',
#         output='screen',
#         parameters=[{
#             'global_frame': 'odom',
#             'camera_frame': 'camera_color_optical_frame',

#             'depth_topic': '/camera/aligned_depth_to_color/image_raw',
#             'color_topic': '/camera/color/image_raw',
#             'camera_info_topic': '/camera/color/camera_info',

#             'voxel_size': 0.05,
#             'publish_period_ms': 500,

#             'esdf_slice_height': 0.5,
#             'esdf_xy_min': -5.0,
#             'esdf_xy_max': 5.0,
#             'esdf_resolution': 0.1,
#         }]
#     )

#     return LaunchDescription([
#         realsense_node,
#         static_tf_node,
#         my_nvblox_node
#     ])
from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    realsense_node = Node(
        package='realsense2_camera',
        executable='realsense2_camera_node',
        namespace='',
        output='screen',
        parameters=[{
            'enable_color': True,
            'enable_depth': True,
            'align_depth.enable': True,
            'rgb_camera.profile': '1280x720x15',
            'depth_module.profile': '1280x720x15',
            'pointcloud.enable': False,
            'enable_infra1': False,
            'enable_infra2': False,
        }]
    )

    static_tf_node = Node(
        package='tf2_ros',
        executable='static_transform_publisher',
        name='odom_to_camera_link_broadcaster',
        arguments=[
            '0', '0', '0',
            '0', '0', '0',
            'odom', 'camera_link'
        ],
        output='screen'
    )

    my_nvblox_node = Node(
        package='my_nvblox',
        executable='nvblox_node',
        name='my_nvblox',
        output='screen',
        parameters=[{
            'global_frame': 'odom',
            'camera_frame': 'camera_color_optical_frame',
            'depth_topic': '/camera/aligned_depth_to_color/image_raw',
            'color_topic': '/camera/color/image_raw',
            'camera_info_topic': '/camera/color/camera_info',
            'voxel_size': 0.05,
            # ESDF is latency-sensitive; mesh visualization is not.
            'esdf_update_period_ms': 100,
            'mesh_update_period_ms': 1000,
            'esdf_slice_height': 0.5,
            'esdf_slice_min_height': 0.0,
            'esdf_slice_max_height': 1.0,
        }]
    )

    return LaunchDescription([
        realsense_node,
        static_tf_node,
        my_nvblox_node
    ])
