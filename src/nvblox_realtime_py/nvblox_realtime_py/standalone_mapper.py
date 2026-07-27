from __future__ import annotations

import argparse
import time

from .realtime_pipeline import (
    RealtimeMappingPipeline,
)


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "RealSense + cuVSLAM + "
            "nvblox standalone mapper"
        )
    )

    parser.add_argument(
        "--voxel-size-m",
        type=float,
        default=0.05,
    )

    parser.add_argument(
        "--max-distance-m",
        type=float,
        default=5.0,
    )

    parser.add_argument(
        "--esdf-update-hz",
        type=float,
        default=5.0,
    )

    parser.add_argument(
        "--warmup-ir-frames",
        type=int,
        default=60,
    )

    parser.add_argument(
        "--max-frames",
        type=int,
        default=0,
        help=(
            "0 means run until Ctrl+C."
        ),
    )

    parser.add_argument(
        "--output-map",
        default=(
            "/home/czz/nvblox_ws/"
            "output/realtime_map.nvblox"
        ),
    )

    parser.add_argument(
        "--output-mesh",
        default=(
            "/home/czz/nvblox_ws/"
            "output/realtime_mesh.ply"
        ),
    )

    return parser.parse_args()


def main() -> None:
    args = parse_arguments()

    pipeline = RealtimeMappingPipeline(
        voxel_size_m=args.voxel_size_m,
        max_integration_distance_m=(
            args.max_distance_m
        ),
        esdf_update_hz=args.esdf_update_hz,
        warmup_ir_frames=(
            args.warmup_ir_frames
        ),
    )

    processed_frames = 0
    last_status_time = time.monotonic()

    print(
        "Realtime mapper started. "
        "Press Ctrl+C to stop and save."
    )

    try:
        while (
            args.max_frames <= 0
            or processed_frames
            < args.max_frames
        ):
            pipeline.step()
            processed_frames += 1

            now_s = time.monotonic()

            if (
                now_s - last_status_time
                >= 1.0
            ):
                print(
                    pipeline.status()
                )

                last_status_time = now_s

    except KeyboardInterrupt:
        print(
            "\nCtrl+C received. "
            "Saving reconstruction..."
        )

    finally:
        status = pipeline.status()

        print("Final status:", status)

        if status["depth_integrated"] > 0:
            try:
                map_path = pipeline.save_map(
                    args.output_map
                )

                print(
                    f"Map saved to: {map_path}"
                )

            except Exception as exception:
                print(
                    "Map save failed:",
                    exception,
                )

            try:
                mesh_path = pipeline.save_mesh(
                    args.output_mesh
                )

                print(
                    f"Mesh saved to: {mesh_path}"
                )

            except Exception as exception:
                print(
                    "Mesh save failed:",
                    exception,
                )

        else:
            print(
                "No depth frame was integrated; "
                "nothing will be saved."
            )

        pipeline.close()


if __name__ == "__main__":
    main()
