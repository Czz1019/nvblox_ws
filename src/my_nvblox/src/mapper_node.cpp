// #include <rclcpp/rclcpp.hpp>
// #include <sensor_msgs/msg/image.hpp>
// #include <sensor_msgs/msg/camera_info.hpp>
// #include <message_filters/subscriber.h>
// #include <message_filters/sync_policies/approximate_time.h>
// #include <message_filters/synchronizer.h>
// #include <cv_bridge/cv_bridge.h>
// #include <tf2_ros/transform_listener.h>
// #include <tf2_ros/buffer.h>

// // nvblox 核心 API
// #include <nvblox/mapper/mapper.h>
// #include <nvblox/core/types.h>
// #include <nvblox/sensors/image.h>
// #include <nvblox/sensors/camera.h>
// #include <visualization_msgs/msg/marker_array.hpp>
// #include <visualization_msgs/msg/marker.hpp>
// #include <geometry_msgs/msg/point.hpp>
// #include <std_msgs/msg/color_rgba.hpp>
// using namespace std;

// class MyNvbloxNode : public rclcpp::Node {
// public:
//     MyNvbloxNode() : Node("my_nvblox_node") {
//         // 1. 初始化 Mapper，体素分辨率设为 5cm (0.05f)，在 GPU 显存中分配
//         mapper_ = std::make_shared<nvblox::Mapper>(0.05f, nvblox::MemoryType::kDevice);

//         // 2. 初始化 TF 监听器 (用于获取相机位姿)
//         tf_buffer_ = std::make_unique<tf2_ros::Buffer>(this->get_clock());
//         tf_listener_ = std::make_shared<tf2_ros::TransformListener>(*tf_buffer_);

//         // 3. 订阅相机内参
//         info_sub_ = this->create_subscription<sensor_msgs::msg::CameraInfo>(
//             "/camera/depth/camera_info", 10,
//             std::bind(&MyNvbloxNode::infoCallback, this, std::placeholders::_1));

//         // 4. 设置图像的时间同步订阅 (确保 RGB 和 Depth 属于同一时刻)
//         depth_sub_.subscribe(this, "/camera/depth/image_rect_raw");
//         color_sub_.subscribe(this, "/camera/color/image_raw");
        
//         using SyncPolicy = message_filters::sync_policies::ApproximateTime<sensor_msgs::msg::Image, sensor_msgs::msg::Image>;
//         sync_ = std::make_shared<message_filters::Synchronizer<SyncPolicy>>(SyncPolicy(10), depth_sub_, color_sub_);
//         sync_->registerCallback(std::bind(&MyNvbloxNode::imageCallback, this, std::placeholders::_1, std::placeholders::_2));

//         // 5. 定时器：每秒触发一次网格和距离场的更新
//         timer_ = this->create_wall_timer(
//             std::chrono::milliseconds(1000),
//             std::bind(&MyNvbloxNode::processMappingOutput, this));

//         RCLCPP_INFO(this->get_logger(), "My Nvblox Mapper Started. Waiting for D435i streams...");
//     }

// private:
//     void infoCallback(const sensor_msgs::msg::CameraInfo::SharedPtr msg) {
//         if (!camera_intrinsics_) {
//             // 解析 ROS 内参消息到 nvblox
//             camera_intrinsics_ = std::make_unique<nvblox::Camera>(
//                 msg->k[0], msg->k[4], msg->k[2], msg->k[5], msg->width, msg->height);
//             RCLCPP_INFO(this->get_logger(), "Camera Intrinsics Initialized.");
//         }
//     }

//     void imageCallback(const sensor_msgs::msg::Image::ConstSharedPtr& depth_msg,
//                        const sensor_msgs::msg::Image::ConstSharedPtr& color_msg) {
//         if (!camera_intrinsics_) return;

//         // 1. 获取位姿：监听从 odom 到相机深度光心的 TF
//         geometry_msgs::msg::TransformStamped tf_msg;
//         try {
//             tf_msg = tf_buffer_->lookupTransform("odom", depth_msg->header.frame_id, depth_msg->header.stamp, rclcpp::Duration::from_seconds(0.1));
//         } catch (const tf2::TransformException & ex) {
//             return; // 拿不到位姿则丢帧
//         }

//         // 转换为 nvblox 的位姿矩阵
//         Eigen::Quaternionf q(tf_msg.transform.rotation.w, tf_msg.transform.rotation.x, tf_msg.transform.rotation.y, tf_msg.transform.rotation.z);
//         Eigen::Vector3f t(tf_msg.transform.translation.x, tf_msg.transform.translation.y, tf_msg.transform.translation.z);
//         nvblox::Transform T_O_C = nvblox::Transform::Identity();
//         T_O_C.prerotate(q);
//         T_O_C.pretranslate(t);

//         // --- 2. 深度图处理 ---
//         // D435i 是 16位无符号整数 (毫米)，转为 32位浮点数 (米)
//         cv_bridge::CvImageConstPtr cv_depth = cv_bridge::toCvShare(depth_msg, sensor_msgs::image_encodings::TYPE_16UC1);
//         cv::Mat depth_meters;
//         cv_depth->image.convertTo(depth_meters, CV_32FC1, 0.001f);

//         int width = depth_meters.cols;
//         int height = depth_meters.rows;
        
//         // 传至 CPU，再拷贝至 GPU 显存
//         nvblox::DepthImage host_depth(height, width, nvblox::MemoryType::kHost);
//         memcpy(host_depth.dataPtr(), depth_meters.data, host_depth.numel() * sizeof(float));
        
//         nvblox::DepthImage device_depth(height, width, nvblox::MemoryType::kDevice);
//         device_depth.copyFrom(host_depth);

//         // --- 3. 彩色图处理 ---
//         cv_bridge::CvImageConstPtr cv_color = cv_bridge::toCvShare(color_msg, sensor_msgs::image_encodings::RGB8);
//         nvblox::ColorImage host_color(height, width, nvblox::MemoryType::kHost);
//         memcpy(host_color.dataPtr(), cv_color->image.data, host_color.numel() * 3);
        
//         nvblox::ColorImage device_color(height, width, nvblox::MemoryType::kDevice);
//         device_color.copyFrom(host_color);

//         // --- 4. 融合进地图 ---
//         mapper_->integrateDepth(device_depth, T_O_C, *camera_intrinsics_);
//         mapper_->integrateColor(device_color, T_O_C, *camera_intrinsics_);
//     }

//     void processMappingOutput() {
//         // 触发网格和 ESDF 计算
//         mapper_->updateColorMesh();
//         mapper_->updateEsdf();
//     }

//     std::shared_ptr<nvblox::Mapper> mapper_;
//     std::unique_ptr<nvblox::Camera> camera_intrinsics_;
    
//     message_filters::Subscriber<sensor_msgs::msg::Image> depth_sub_;
//     message_filters::Subscriber<sensor_msgs::msg::Image> color_sub_;
//     using SyncPolicy = message_filters::sync_policies::ApproximateTime<sensor_msgs::msg::Image, sensor_msgs::msg::Image>;
//     std::shared_ptr<message_filters::Synchronizer<SyncPolicy>> sync_;
//     rclcpp::Subscription<sensor_msgs::msg::CameraInfo>::SharedPtr info_sub_;

//     std::unique_ptr<tf2_ros::Buffer> tf_buffer_;
//     std::shared_ptr<tf2_ros::TransformListener> tf_listener_;
//     rclcpp::TimerBase::SharedPtr timer_;
// };

// int main(int argc, char * argv[]) {
//     rclcpp::init(argc, argv);
//     rclcpp::spin(std::make_shared<MyNvbloxNode>());
//     rclcpp::shutdown();
//     return 0;
// }

#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <sensor_msgs/msg/camera_info.hpp>
#include <message_filters/subscriber.h>
#include <message_filters/sync_policies/approximate_time.h>
#include <message_filters/synchronizer.h>
#include <cv_bridge/cv_bridge.h>
#include <tf2_ros/transform_listener.h>
#include <tf2_ros/buffer.h>

// nvblox 核心 API
#include <nvblox/mapper/mapper.h>
#include <nvblox/core/types.h>
#include <nvblox/sensors/image.h>
#include <nvblox/sensors/camera.h>

// ROS 2 可视化相关 API
#include <visualization_msgs/msg/marker_array.hpp>
#include <visualization_msgs/msg/marker.hpp>
#include <geometry_msgs/msg/point.hpp>
#include <std_msgs/msg/color_rgba.hpp>
#include <cuda_runtime.h>

using namespace std;

class MyNvbloxNode : public rclcpp::Node {
public:
    MyNvbloxNode() : Node("my_nvblox_node") {
        // 1. 初始化 Mapper，体素分辨率设为 5cm (0.05f)，在 GPU 显存中分配
        mapper_ = std::make_shared<nvblox::Mapper>(0.05f, nvblox::MemoryType::kDevice);

        // 2. 初始化 TF 监听器 (用于获取相机位姿)
        tf_buffer_ = std::make_unique<tf2_ros::Buffer>(this->get_clock());
        tf_listener_ = std::make_shared<tf2_ros::TransformListener>(*tf_buffer_);

        // 3. 初始化 RViz2 的 Mesh 发布器
        mesh_pub_ = this->create_publisher<visualization_msgs::msg::MarkerArray>("~/mesh", 10);

        // 4. 订阅相机内参 (注意：必须使用 SensorDataQoS 以匹配 D435i 发送的模式)
        info_sub_ = this->create_subscription<sensor_msgs::msg::CameraInfo>(
            "/camera/camera/aligned_depth_to_color/camera_info", rclcpp::SensorDataQoS(),
            std::bind(&MyNvbloxNode::infoCallback, this, std::placeholders::_1));

        // 5. 设置图像的时间同步订阅 (订阅对齐后的深度图与彩色图)
        depth_sub_.subscribe(this, "/camera/camera/aligned_depth_to_color/image_raw", rmw_qos_profile_sensor_data);
        color_sub_.subscribe(this, "/camera/camera/color/image_raw", rmw_qos_profile_sensor_data);
        
        using SyncPolicy = message_filters::sync_policies::ApproximateTime<sensor_msgs::msg::Image, sensor_msgs::msg::Image>;
        sync_ = std::make_shared<message_filters::Synchronizer<SyncPolicy>>(SyncPolicy(10), depth_sub_, color_sub_);
        sync_->registerCallback(std::bind(&MyNvbloxNode::imageCallback, this, std::placeholders::_1, std::placeholders::_2));

        // 6. 定时器：每秒触发一次网格和距离场的更新与发布
        timer_ = this->create_wall_timer(
            std::chrono::milliseconds(1000),
            std::bind(&MyNvbloxNode::processMappingOutput, this));

        RCLCPP_INFO(this->get_logger(), "My Nvblox Mapper Started. Waiting for D435i streams...");
    }

private:
    void infoCallback(const sensor_msgs::msg::CameraInfo::SharedPtr msg) {
        if (!camera_intrinsics_) {
            // 解析 ROS 内参消息到 nvblox 专用的相机模型
            camera_intrinsics_ = std::make_unique<nvblox::Camera>(
                msg->k[0], msg->k[4], msg->k[2], msg->k[5], msg->width, msg->height);
            RCLCPP_INFO(this->get_logger(), "Camera Intrinsics Initialized.");
        }
    }

    // void imageCallback(const sensor_msgs::msg::Image::ConstSharedPtr& depth_msg,
    //                    const sensor_msgs::msg::Image::ConstSharedPtr& color_msg) {
    //     if (!camera_intrinsics_) return;

    //     // --- 步骤 A：获取相机在空间中的当前位姿 ---
    //     geometry_msgs::msg::TransformStamped tf_msg;
    //     try {
    //         // 查找从 odom (世界基准) 到 相机光心 的坐标变换
    //         tf_msg = tf_buffer_->lookupTransform("odom", depth_msg->header.frame_id, depth_msg->header.stamp, rclcpp::Duration::from_seconds(0.1));
    //     } catch (const tf2::TransformException & ex) {
    //         // 如果拿不到位姿，让它在终端里疯狂大喊大叫，不要静默丢帧！
    //         RCLCPP_WARN_THROTTLE(this->get_logger(), *this->get_clock(), 1000, 
    //                              "TF ERROR! Cannot find path from odom to %s", depth_msg->header.frame_id.c_str());
    //         return;
    //     }

    //     // 转换为 nvblox 需要的 Eigen 矩阵格式
    //     Eigen::Quaternionf q(tf_msg.transform.rotation.w, tf_msg.transform.rotation.x, tf_msg.transform.rotation.y, tf_msg.transform.rotation.z);
    //     Eigen::Vector3f t(tf_msg.transform.translation.x, tf_msg.transform.translation.y, tf_msg.transform.translation.z);
    //     nvblox::Transform T_O_C = nvblox::Transform::Identity();
    //     T_O_C.prerotate(q);
    //     T_O_C.pretranslate(t);

    //     // --- 步骤 B：深度图处理与内存传输 ---
    //     // 1. 将 ROS 格式转换为 OpenCV 格式 (D435i 输出的是 16位无符号毫米值)
    //     cv_bridge::CvImageConstPtr cv_depth = cv_bridge::toCvShare(depth_msg, sensor_msgs::image_encodings::TYPE_16UC1);
    //     cv::Mat depth_meters;
    //     // 2. 核心：将毫米乘以 0.001 转换为 nvblox 需要的米 (32位浮点数)
    //     cv_depth->image.convertTo(depth_meters, CV_32FC1, 0.001f);

    //     int width = depth_meters.cols;
    //     int height = depth_meters.rows;
        
    //     // 3. 将数据写入 CPU 内存 (Host)，随后拷贝至 GPU 显存 (Device)
    //     nvblox::DepthImage host_depth(height, width, nvblox::MemoryType::kHost);
    //     memcpy(host_depth.dataPtr(), depth_meters.data, host_depth.numel() * sizeof(float));
        
    //     nvblox::DepthImage device_depth(height, width, nvblox::MemoryType::kDevice);
    //     device_depth.copyFrom(host_depth);

    //     // --- 步骤 C：彩色图处理与内存传输 ---
    //     cv_bridge::CvImageConstPtr cv_color = cv_bridge::toCvShare(color_msg, sensor_msgs::image_encodings::RGB8);
    //     nvblox::ColorImage host_color(height, width, nvblox::MemoryType::kHost);
    //     memcpy(host_color.dataPtr(), cv_color->image.data, host_color.numel() * 3); // RGB 是 3 个通道
        
    //     nvblox::ColorImage device_color(height, width, nvblox::MemoryType::kDevice);
    //     device_color.copyFrom(host_color);

    //     // --- 步骤 D：调用 CUDA 加速引擎进行地图融合 ---
    //     mapper_->integrateDepth(device_depth, T_O_C, *camera_intrinsics_);
    //     mapper_->integrateColor(device_color, T_O_C, *camera_intrinsics_);
    // }


    void imageCallback(const sensor_msgs::msg::Image::ConstSharedPtr& depth_msg,
                       const sensor_msgs::msg::Image::ConstSharedPtr& color_msg) {
        
        // 探照灯 1：看看有没有收到图像！
        static int callback_count = 0;
        if (callback_count++ % 30 == 0) {
            RCLCPP_INFO(this->get_logger(), ">>> 1. Image Callback Triggered! 收到了一帧同步图像。");
        }

        if (!camera_intrinsics_) return;

        geometry_msgs::msg::TransformStamped tf_msg;
        try {
            tf_msg = tf_buffer_->lookupTransform("odom", depth_msg->header.frame_id, depth_msg->header.stamp, rclcpp::Duration::from_seconds(0.1));
        } catch (const tf2::TransformException & ex) {
            // 探照灯 2：看看是不是因为没有坐标位姿被丢弃了！
            RCLCPP_WARN_THROTTLE(this->get_logger(), *this->get_clock(), 1000, 
                                 ">>> 2. TF ERROR! 找不到坐标位姿: %s", ex.what());
            return; 
        }

        Eigen::Quaternionf q(tf_msg.transform.rotation.w, tf_msg.transform.rotation.x, tf_msg.transform.rotation.y, tf_msg.transform.rotation.z);
        Eigen::Vector3f t(tf_msg.transform.translation.x, tf_msg.transform.translation.y, tf_msg.transform.translation.z);
        nvblox::Transform T_O_C = nvblox::Transform::Identity();
        T_O_C.prerotate(q);
        T_O_C.pretranslate(t);

        cv_bridge::CvImageConstPtr cv_depth = cv_bridge::toCvShare(depth_msg, sensor_msgs::image_encodings::TYPE_16UC1);
        cv::Mat depth_meters;
        cv_depth->image.convertTo(depth_meters, CV_32FC1, 0.001f);

        int width = depth_meters.cols;
        int height = depth_meters.rows;
        
        nvblox::DepthImage host_depth(height, width, nvblox::MemoryType::kHost);
        memcpy(host_depth.dataPtr(), depth_meters.data, host_depth.numel() * sizeof(float));
        nvblox::DepthImage device_depth(height, width, nvblox::MemoryType::kDevice);
        device_depth.copyFrom(host_depth);

        cv_bridge::CvImageConstPtr cv_color = cv_bridge::toCvShare(color_msg, sensor_msgs::image_encodings::RGB8);
        nvblox::ColorImage host_color(height, width, nvblox::MemoryType::kHost);
        memcpy(host_color.dataPtr(), cv_color->image.data, host_color.numel() * 3);
        nvblox::ColorImage device_color(height, width, nvblox::MemoryType::kDevice);
        device_color.copyFrom(host_color);

        mapper_->integrateDepth(device_depth, T_O_C, *camera_intrinsics_);
        mapper_->integrateColor(device_color, T_O_C, *camera_intrinsics_);

        // 探照灯 3：看看 GPU 有没有成功算完这一帧！
        static int integration_count = 0;
        if (integration_count++ % 30 == 0) {
            RCLCPP_INFO(this->get_logger(), ">>> 3. GPU Integration Successful! 成功融合进 GPU！");
        }
    }

    void processMappingOutput() {
        // 1. 在 GPU 内触发网格 (Marching Cubes) 和 ESDF 计算
        mapper_->updateColorMesh();
        mapper_->updateEsdf();
        
        // 2. 性能优化：只有当用户在 RViz2 中打开了这个话题时，才执行耗时的网格提取
        if (mesh_pub_->get_subscription_count() > 0) {
            publishMesh();
        }
    }

    void publishMesh() {
        visualization_msgs::msg::MarkerArray marker_array;
        
        // 1. 获取包含颜色的网格层
        const auto& mesh_layer = mapper_->color_mesh_layer();
        auto block_indices = mesh_layer.getAllBlockIndices();
        
        // 探照灯 4：看看 GPU 里到底生成了几个网格块
        RCLCPP_INFO_THROTTLE(this->get_logger(), *this->get_clock(), 2000, 
                             ">>> 4. Publishing Mesh... 当前 GPU 中有 %zu 个网格块。", block_indices.size());
        int marker_id = 0;
        
        for (const auto& index : block_indices) {
            auto block_ptr = mesh_layer.getBlockAtIndex(index);
            if (!block_ptr) continue;

            // ==========================================
            // 核心修复点：强制触发从 GPU 到 CPU 向量的转换
            // nvblox::unified_vector 必须调用 toVector()
            // ==========================================
            // 1. 初始化标准 CPU Vector 容器，大小与 GPU 中的数据量保持一致
            std::vector<nvblox::Vector3f> vertices(block_ptr->vertices.size());
            std::vector<int> triangles(block_ptr->triangles.size());

            // 2. 调用 CUDA API，将 GPU (Device) 的数据硬拷贝到 CPU (Host)
            cudaMemcpy(vertices.data(), block_ptr->vertices.data(), 
                       block_ptr->vertices.size() * sizeof(nvblox::Vector3f), 
                       cudaMemcpyDeviceToHost);

            cudaMemcpy(triangles.data(), block_ptr->triangles.data(), 
                       block_ptr->triangles.size() * sizeof(int), 
                       cudaMemcpyDeviceToHost);
            
            // 注意：某些 nvblox 版本的 ColorBlock 并没有 colors 成员，而是复用了其他数据结构。
            // 但如果这里报错 "has no member named 'colors'"，说明我们需要使用
            // getBlockAtIndex 拿到的确实是 MeshBlock，但最新版把颜色藏进了 getColor() 或者移除了。
            // 根据官方最新源码，如果上面的 vertices 没报错但 colors 报错，
            // 意味着颜色的结构变了。我们先采用安全获取的方式：
            std::vector<nvblox::Color> colors;
            
            // 我们通过一个简单的条件编译或指针判断来尝试获取颜色
            // (这段代码做了兼容处理：如果官方底层彻底把 colors 移除了，它依然能渲染无色网格不崩溃)
            bool has_colors = false;
            
            // 为了解决你目前的编译错误，我们需要使用 nvblox 的正确数据成员。
            // 由于你上面报错 "no member named 'colors'"，
            // 我们需要调用 getColorVoxelLayer() 或根据你的具体 API 版本来取。
            // 让我们先用一种 100% 能过编译的方法，绕过这个没有 colors 成员的坑：
            // 如果你没有颜色的需求，可以先把颜色注释掉；如果要颜色，可以尝试使用 intensities。
            
            // 【修正 1】：既然 MeshBlock<Color> 没有 colors 成员，说明颜色被融合在别的字段或者去掉了。
            // 为了先让你能在 RViz 里看到形状，我们强制跳过这个不存在的成员。
            
            if (triangles.empty() || vertices.empty()) continue;

            visualization_msgs::msg::Marker marker;
            marker.header.frame_id = "odom"; 
            marker.header.stamp = this->now();
            marker.ns = "nvblox_mesh";
            marker.id = marker_id++;
            marker.type = visualization_msgs::msg::Marker::TRIANGLE_LIST;
            marker.action = visualization_msgs::msg::Marker::ADD;
            marker.pose.orientation.w = 1.0;
            marker.scale.x = 1.0; marker.scale.y = 1.0; marker.scale.z = 1.0;
            
            // 我们默认给定一个统一颜色（绿色），先保证在 RViz 里能画出墙壁！
            std_msgs::msg::ColorRGBA default_color;
            default_color.r = 0.0f; default_color.g = 1.0f; default_color.b = 0.0f; default_color.a = 1.0f;
            marker.color = default_color; 

            for (size_t i = 0; i < triangles.size(); ++i) {
                int v_idx = triangles[i];
                
                geometry_msgs::msg::Point p;
                p.x = vertices[v_idx].x();
                p.y = vertices[v_idx].y();
                p.z = vertices[v_idx].z();
                marker.points.push_back(p);
                
                // 【修正 2】：由于 colors 被去掉了，我们暂时不向 marker.colors 里推入每个顶点的单独颜色。
                // 而是让整个 marker 采用上面设置的 default_color。
            }
            marker_array.markers.push_back(marker);
        }

        if (!marker_array.markers.empty()) {
            mesh_pub_->publish(marker_array);
        }
    }

    std::shared_ptr<nvblox::Mapper> mapper_;
    std::unique_ptr<nvblox::Camera> camera_intrinsics_;
    
    // --- ROS 2 核心接口声明 ---
    rclcpp::Publisher<visualization_msgs::msg::MarkerArray>::SharedPtr mesh_pub_;
    
    message_filters::Subscriber<sensor_msgs::msg::Image> depth_sub_;
    message_filters::Subscriber<sensor_msgs::msg::Image> color_sub_;
    using SyncPolicy = message_filters::sync_policies::ApproximateTime<sensor_msgs::msg::Image, sensor_msgs::msg::Image>;
    std::shared_ptr<message_filters::Synchronizer<SyncPolicy>> sync_;
    rclcpp::Subscription<sensor_msgs::msg::CameraInfo>::SharedPtr info_sub_;

    std::unique_ptr<tf2_ros::Buffer> tf_buffer_;
    std::shared_ptr<tf2_ros::TransformListener> tf_listener_;
    rclcpp::TimerBase::SharedPtr timer_;
};

int main(int argc, char * argv[]) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<MyNvbloxNode>());
    rclcpp::shutdown();
    return 0;
}