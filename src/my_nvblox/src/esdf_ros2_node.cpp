#include "my_nvblox/esdf_ros2_node.hpp"

EsdfRos2Node::EsdfRos2Node() : Node("nvblox_esdf_node") {
    // 实例化 Builder，设置体素分辨率为 5cm
    esdf_builder_ = std::make_unique<EsdfBuilder>(0.05f); 

    // 初始化 TF 监听器
    tf_buffer_ = std::make_shared<tf2_ros::Buffer>(this->get_clock());
    tf_listener_ = std::make_shared<tf2_ros::TransformListener>(*tf_buffer_);

    // 订阅内参和深度图
    cam_info_sub_ = this->create_subscription<sensor_msgs::msg::CameraInfo>(
        "/camera/depth/camera_info", 10,
        std::bind(&EsdfRos2Node::cameraInfoCallback, this, std::placeholders::_1));

    depth_sub_ = this->create_subscription<sensor_msgs::msg::Image>(
        "/camera/depth/image_raw", 10,
        std::bind(&EsdfRos2Node::depthCallback, this, std::placeholders::_1));

    RCLCPP_INFO(this->get_logger(), "ESDF Node initialized and waiting for data...");
}

void EsdfRos2Node::cameraInfoCallback(const sensor_msgs::msg::CameraInfo::SharedPtr msg) {
    if (!has_camera_info_) {
        fx_ = msg->k[0];
        fy_ = msg->k[4];
        cx_ = msg->k[2];
        cy_ = msg->k[5];
        has_camera_info_ = true;
        RCLCPP_INFO(this->get_logger(), "Received Camera Info: fx=%f, fy=%f", fx_, fy_);
    }
}

void EsdfRos2Node::depthCallback(const sensor_msgs::msg::Image::SharedPtr msg) {
    if (!has_camera_info_) {
        RCLCPP_WARN_THROTTLE(this->get_logger(), *this->get_clock(), 1000, "Waiting for camera_info...");
        return;
    }

    // nvblox 需要浮点数格式的深度图（单位：米）
    if (msg->encoding != "32FC1") {
        RCLCPP_ERROR_THROTTLE(this->get_logger(), *this->get_clock(), 1000, "Depth image must be 32FC1 format!");
        return;
    }

    geometry_msgs::msg::TransformStamped transform_stamped;
    try {
        // 获取当前时间戳从全局坐标系(odom)到相机坐标系(frame_id)的变换
        transform_stamped = tf_buffer_->lookupTransform(
            "odom", msg->header.frame_id, msg->header.stamp, rclcpp::Duration::from_seconds(0.1));
    } catch (const tf2::TransformException & ex) {
        RCLCPP_WARN_THROTTLE(this->get_logger(), *this->get_clock(), 1000, "TF Error: %s", ex.what());
        return;
    }

    // 转为 Eigen 格式
    Eigen::Isometry3d T_W_C_64 = tf2::transformToEigen(transform_stamped);
    Eigen::Isometry3f T_W_C = T_W_C_64.cast<float>();

    // 取出图像裸数据并传给底层建图算法
    const float* depth_data = reinterpret_cast<const float*>(msg->data.data());
    esdf_builder_->integrateDepthFrame(depth_data, msg->width, msg->height, T_W_C, fx_, fy_, cx_, cy_);
    esdf_builder_->updateEsdf();

    RCLCPP_INFO_THROTTLE(this->get_logger(), *this->get_clock(), 2000, "ESDF Field Updated!");
}

int main(int argc, char ** argv) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<EsdfRos2Node>());
    rclcpp::shutdown();
    return 0;
}