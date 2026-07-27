from __future__ import annotations

from typing import Any

import cuvslam as vslam
import numpy as np
from scipy.spatial.transform import Rotation


def _identity_vslam_pose() -> Any:
    quaternion = Rotation.identity().as_quat()

    return vslam.Pose(
        rotation=quaternion,
        translation=np.zeros(
            3,
            dtype=np.float64,
        ),
    )


def _extrinsics_to_vslam_pose(
    extrinsics: Any,
) -> Any:
    rotation_matrix = np.asarray(
        extrinsics.rotation,
        dtype=np.float64,
    ).reshape(3, 3)

    translation = np.asarray(
        extrinsics.translation,
        dtype=np.float64,
    )

    quaternion = (
        Rotation
        .from_matrix(rotation_matrix)
        .as_quat()
    )

    return vslam.Pose(
        rotation=quaternion,
        translation=translation,
    )


def _create_camera(
    intrinsics: Any,
    rig_from_camera: Any,
) -> Any:
    camera = vslam.Camera()

    # cuVSLAM v15 API
    camera.distortion = vslam.Distortion(
        vslam.Distortion.Model.Pinhole
    )

    camera.focal = (
        float(intrinsics.fx),
        float(intrinsics.fy),
    )

    camera.principal = (
        float(intrinsics.ppx),
        float(intrinsics.ppy),
    )

    camera.size = (
        int(intrinsics.width),
        int(intrinsics.height),
    )

    camera.rig_from_camera = rig_from_camera

    return camera


def _pose_to_matrix(
    pose: Any,
) -> np.ndarray:
    translation = np.asarray(
        pose.translation,
        dtype=np.float64,
    ).reshape(3)

    quaternion = np.asarray(
        pose.rotation,
        dtype=np.float64,
    ).reshape(4)

    matrix = np.eye(
        4,
        dtype=np.float32,
    )

    matrix[:3, :3] = (
        Rotation
        .from_quat(quaternion)
        .as_matrix()
        .astype(np.float32)
    )

    matrix[:3, 3] = translation.astype(
        np.float32
    )

    return matrix


class StereoPoseEstimator:
    """cuVSLAM stereo visual odometry wrapper."""

    def __init__(
        self,
        left_intrinsics: Any,
        right_intrinsics: Any,
        left_from_right_extrinsics: Any,
        warmup_ir_frames: int = 60,
    ) -> None:
        if not hasattr(
            vslam.Tracker,
            "OdometryConfig",
        ):
            raise RuntimeError(
                "This code requires cuVSLAM v15 API. "
                "Tracker.OdometryConfig was not found."
            )

        rig = vslam.Rig()

        left_camera = _create_camera(
            left_intrinsics,
            _identity_vslam_pose(),
        )

        right_camera = _create_camera(
            right_intrinsics,
            _extrinsics_to_vslam_pose(
                left_from_right_extrinsics
            ),
        )

        rig.cameras = [
            left_camera,
            right_camera,
        ]

        config = vslam.Tracker.OdometryConfig(
            async_sba=False,
            enable_final_landmarks_export=True,
            enable_observations_export=False,
            rectified_stereo_camera=True,
        )

        self._tracker = vslam.Tracker(
            rig,
            config,
        )

        self._warmup_ir_frames = max(
            0,
            int(warmup_ir_frames),
        )

        self._received_ir_frames = 0
        self._valid_poses = 0
        self._failed_poses = 0

    @property
    def valid_poses(self) -> int:
        return self._valid_poses

    @property
    def failed_poses(self) -> int:
        return self._failed_poses

    def track(
        self,
        timestamp_ns: int,
        left_image: np.ndarray,
        right_image: np.ndarray,
    ) -> np.ndarray | None:
        self._received_ir_frames += 1

        if (
            self._received_ir_frames
            <= self._warmup_ir_frames
        ):
            return None

        result = self._tracker.track(
            int(timestamp_ns),
            (
                left_image,
                right_image,
            ),
        )

        # cuVSLAM v15 returns:
        # (odometry_pose_estimate, status)
        if isinstance(result, tuple):
            pose_estimate = result[0]
        else:
            pose_estimate = result

        world_from_rig = getattr(
            pose_estimate,
            "world_from_rig",
            None,
        )

        if world_from_rig is None:
            self._failed_poses += 1
            return None

        pose = getattr(
            world_from_rig,
            "pose",
            world_from_rig,
        )

        matrix = _pose_to_matrix(pose)

        if not np.all(np.isfinite(matrix)):
            self._failed_poses += 1
            return None

        self._valid_poses += 1

        return matrix
