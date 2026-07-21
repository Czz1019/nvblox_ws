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
#include <cmath>
#include <cstdint>
#include <cstring>
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
  resolution_(resolution)
{
  publisher_ = node->create_publisher<sensor_msgs::msg::PointCloud2>(topic_name, 1);
}

void EsdfPublisher::publish(const nvblox::Mapper & mapper)
{
  const auto & esdf_layer = mapper.esdf_layer();
  auto samples = sample_esdf_slice(esdf_layer);
  auto cloud = make_cloud(samples);
  publisher_->publish(cloud);
}

std::vector<EsdfPublisher::SamplePoint> EsdfPublisher::sample_esdf_slice(
  const nvblox::EsdfLayer & esdf_layer)
{
  std::vector<SamplePoint> samples;

  const float voxel_size = esdf_layer.voxel_size();
  const float block_size = esdf_layer.block_size();
  constexpr int kVoxelsPerSide = nvblox::VoxelBlock<nvblox::EsdfVoxel>::kVoxelsPerSide;

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

      auto block = esdf_layer.getBlockAtIndex(block_idx);
      if (block) {
        s.valid_block = true;
        ++valid_block_count;

        const auto & voxel =
          block->voxels[voxel_idx.x()][voxel_idx.y()][voxel_idx.z()];

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