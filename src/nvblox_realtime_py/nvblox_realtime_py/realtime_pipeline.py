from __future__ import annotations

from collections import deque
from dataclasses import dataclass
import time

import numpy as np
import torch

from .cuvslam_tracker import (
    StereoPoseEstimator,
)
from .fusion_engine import (
    NvbloxFusionEngine,
)
from .pose_buffer import PoseBuffer
from .realsense_source import RealSenseSource


@dataclass
class PendingDepthFrame:
    timestamp_ns: int
    depth_gpu: torch.Tensor


class RealtimeMappingPipeline:
    """RealSense -> cuVSLAM -> nvblox pipeline."""

    def __init__(
        self,
        voxel_size_m: float = 0.05,
        max_integration_distance_m: float = 5.0,
        esdf_update_hz: float = 5.0,
        warmup_ir_frames: int = 60,
        max_pending_depth_frames: int = 64,
    ) -> None:
        self._source = RealSenseSource()

        self._tracker = StereoPoseEstimator(
            left_intrinsics=(
                self._source.left_intrinsics
            ),
            right_intrinsics=(
                self._source.right_intrinsics
            ),
            left_from_right_extrinsics=(
                self._source
                .left_from_right_extrinsics
            ),
            warmup_ir_frames=(
                warmup_ir_frames
            ),
        )

        self._fusion = NvbloxFusionEngine(
            depth_intrinsics=(
                self._source.depth_intrinsics
            ),
            voxel_size_m=voxel_size_m,
            max_integration_distance_m=(
                max_integration_distance_m
            ),
        )

        self._pose_buffer = PoseBuffer(
            max_size=300
        )

        self._pending_depth: deque[
            PendingDepthFrame
        ] = deque()

        self._max_pending_depth_frames = max(
            4,
            int(max_pending_depth_frames),
        )

        self._esdf_period_s = (
            1.0 / max(
                0.1,
                float(esdf_update_hz),
            )
        )

        self._last_esdf_time_s = (
            time.monotonic()
        )

        self._last_esdf_depth_count = 0

        self._received_framesets = 0
        self._received_depth_frames = 0
        self._received_ir_pairs = 0
        self._dropped_depth_frames = 0

        self._latest_pose: (
            tuple[int, np.ndarray] | None
        ) = None

        self._closed = False

    @property
    def fusion(
        self,
    ) -> NvbloxFusionEngine:
        return self._fusion

    @property
    def latest_pose(
        self,
    ) -> tuple[int, np.ndarray] | None:
        if self._latest_pose is None:
            return None

        timestamp_ns, pose = self._latest_pose

        return timestamp_ns, pose.copy()

    def _enqueue_depth(
        self,
        timestamp_ns: int,
        depth_gpu: torch.Tensor,
    ) -> None:
        while (
            len(self._pending_depth)
            >= self._max_pending_depth_frames
        ):
            self._pending_depth.popleft()
            self._dropped_depth_frames += 1

        self._pending_depth.append(
            PendingDepthFrame(
                timestamp_ns=int(timestamp_ns),
                depth_gpu=depth_gpu,
            )
        )

    def _drain_depth_queue(self) -> None:
        oldest_pose_time = (
            self._pose_buffer
            .oldest_timestamp_ns
        )

        newest_pose_time = (
            self._pose_buffer
            .newest_timestamp_ns
        )

        if (
            oldest_pose_time is None
            or newest_pose_time is None
        ):
            return

        while self._pending_depth:
            pending = self._pending_depth[0]

            if (
                pending.timestamp_ns
                < oldest_pose_time
            ):
                self._pending_depth.popleft()
                self._dropped_depth_frames += 1
                continue

            if (
                pending.timestamp_ns
                > newest_pose_time
            ):
                # 等待下一帧cuVSLAM位姿到来后插值。
                break

            interpolated_pose = (
                self._pose_buffer.interpolate(
                    pending.timestamp_ns
                )
            )

            if interpolated_pose is None:
                break

            self._pending_depth.popleft()

            # D435i深度坐标系与左红外参考坐标
            # 在官方实时示例中直接共用该位姿。
            self._fusion.integrate_depth(
                pending.depth_gpu,
                interpolated_pose,
            )

    def _maybe_update_esdf(self) -> None:
        integrated_count = (
            self._fusion
            .integrated_depth_frames
        )

        if integrated_count == 0:
            return

        if (
            integrated_count
            == self._last_esdf_depth_count
        ):
            return

        now_s = time.monotonic()

        if (
            now_s - self._last_esdf_time_s
            < self._esdf_period_s
        ):
            return

        self._fusion.update_esdf()

        self._last_esdf_time_s = now_s
        self._last_esdf_depth_count = (
            integrated_count
        )

    def step(
        self,
    ) -> tuple[int, np.ndarray] | None:
        if self._closed:
            raise RuntimeError(
                "Pipeline is closed"
            )

        frame = self._source.next_frame()

        self._received_framesets += 1

        timestamp_ns = int(
            frame["timestamp"]
        )

        depth_gpu = frame.get("depth")

        if depth_gpu is not None:
            self._received_depth_frames += 1

            self._enqueue_depth(
                timestamp_ns,
                depth_gpu,
            )

        left_image = frame.get(
            "left_infrared_image"
        )

        right_image = frame.get(
            "right_infrared_image"
        )

        new_pose = None

        if (
            left_image is not None
            and right_image is not None
        ):
            self._received_ir_pairs += 1

            world_from_left = (
                self._tracker.track(
                    timestamp_ns,
                    left_image,
                    right_image,
                )
            )

            if world_from_left is not None:
                inserted = self._pose_buffer.add(
                    timestamp_ns,
                    world_from_left,
                )

                if inserted:
                    self._latest_pose = (
                        timestamp_ns,
                        world_from_left.copy(),
                    )

                    new_pose = (
                        timestamp_ns,
                        world_from_left.copy(),
                    )

                    self._drain_depth_queue()

        self._maybe_update_esdf()

        return new_pose

    def status(self) -> dict[str, int]:
        return {
            "framesets": (
                self._received_framesets
            ),
            "ir_pairs": (
                self._received_ir_pairs
            ),
            "valid_poses": (
                self._tracker.valid_poses
            ),
            "failed_poses": (
                self._tracker.failed_poses
            ),
            "depth_received": (
                self._received_depth_frames
            ),
            "depth_integrated": (
                self._fusion
                .integrated_depth_frames
            ),
            "depth_pending": len(
                self._pending_depth
            ),
            "depth_dropped": (
                self._dropped_depth_frames
            ),
            "esdf_updates": (
                self._fusion.esdf_updates
            ),
        }

    def save_map(
        self,
        filename: str,
    ):
        return self._fusion.save_map(
            filename
        )

    def save_mesh(
        self,
        filename: str,
    ):
        return self._fusion.save_mesh(
            filename
        )

    def close(self) -> None:
        if self._closed:
            return

        self._closed = True
        self._source.close()
