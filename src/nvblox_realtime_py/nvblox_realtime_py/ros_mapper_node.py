from __future__ import annotations

from collections import deque
import threading
import time

import numpy as np
import rclpy

from geometry_msgs.msg import (
    Pose,
    PoseStamped,
    TransformStamped,
)
from nav_msgs.msg import Odometry, Path
from rclpy.node import Node
from scipy.spatial.transform import Rotation
from std_srvs.srv import Trigger
from tf2_ros import TransformBroadcaster

from .realtime_pipeline import (
    RealtimeMappingPipeline,
)


def matrix_to_pose_message(
    matrix: np.ndarray,
) -> Pose:
    pose = Pose()

    quaternion = (
        Rotation
        .from_matrix(matrix[:3, :3])
        .as_quat()
    )

    pose.position.x = float(
        matrix[0, 3]
    )

    pose.position.y = float(
        matrix[1, 3]
    )

    pose.position.z = float(
        matrix[2, 3]
    )

    pose.orientation.x = float(
        quaternion[0]
    )

    pose.orientation.y = float(
        quaternion[1]
    )

    pose.orientation.z = float(
        quaternion[2]
    )

    pose.orientation.w = float(
        quaternion[3]
    )

    return pose


class RealtimeMapperNode(Node):
    def __init__(self) -> None:
        super().__init__(
            "nvblox_realtime_node"
        )

        self._declare_parameters()

        self._world_frame = str(
            self.get_parameter(
                "world_frame"
            ).value
        )

        self._camera_frame = str(
            self.get_parameter(
                "camera_frame"
            ).value
        )

        self._map_output_path = str(
            self.get_parameter(
                "map_output_path"
            ).value
        )

        self._mesh_output_path = str(
            self.get_parameter(
                "mesh_output_path"
            ).value
        )

        self._pipeline = (
            RealtimeMappingPipeline(
                voxel_size_m=float(
                    self.get_parameter(
                        "voxel_size_m"
                    ).value
                ),
                max_integration_distance_m=float(
                    self.get_parameter(
                        "max_integration_distance_m"
                    ).value
                ),
                esdf_update_hz=float(
                    self.get_parameter(
                        "esdf_update_hz"
                    ).value
                ),
                warmup_ir_frames=int(
                    self.get_parameter(
                        "warmup_ir_frames"
                    ).value
                ),
                max_pending_depth_frames=int(
                    self.get_parameter(
                        "max_pending_depth_frames"
                    ).value
                ),
            )
        )

        self._odometry_publisher = (
            self.create_publisher(
                Odometry,
                "~/odometry",
                10,
            )
        )

        self._path_publisher = (
            self.create_publisher(
                Path,
                "~/path",
                10,
            )
        )

        self._tf_broadcaster = (
            TransformBroadcaster(self)
        )

        maximum_path_poses = int(
            self.get_parameter(
                "maximum_path_poses"
            ).value
        )

        self._path_poses: deque[
            PoseStamped
        ] = deque(
            maxlen=max(
                100,
                maximum_path_poses,
            )
        )

        self._latest_pose_lock = (
            threading.Lock()
        )

        self._latest_pose: (
            tuple[int, np.ndarray] | None
        ) = None

        self._last_published_device_time = None

        self._stop_event = threading.Event()
        self._worker_exception: (
            Exception | None
        ) = None

        self._worker_thread = threading.Thread(
            target=self._mapping_worker,
            name="nvblox_mapping_worker",
            daemon=True,
        )

        self._worker_thread.start()

        self._publish_timer = self.create_timer(
            1.0 / 30.0,
            self._publish_latest_pose,
        )

        self._status_timer = self.create_timer(
            2.0,
            self._print_status,
        )

        self._save_map_service = (
            self.create_service(
                Trigger,
                "~/save_map",
                self._save_map_callback,
            )
        )

        self._save_mesh_service = (
            self.create_service(
                Trigger,
                "~/save_mesh",
                self._save_mesh_callback,
            )
        )

        self._closed = False

        self.get_logger().info(
            "RealSense + cuVSLAM + "
            "nvblox Python node started"
        )

    def _declare_parameters(self) -> None:
        self.declare_parameter(
            "voxel_size_m",
            0.05,
        )

        self.declare_parameter(
            "max_integration_distance_m",
            5.0,
        )

        self.declare_parameter(
            "esdf_update_hz",
            5.0,
        )

        self.declare_parameter(
            "warmup_ir_frames",
            60,
        )

        self.declare_parameter(
            "max_pending_depth_frames",
            64,
        )

        self.declare_parameter(
            "maximum_path_poses",
            2000,
        )

        self.declare_parameter(
            "world_frame",
            "odom",
        )

        self.declare_parameter(
            "camera_frame",
            "camera_left_ir",
        )

        self.declare_parameter(
            "map_output_path",
            (
                "/home/czz/nvblox_ws/"
                "output/realtime_map.nvblox"
            ),
        )

        self.declare_parameter(
            "mesh_output_path",
            (
                "/home/czz/nvblox_ws/"
                "output/realtime_mesh.ply"
            ),
        )

    def _mapping_worker(self) -> None:
        try:
            while not self._stop_event.is_set():
                result = self._pipeline.step()

                if result is None:
                    continue

                timestamp_ns, matrix = result

                with self._latest_pose_lock:
                    self._latest_pose = (
                        timestamp_ns,
                        matrix.copy(),
                    )

        except Exception as exception:
            self._worker_exception = exception

            self.get_logger().error(
                "Mapping worker stopped: "
                f"{exception}"
            )

            self._stop_event.set()

    def _publish_latest_pose(self) -> None:
        with self._latest_pose_lock:
            if self._latest_pose is None:
                return

            timestamp_ns, matrix = (
                self._latest_pose
            )

            matrix = matrix.copy()

        if (
            self._last_published_device_time
            == timestamp_ns
        ):
            return

        self._last_published_device_time = (
            timestamp_ns
        )

        stamp = self.get_clock().now().to_msg()

        pose = matrix_to_pose_message(
            matrix
        )

        odometry = Odometry()

        odometry.header.stamp = stamp
        odometry.header.frame_id = (
            self._world_frame
        )

        odometry.child_frame_id = (
            self._camera_frame
        )

        odometry.pose.pose = pose

        self._odometry_publisher.publish(
            odometry
        )

        pose_stamped = PoseStamped()

        pose_stamped.header.stamp = stamp
        pose_stamped.header.frame_id = (
            self._world_frame
        )

        pose_stamped.pose = pose

        self._path_poses.append(
            pose_stamped
        )

        path = Path()

        path.header.stamp = stamp
        path.header.frame_id = (
            self._world_frame
        )

        path.poses = list(
            self._path_poses
        )

        self._path_publisher.publish(path)

        transform = TransformStamped()

        transform.header.stamp = stamp
        transform.header.frame_id = (
            self._world_frame
        )

        transform.child_frame_id = (
            self._camera_frame
        )

        transform.transform.translation.x = (
            pose.position.x
        )

        transform.transform.translation.y = (
            pose.position.y
        )

        transform.transform.translation.z = (
            pose.position.z
        )

        transform.transform.rotation.x = (
            pose.orientation.x
        )

        transform.transform.rotation.y = (
            pose.orientation.y
        )

        transform.transform.rotation.z = (
            pose.orientation.z
        )

        transform.transform.rotation.w = (
            pose.orientation.w
        )

        self._tf_broadcaster.sendTransform(
            transform
        )

    def _print_status(self) -> None:
        status = self._pipeline.status()

        self.get_logger().info(
            "framesets={framesets}, "
            "poses={valid_poses}, "
            "depth_integrated={depth_integrated}, "
            "pending={depth_pending}, "
            "dropped={depth_dropped}, "
            "esdf={esdf_updates}".format(
                **status
            )
        )

        if self._worker_exception is not None:
            self.get_logger().error(
                "Worker exception: "
                f"{self._worker_exception}"
            )

    def _save_map_callback(
        self,
        request: Trigger.Request,
        response: Trigger.Response,
    ) -> Trigger.Response:
        del request

        try:
            output = self._pipeline.save_map(
                self._map_output_path
            )

            response.success = True
            response.message = (
                f"Map saved to {output}"
            )

        except Exception as exception:
            response.success = False
            response.message = (
                f"Map save failed: {exception}"
            )

        return response

    def _save_mesh_callback(
        self,
        request: Trigger.Request,
        response: Trigger.Response,
    ) -> Trigger.Response:
        del request

        try:
            output = self._pipeline.save_mesh(
                self._mesh_output_path
            )

            response.success = True
            response.message = (
                f"Mesh saved to {output}"
            )

        except Exception as exception:
            response.success = False
            response.message = (
                f"Mesh save failed: {exception}"
            )

        return response

    def close(self) -> None:
        if self._closed:
            return

        self._closed = True
        self._stop_event.set()

        if self._worker_thread.is_alive():
            self._worker_thread.join(
                timeout=2.0
            )

        self._pipeline.close()

    def destroy_node(self) -> None:
        self.close()
        super().destroy_node()


def main(args=None) -> None:
    rclpy.init(args=args)

    node = RealtimeMapperNode()

    try:
        rclpy.spin(node)

    except KeyboardInterrupt:
        pass

    finally:
        node.destroy_node()

        if rclpy.ok():
            rclpy.shutdown()


if __name__ == "__main__":
    main()
