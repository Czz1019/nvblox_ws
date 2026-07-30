// // #include "my_nvblox/esdf_publisher.hpp"

// // #include <cmath>
// // #include <cstdint>
// // #include <cstring>
// // #include <vector>

// // #include "sensor_msgs/msg/point_field.hpp"

// // namespace my_nvblox
// // {

// // namespace
// // {

// // inline nvblox::Index3D compute_block_index_from_position(
// //   const nvblox::Vector3f & p_L,
// //   const float block_size)
// // {
// //   return nvblox::Index3D(
// //     static_cast<int>(std::floor(p_L.x() / block_size)),
// //     static_cast<int>(std::floor(p_L.y() / block_size)),
// //     static_cast<int>(std::floor(p_L.z() / block_size)));
// // }

// // inline nvblox::Index3D compute_voxel_index_in_block_from_position(
// //   const nvblox::Vector3f & p_L,
// //   const float block_size,
// //   const float voxel_size,
// //   const int voxels_per_side)
// // {
// //   const nvblox::Vector3f block_origin =
// //     (p_L / block_size).array().floor().matrix() * block_size;

// //   const nvblox::Vector3f p_block = p_L - block_origin;

// //   int vx = static_cast<int>(std::floor(p_block.x() / voxel_size));
// //   int vy = static_cast<int>(std::floor(p_block.y() / voxel_size));
// //   int vz = static_cast<int>(std::floor(p_block.z() / voxel_size));

// //   vx = std::max(0, std::min(voxels_per_side - 1, vx));
// //   vy = std::max(0, std::min(voxels_per_side - 1, vy));
// //   vz = std::max(0, std::min(voxels_per_side - 1, vz));

// //   return nvblox::Index3D(vx, vy, vz);
// // }

// // }  // namespace

// // EsdfPublisher::EsdfPublisher(
// //   rclcpp::Node * node,
// //   const std::string & topic_name,
// //   const std::string & frame_id,
// //   float slice_height,
// //   float xy_min,
// //   float xy_max,
// //   float resolution)
// // : frame_id_(frame_id),
// //   slice_height_(slice_height),
// //   xy_min_(xy_min),
// //   xy_max_(xy_max),
// //   resolution_(resolution)
// // {
// //   publisher_ = node->create_publisher<sensor_msgs::msg::PointCloud2>(topic_name, 1);
// // }

// // void EsdfPublisher::publish(const nvblox::Mapper & mapper)
// // {
// //   const auto & esdf_layer = mapper.esdf_layer();
// //   auto samples = sample_esdf_slice(esdf_layer);
// //   auto cloud = make_cloud(samples);
// //   publisher_->publish(cloud);
// // }

// // std::vector<EsdfPublisher::SamplePoint> EsdfPublisher::sample_esdf_slice(
// //   const nvblox::EsdfLayer & esdf_layer) const
// // {
// //   std::vector<SamplePoint> samples;

// //   const float voxel_size = esdf_layer.voxel_size();
// //   const float block_size = esdf_layer.block_size();
// //   constexpr int kVoxelsPerSide = nvblox::VoxelBlock<nvblox::EsdfVoxel>::kVoxelsPerSide;

// //   for (float x = xy_min_; x <= xy_max_; x += resolution_) {
// //     for (float y = xy_min_; y <= xy_max_; y += resolution_) {
// //       SamplePoint s;
// //       s.x = x;
// //       s.y = y;
// //       s.z = slice_height_;
// //       s.distance = 0.0f;
// //       s.observed = false;
// //       s.inside = false;

// //       const nvblox::Vector3f p_L(x, y, slice_height_);

// //       const auto block_idx = compute_block_index_from_position(p_L, block_size);
// //       const auto voxel_idx = compute_voxel_index_in_block_from_position(
// //         p_L, block_size, voxel_size, kVoxelsPerSide);

// //       auto block = esdf_layer.getBlockAtIndex(block_idx);
// //       if (block) {
// //         const auto & voxel =
// //           block->voxels[voxel_idx.x()][voxel_idx.y()][voxel_idx.z()];
// //         s.observed = voxel.observed;
// //         s.inside = voxel.is_inside;
// //         s.distance = std::sqrt(voxel.squared_distance_vox) * voxel_size;
// //       }

// //       samples.push_back(s);
// //     }
// //   }

// //   return samples;
// // }

// // sensor_msgs::msg::PointCloud2 EsdfPublisher::make_cloud(
// //   const std::vector<SamplePoint> & samples) const
// // {
// //   sensor_msgs::msg::PointCloud2 cloud;
// //   cloud.header.frame_id = frame_id_;
// //   cloud.header.stamp = rclcpp::Clock().now();

// //   cloud.height = 1;
// //   cloud.width = static_cast<uint32_t>(samples.size());
// //   cloud.is_dense = false;
// //   cloud.is_bigendian = false;

// //   cloud.fields.resize(4);

// //   cloud.fields[0].name = "x";
// //   cloud.fields[0].offset = 0;
// //   cloud.fields[0].datatype = sensor_msgs::msg::PointField::FLOAT32;
// //   cloud.fields[0].count = 1;

// //   cloud.fields[1].name = "y";
// //   cloud.fields[1].offset = 4;
// //   cloud.fields[1].datatype = sensor_msgs::msg::PointField::FLOAT32;
// //   cloud.fields[1].count = 1;

// //   cloud.fields[2].name = "z";
// //   cloud.fields[2].offset = 8;
// //   cloud.fields[2].datatype = sensor_msgs::msg::PointField::FLOAT32;
// //   cloud.fields[2].count = 1;

// //   cloud.fields[3].name = "rgb";
// //   cloud.fields[3].offset = 12;
// //   cloud.fields[3].datatype = sensor_msgs::msg::PointField::FLOAT32;
// //   cloud.fields[3].count = 1;

// //   cloud.point_step = 16;
// //   cloud.row_step = cloud.point_step * cloud.width;
// //   cloud.data.resize(cloud.row_step);

// //   for (size_t i = 0; i < samples.size(); ++i) {
// //     const auto & s = samples[i];
// //     uint8_t * ptr = &cloud.data[i * cloud.point_step];

// //     std::memcpy(ptr + 0, &s.x, sizeof(float));
// //     std::memcpy(ptr + 4, &s.y, sizeof(float));
// //     std::memcpy(ptr + 8, &s.z, sizeof(float));

// //     uint8_t r = 128, g = 128, b = 128;
// //     if (s.observed) {
// //       const float d = std::max(0.0f, std::min(1.0f, s.distance));
// //       r = static_cast<uint8_t>((1.0f - d) * 255.0f);
// //       g = 0;
// //       b = static_cast<uint8_t>(d * 255.0f);
// //     }

// //     uint32_t rgb = (static_cast<uint32_t>(r) << 16) |
// //                    (static_cast<uint32_t>(g) << 8) |
// //                    static_cast<uint32_t>(b);
// //     float rgb_float;
// //     std::memcpy(&rgb_float, &rgb, sizeof(float));
// //     std::memcpy(ptr + 12, &rgb_float, sizeof(float));
// //   }

// //   return cloud;
// // }

// // }  // namespace my_nvblox

// #include "my_nvblox/esdf_publisher.hpp"

// #include <algorithm>
// #include <cmath>
// #include <cstdint>
// #include <cstring>
// #include <vector>

// #include "sensor_msgs/msg/point_field.hpp"

// namespace my_nvblox
// {

// namespace
// {

// inline nvblox::Index3D compute_block_index_from_position(
//   const nvblox::Vector3f & p_L,
//   const float block_size)
// {
//   return nvblox::Index3D(
//     static_cast<int>(std::floor(p_L.x() / block_size)),
//     static_cast<int>(std::floor(p_L.y() / block_size)),
//     static_cast<int>(std::floor(p_L.z() / block_size)));
// }

// inline nvblox::Index3D compute_voxel_index_in_block_from_position(
//   const nvblox::Vector3f & p_L,
//   const float block_size,
//   const float voxel_size,
//   const int voxels_per_side)
// {
//   const nvblox::Vector3f block_origin =
//     (p_L / block_size).array().floor().matrix() * block_size;

//   const nvblox::Vector3f p_block = p_L - block_origin;

//   int vx = static_cast<int>(std::floor(p_block.x() / voxel_size));
//   int vy = static_cast<int>(std::floor(p_block.y() / voxel_size));
//   int vz = static_cast<int>(std::floor(p_block.z() / voxel_size));

//   vx = std::max(0, std::min(voxels_per_side - 1, vx));
//   vy = std::max(0, std::min(voxels_per_side - 1, vy));
//   vz = std::max(0, std::min(voxels_per_side - 1, vz));

//   return nvblox::Index3D(vx, vy, vz);
// }

// }  // namespace

// EsdfPublisher::EsdfPublisher(
//   rclcpp::Node * node,
//   const std::string & topic_name,
//   const std::string & frame_id,
//   float slice_height,
//   float xy_min,
//   float xy_max,
//   float resolution,
//   float max_visualized_distance)
// : frame_id_(frame_id),
//   slice_height_(slice_height),
//   xy_min_(xy_min),
//   xy_max_(xy_max),
//   resolution_(resolution),
//   max_visualized_distance_(max_visualized_distance)
// {
//   publisher_ = node->create_publisher<sensor_msgs::msg::PointCloud2>(topic_name, 1);
// }

// void EsdfPublisher::publish(const nvblox::Mapper & mapper)
// {
//   const auto & esdf_layer = mapper.esdf_layer();
//   auto samples = sample_esdf_slice(esdf_layer);
//   auto cloud = make_cloud(samples);
//   publisher_->publish(cloud);
// }

// std::vector<EsdfPublisher::SamplePoint> EsdfPublisher::sample_esdf_slice(
//   const nvblox::EsdfLayer & esdf_layer) const
// {
//   std::vector<SamplePoint> samples;

//   const float voxel_size = esdf_layer.voxel_size();
//   const float block_size = esdf_layer.block_size();
//   constexpr int kVoxelsPerSide = nvblox::VoxelBlock<nvblox::EsdfVoxel>::kVoxelsPerSide;

//   for (float x = xy_min_; x <= xy_max_; x += resolution_) {
//     for (float y = xy_min_; y <= xy_max_; y += resolution_) {
//       const nvblox::Vector3f p_L(x, y, slice_height_);

//       const auto block_idx = compute_block_index_from_position(p_L, block_size);
//       const auto voxel_idx = compute_voxel_index_in_block_from_position(
//         p_L, block_size, voxel_size, kVoxelsPerSide);

//       auto block = esdf_layer.getBlockAtIndex(block_idx);
//       if (!block) {
//         continue;
//       }

//       const auto & voxel =
//         block->voxels[voxel_idx.x()][voxel_idx.y()][voxel_idx.z()];

//       if (!voxel.observed) {
//         continue;
//       }

//       const float distance_m =
//         std::sqrt(voxel.squared_distance_vox) * voxel_size;

//       if (distance_m > max_visualized_distance_) {
//         continue;
//       }

//       SamplePoint s;
//       s.x = x;
//       s.y = y;
//       s.z = slice_height_;
//       s.distance = distance_m;
//       s.observed = voxel.observed;
//       s.inside = voxel.is_inside;

//       samples.push_back(s);
//     }
//   }

//   return samples;
// }

// sensor_msgs::msg::PointCloud2 EsdfPublisher::make_cloud(
//   const std::vector<SamplePoint> & samples) const
// {
//   sensor_msgs::msg::PointCloud2 cloud;
//   cloud.header.frame_id = frame_id_;
//   cloud.header.stamp = rclcpp::Clock().now();

//   cloud.height = 1;
//   cloud.width = static_cast<uint32_t>(samples.size());
//   cloud.is_dense = false;
//   cloud.is_bigendian = false;

//   cloud.fields.resize(4);

//   cloud.fields[0].name = "x";
//   cloud.fields[0].offset = 0;
//   cloud.fields[0].datatype = sensor_msgs::msg::PointField::FLOAT32;
//   cloud.fields[0].count = 1;

//   cloud.fields[1].name = "y";
//   cloud.fields[1].offset = 4;
//   cloud.fields[1].datatype = sensor_msgs::msg::PointField::FLOAT32;
//   cloud.fields[1].count = 1;

//   cloud.fields[2].name = "z";
//   cloud.fields[2].offset = 8;
//   cloud.fields[2].datatype = sensor_msgs::msg::PointField::FLOAT32;
//   cloud.fields[2].count = 1;

//   cloud.fields[3].name = "rgb";
//   cloud.fields[3].offset = 12;
//   cloud.fields[3].datatype = sensor_msgs::msg::PointField::FLOAT32;
//   cloud.fields[3].count = 1;

//   cloud.point_step = 16;
//   cloud.row_step = cloud.point_step * cloud.width;
//   cloud.data.resize(cloud.row_step);

//   for (size_t i = 0; i < samples.size(); ++i) {
//     const auto & s = samples[i];
//     uint8_t * ptr = &cloud.data[i * cloud.point_step];

//     std::memcpy(ptr + 0, &s.x, sizeof(float));
//     std::memcpy(ptr + 4, &s.y, sizeof(float));
//     std::memcpy(ptr + 8, &s.z, sizeof(float));

//     uint8_t r = 0;
//     uint8_t g = 0;
//     uint8_t b = 0;

//     if (s.inside) {
//       // 障碍内部：紫红色
//       r = 255;
//       g = 0;
//       b = 255;
//     } else {
//       // 距离越小越红，越大越蓝
//       const float d =
//         std::max(0.0f, std::min(1.0f, s.distance / max_visualized_distance_));
//       r = static_cast<uint8_t>((1.0f - d) * 255.0f);
//       g = 0;
//       b = static_cast<uint8_t>(d * 255.0f);
//     }

//     const uint32_t rgb =
//       (static_cast<uint32_t>(r) << 16) |
//       (static_cast<uint32_t>(g) << 8) |
//       static_cast<uint32_t>(b);

//     float rgb_float;
//     std::memcpy(&rgb_float, &rgb, sizeof(float));
//     std::memcpy(ptr + 12, &rgb_float, sizeof(float));
//   }

//   return cloud;
// }

// }  // namespace my_nvblox

#include "my_nvblox/esdf_publisher.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <vector>

#include "sensor_msgs/msg/point_field.hpp"

namespace my_nvblox
{

namespace
{

inline nvblox::Index3D compute_block_index_from_position(
  const nvblox::Vector3f & p_L,
  const float block_size)
{
  return nvblox::Index3D(
    static_cast<int>(std::floor(p_L.x() / block_size)),
    static_cast<int>(std::floor(p_L.y() / block_size)),
    static_cast<int>(std::floor(p_L.z() / block_size)));
}

inline nvblox::Index3D compute_voxel_index_in_block_from_position(
  const nvblox::Vector3f & p_L,
  const float block_size,
  const float voxel_size,
  const int voxels_per_side)
{
  const nvblox::Vector3f block_origin =
    (p_L / block_size).array().floor().matrix() * block_size;

  const nvblox::Vector3f p_block = p_L - block_origin;

  int vx = static_cast<int>(std::floor(p_block.x() / voxel_size));
  int vy = static_cast<int>(std::floor(p_block.y() / voxel_size));
  int vz = static_cast<int>(std::floor(p_block.z() / voxel_size));

  vx = std::max(0, std::min(voxels_per_side - 1, vx));
  vy = std::max(0, std::min(voxels_per_side - 1, vy));
  vz = std::max(0, std::min(voxels_per_side - 1, vz));

  return nvblox::Index3D(vx, vy, vz);
}

}  // namespace

EsdfPublisher::EsdfPublisher(
  rclcpp::Node * node,
  const std::string & topic_name,
  const std::string & frame_id,
  float slice_height,
  float xy_min,
  float xy_max,
  float resolution)
: logger_(node->get_logger()),
  clock_(node->get_clock()),
  frame_id_(frame_id),
  slice_height_(slice_height),
  xy_min_(xy_min),
  xy_max_(xy_max),
  resolution_(resolution),
  cuda_stream_(cudaStreamNonBlocking)
{
  if (resolution_ <= 0.0F || xy_max_ < xy_min_) {
    throw std::invalid_argument("Invalid ESDF query bounds or resolution");
  }
  publisher_ = node->create_publisher<sensor_msgs::msg::PointCloud2>(topic_name, 1);
}

bool EsdfPublisher::has_subscribers() const
{
  return publisher_->get_subscription_count() > 0 ||
         publisher_->get_intra_process_subscription_count() > 0;
}

void EsdfPublisher::publish(const nvblox::Mapper & mapper)
{
  const auto start = std::chrono::steady_clock::now();
  const auto & esdf_layer = mapper.esdf_layer();
  auto samples = sample_esdf_slice(esdf_layer);
  const auto sampled = std::chrono::steady_clock::now();
  auto cloud = make_cloud(samples);
  const auto cloud_built = std::chrono::steady_clock::now();
  publisher_->publish(cloud);
  const auto finished = std::chrono::steady_clock::now();

  RCLCPP_INFO_THROTTLE(
    logger_, *clock_, 2000,
    "ESDF publish timing: device_snapshot+query=%.2f ms cloud=%.2f ms ros_publish=%.2f ms",
    std::chrono::duration<double, std::milli>(sampled - start).count(),
    std::chrono::duration<double, std::milli>(cloud_built - sampled).count(),
    std::chrono::duration<double, std::milli>(finished - cloud_built).count());
}

std::vector<EsdfPublisher::SamplePoint> EsdfPublisher::sample_esdf_slice(
  const nvblox::EsdfLayer & esdf_layer)
{
  std::vector<SamplePoint> samples;

  const float voxel_size = esdf_layer.voxel_size();
  const float block_size = esdf_layer.block_size();
  constexpr int kVoxelsPerSide = nvblox::VoxelBlock<nvblox::EsdfVoxel>::kVoxelsPerSide;

  const int slice_block_z = static_cast<int>(std::floor(slice_height_ / block_size));
  const int min_block_x = static_cast<int>(std::floor(xy_min_ / block_size));
  const int max_block_x = static_cast<int>(std::floor(xy_max_ / block_size));
  const int min_block_y = static_cast<int>(std::floor(xy_min_ / block_size));
  const int max_block_y = static_cast<int>(std::floor(xy_max_ / block_size));

  std::vector<nvblox::Index3D> slice_blocks;
  for (const auto & block_idx : esdf_layer.getAllBlockIndices()) {
    if (block_idx.z() == slice_block_z &&
      block_idx.x() >= min_block_x && block_idx.x() <= max_block_x &&
      block_idx.y() >= min_block_y && block_idx.y() <= max_block_y)
    {
      slice_blocks.push_back(block_idx);
    }
  }

  if (slice_blocks.empty()) {
    return samples;
  }

  const auto serialized = serializer_.serialize(esdf_layer, slice_blocks, cuda_stream_);
  nvblox::Index3DHashMapType<size_t>::type block_to_serialized_index;
  block_to_serialized_index.reserve(serialized->block_indices.size());
  for (size_t i = 0; i < serialized->block_indices.size(); ++i) {
    block_to_serialized_index.emplace(serialized->block_indices[i], i);
  }

  const size_t query_count_per_axis = static_cast<size_t>(
    std::floor((xy_max_ - xy_min_) / resolution_)) + 1U;
  samples.reserve(query_count_per_axis * query_count_per_axis);

  size_t total_queries = 0;
  size_t valid_block_count = 0;
  size_t observed_count = 0;
  size_t inside_count = 0;

  for (float x = xy_min_; x <= xy_max_; x += resolution_) {
    for (float y = xy_min_; y <= xy_max_; y += resolution_) {
      ++total_queries;

      SamplePoint s;
      s.x = x;
      s.y = y;
      s.z = slice_height_;
      s.distance = 0.0f;
      s.observed = false;
      s.inside = false;
      s.valid_block = false;

      const nvblox::Vector3f p_L(x, y, slice_height_);

      const auto block_idx = compute_block_index_from_position(p_L, block_size);
      const auto voxel_idx = compute_voxel_index_in_block_from_position(
        p_L, block_size, voxel_size, kVoxelsPerSide);

      const auto block_it = block_to_serialized_index.find(block_idx);
      if (block_it != block_to_serialized_index.end()) {
        s.valid_block = true;
        ++valid_block_count;

        const size_t serialized_block_idx = block_it->second;
        const size_t voxel_offset = static_cast<size_t>(
          serialized->block_offsets[serialized_block_idx]) +
          static_cast<size_t>(voxel_idx.x() * kVoxelsPerSide * kVoxelsPerSide +
          voxel_idx.y() * kVoxelsPerSide + voxel_idx.z());
        const auto & voxel = serialized->voxels[voxel_offset];

        s.observed = voxel.observed;
        s.inside = voxel.is_inside;
        s.distance = std::sqrt(voxel.squared_distance_vox) * voxel_size;

        if (s.observed) {
          ++observed_count;
        }
        if (s.inside) {
          ++inside_count;
        }

        samples.push_back(s);
      }
    }
  }

  RCLCPP_INFO_THROTTLE(
    logger_, *clock_, 3000,
    "ESDF slice stats: total_queries=%zu valid_blocks=%zu observed=%zu inside=%zu published_points=%zu",
    total_queries, valid_block_count, observed_count, inside_count, samples.size());

  return samples;
}

sensor_msgs::msg::PointCloud2 EsdfPublisher::make_cloud(
  const std::vector<SamplePoint> & samples) const
{
  sensor_msgs::msg::PointCloud2 cloud;
  cloud.header.frame_id = frame_id_;
  cloud.header.stamp = clock_->now();

  cloud.height = 1;
  cloud.width = static_cast<uint32_t>(samples.size());
  cloud.is_dense = false;
  cloud.is_bigendian = false;

  cloud.fields.resize(4);

  cloud.fields[0].name = "x";
  cloud.fields[0].offset = 0;
  cloud.fields[0].datatype = sensor_msgs::msg::PointField::FLOAT32;
  cloud.fields[0].count = 1;

  cloud.fields[1].name = "y";
  cloud.fields[1].offset = 4;
  cloud.fields[1].datatype = sensor_msgs::msg::PointField::FLOAT32;
  cloud.fields[1].count = 1;

  cloud.fields[2].name = "z";
  cloud.fields[2].offset = 8;
  cloud.fields[2].datatype = sensor_msgs::msg::PointField::FLOAT32;
  cloud.fields[2].count = 1;

  cloud.fields[3].name = "rgb";
  cloud.fields[3].offset = 12;
  cloud.fields[3].datatype = sensor_msgs::msg::PointField::FLOAT32;
  cloud.fields[3].count = 1;

  cloud.point_step = 16;
  cloud.row_step = cloud.point_step * cloud.width;
  cloud.data.resize(cloud.row_step);

  for (size_t i = 0; i < samples.size(); ++i) {
    const auto & s = samples[i];
    uint8_t * ptr = &cloud.data[i * cloud.point_step];

    std::memcpy(ptr + 0, &s.x, sizeof(float));
    std::memcpy(ptr + 4, &s.y, sizeof(float));
    std::memcpy(ptr + 8, &s.z, sizeof(float));

    uint8_t r = 80, g = 80, b = 80;

    if (s.inside) {
      r = 255;
      g = 0;
      b = 255;
    } else if (s.observed) {
      const float d = std::max(0.0f, std::min(1.0f, s.distance / 2.0f));
      r = static_cast<uint8_t>((1.0f - d) * 255.0f);
      g = 0;
      b = static_cast<uint8_t>(d * 255.0f);
    } else if (s.valid_block) {
      r = 120;
      g = 120;
      b = 120;
    }

    const uint32_t rgb =
      (static_cast<uint32_t>(r) << 16) |
      (static_cast<uint32_t>(g) << 8) |
      static_cast<uint32_t>(b);

    float rgb_float;
    std::memcpy(&rgb_float, &rgb, sizeof(float));
    std::memcpy(ptr + 12, &rgb_float, sizeof(float));
  }

  return cloud;
}

}  // namespace my_nvblox
