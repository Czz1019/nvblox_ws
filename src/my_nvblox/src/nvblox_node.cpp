#include "my_nvblox/nvblox_node.hpp"
#include "my_nvblox/mesh_publisher.hpp"
#include "my_nvblox/esdf_publisher.hpp"

NvbloxNode::NvbloxNode() : Node("nvblox_node") {
    // 初始化 Mapper：设定体素大小 (5cm) 与 显存位置 (MemoryType::kDevice) [cite: 105, 110]
    float voxel_size_m = 0.05f;
    mapper_ = std::make_shared<nvblox::Mapper>(voxel_size_m, nvblox::MemoryType::kDevice);

    // 初始化 TF
    tf_buffer_ = std::make_unique<tf2_ros::Buffer>(this->get_clock());
    tf_listener_ = std::make_shared<tf2_ros::TransformListener>(*tf_buffer_);

    // 订阅话题
    depth_sub_.subscribe(this, "/camera/aligned_depth_to_color/image_raw");
    color_sub_.subscribe(this, "/camera/color/image_raw");
    info_sub_.subscribe(this, "/camera/aligned_depth_to_color/camera_info");

    // 设置同步回调
    sync_ = std::make_shared<message_filters::Synchronizer<SyncPolicy>>(
        SyncPolicy(10), depth_sub_, color_sub_, info_sub_);
    sync_->registerCallback(std::bind(&NvbloxNode::dataCallback, this, 
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

    RCLCPP_INFO(this->get_logger(), "调度层初始化完成：GPU 显存装载就绪。");
}

void NvbloxNode::dataCallback(
    const sensor_msgs::msg::Image::ConstSharedPtr& depth_msg,
    const sensor_msgs::msg::Image::ConstSharedPtr& color_msg,
    const sensor_msgs::msg::CameraInfo::ConstSharedPtr& info_msg) 
{
    // 1. 获取最新位姿 T_L_C (odom -> camera_link) [cite: 96, 121]
    nvblox::Transform T_L_C;
    try {
        auto t = tf_buffer_->lookupTransform("odom", depth_msg->header.frame_id, depth_msg->header.stamp);
        Eigen::Quaternionf q(t.transform.rotation.w, t.transform.rotation.x, t.transform.rotation.y, t.transform.rotation.z);
        Eigen::Vector3f trans(t.transform.translation.x, t.transform.translation.y, t.transform.translation.z);
        T_L_C = nvblox::Transform::Identity();
        T_L_C.prerotate(q);
        T_L_C.pretranslate(trans);
    } catch (const tf2::TransformException & ex) { return; }

    // 2. 构造相机内参对象 [cite: 122, 128]
    nvblox::Camera camera(info_msg->k[0], info_msg->k[4], info_msg->k[2], info_msg->k[5], info_msg->width, info_msg->height);

    // 3. 内存转换：ROS -> CPU Mat -> GPU Image
    auto cv_depth = cv_bridge::toCvShare(depth_msg, "16UC1");
    auto cv_color = cv_bridge::toCvShare(color_msg, "rgb8");

    // 直接在显存分配空间并执行 CUDA 拷贝 [cite: 110, 117]
    nvblox::DepthImage gpu_depth(info_msg->height, info_msg->width, nvblox::MemoryType::kDevice);
    nvblox::ColorImage gpu_color(info_msg->height, info_msg->width, nvblox::MemoryType::kDevice);

    cudaMemcpy(gpu_depth.dataPtr(), cv_depth->image.data, gpu_depth.numBytes(), cudaMemcpyHostToDevice);
    cudaMemcpy(gpu_color.dataPtr(), cv_color->image.data, gpu_color.numBytes(), cudaMemcpyHostToDevice);

    // 4. 调用 GPU 引擎执行 TSDF 与 Color 积分 [cite: 92, 137]
    mapper_->integrateDepth(gpu_depth, T_L_C, camera);
    mapper_->integrateColor(gpu_color, T_L_C, camera);
}

int main(int argc, char * argv[]) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<NvbloxNode>());
    rclcpp::shutdown();
    return 0;
}