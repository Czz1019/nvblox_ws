#ifndef MY_NVBLOX_MESH_PUBLISHER_HPP_
#define MY_NVBLOX_MESH_PUBLISHER_HPP_

#include <rclcpp/rclcpp.hpp>
#include <visualization_msgs/msg/marker_array.hpp>
#include <nvblox/nvblox.h>

class MeshPublisher {
public:
    explicit MeshPublisher(rclcpp::Node* node);
    // 提取 GPU 中的 MeshLayer 并发布 [cite: 114]
    void publish(const std::shared_ptr<nvblox::Mapper>& mapper);

private:
    rclcpp::Publisher<visualization_msgs::msg::MarkerArray>::SharedPtr mesh_pub_;
};

#endif