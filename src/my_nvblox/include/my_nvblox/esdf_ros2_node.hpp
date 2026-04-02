#ifndef MY_NVBLOX_ESDF_ROS2_NODE_HPP_
#define MY_NVBLOX_ESDF_ROS2_NODE_HPP_

#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <sensor_msgs/msg/camera_info.hpp>
#include <geometry_msgs/msg/transform_stamped.hpp>
#include <tf2_ros/transform_listener.h>
#include <tf2_ros/buffer.h>
#include <tf2_eigen/tf2_eigen.hpp>

#include "my_nvblox/esdf_builder.hpp"

class EsdfRos2Node : public rclcpp::Node {
public:
    EsdfRos2Node();

private:
    void depthCallback(const sensor_msgs::msg::Image::SharedPtr msg);
    void cameraInfoCallback(const sensor_msgs::msg::CameraInfo::SharedPtr msg);

    std::unique_ptr<EsdfBuilder> esdf_builder_;

    rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr depth_sub_;
    rclcpp::Subscription<sensor_msgs::msg::CameraInfo>::SharedPtr cam_info_sub_;
    
    std::shared_ptr<tf2_ros::Buffer> tf_buffer_;
    std::shared_ptr<tf2_ros::TransformListener> tf_listener_;

    bool has_camera_info_{false};
    float fx_, fy_, cx_, cy_;
};

#endif