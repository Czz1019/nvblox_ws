#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "rclcpp/rclcpp.hpp"

#include "sensor_msgs/msg/point_cloud2.hpp"
#include "sensor_msgs/msg/point_field.hpp"
#include "sensor_msgs/point_cloud2_iterator.hpp"

#include "std_msgs/msg/color_rgba.hpp"
#include "visualization_msgs/msg/marker.hpp"

// nvblox
#include "nvblox/nvblox.h"
#include "nvblox/core/indexing.h"
#include "nvblox/map_saving/internal/layer_type_register.h"

namespace
{

struct VisualizationPoint
{
  float x;
  float y;
  float z;
  float intensity;
};

sensor_msgs::msg::PointCloud2 createPointCloudMessage(
  const std::vector<VisualizationPoint>& points,
  const std::string& frame_id,
  const builtin_interfaces::msg::Time& stamp)
{
  sensor_msgs::msg::PointCloud2 cloud;

  cloud.header.frame_id = frame_id;
  cloud.header.stamp = stamp;
  cloud.height = 1;
  cloud.width = static_cast<std::uint32_t>(points.size());
  cloud.is_bigendian = false;
  cloud.is_dense = true;

  sensor_msgs::PointCloud2Modifier modifier(cloud);

  modifier.setPointCloud2Fields(
    4,
    "x", 1, sensor_msgs::msg::PointField::FLOAT32,
    "y", 1, sensor_msgs::msg::PointField::FLOAT32,
    "z", 1, sensor_msgs::msg::PointField::FLOAT32,
    "intensity", 1, sensor_msgs::msg::PointField::FLOAT32);

  modifier.resize(points.size());

  sensor_msgs::PointCloud2Iterator<float> iter_x(cloud, "x");
  sensor_msgs::PointCloud2Iterator<float> iter_y(cloud, "y");
  sensor_msgs::PointCloud2Iterator<float> iter_z(cloud, "z");
  sensor_msgs::PointCloud2Iterator<float> iter_intensity(cloud, "intensity");

  for (const VisualizationPoint& point : points) {
    *iter_x = point.x;
    *iter_y = point.y;
    *iter_z = point.z;
    *iter_intensity = point.intensity;

    ++iter_x;
    ++iter_y;
    ++iter_z;
    ++iter_intensity;
  }

  return cloud;
}

}  // namespace

class SavedMapVisualizer : public rclcpp::Node
{
public:
  SavedMapVisualizer()
  : Node("saved_nvblox_map_visualizer")
  {
    map_path_ = declare_parameter<std::string>(
      "map_path",
      "/home/czz/nvblox_ws/output/realtime_map.nvblox");

    frame_id_ = declare_parameter<std::string>("frame_id", "odom");

    publish_mesh_ =
      declare_parameter<bool>("publish_mesh", true);

    publish_tsdf_ =
      declare_parameter<bool>("publish_tsdf", true);

    publish_esdf_ =
      declare_parameter<bool>("publish_esdf", true);

    point_stride_ =
      std::max(1, declare_parameter<int>("point_stride", 1));

    tsdf_min_weight_ =
      static_cast<float>(
      declare_parameter<double>("tsdf_min_weight", 0.1));

    tsdf_max_abs_distance_m_ =
      static_cast<float>(
      declare_parameter<double>("tsdf_max_abs_distance_m", 0.0));

    esdf_max_distance_m_ =
      static_cast<float>(
      declare_parameter<double>("esdf_max_distance_m", 0.0));

    mesh_min_weight_ =
      static_cast<float>(
      declare_parameter<double>("mesh_min_weight", 0.1));

    mesh_alpha_ =
      static_cast<float>(
      declare_parameter<double>("mesh_alpha", 1.0));

    republish_period_s_ =
      std::max(
      1.0,
      declare_parameter<double>("republish_period_s", 5.0));

    // 每个 MeshBlock 使用单独的 Marker。
    // 深度设置大一些，让 transient_local 能保存所有静态网格块。
    rclcpp::QoS marker_qos(rclcpp::KeepLast(2048));
    marker_qos.reliable();
    marker_qos.transient_local();

    rclcpp::QoS cloud_qos(rclcpp::KeepLast(1));
    cloud_qos.reliable();
    cloud_qos.transient_local();

    mesh_publisher_ =
      create_publisher<visualization_msgs::msg::Marker>(
      "/nvblox_saved_map/mesh_blocks",
      marker_qos);

    tsdf_publisher_ =
      create_publisher<sensor_msgs::msg::PointCloud2>(
      "/nvblox_saved_map/tsdf_voxels",
      cloud_qos);

    esdf_publisher_ =
      create_publisher<sensor_msgs::msg::PointCloud2>(
      "/nvblox_saved_map/esdf_voxels",
      cloud_qos);

    loadMap();

    if (publish_mesh_) {
      generateMesh();
    }

    if (publish_tsdf_) {
      buildTsdfPointCloud();
    }

    if (publish_esdf_) {
      buildEsdfPointCloud();
    }

    timer_ = create_wall_timer(
      std::chrono::duration<double>(republish_period_s_),
      std::bind(&SavedMapVisualizer::publishVisualization, this));

    RCLCPP_INFO(
      get_logger(),
      "Saved map visualizer ready. frame_id=%s",
      frame_id_.c_str());
  }

private:
  void loadMap()
  {
    if (!std::filesystem::exists(map_path_)) {
      throw std::runtime_error(
              "Map file does not exist: " + map_path_);
    }

    RCLCPP_INFO(
      get_logger(),
      "Loading nvblox map: %s",
      map_path_.c_str());

    /*
     * 使用 Unified Memory 加载。
     *
     * Mesh 和 ESDF 仍然可以被 GPU 算法使用，同时 CPU 可以读取体素，
     * 转换成 ROS Marker 和 PointCloud2。
     */
    layer_cake_ = nvblox::io::loadLayerCakeFromFile(
      map_path_,
      nvblox::MemoryType::kUnified);

    if (layer_cake_.empty()) {
      throw std::runtime_error(
              "The nvblox map contains no readable layers.");
    }

    RCLCPP_INFO(
      get_logger(),
      "Map loaded: voxel_size=%.4f m, block_size=%.4f m",
      layer_cake_.voxel_size(),
      layer_cake_.block_size());

    if (layer_cake_.exists<nvblox::TsdfLayer>()) {
      const auto& tsdf_layer =
        layer_cake_.get<nvblox::TsdfLayer>();

      RCLCPP_INFO(
        get_logger(),
        "TSDF blocks: %d",
        tsdf_layer.numBlocks());
    } else {
      RCLCPP_WARN(
        get_logger(),
        "No TSDF layer found.");
    }

    if (layer_cake_.exists<nvblox::EsdfLayer>()) {
      const auto& esdf_layer =
        layer_cake_.get<nvblox::EsdfLayer>();

      RCLCPP_INFO(
        get_logger(),
        "ESDF blocks: %d",
        esdf_layer.numBlocks());
    } else {
      RCLCPP_WARN(
        get_logger(),
        "No ESDF layer found.");
    }

    if (layer_cake_.exists<nvblox::ColorLayer>()) {
      const auto& color_layer =
        layer_cake_.get<nvblox::ColorLayer>();

      RCLCPP_INFO(
        get_logger(),
        "Color blocks: %d",
        color_layer.numBlocks());
    }
  }

  void generateMesh()
  {
    if (!layer_cake_.exists<nvblox::TsdfLayer>()) {
      RCLCPP_WARN(
        get_logger(),
        "Cannot generate mesh because TSDF layer is absent.");
      return;
    }

    RCLCPP_INFO(
      get_logger(),
      "Generating mesh from TSDF...");

    mesh_layer_ =
      std::make_unique<nvblox::ColorMeshLayer>(
      layer_cake_.block_size(),
      nvblox::MemoryType::kUnified);

    nvblox::ColorMeshIntegrator mesh_integrator;

    // 允许低权重区域参与网格提取。
    mesh_integrator.min_weight(mesh_min_weight_);

    const bool success =
      mesh_integrator.integrateMeshFromDistanceField(
      layer_cake_.get<nvblox::TsdfLayer>(),
      mesh_layer_.get());

    if (!success) {
      RCLCPP_ERROR(
        get_logger(),
        "Failed to generate mesh from TSDF.");
      mesh_layer_.reset();
      return;
    }

    /*
     * 只有真正存在 Color 体素块时才更新颜色。
     * 你的 realtime_map.nvblox 中 Color 数据为空，因此会跳过。
     */
    if (
      layer_cake_.exists<nvblox::ColorLayer>() &&
      layer_cake_.get<nvblox::ColorLayer>().numBlocks() > 0)
    {
      mesh_integrator.updateAppearance(
        layer_cake_.get<nvblox::ColorLayer>(),
        mesh_layer_.get());

      mesh_has_color_ = true;
    }

    RCLCPP_INFO(
      get_logger(),
      "Mesh generated: %d blocks",
      mesh_layer_->numBlocks());
  }

  void buildTsdfPointCloud()
  {
    if (!layer_cake_.exists<nvblox::TsdfLayer>()) {
      return;
    }

    const nvblox::TsdfLayer& tsdf_layer =
      layer_cake_.get<nvblox::TsdfLayer>();

    const float block_size_m = tsdf_layer.block_size();

    std::vector<VisualizationPoint> points;
    points.reserve(
      static_cast<std::size_t>(tsdf_layer.numBlocks()) *
      nvblox::VoxelBlock<nvblox::TsdfVoxel>::kNumVoxels);

    std::size_t accepted_point_count = 0;

    const std::vector<nvblox::Index3D> block_indices =
      tsdf_layer.getAllBlockIndices();

    for (const nvblox::Index3D& block_index : block_indices) {
      const auto block =
        tsdf_layer.getBlockAtIndex(block_index);

      if (!block) {
        continue;
      }

      for (
        int x = 0;
        x < nvblox::VoxelBlock<nvblox::TsdfVoxel>::kVoxelsPerSide;
        ++x)
      {
        for (
          int y = 0;
          y < nvblox::VoxelBlock<nvblox::TsdfVoxel>::kVoxelsPerSide;
          ++y)
        {
          for (
            int z = 0;
            z < nvblox::VoxelBlock<nvblox::TsdfVoxel>::kVoxelsPerSide;
            ++z)
          {
            const nvblox::TsdfVoxel& voxel =
              block->voxels[x][y][z];

            // weight 为 0 表示没有被观测。
            if (voxel.weight < tsdf_min_weight_) {
              continue;
            }

            if (
              tsdf_max_abs_distance_m_ > 0.0F &&
              std::abs(voxel.distance) > tsdf_max_abs_distance_m_)
            {
              continue;
            }

            if (
              accepted_point_count %
              static_cast<std::size_t>(point_stride_) != 0)
            {
              ++accepted_point_count;
              continue;
            }

            ++accepted_point_count;

            const nvblox::Index3D voxel_index(x, y, z);

            const nvblox::Vector3f position =
              nvblox::getCenterPositionFromBlockIndexAndVoxelIndex(
              block_size_m,
              block_index,
              voxel_index);

            points.push_back(
              {
                position.x(),
                position.y(),
                position.z(),
                voxel.distance
              });
          }
        }
      }
    }

    tsdf_cloud_ = createPointCloudMessage(
      points,
      frame_id_,
      now());

    RCLCPP_INFO(
      get_logger(),
      "TSDF point cloud built: %zu points",
      points.size());
  }

  void buildEsdfPointCloud()
  {
    if (!layer_cake_.exists<nvblox::EsdfLayer>()) {
      return;
    }

    const nvblox::EsdfLayer& esdf_layer =
      layer_cake_.get<nvblox::EsdfLayer>();

    const float block_size_m = esdf_layer.block_size();
    const float voxel_size_m = esdf_layer.voxel_size();

    std::vector<VisualizationPoint> points;
    points.reserve(
      static_cast<std::size_t>(esdf_layer.numBlocks()) *
      nvblox::VoxelBlock<nvblox::EsdfVoxel>::kNumVoxels);

    std::size_t accepted_point_count = 0;

    const std::vector<nvblox::Index3D> block_indices =
      esdf_layer.getAllBlockIndices();

    for (const nvblox::Index3D& block_index : block_indices) {
      const auto block =
        esdf_layer.getBlockAtIndex(block_index);

      if (!block) {
        continue;
      }

      for (
        int x = 0;
        x < nvblox::VoxelBlock<nvblox::EsdfVoxel>::kVoxelsPerSide;
        ++x)
      {
        for (
          int y = 0;
          y < nvblox::VoxelBlock<nvblox::EsdfVoxel>::kVoxelsPerSide;
          ++y)
        {
          for (
            int z = 0;
            z < nvblox::VoxelBlock<nvblox::EsdfVoxel>::kVoxelsPerSide;
            ++z)
          {
            const nvblox::EsdfVoxel& voxel =
              block->voxels[x][y][z];

            if (!voxel.observed) {
              continue;
            }

            /*
             * nvblox 的 ESDF 中没有 distance 成员。
             *
             * squared_distance_vox:
             *   以“体素个数平方”为单位的距离平方。
             */
            float distance_m =
              std::sqrt(
              std::max(
                0.0F,
                voxel.squared_distance_vox)) *
              voxel_size_m;

            if (voxel.is_inside) {
              distance_m = -distance_m;
            }

            if (
              esdf_max_distance_m_ > 0.0F &&
              std::abs(distance_m) > esdf_max_distance_m_)
            {
              continue;
            }

            if (
              accepted_point_count %
              static_cast<std::size_t>(point_stride_) != 0)
            {
              ++accepted_point_count;
              continue;
            }

            ++accepted_point_count;

            const nvblox::Index3D voxel_index(x, y, z);

            const nvblox::Vector3f position =
              nvblox::getCenterPositionFromBlockIndexAndVoxelIndex(
              block_size_m,
              block_index,
              voxel_index);

            points.push_back(
              {
                position.x(),
                position.y(),
                position.z(),
                distance_m
              });
          }
        }
      }
    }

    esdf_cloud_ = createPointCloudMessage(
      points,
      frame_id_,
      now());

    RCLCPP_INFO(
      get_logger(),
      "ESDF point cloud built: %zu points",
      points.size());
  }

  void publishMesh()
  {
    if (!mesh_layer_) {
      return;
    }

    // 清除 RViz 中旧的 MeshBlock Marker。
    visualization_msgs::msg::Marker clear_marker;
    clear_marker.header.frame_id = frame_id_;
    clear_marker.header.stamp = now();
    clear_marker.ns = "nvblox_saved_mesh";
    clear_marker.id = 0;
    clear_marker.action =
      visualization_msgs::msg::Marker::DELETEALL;

    mesh_publisher_->publish(clear_marker);

    const auto stamp = now();
    int marker_id = 0;
    std::size_t total_triangle_count = 0;

    const std::vector<nvblox::Index3D> block_indices =
      mesh_layer_->getAllBlockIndices();

    for (const nvblox::Index3D& block_index : block_indices) {
      const auto block =
        mesh_layer_->getBlockAtIndex(block_index);

      if (!block || block->triangles.empty()) {
        continue;
      }

      visualization_msgs::msg::Marker marker;

      marker.header.frame_id = frame_id_;
      marker.header.stamp = stamp;

      marker.ns = "nvblox_saved_mesh";
      marker.id = marker_id++;

      marker.type =
        visualization_msgs::msg::Marker::TRIANGLE_LIST;

      marker.action =
        visualization_msgs::msg::Marker::ADD;

      marker.pose.orientation.w = 1.0;

      marker.scale.x = 1.0;
      marker.scale.y = 1.0;
      marker.scale.z = 1.0;

      // 当文件中没有 ColorLayer 时使用统一颜色。
      marker.color.r = 0.65F;
      marker.color.g = 0.72F;
      marker.color.b = 0.82F;
      marker.color.a = mesh_alpha_;

      const bool block_has_color =
        mesh_has_color_ &&
        block->vertex_appearances.size() ==
        block->vertices.size();

      marker.points.reserve(block->triangles.size());

      if (block_has_color) {
        marker.colors.reserve(block->triangles.size());
      }

      /*
       * triangles 中保存的是顶点索引。
       * 每连续三个索引构成一个三角形。
       */
      for (
        std::size_t triangle_offset = 0;
        triangle_offset + 2 < block->triangles.size();
        triangle_offset += 3)
      {
        const int vertex_indices[3] = {
          block->triangles[triangle_offset],
          block->triangles[triangle_offset + 1],
          block->triangles[triangle_offset + 2]
        };

        bool valid_triangle = true;

        for (const int vertex_index : vertex_indices) {
          if (
            vertex_index < 0 ||
            static_cast<std::size_t>(vertex_index) >=
            block->vertices.size())
          {
            valid_triangle = false;
            break;
          }
        }

        if (!valid_triangle) {
          continue;
        }

        for (const int vertex_index : vertex_indices) {
          const nvblox::Vector3f& vertex =
            block->vertices[vertex_index];

          geometry_msgs::msg::Point point;
          point.x = vertex.x();
          point.y = vertex.y();
          point.z = vertex.z();

          marker.points.push_back(point);

          if (block_has_color) {
            const nvblox::Color& color =
              block->vertex_appearances[vertex_index];

            std_msgs::msg::ColorRGBA ros_color;
            ros_color.r =
              static_cast<float>(color.r()) / 255.0F;
            ros_color.g =
              static_cast<float>(color.g()) / 255.0F;
            ros_color.b =
              static_cast<float>(color.b()) / 255.0F;
            ros_color.a = mesh_alpha_;

            marker.colors.push_back(ros_color);
          }
        }

        ++total_triangle_count;
      }

      if (!marker.points.empty()) {
        mesh_publisher_->publish(marker);
      }
    }

    RCLCPP_DEBUG(
      get_logger(),
      "Published %d mesh markers and %zu triangles",
      marker_id,
      total_triangle_count);
  }

  void publishVisualization()
  {
    /*
     * 只在有订阅者时发布，避免 RViz 未启动时不断发送大量数据。
     */
    if (
      publish_mesh_ &&
      mesh_layer_ &&
      mesh_publisher_->get_subscription_count() > 0)
    {
      publishMesh();
    }

    if (
      publish_tsdf_ &&
      !tsdf_cloud_.data.empty() &&
      tsdf_publisher_->get_subscription_count() > 0)
    {
      tsdf_cloud_.header.stamp = now();
      tsdf_publisher_->publish(tsdf_cloud_);
    }

    if (
      publish_esdf_ &&
      !esdf_cloud_.data.empty() &&
      esdf_publisher_->get_subscription_count() > 0)
    {
      esdf_cloud_.header.stamp = now();
      esdf_publisher_->publish(esdf_cloud_);
    }
  }

private:
  std::string map_path_;
  std::string frame_id_;

  bool publish_mesh_{true};
  bool publish_tsdf_{true};
  bool publish_esdf_{true};

  int point_stride_{1};

  float tsdf_min_weight_{0.1F};
  float tsdf_max_abs_distance_m_{0.0F};
  float esdf_max_distance_m_{0.0F};

  float mesh_min_weight_{0.1F};
  float mesh_alpha_{1.0F};

  double republish_period_s_{5.0};

  bool mesh_has_color_{false};

  nvblox::LayerCake layer_cake_;

  std::unique_ptr<nvblox::ColorMeshLayer> mesh_layer_;

  sensor_msgs::msg::PointCloud2 tsdf_cloud_;
  sensor_msgs::msg::PointCloud2 esdf_cloud_;

  rclcpp::Publisher<visualization_msgs::msg::Marker>::SharedPtr
    mesh_publisher_;

  rclcpp::Publisher<sensor_msgs::msg::PointCloud2>::SharedPtr
    tsdf_publisher_;

  rclcpp::Publisher<sensor_msgs::msg::PointCloud2>::SharedPtr
    esdf_publisher_;

  rclcpp::TimerBase::SharedPtr timer_;
};

int main(int argc, char** argv)
{
  rclcpp::init(argc, argv);

  try {
    rclcpp::spin(
      std::make_shared<SavedMapVisualizer>());
  } catch (const std::exception& exception) {
    RCLCPP_FATAL(
      rclcpp::get_logger("saved_map_visualizer"),
      "Fatal error: %s",
      exception.what());

    rclcpp::shutdown();
    return 1;
  }

  rclcpp::shutdown();
  return 0;
}
