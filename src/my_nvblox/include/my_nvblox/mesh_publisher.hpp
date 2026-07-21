// #pragma once

// #include <string>
// #include <vector>

// #include "rclcpp/rclcpp.hpp"
// #include "visualization_msgs/msg/marker_array.hpp"

// #include "nvblox/mapper/mapper.h"
// #include "nvblox/map/common_names.h"
// #include "nvblox/mesh/mesh_block.h"

// namespace my_nvblox
// {

// class MeshPublisher
// {
// public:
//   MeshPublisher(
//     rclcpp::Node * node,
//     const std::string & topic_name,
//     const std::string & frame_id);

//   void publish(const nvblox::Mapper & mapper);

// private:
//   visualization_msgs::msg::Marker make_triangle_list_marker(
//     const nvblox::ColorMeshLayer & mesh_layer,
//     const std::vector<nvblox::Index3D> & block_indices) const;

//   rclcpp::Publisher<visualization_msgs::msg::MarkerArray>::SharedPtr publisher_;
//   std::string frame_id_;
// };

// }  // namespace my_nvblox


#pragma once

#include <string>

#include "rclcpp/rclcpp.hpp"
#include "visualization_msgs/msg/marker_array.hpp"

#include "nvblox/core/cuda_stream.h"
#include "nvblox/mapper/mapper.h"
#include "nvblox/map/common_names.h"
#include "nvblox/serialization/mesh_serializer_gpu.h"

namespace my_nvblox
{

class MeshPublisher
{
public:
  MeshPublisher(
    rclcpp::Node * node,
    const std::string & topic_name,
    const std::string & frame_id);

  void publish(const nvblox::Mapper & mapper);

private:
  visualization_msgs::msg::Marker make_triangle_list_marker(
    const nvblox::SerializedColorMeshLayer & serialized_mesh) const;

  rclcpp::Publisher<visualization_msgs::msg::MarkerArray>::SharedPtr publisher_;
  std::string frame_id_;

  nvblox::CudaStreamOwning cuda_stream_;
  nvblox::ColorMeshSerializerGpu serializer_;
};

}  // namespace my_nvblox