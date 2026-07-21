// #pragma once

// #include <memory>
// #include <mutex>
// #include <string>

// #include <cuda_runtime.h>
// #include <opencv2/core.hpp>

// #include "cv_bridge/cv_bridge.h"
// #include "message_filters/subscriber.h"
// #include "message_filters/sync_policies/approximate_time.h"
// #include "message_filters/synchronizer.h"

// #include "rclcpp/rclcpp.hpp"
// #include "sensor_msgs/msg/camera_info.hpp"
// #include "sensor_msgs/msg/image.hpp"

// #include "tf2_eigen/tf2_eigen.hpp"
// #include "tf2_ros/buffer.h"
// #include "tf2_ros/transform_listener.h"

// #include "nvblox/mapper/mapper.h"
// #include "nvblox/sensors/camera.h"
// #include "nvblox/sensors/image.h"

// #include "my_nvblox/esdf_publisher.hpp"
// #include "my_nvblox/mesh_publisher.hpp"

// namespace my_nvblox
// {

// class NvbloxNode : public rclcpp::Node
// {
// public:
//   explicit NvbloxNode(const rclcpp::NodeOptions & options = rclcpp::NodeOptions());

// private:
//   using ImageMsg = sensor_msgs::msg::Image;
//   using CameraInfoMsg = sensor_msgs::msg::CameraInfo;

//   using ApproxSyncPolicy = message_filters::sync_policies::ApproximateTime<
//     ImageMsg, ImageMsg>;
//   using ApproxSynchronizer = message_filters::Synchronizer<ApproxSyncPolicy>;

//   void synced_callback(
//     const ImageMsg::ConstSharedPtr & depth_msg,
//     const ImageMsg::ConstSharedPtr & color_msg);

//   void camera_info_callback(const CameraInfoMsg::SharedPtr msg);

//   void publish_timer_callback();

//   nvblox::Camera make_camera_from_info(const CameraInfoMsg & msg) const;

//   nvblox::Transform lookup_pose(
//     const std::string & target_frame,
//     const std::string & source_frame,
//     const rclcpp::Time & stamp) const;

//   void integrate_depth(
//     const cv::Mat & depth_mat,
//     const nvblox::Transform & T_L_C,
//     const nvblox::Camera & camera);

//   void integrate_color(
//     const cv::Mat & color_bgr_mat,
//     const nvblox::Transform & T_L_C,
//     const nvblox::Camera & camera);

//   static void check_cuda(cudaError_t code, const char * expr);

//   std::string global_frame_;
//   std::string camera_frame_;
//   std::string depth_topic_;
//   std::string color_topic_;
//   std::string camera_info_topic_;

//   double voxel_size_;
//   int publish_period_ms_;

//   message_filters::Subscriber<ImageMsg> depth_sub_;
//   message_filters::Subscriber<ImageMsg> color_sub_;
//   std::shared_ptr<ApproxSynchronizer> sync_;

//   rclcpp::Subscription<CameraInfoMsg>::SharedPtr camera_info_sub_raw_;
//   CameraInfoMsg::SharedPtr latest_camera_info_;
//   std::mutex camera_info_mutex_;

//   std::shared_ptr<tf2_ros::Buffer> tf_buffer_;
//   std::shared_ptr<tf2_ros::TransformListener> tf_listener_;

//   std::shared_ptr<nvblox::Mapper> mapper_;

//   std::unique_ptr<MeshPublisher> mesh_publisher_;
//   std::unique_ptr<EsdfPublisher> esdf_publisher_;

//   rclcpp::TimerBase::SharedPtr publish_timer_;
//   mutable std::mutex mapper_mutex_;

//   size_t integrated_frame_count_{0};
// };

// }  // namespace my_nvblox

/*-------yijieduan*/
// #pragma once

// #include <memory>
// #include <mutex>
// #include <string>

// #include <cuda_runtime.h>
// #include <opencv2/core.hpp>

// #include "cv_bridge/cv_bridge.h"
// #include "message_filters/subscriber.h"
// #include "message_filters/sync_policies/approximate_time.h"
// #include "message_filters/synchronizer.h"

// #include "rclcpp/rclcpp.hpp"
// #include "sensor_msgs/msg/camera_info.hpp"
// #include "sensor_msgs/msg/image.hpp"

// #include "tf2_eigen/tf2_eigen.hpp"
// #include "tf2_ros/buffer.h"
// #include "tf2_ros/transform_listener.h"

// #include "nvblox/mapper/mapper.h"
// #include "nvblox/sensors/camera.h"
// #include "nvblox/sensors/image.h"

// namespace my_nvblox
// {

// class NvbloxNode : public rclcpp::Node
// {
// public:
//   explicit NvbloxNode(const rclcpp::NodeOptions & options = rclcpp::NodeOptions());

// private:
//   using ImageMsg = sensor_msgs::msg::Image;
//   using CameraInfoMsg = sensor_msgs::msg::CameraInfo;

//   using ApproxSyncPolicy =
//     message_filters::sync_policies::ApproximateTime<ImageMsg, ImageMsg>;
//   using ApproxSynchronizer = message_filters::Synchronizer<ApproxSyncPolicy>;

//   void synced_callback(
//     const ImageMsg::ConstSharedPtr & depth_msg,
//     const ImageMsg::ConstSharedPtr & color_msg);

//   void camera_info_callback(const CameraInfoMsg::SharedPtr msg);

//   nvblox::Camera make_camera_from_info(const CameraInfoMsg & msg) const;

//   nvblox::Transform lookup_pose(
//     const std::string & target_frame,
//     const std::string & source_frame,
//     const rclcpp::Time & stamp) const;

//   void integrate_depth(
//     const cv::Mat & depth_mat,
//     const nvblox::Transform & T_L_C,
//     const nvblox::Camera & camera);

//   void integrate_color(
//     const cv::Mat & color_bgr_mat,
//     const nvblox::Transform & T_L_C,
//     const nvblox::Camera & camera);

//   static void check_cuda(cudaError_t code, const char * expr);

//   std::string global_frame_;
//   std::string camera_frame_;
//   std::string depth_topic_;
//   std::string color_topic_;
//   std::string camera_info_topic_;

//   double voxel_size_;
//   int publish_period_ms_;

//   message_filters::Subscriber<ImageMsg> depth_sub_;
//   message_filters::Subscriber<ImageMsg> color_sub_;
//   std::shared_ptr<ApproxSynchronizer> sync_;

//   rclcpp::Subscription<CameraInfoMsg>::SharedPtr camera_info_sub_raw_;
//   CameraInfoMsg::SharedPtr latest_camera_info_;
//   std::mutex camera_info_mutex_;

//   std::shared_ptr<tf2_ros::Buffer> tf_buffer_;
//   std::shared_ptr<tf2_ros::TransformListener> tf_listener_;

//   std::shared_ptr<nvblox::Mapper> mapper_;
//   mutable std::mutex mapper_mutex_;

//   size_t integrated_frame_count_{0};
// };

// }  // namespace my_nvblox



/*----------------------er jie duan-------------------*/
// #pragma once

// #include <memory>
// #include <mutex>
// #include <string>

// #include <cuda_runtime.h>
// #include <opencv2/core.hpp>

// #include "cv_bridge/cv_bridge.h"
// #include "message_filters/subscriber.h"
// #include "message_filters/sync_policies/approximate_time.h"
// #include "message_filters/synchronizer.h"

// #include "rclcpp/rclcpp.hpp"
// #include "sensor_msgs/msg/camera_info.hpp"
// #include "sensor_msgs/msg/image.hpp"

// #include "tf2_eigen/tf2_eigen.hpp"
// #include "tf2_ros/buffer.h"
// #include "tf2_ros/transform_listener.h"

// #include "nvblox/mapper/mapper.h"
// #include "nvblox/sensors/camera.h"
// #include "nvblox/sensors/image.h"

// namespace my_nvblox
// {

// class NvbloxNode : public rclcpp::Node
// {
// public:
//   explicit NvbloxNode(const rclcpp::NodeOptions & options = rclcpp::NodeOptions());

// private:
//   using ImageMsg = sensor_msgs::msg::Image;
//   using CameraInfoMsg = sensor_msgs::msg::CameraInfo;

//   using ApproxSyncPolicy =
//     message_filters::sync_policies::ApproximateTime<ImageMsg, ImageMsg>;
//   using ApproxSynchronizer = message_filters::Synchronizer<ApproxSyncPolicy>;

//   void synced_callback(
//     const ImageMsg::ConstSharedPtr & depth_msg,
//     const ImageMsg::ConstSharedPtr & color_msg);

//   void camera_info_callback(const CameraInfoMsg::SharedPtr msg);

//   void publish_timer_callback();

//   nvblox::Camera make_camera_from_info(const CameraInfoMsg & msg) const;

//   nvblox::Transform lookup_pose(
//     const std::string & target_frame,
//     const std::string & source_frame,
//     const rclcpp::Time & stamp) const;

//   void integrate_depth(
//     const cv::Mat & depth_mat,
//     const nvblox::Transform & T_L_C,
//     const nvblox::Camera & camera);

//   void integrate_color(
//     const cv::Mat & color_bgr_mat,
//     const nvblox::Transform & T_L_C,
//     const nvblox::Camera & camera);

//   static void check_cuda(cudaError_t code, const char * expr);

//   std::string global_frame_;
//   std::string camera_frame_;
//   std::string depth_topic_;
//   std::string color_topic_;
//   std::string camera_info_topic_;

//   double voxel_size_;
//   int publish_period_ms_;

//   message_filters::Subscriber<ImageMsg> depth_sub_;
//   message_filters::Subscriber<ImageMsg> color_sub_;
//   std::shared_ptr<ApproxSynchronizer> sync_;

//   rclcpp::Subscription<CameraInfoMsg>::SharedPtr camera_info_sub_raw_;
//   CameraInfoMsg::SharedPtr latest_camera_info_;
//   std::mutex camera_info_mutex_;

//   std::shared_ptr<tf2_ros::Buffer> tf_buffer_;
//   std::shared_ptr<tf2_ros::TransformListener> tf_listener_;

//   std::shared_ptr<nvblox::Mapper> mapper_;
//   mutable std::mutex mapper_mutex_;

//   rclcpp::TimerBase::SharedPtr publish_timer_;

//   size_t integrated_frame_count_{0};
// };

// }  // namespace my_nvblox

/*----2B---*/
// #pragma once

// #include <memory>
// #include <mutex>
// #include <string>

// #include <cuda_runtime.h>
// #include <opencv2/core.hpp>

// #include "cv_bridge/cv_bridge.h"
// #include "message_filters/subscriber.h"
// #include "message_filters/sync_policies/approximate_time.h"
// #include "message_filters/synchronizer.h"

// #include "rclcpp/rclcpp.hpp"
// #include "sensor_msgs/msg/camera_info.hpp"
// #include "sensor_msgs/msg/image.hpp"

// #include "tf2_eigen/tf2_eigen.hpp"
// #include "tf2_ros/buffer.h"
// #include "tf2_ros/transform_listener.h"

// #include "nvblox/mapper/mapper.h"
// #include "nvblox/sensors/camera.h"
// #include "nvblox/sensors/image.h"

// #include "my_nvblox/mesh_publisher.hpp"

// namespace my_nvblox
// {

// class NvbloxNode : public rclcpp::Node
// {
// public:
//   explicit NvbloxNode(const rclcpp::NodeOptions & options = rclcpp::NodeOptions());

// private:
//   using ImageMsg = sensor_msgs::msg::Image;
//   using CameraInfoMsg = sensor_msgs::msg::CameraInfo;

//   using ApproxSyncPolicy =
//     message_filters::sync_policies::ApproximateTime<ImageMsg, ImageMsg>;
//   using ApproxSynchronizer = message_filters::Synchronizer<ApproxSyncPolicy>;

//   void synced_callback(
//     const ImageMsg::ConstSharedPtr & depth_msg,
//     const ImageMsg::ConstSharedPtr & color_msg);

//   void camera_info_callback(const CameraInfoMsg::SharedPtr msg);

//   void publish_timer_callback();

//   nvblox::Camera make_camera_from_info(const CameraInfoMsg & msg) const;

//   nvblox::Transform lookup_pose(
//     const std::string & target_frame,
//     const std::string & source_frame,
//     const rclcpp::Time & stamp) const;

//   void integrate_depth(
//     const cv::Mat & depth_mat,
//     const nvblox::Transform & T_L_C,
//     const nvblox::Camera & camera);

//   void integrate_color(
//     const cv::Mat & color_bgr_mat,
//     const nvblox::Transform & T_L_C,
//     const nvblox::Camera & camera);

//   static void check_cuda(cudaError_t code, const char * expr);

//   std::string global_frame_;
//   std::string camera_frame_;
//   std::string depth_topic_;
//   std::string color_topic_;
//   std::string camera_info_topic_;

//   double voxel_size_;
//   int publish_period_ms_;

//   message_filters::Subscriber<ImageMsg> depth_sub_;
//   message_filters::Subscriber<ImageMsg> color_sub_;
//   std::shared_ptr<ApproxSynchronizer> sync_;

//   rclcpp::Subscription<CameraInfoMsg>::SharedPtr camera_info_sub_raw_;
//   CameraInfoMsg::SharedPtr latest_camera_info_;
//   std::mutex camera_info_mutex_;

//   std::shared_ptr<tf2_ros::Buffer> tf_buffer_;
//   std::shared_ptr<tf2_ros::TransformListener> tf_listener_;

//   std::shared_ptr<nvblox::Mapper> mapper_;
//   mutable std::mutex mapper_mutex_;

//   std::unique_ptr<MeshPublisher> mesh_publisher_;
//   rclcpp::TimerBase::SharedPtr publish_timer_;

//   size_t integrated_frame_count_{0};
// };

// }  // namespace my_nvblox

#pragma once

#include <memory>
#include <mutex>
#include <string>

#include <cuda_runtime.h>
#include <opencv2/core.hpp>

#include "cv_bridge/cv_bridge.h"
#include "message_filters/subscriber.h"
#include "message_filters/sync_policies/approximate_time.h"
#include "message_filters/synchronizer.h"

#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/camera_info.hpp"
#include "sensor_msgs/msg/image.hpp"

#include "tf2_eigen/tf2_eigen.hpp"
#include "tf2_ros/buffer.h"
#include "tf2_ros/transform_listener.h"

#include "nvblox/mapper/mapper.h"
#include "nvblox/sensors/camera.h"
#include "nvblox/sensors/image.h"

#include "my_nvblox/esdf_publisher.hpp"
#include "my_nvblox/mesh_publisher.hpp"

namespace my_nvblox
{

class NvbloxNode : public rclcpp::Node
{
public:
  explicit NvbloxNode(const rclcpp::NodeOptions & options = rclcpp::NodeOptions());

private:
  using ImageMsg = sensor_msgs::msg::Image;
  using CameraInfoMsg = sensor_msgs::msg::CameraInfo;

  using ApproxSyncPolicy =
    message_filters::sync_policies::ApproximateTime<ImageMsg, ImageMsg>;
  using ApproxSynchronizer = message_filters::Synchronizer<ApproxSyncPolicy>;

  void synced_callback(
    const ImageMsg::ConstSharedPtr & depth_msg,
    const ImageMsg::ConstSharedPtr & color_msg);

  void camera_info_callback(const CameraInfoMsg::SharedPtr msg);

  void publish_timer_callback();

  nvblox::Camera make_camera_from_info(const CameraInfoMsg & msg) const;

  nvblox::Transform lookup_pose(
    const std::string & target_frame,
    const std::string & source_frame,
    const rclcpp::Time & stamp) const;

  void integrate_depth(
    const cv::Mat & depth_mat,
    const nvblox::Transform & T_L_C,
    const nvblox::Camera & camera);

  void integrate_color(
    const cv::Mat & color_bgr_mat,
    const nvblox::Transform & T_L_C,
    const nvblox::Camera & camera);

  static void check_cuda(cudaError_t code, const char * expr);

  std::string global_frame_;
  std::string camera_frame_;
  std::string depth_topic_;
  std::string color_topic_;
  std::string camera_info_topic_;

  double voxel_size_;
  int publish_period_ms_;
  double esdf_slice_height_;
  double esdf_xy_min_;
  double esdf_xy_max_;
  double esdf_resolution_;

  message_filters::Subscriber<ImageMsg> depth_sub_;
  message_filters::Subscriber<ImageMsg> color_sub_;
  std::shared_ptr<ApproxSynchronizer> sync_;

  rclcpp::Subscription<CameraInfoMsg>::SharedPtr camera_info_sub_raw_;
  CameraInfoMsg::SharedPtr latest_camera_info_;
  std::mutex camera_info_mutex_;

  std::shared_ptr<tf2_ros::Buffer> tf_buffer_;
  std::shared_ptr<tf2_ros::TransformListener> tf_listener_;

  std::shared_ptr<nvblox::Mapper> mapper_;
  mutable std::mutex mapper_mutex_;

  std::unique_ptr<MeshPublisher> mesh_publisher_;
  std::unique_ptr<EsdfPublisher> esdf_publisher_;
  rclcpp::TimerBase::SharedPtr publish_timer_;

  size_t integrated_frame_count_{0};
};

}  // namespace my_nvblox