from __future__ import annotations

from typing import Any

import numpy as np
import pyrealsense2 as rs
import torch


class RealSenseSource:
    """
    Robust RealSense D435i input source.

    First-stage configuration:
      - Left infrared
      - Right infrared
      - Depth
      - No color stream
      - IR emitter disabled

    The class automatically tries several commonly supported
    D435i resolutions and frame rates.
    """

    # 按优先级依次尝试。
    STREAM_CANDIDATES = [
        (640, 480, 30),
        (848, 480, 30),
        (640, 480, 15),
    ]

    def __init__(self) -> None:
        if not torch.cuda.is_available():
            raise RuntimeError(
                "CUDA is unavailable. "
                "RealSense depth frames must be transferred "
                "to CUDA for nvblox."
            )

        self._pipeline: rs.pipeline | None = None
        self._profile: rs.pipeline_profile | None = None
        self._started = False

        self._depth_scale_m = 0.001

        self._left_intrinsics: Any = None
        self._right_intrinsics: Any = None
        self._depth_intrinsics: Any = None
        self._left_from_right_extrinsics: Any = None

        context = rs.context()
        devices = context.query_devices()

        if len(devices) == 0:
            raise RuntimeError(
                "No RealSense device was detected. "
                "Check the USB cable and camera connection."
            )

        device = devices[0]

        self._serial_number = device.get_info(
            rs.camera_info.serial_number
        )

        self._device_name = device.get_info(
            rs.camera_info.name
        )

        print(
            "Opening RealSense:",
            self._device_name,
            "serial:",
            self._serial_number,
        )

        self._start_with_fallback_profiles()
        self._configure_depth_sensor()
        self._read_calibration()

    def _start_with_fallback_profiles(self) -> None:
        errors: list[str] = []

        for width, height, fps in self.STREAM_CANDIDATES:
            pipeline = rs.pipeline()
            config = rs.config()

            config.enable_device(
                self._serial_number
            )

            config.enable_stream(
                rs.stream.infrared,
                1,
                width,
                height,
                rs.format.y8,
                fps,
            )

            config.enable_stream(
                rs.stream.infrared,
                2,
                width,
                height,
                rs.format.y8,
                fps,
            )

            config.enable_stream(
                rs.stream.depth,
                width,
                height,
                rs.format.z16,
                fps,
            )

            print(
                f"Trying RealSense profile: "
                f"{width}x{height}@{fps}"
            )

            try:
                profile = pipeline.start(config)

            except RuntimeError as exception:
                errors.append(
                    f"{width}x{height}@{fps}: "
                    f"{exception}"
                )

                try:
                    pipeline.stop()
                except RuntimeError:
                    pass

                continue

            self._pipeline = pipeline
            self._profile = profile
            self._started = True

            self._width = width
            self._height = height
            self._fps = fps

            print(
                f"Selected RealSense profile: "
                f"{width}x{height}@{fps}"
            )

            return

        error_text = "\n".join(errors)

        raise RuntimeError(
            "Unable to start D435i using any supported "
            "fallback profile.\n"
            f"{error_text}"
        )

    def _configure_depth_sensor(self) -> None:
        if self._profile is None:
            raise RuntimeError(
                "RealSense profile is not initialized"
            )

        device = self._profile.get_device()
        depth_sensor = device.first_depth_sensor()

        self._depth_scale_m = float(
            depth_sensor.get_depth_scale()
        )

        print(
            "Depth scale:",
            self._depth_scale_m,
            "m/unit",
        )

        # 第一阶段关闭投影器：
        # 红外双目与深度帧能够同时连续输出，
        # 避免官方闪烁模式带来的流配置兼容性问题。
        if depth_sensor.supports(
            rs.option.emitter_enabled
        ):
            try:
                depth_sensor.set_option(
                    rs.option.emitter_enabled,
                    0.0,
                )

                print("IR emitter disabled")

            except RuntimeError as exception:
                print(
                    "Warning: failed to disable emitter:",
                    exception,
                )

    def _wait_for_complete_frameset(
        self,
        attempts: int = 30,
    ) -> tuple[
        rs.composite_frame,
        rs.video_frame,
        rs.video_frame,
        rs.depth_frame,
    ]:
        if self._pipeline is None:
            raise RuntimeError(
                "RealSense pipeline is not initialized"
            )

        for _ in range(attempts):
            frames = self._pipeline.wait_for_frames(
                5000
            )

            left_frame = frames.get_infrared_frame(1)
            right_frame = frames.get_infrared_frame(2)
            depth_frame = frames.get_depth_frame()

            if (
                left_frame
                and right_frame
                and depth_frame
            ):
                return (
                    frames,
                    left_frame,
                    right_frame,
                    depth_frame,
                )

        raise RuntimeError(
            "RealSense started, but complete "
            "left/right/depth frames were not received."
        )

    def _read_calibration(self) -> None:
        # 丢弃启动初期的不稳定帧。
        for _ in range(15):
            self._wait_for_complete_frameset()

        (
            _,
            left_frame,
            right_frame,
            depth_frame,
        ) = self._wait_for_complete_frameset()

        left_profile = (
            left_frame
            .profile
            .as_video_stream_profile()
        )

        right_profile = (
            right_frame
            .profile
            .as_video_stream_profile()
        )

        depth_profile = (
            depth_frame
            .profile
            .as_video_stream_profile()
        )

        self._left_intrinsics = (
            left_profile.get_intrinsics()
        )

        self._right_intrinsics = (
            right_profile.get_intrinsics()
        )

        self._depth_intrinsics = (
            depth_profile.get_intrinsics()
        )

        # right -> left，与cuVSLAM rig定义保持一致。
        self._left_from_right_extrinsics = (
            right_profile.get_extrinsics_to(
                left_profile
            )
        )

        print(
            "Left IR intrinsics:",
            self._left_intrinsics.width,
            "x",
            self._left_intrinsics.height,
        )

        print(
            "Depth intrinsics:",
            self._depth_intrinsics.width,
            "x",
            self._depth_intrinsics.height,
        )

    @property
    def left_intrinsics(self) -> Any:
        return self._left_intrinsics

    @property
    def right_intrinsics(self) -> Any:
        return self._right_intrinsics

    @property
    def depth_intrinsics(self) -> Any:
        return self._depth_intrinsics

    @property
    def left_from_right_extrinsics(self) -> Any:
        return self._left_from_right_extrinsics

    def next_frame(self) -> dict[str, Any]:
        (
            _,
            left_frame,
            right_frame,
            depth_frame,
        ) = self._wait_for_complete_frameset(
            attempts=10
        )

        left_image = np.asanyarray(
            left_frame.get_data()
        ).copy()

        right_image = np.asanyarray(
            right_frame.get_data()
        ).copy()

        depth_image_m = (
            np.asanyarray(
                depth_frame.get_data()
            )
            .astype(
                np.float32,
                copy=True,
            )
            * self._depth_scale_m
        )

        depth_gpu = (
            torch
            .from_numpy(depth_image_m)
            .to(
                device="cuda",
                dtype=torch.float32,
            )
            .contiguous()
        )

        # RealSense timestamp单位是毫秒，
        # cuVSLAM接口需要纳秒。
        timestamp_ns = int(
            float(left_frame.get_timestamp())
            * 1_000_000.0
        )

        return {
            "timestamp": timestamp_ns,
            "left_infrared_image": left_image,
            "right_infrared_image": right_image,
            "depth": depth_gpu,
            "rgb": None,
        }

    def close(self) -> None:
        if (
            self._pipeline is not None
            and self._started
        ):
            try:
                self._pipeline.stop()
            except RuntimeError:
                pass

        self._pipeline = None
        self._profile = None
        self._started = False

    def __del__(self) -> None:
        try:
            self.close()
        except Exception:
            # 析构阶段不能再抛出异常。
            pass
