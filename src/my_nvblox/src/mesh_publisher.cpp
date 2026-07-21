// // #include "my_nvblox/mesh_publisher.hpp"

// // #include <vector>

// // #include "geometry_msgs/msg/point.hpp"
// // #include "std_msgs/msg/color_rgba.hpp"
// // #include "visualization_msgs/msg/marker.hpp"

// // namespace my_nvblox
// // {

// // MeshPublisher::MeshPublisher(
// //   rclcpp::Node * node,
// //   const std::string & topic_name,
// //   const std::string & frame_id)
// // : frame_id_(frame_id)
// // {
// //   publisher_ = node->create_publisher<visualization_msgs::msg::MarkerArray>(topic_name, 1);
// // }

// // void MeshPublisher::publish(const nvblox::Mapper & mapper)
// // {
// //   const auto & mesh_layer = mapper.color_mesh_layer();
// //   const auto block_indices = mesh_layer.getAllBlockIndices();

// //   visualization_msgs::msg::MarkerArray array;

// //   visualization_msgs::msg::Marker clear_marker;
// //   clear_marker.header.frame_id = frame_id_;
// //   clear_marker.header.stamp = rclcpp::Clock().now();
// //   clear_marker.ns = "nvblox_mesh";
// //   clear_marker.id = 0;
// //   clear_marker.action = visualization_msgs::msg::Marker::DELETEALL;
// //   array.markers.push_back(clear_marker);

// //   if (!block_indices.empty()) {
// //     array.markers.push_back(make_triangle_list_marker(mesh_layer, block_indices));
// //   }

// //   publisher_->publish(array);
// // }

// // visualization_msgs::msg::Marker MeshPublisher::make_triangle_list_marker(
// //   const nvblox::ColorMeshLayer & mesh_layer,
// //   const std::vector<nvblox::Index3D> & block_indices) const
// // {
// //   visualization_msgs::msg::Marker marker;
// //   marker.header.frame_id = frame_id_;
// //   marker.header.stamp = rclcpp::Clock().now();
// //   marker.ns = "nvblox_mesh";
// //   marker.id = 1;
// //   marker.type = visualization_msgs::msg::Marker::TRIANGLE_LIST;
// //   marker.action = visualization_msgs::msg::Marker::ADD;
// //   marker.pose.orientation.w = 1.0;
// //   marker.scale.x = 1.0;
// //   marker.scale.y = 1.0;
// //   marker.scale.z = 1.0;
// //   marker.lifetime = rclcpp::Duration::from_seconds(0.0);

// //   for (const auto & index : block_indices) {
// //     auto block = mesh_layer.getBlockAtIndex(index);
// //     if (!block) {
// //       continue;
// //     }

// //     const auto & vertices = block->vertices;
// //     const auto & triangles = block->triangles;
// //     const auto & colors = block->vertex_appearances;

// //     for (size_t i = 0; i + 2 < triangles.size(); i += 3) {
// //       for (int k = 0; k < 3; ++k) {
// //         const int vidx = triangles[i + k];
// //         if (vidx < 0 || static_cast<size_t>(vidx) >= vertices.size()) {
// //           continue;
// //         }

// //         geometry_msgs::msg::Point p;
// //         p.x = vertices[vidx].x();
// //         p.y = vertices[vidx].y();
// //         p.z = vertices[vidx].z();
// //         marker.points.push_back(p);

// //         std_msgs::msg::ColorRGBA c;
// //         c.a = 1.0f;
// //         if (static_cast<size_t>(vidx) < colors.size()) {
// //           c.r = static_cast<float>(colors[vidx].r()) / 255.0f;
// //           c.g = static_cast<float>(colors[vidx].g()) / 255.0f;
// //           c.b = static_cast<float>(colors[vidx].b()) / 255.0f;
// //         } else {
// //           c.r = 0.7f;
// //           c.g = 0.7f;
// //           c.b = 0.7f;
// //         }
// //         marker.colors.push_back(c);
// //       }
// //     }
// //   }

// //   return marker;
// // }

// // }  // namespace my_nvblox

// #include "my_nvblox/mesh_publisher.hpp"

// #include "visualization_msgs/msg/marker.hpp"

// namespace my_nvblox
// {

// MeshPublisher::MeshPublisher(
//   rclcpp::Node * node,
//   const std::string & topic_name,
//   const std::string & frame_id)
// : frame_id_(frame_id)
// {
//   publisher_ =
//     node->create_publisher<visualization_msgs::msg::MarkerArray>(topic_name, 1);
// }

// void MeshPublisher::publish(const nvblox::Mapper & /*mapper*/)
// {
//   visualization_msgs::msg::MarkerArray array;

//   visualization_msgs::msg::Marker clear_marker;
//   clear_marker.header.frame_id = frame_id_;
//   clear_marker.header.stamp = rclcpp::Clock().now();
//   clear_marker.ns = "nvblox_mesh";
//   clear_marker.id = 0;
//   clear_marker.action = visualization_msgs::msg::Marker::DELETEALL;
//   array.markers.push_back(clear_marker);

//   publisher_->publish(array);
// }

// visualization_msgs::msg::Marker MeshPublisher::make_triangle_list_marker(
//   const nvblox::ColorMeshLayer & /*mesh_layer*/,
//   const std::vector<nvblox::Index3D> & /*block_indices*/) const
// {
//   visualization_msgs::msg::Marker marker;
//   marker.header.frame_id = frame_id_;
//   marker.header.stamp = rclcpp::Clock().now();
//   marker.ns = "nvblox_mesh";
//   marker.id = 1;
//   marker.type = visualization_msgs::msg::Marker::TRIANGLE_LIST;
//   marker.action = visualization_msgs::msg::Marker::ADD;
//   marker.pose.orientation.w = 1.0;
//   marker.scale.x = 1.0;
//   marker.scale.y = 1.0;
//   marker.scale.z = 1.0;
//   marker.lifetime = rclcpp::Duration::from_seconds(0.0);
//   return marker;
// }

// }  // namespace my_nvblox



#include "my_nvblox/mesh_publisher.hpp"

#include "geometry_msgs/msg/point.hpp"
#include "std_msgs/msg/color_rgba.hpp"
#include "visualization_msgs/msg/marker.hpp"

namespace my_nvblox
{

MeshPublisher::MeshPublisher(
  rclcpp::Node * node,
  const std::string & topic_name,
  const std::string & frame_id)
: frame_id_(frame_id), cuda_stream_(cudaStreamDefault)
{
  publisher_ =
    node->create_publisher<visualization_msgs::msg::MarkerArray>(topic_name, 1);
}

void MeshPublisher::publish(const nvblox::Mapper & mapper)
{
  const auto & mesh_layer = mapper.color_mesh_layer();
  const auto block_indices = mesh_layer.getAllBlockIndices();

  visualization_msgs::msg::MarkerArray array;

  visualization_msgs::msg::Marker clear_marker;
  clear_marker.header.frame_id = frame_id_;
  clear_marker.header.stamp = rclcpp::Clock().now();
  clear_marker.ns = "nvblox_mesh";
  clear_marker.id = 0;
  clear_marker.action = visualization_msgs::msg::Marker::DELETEALL;
  array.markers.push_back(clear_marker);

  if (block_indices.empty()) {
    publisher_->publish(array);
    return;
  }

  const auto serialized_mesh =
    serializer_.serialize(mesh_layer, block_indices, cuda_stream_);
  cuda_stream_.synchronize();

  if (serialized_mesh &&
      !serialized_mesh->vertices.empty() &&
      !serialized_mesh->triangle_indices.empty())
  {
    array.markers.push_back(make_triangle_list_marker(*serialized_mesh));
  }

  publisher_->publish(array);
}

visualization_msgs::msg::Marker MeshPublisher::make_triangle_list_marker(
  const nvblox::SerializedColorMeshLayer & serialized_mesh) const
{
  visualization_msgs::msg::Marker marker;
  marker.header.frame_id = frame_id_;
  marker.header.stamp = rclcpp::Clock().now();
  marker.ns = "nvblox_mesh";
  marker.id = 1;
  marker.type = visualization_msgs::msg::Marker::TRIANGLE_LIST;
  marker.action = visualization_msgs::msg::Marker::ADD;
  marker.pose.orientation.w = 1.0;
  marker.scale.x = 1.0;
  marker.scale.y = 1.0;
  marker.scale.z = 1.0;
  marker.lifetime = rclcpp::Duration::from_seconds(0.0);

  const size_t num_blocks = serialized_mesh.block_indices.size();

  for (size_t block_idx = 0; block_idx < num_blocks; ++block_idx) {
    const size_t num_triangle_indices =
      serialized_mesh.getNumTriangleIndicesInBlock(block_idx);

    for (size_t i = 0; i + 2 < num_triangle_indices; i += 3) {
      for (int k = 0; k < 3; ++k) {
        const int local_vidx =
          serialized_mesh.getTriangleIndex(block_idx, i + static_cast<size_t>(k));

        if (local_vidx < 0 ||
            static_cast<size_t>(local_vidx) >=
              serialized_mesh.getNumVerticesInBlock(block_idx))
        {
          continue;
        }

        const auto & v =
          serialized_mesh.getVertex(block_idx, static_cast<size_t>(local_vidx));

        geometry_msgs::msg::Point p;
        p.x = v.x();
        p.y = v.y();
        p.z = v.z();
        marker.points.push_back(p);

        std_msgs::msg::ColorRGBA c;
        c.a = 1.0f;

        const auto & appearance =
          serialized_mesh.getAppearance(block_idx, static_cast<size_t>(local_vidx));
        c.r = static_cast<float>(appearance.r()) / 255.0f;
        c.g = static_cast<float>(appearance.g()) / 255.0f;
        c.b = static_cast<float>(appearance.b()) / 255.0f;

        marker.colors.push_back(c);
      }
    }
  }

  return marker;
}

}  // namespace my_nvblox