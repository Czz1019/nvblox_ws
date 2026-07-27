from glob import glob
import os

from setuptools import find_packages, setup


package_name = "nvblox_realtime_py"


setup(
    name=package_name,
    version="0.1.0",

    packages=find_packages(
        exclude=["test"]
    ),

    data_files=[
        (
            "share/ament_index/resource_index/packages",
            [
                "resource/" + package_name
            ],
        ),
        (
            "share/" + package_name,
            [
                "package.xml"
            ],
        ),
        (
            os.path.join(
                "share",
                package_name,
                "launch",
            ),
            glob("launch/*.launch.py"),
        ),
        (
            os.path.join(
                "share",
                package_name,
                "config",
            ),
            glob("config/*.yaml"),
        ),
        (
            os.path.join(
                "share",
                package_name,
                "rviz",
            ),
            glob("rviz/*.rviz"),
        ),
    ],

    install_requires=[
        "setuptools",
    ],

    zip_safe=True,

    maintainer="czz",
    maintainer_email="zhizhengcao1019@gmail.com",

    description=(
        "RealSense, cuVSLAM and nvblox "
        "realtime mapping package"
    ),

    license="Apache-2.0",

    entry_points={
        "console_scripts": [
            (
                "standalone_mapper = "
                "nvblox_realtime_py."
                "standalone_mapper:main"
            ),
            (
                "ros_mapper_node = "
                "nvblox_realtime_py."
                "ros_mapper_node:main"
            ),
        ],
    },
)
