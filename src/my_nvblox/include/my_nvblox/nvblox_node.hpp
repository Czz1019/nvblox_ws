#ifndef MY_NVBLOX_NVBLOX_NODE_HPP_
#define MY_NVBLOX_NVBLOX_NODE_HPP_

#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <sensor_msgs/msg/camera_info.hpp>
#include <message_filters/subscriber.h>
#include <message_filters/sync_policies/approximate_time.h>
#include <tf2_ros/transform_listener.h>
#include <tf2_ros/buffer.h>
#include <cv_bridge/cv_bridge.h>

// nvblox 核心头文件
#include <nvblox/nvblox.h>

class NvbloxNode : public rclcpp::Node {
public:
    NvbloxNode();

private:
    // 同步回调函数
    void dataCallback(
        const sensor_msgs::msg::Image::ConstSharedPtr& depth_msg,
        const sensor_msgs::msg::Image::ConstSharedPtr& color_msg,
        const sensor_msgs::msg::CameraInfo::ConstSharedPtr& info_msg);

    // 核心引擎：Mapper 负责管理 GPU 上的 TSDF、Color、ESDF 等层 [cite: 101, 114]
    std::shared_ptr<nvblox::Mapper> mapper_;

    // TF 监听
    std::unique_ptr<tf2_ros::Buffer> tf_buffer_;
    std::shared_ptr<tf2_ros::TransformListener> tf_listener_;
    
    std::unique_ptr<MeshPublisher> mesh_publisher_;
    std::unique_ptr<EsdfPublisher> esdf_publisher_;
    rclcpp::TimerBase::SharedPtr timer_;
    
    // Message Filters 订阅者
    message_filters::Subscriber<sensor_msgs::msg::Image> depth_sub_;
    message_filters::Subscriber<sensor_msgs::msg::Image> color_sub_;
    message_filters::Subscriber<sensor_msgs::msg::CameraInfo> info_sub_;

    // 同步策略：深度、彩色、内参三者对齐
    typedef message_filters::sync_policies::ApproximateTime<
        sensor_msgs::msg::Image, sensor_msgs::msg::Image, sensor_msgs::msg::CameraInfo> SyncPolicy;
    std::shared_ptr<message_filters::Synchronizer<SyncPolicy>> sync_;
};

#endif