#include "my_nvblox/esdf_publisher.hpp"
#include <sensor_msgs/point_cloud2_iterator.hpp>

EsdfPublisher::EsdfPublisher(rclcpp::Node* node) {
    esdf_pub_ = node->create_publisher<sensor_msgs::msg::PointCloud2>("~/static_esdf_pointcloud", 10);
}

void EsdfPublisher::publish(const std::shared_ptr<nvblox::Mapper>& mapper) {
    // 触发 GPU 上的并行增量 ESDF 更新 [cite: 154, 163]
    mapper->updateEsdf();

    const auto& esdf_layer = mapper->esdf_layer();
    float slice_z = 0.5f; // 设定的切片高度
    
    sensor_msgs::msg::PointCloud2 cloud;
    cloud.header.frame_id = "odom";
    cloud.header.stamp = rclcpp::Clock().now();
    
    sensor_msgs::PointCloud2Modifier modifier(cloud);
    modifier.setPointCloud2FieldsByString(2, "xyz", "rgb");
    
    // 逻辑：遍历活跃区域，查询距离值 [cite: 170, 261]
    // 此处简化演示：实际中需遍历 esdf_layer 中的 VoxelBlocks [cite: 110]
    // ... (执行查询逻辑并填充点云) ...

    esdf_pub_->publish(cloud);
}