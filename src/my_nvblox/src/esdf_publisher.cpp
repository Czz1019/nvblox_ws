#include "my_nvblox/esdf_publisher.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>
using namespace std;

namespace my_nvblox
{

EsdfPublisher::EsdfPublisher(
  rclcpp::Node * node,
  const std::string & topic_name,
  const std::string & frame_id,
  float slice_height,
  float xy_min,
  float xy_max,
  float resolution)
: frame_id_(frame_id),
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
  const auto samples = sample_esdf_slice(esdf_layer);

  sensor_msgs::msg::PointCloud2 cloud;
  cloud.header.frame_id = frame_id_;
  cloud.header.stamp = rclcpp::Clock().now();
  cloud.height = 1;
  cloud.width = 0;
  cloud.is_dense = false;
  cloud.is_bigendian = false;

  sensor_msgs::PointCloud2Modifier modifier(cloud);
  modifier.setPointCloud2Fields(
    4,
    "x", 1, sensor_msgs::msg::PointField::FLOAT32,
    "y", 1, sensor_msgs::msg::PointField::FLOAT32,
    "z", 1, sensor_msgs::msg::PointField::FLOAT32,
    "rgb", 1, sensor_msgs::msg::PointField::FLOAT32);

  size_t valid_count = 0;
  for (const auto & s : samples) {
    if (s.valid) {
      ++valid_count;
    }
  }

  modifier.resize(valid_count);

  sensor_msgs::PointCloud2Iterator<float> iter_x(cloud, "x");
  sensor_msgs::PointCloud2Iterator<float> iter_y(cloud, "y");
  sensor_msgs::PointCloud2Iterator<float> iter_z(cloud, "z");
  sensor_msgs::PointCloud2Iterator<float> iter_rgb(cloud, "rgb");

  for (const auto & s : samples) {
    if (!s.valid) {
      continue;
    }

    uint8_t r = 0, g = 0, b = 0;
    distance_to_rgb(s.distance, r, g, b);

    uint32_t rgb = (static_cast<uint32_t>(r) << 16) |
                   (static_cast<uint32_t>(g) << 8) |
                   static_cast<uint32_t>(b);
    float rgb_float;
    std::memcpy(&rgb_float, &rgb, sizeof(float));

    *iter_x = s.x;
    *iter_y = s.y;
    *iter_z = s.z;
    *iter_rgb = rgb_float;

    ++iter_x;
    ++iter_y;
    ++iter_z;
    ++iter_rgb;
  }

  publisher_->publish(cloud);
}

std::vector<EsdfPublisher::SamplePoint> EsdfPublisher::sample_esdf_slice(
  const nvblox::EsdfLayer & esdf_layer) const
{
  std::vector<SamplePoint> out;

  for (float y = xy_min_; y <= xy_max_; y += resolution_) {
    for (float x = xy_min_; x <= xy_max_; x += resolution_) {
      SamplePoint s;
      s.x = x;
      s.y = y;
      s.z = slice_height_;
      s.distance = 0.0f;
      s.valid = false;

      std::vector<nvblox::Vector3f> positions{
        nvblox::Vector3f(x, y, slice_height_)
      };
      std::vector<nvblox::EsdfVoxel> voxels;
      std::vector<bool> success;

      esdf_layer.getVoxels(positions, &voxels, &success);

      if (!success.empty() && success[0] && !voxels.empty()) {
        const auto & voxel = voxels[0];
        if (voxel.observed) {
          float distance_m =
            std::sqrt(voxel.squared_distance_vox) * esdf_layer.voxel_size();
          if (voxel.is_inside) {
            distance_m = -distance_m;
          }
          s.distance = distance_m;
          s.valid = std::isfinite(s.distance);
        }
      }

      out.push_back(s);
    }
  }

  return out;
}

void EsdfPublisher::distance_to_rgb(float distance, uint8_t & r, uint8_t & g, uint8_t & b)
{
  const float d = std::clamp(distance, 0.0f, 1.0f);
  const float t = d / 1.0f;

  r = static_cast<uint8_t>((1.0f - t) * 255.0f);
  g = 0;
  b = static_cast<uint8_t>(t * 255.0f);
}

}  // namespace my_nvblox