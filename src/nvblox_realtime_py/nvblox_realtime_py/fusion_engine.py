from __future__ import annotations

from pathlib import Path
import threading
from typing import Any

import numpy as np
import torch

from nvblox_torch.mapper import Mapper
from nvblox_torch.mapper_params import (
    MapperParams,
    ProjectiveIntegratorParams,
)
from nvblox_torch.projective_integrator_types import (
    ProjectiveIntegratorType,
)
from nvblox_torch.sensor import Sensor


def _sensor_from_realsense_intrinsics(
    intrinsics: Any,
) -> Sensor:
    return Sensor.from_camera(
        fu=float(intrinsics.fx),
        fv=float(intrinsics.fy),
        cu=float(intrinsics.ppx),
        cv=float(intrinsics.ppy),
        width=int(intrinsics.width),
        height=int(intrinsics.height),
    )


class NvbloxFusionEngine:
    """Thread-safe wrapper around nvblox_torch.Mapper."""

    def __init__(
        self,
        depth_intrinsics: Any,
        voxel_size_m: float = 0.05,
        max_integration_distance_m: float = 5.0,
    ) -> None:
        if not torch.cuda.is_available():
            raise RuntimeError(
                "CUDA is unavailable. "
                "nvblox_torch requires CUDA."
            )

        if voxel_size_m <= 0.0:
            raise ValueError(
                "voxel_size_m must be positive"
            )

        projective_params = (
            ProjectiveIntegratorParams()
        )

        projective_params.\
            projective_integrator_max_integration_distance_m = (
                float(
                    max_integration_distance_m
                )
            )

        mapper_params = MapperParams()

        mapper_params.set_projective_integrator_params(
            projective_params
        )

        self._mapper = Mapper(
            voxel_sizes_m=float(
                voxel_size_m
            ),
            integrator_types=(
                ProjectiveIntegratorType.TSDF
            ),
            mapper_parameters=mapper_params,
        )

        self._depth_sensor = (
            _sensor_from_realsense_intrinsics(
                depth_intrinsics
            )
        )

        self._lock = threading.RLock()

        self._integrated_depth_frames = 0
        self._esdf_updates = 0

    @property
    def integrated_depth_frames(
        self,
    ) -> int:
        return self._integrated_depth_frames

    @property
    def esdf_updates(self) -> int:
        return self._esdf_updates

    @torch.inference_mode()
    def integrate_depth(
        self,
        depth_gpu: torch.Tensor,
        world_from_depth: np.ndarray,
    ) -> None:
        if world_from_depth.shape != (4, 4):
            raise ValueError(
                "world_from_depth must be 4x4"
            )

        if depth_gpu.ndim != 2:
            raise ValueError(
                "Depth image must have shape HxW"
            )

        depth_gpu = depth_gpu.to(
            device="cuda",
            dtype=torch.float32,
        ).contiguous()

        pose_cpu = torch.as_tensor(
            world_from_depth,
            dtype=torch.float32,
            device="cpu",
        ).contiguous()

        with self._lock:
            self._mapper.add_depth_frame(
                depth_gpu,
                pose_cpu,
                self._depth_sensor,
                mapper_id=0,
            )

            self._integrated_depth_frames += 1

    @torch.inference_mode()
    def update_esdf(self) -> None:
        with self._lock:
            self._mapper.update_esdf(
                mapper_id=0
            )

            self._esdf_updates += 1

    @torch.inference_mode()
    def update_mesh(self) -> None:
        with self._lock:
            self._mapper.update_color_mesh(
                mapper_id=0
            )

    @torch.inference_mode()
    def mesh_statistics(
        self,
    ) -> tuple[int, int]:
        with self._lock:
            self._mapper.update_color_mesh(
                mapper_id=0
            )

            mesh = self._mapper.get_color_mesh(
                mapper_id=0
            )

            vertex_count = int(
                mesh.vertices().shape[0]
            )

            triangle_count = int(
                mesh.triangles().shape[0]
            )

        return vertex_count, triangle_count

    @torch.inference_mode()
    def save_map(
        self,
        filename: str,
    ) -> Path:
        output = Path(filename).expanduser()

        output.parent.mkdir(
            parents=True,
            exist_ok=True,
        )

        with self._lock:
            self._mapper.save_map(
                str(output),
                mapper_id=0,
            )

        return output

    @torch.inference_mode()
    def save_mesh(
        self,
        filename: str,
    ) -> Path:
        output = Path(filename).expanduser()

        output.parent.mkdir(
            parents=True,
            exist_ok=True,
        )

        with self._lock:
            self._mapper.update_color_mesh(
                mapper_id=0
            )

            mesh = self._mapper.get_color_mesh(
                mapper_id=0
            )

            vertex_count = int(
                mesh.vertices().shape[0]
            )

            triangle_count = int(
                mesh.triangles().shape[0]
            )

            if (
                vertex_count == 0
                or triangle_count == 0
            ):
                raise RuntimeError(
                    "Mesh is empty. "
                    "No valid depth frames have "
                    "been reconstructed."
                )

            mesh.save(str(output))

        return output
