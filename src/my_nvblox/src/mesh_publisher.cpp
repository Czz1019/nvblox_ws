#include "my_nvblox/mesh_publisher.hpp"

MeshPublisher::MeshPublisher(rclcpp::Node* node) {
    mesh_pub_ = node->create_publisher<visualization_msgs::msg::MarkerArray>("~/mesh", 10);
}

void MeshPublisher::publish(const std::shared_ptr<nvblox::Mapper>& mapper) {
    // 触发 GPU 上的并行网格化算法 
    mapper->updateMesh();
    
    const auto& mesh_layer = mapper->mesh_layer();
    auto block_indices = mesh_layer.getAllBlockIndices();
    if (block_indices.empty()) return;

    visualization_msgs::msg::MarkerArray markers;
    for (const auto& index : block_indices) {
        auto block = mesh_layer.getBlockAtIndex(index);
        if (!block || block->vertices.empty()) continue;

        visualization_msgs::msg::Marker m;
        m.header.frame_id = "odom";
        m.header.stamp = rclcpp::Clock().now();
        m.ns = "mesh";
        m.id = index.x() ^ index.y() ^ index.z(); // 简单哈希 ID
        m.type = visualization_msgs::msg::Marker::TRIANGLE_LIST;
        m.scale.x = m.scale.y = m.scale.z = 1.0;
        m.color.a = 1.0; m.color.g = 1.0; // 默认绿色

        // 将 GPU 顶点转换为 ROS 消息
        for (size_t i = 0; i < block->vertices.size(); ++i) {
            geometry_msgs::msg::Point p;
            p.x = block->vertices[i].x();
            p.y = block->vertices[i].y();
            p.z = block->vertices[i].z();
            m.points.push_back(p);
        }
        markers.markers.push_back(m);
    }
    mesh_pub_->publish(markers);
}