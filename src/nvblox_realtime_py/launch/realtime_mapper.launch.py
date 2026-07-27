import os

from ament_index_python.packages import (
    get_package_share_directory,
)
from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    package_share = (
        get_package_share_directory(
            "nvblox_realtime_py"
        )
    )

    configuration_file = os.path.join(
        package_share,
        "config",
        "realtime.yaml",
    )

    mapper_node = Node(
        package="nvblox_realtime_py",
        executable="ros_mapper_node",
        name="nvblox_realtime_node",
        output="screen",
        emulate_tty=True,
        parameters=[
            configuration_file
        ],
    )

    return LaunchDescription([
        mapper_node,
    ])
