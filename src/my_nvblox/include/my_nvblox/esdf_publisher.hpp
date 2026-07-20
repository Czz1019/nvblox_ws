#pragma once

#include <memory>
#include <string>
#include <vector>

#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/point_cloud2.hpp"
#include "sensor_msgs/point_cloud2_iterator.hpp"

#include "nvblox/mapper/mapper.h"
#include "nvblox/map/common_names.h"

namespace my_nvblox
{

class EsdfPublisher
{
public:
  EsdfPublisher(
    rclcpp::Node * node,
    const std::string & topic_name,
    const std::string & frame_id,
    float slice_height,
    float xy_min,
    float xy_max,
    float resolution);

  void publish(const nvblox::Mapper & mapper);

private:
  struct SamplePoint
  {
    float x;
    float y;
    float z;
    float distance;
    bool valid;
  };

  std::vector<SamplePoint> sample_esdf_slice(const nvblox::EsdfLayer & esdf_layer) const;

  static void distance_to_rgb(float distance, uint8_t & r, uint8_t & g, uint8_t & b);

  rclcpp::Publisher<sensor_msgs::msg::PointCloud2>::SharedPtr publisher_;
  std::string frame_id_;

  float slice_height_;
  float xy_min_;
  float xy_max_;
  float resolution_;
};

}  // namespace my_nvblox