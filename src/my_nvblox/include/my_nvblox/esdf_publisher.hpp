// // #pragma once

// // #include <string>
// // #include <vector>

// // #include "rclcpp/rclcpp.hpp"
// // #include "sensor_msgs/msg/point_cloud2.hpp"

// // #include "nvblox/core/types.h"
// // #include "nvblox/mapper/mapper.h"

// // namespace my_nvblox
// // {

// // class EsdfPublisher
// // {
// // public:
// //   struct SamplePoint
// //   {
// //     float x;
// //     float y;
// //     float z;
// //     float distance;
// //     bool observed;
// //     bool inside;
// //   };

// //   EsdfPublisher(
// //     rclcpp::Node * node,
// //     const std::string & topic_name,
// //     const std::string & frame_id,
// //     float slice_height,
// //     float xy_min,
// //     float xy_max,
// //     float resolution);

// //   void publish(const nvblox::Mapper & mapper);

// // private:
// //   std::vector<SamplePoint> sample_esdf_slice(const nvblox::EsdfLayer & esdf_layer) const;
// //   sensor_msgs::msg::PointCloud2 make_cloud(
// //     const std::vector<SamplePoint> & samples) const;

// //   rclcpp::Publisher<sensor_msgs::msg::PointCloud2>::SharedPtr publisher_;
// //   std::string frame_id_;

// //   float slice_height_;
// //   float xy_min_;
// //   float xy_max_;
// //   float resolution_;
// // };

// // }  // namespace my_nvblox

// #pragma once

// #include <string>
// #include <vector>

// #include "rclcpp/rclcpp.hpp"
// #include "sensor_msgs/msg/point_cloud2.hpp"

// #include "nvblox/core/types.h"
// #include "nvblox/mapper/mapper.h"

// namespace my_nvblox
// {

// class EsdfPublisher
// {
// public:
//   struct SamplePoint
//   {
//     float x;
//     float y;
//     float z;
//     float distance;
//     bool observed;
//     bool inside;
//   };

//   EsdfPublisher(
//     rclcpp::Node * node,
//     const std::string & topic_name,
//     const std::string & frame_id,
//     float slice_height,
//     float xy_min,
//     float xy_max,
//     float resolution,
//     float max_visualized_distance);

//   void publish(const nvblox::Mapper & mapper);

// private:
//   std::vector<SamplePoint> sample_esdf_slice(const nvblox::EsdfLayer & esdf_layer) const;
//   sensor_msgs::msg::PointCloud2 make_cloud(
//     const std::vector<SamplePoint> & samples) const;

//   rclcpp::Publisher<sensor_msgs::msg::PointCloud2>::SharedPtr publisher_;
//   std::string frame_id_;

//   float slice_height_;
//   float xy_min_;
//   float xy_max_;
//   float resolution_;
//   float max_visualized_distance_;
// };

// }  // namespace my_nvblox
#pragma once

#include <string>
#include <vector>

#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/point_cloud2.hpp"

#include "nvblox/core/cuda_stream.h"
#include "nvblox/core/hash.h"
#include "nvblox/core/types.h"
#include "nvblox/mapper/mapper.h"
#include "nvblox/serialization/layer_serializer_gpu.h"

namespace my_nvblox
{

class EsdfPublisher
{
public:
  struct SamplePoint
  {
    float x;
    float y;
    float z;
    float distance;
    bool observed;
    bool inside;
    bool valid_block;
  };

  EsdfPublisher(
    rclcpp::Node * node,
    const std::string & topic_name,
    const std::string & frame_id,
    float slice_height,
    float xy_min,
    float xy_max,
    float resolution);

  void publish(const nvblox::Mapper & mapper);
  bool has_subscribers() const;

private:
  std::vector<SamplePoint> sample_esdf_slice(const nvblox::EsdfLayer & esdf_layer);
  sensor_msgs::msg::PointCloud2 make_cloud(
    const std::vector<SamplePoint> & samples) const;

  rclcpp::Logger logger_;
  rclcpp::Clock::SharedPtr clock_;
  rclcpp::Publisher<sensor_msgs::msg::PointCloud2>::SharedPtr publisher_;
  std::string frame_id_;

  float slice_height_;
  float xy_min_;
  float xy_max_;
  float resolution_;

  // ESDF blocks live in device memory by default.  Serialize the blocks in
  // the requested slice once, rather than dereferencing device pointers from
  // the CPU for every query point.
  nvblox::EsdfLayerSerializerGpu serializer_;
  nvblox::CudaStreamOwning cuda_stream_;
};

}  // namespace my_nvblox
