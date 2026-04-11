#ifndef MY_NVBLOX_ESDF_PUBLISHER_HPP_
#define MY_NVBLOX_ESDF_PUBLISHER_HPP_

#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/point_cloud2.hpp>
#include <nvblox/nvblox.h>

class EsdfPublisher {
public:
    explicit EsdfPublisher(rclcpp::Node* node);
    // 触发增量式 ESDF 更新并提取切片 [cite: 163, 245]
    void publish(const std::shared_ptr<nvblox::Mapper>& mapper);

private:
    rclcpp::Publisher<sensor_msgs::msg::PointCloud2>::SharedPtr esdf_pub_;
};

#endif