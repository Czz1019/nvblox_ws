import os

from ament_index_python.packages import get_package_share_directory

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.conditions import IfCondition
from launch.substitutions import LaunchConfiguration

from launch_ros.actions import Node


def generate_launch_description():
    package_share = get_package_share_directory(
        "nvblox_realtime_py"
    )

    default_parameter_file = os.path.join(
        package_share,
        "config",
        "realtime.yaml",
    )

    default_rviz_file = os.path.join(
        package_share,
        "rviz",
        "nvblox_realtime.rviz",
    )

    parameter_file = LaunchConfiguration(
        "parameter_file"
    )

    rviz_config = LaunchConfiguration(
        "rviz_config"
    )

    use_rviz = LaunchConfiguration(
        "use_rviz"
    )

    mapper_node = Node(
        package="nvblox_realtime_py",
        executable="ros_mapper_node",
        name="nvblox_realtime_node",
        output="screen",
        emulate_tty=True,
        parameters=[
            parameter_file,
        ],
    )

    rviz_node = Node(
        package="rviz2",
        executable="rviz2",
        name="nvblox_realtime_rviz",
        output="screen",
        arguments=[
            "-d",
            rviz_config,
        ],
        condition=IfCondition(use_rviz),
    )

    return LaunchDescription([
        DeclareLaunchArgument(
            "parameter_file",
            default_value=default_parameter_file,
            description="Realtime mapping parameter file",
        ),
        DeclareLaunchArgument(
            "rviz_config",
            default_value=default_rviz_file,
            description="RViz configuration file",
        ),
        DeclareLaunchArgument(
            "use_rviz",
            default_value="true",
            description="Whether to start RViz",
        ),
        mapper_node,
        rviz_node,
    ])
