from __future__ import annotations

from bisect import bisect_right
from collections import deque
from dataclasses import dataclass

import numpy as np
from scipy.spatial.transform import (
    Rotation,
    Slerp,
)


@dataclass
class PoseSample:
    timestamp_ns: int
    world_from_camera: np.ndarray


class PoseBuffer:
    """Stores and interpolates timestamped 6-DoF poses."""

    def __init__(
        self,
        max_size: int = 300,
    ) -> None:
        self._samples: deque[PoseSample] = deque(
            maxlen=max_size
        )

    def __len__(self) -> int:
        return len(self._samples)

    @property
    def oldest_timestamp_ns(
        self,
    ) -> int | None:
        if not self._samples:
            return None

        return self._samples[0].timestamp_ns

    @property
    def newest_timestamp_ns(
        self,
    ) -> int | None:
        if not self._samples:
            return None

        return self._samples[-1].timestamp_ns

    def add(
        self,
        timestamp_ns: int,
        world_from_camera: np.ndarray,
    ) -> bool:
        if world_from_camera.shape != (4, 4):
            raise ValueError(
                "Pose must be a 4x4 matrix"
            )

        if self._samples:
            if (
                timestamp_ns
                <= self._samples[-1].timestamp_ns
            ):
                return False

        self._samples.append(
            PoseSample(
                timestamp_ns=int(timestamp_ns),
                world_from_camera=(
                    world_from_camera
                    .astype(
                        np.float32,
                        copy=True,
                    )
                ),
            )
        )

        return True

    def interpolate(
        self,
        timestamp_ns: int,
    ) -> np.ndarray | None:
        if not self._samples:
            return None

        timestamp_ns = int(timestamp_ns)

        first = self._samples[0]
        last = self._samples[-1]

        if timestamp_ns == first.timestamp_ns:
            return first.world_from_camera.copy()

        if timestamp_ns == last.timestamp_ns:
            return last.world_from_camera.copy()

        if (
            timestamp_ns < first.timestamp_ns
            or timestamp_ns > last.timestamp_ns
        ):
            return None

        timestamps = [
            sample.timestamp_ns
            for sample in self._samples
        ]

        upper_index = bisect_right(
            timestamps,
            timestamp_ns,
        )

        if (
            upper_index <= 0
            or upper_index >= len(self._samples)
        ):
            return None

        lower = self._samples[
            upper_index - 1
        ]

        upper = self._samples[
            upper_index
        ]

        interval_ns = (
            upper.timestamp_ns
            - lower.timestamp_ns
        )

        if interval_ns <= 0:
            return None

        alpha = (
            timestamp_ns
            - lower.timestamp_ns
        ) / interval_ns

        alpha = float(
            np.clip(
                alpha,
                0.0,
                1.0,
            )
        )

        lower_translation = (
            lower.world_from_camera[:3, 3]
        )

        upper_translation = (
            upper.world_from_camera[:3, 3]
        )

        translation = (
            (1.0 - alpha)
            * lower_translation
            + alpha
            * upper_translation
        )

        rotations = Rotation.from_matrix(
            np.stack(
                [
                    lower.world_from_camera[
                        :3, :3
                    ],
                    upper.world_from_camera[
                        :3, :3
                    ],
                ],
                axis=0,
            )
        )

        interpolator = Slerp(
            [0.0, 1.0],
            rotations,
        )

        rotation = (
            interpolator([alpha])
            .as_matrix()[0]
        )

        result = np.eye(
            4,
            dtype=np.float32,
        )

        result[:3, :3] = rotation.astype(
            np.float32
        )

        result[:3, 3] = translation.astype(
            np.float32
        )

        return result
