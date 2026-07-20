#include "my_nvblox/nvblox_node.hpp"

#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <utility>
#include <vector>

#include <opencv2/imgproc.hpp>

#include "geometry_msgs/msg/transform_stamped.hpp"
#include "sensor_msgs/image_encodings.hpp"

namespace my_nvblox
{

namespace
{
inline bool is_bgr8(const std::string & encoding)
{
  return encoding == sensor_msgs::image_encodings::BGR8;
}

inline bool is_rgb8(const std::string & encoding)
{
  return encoding == sensor_msgs::image_encodings::RGB8;
}
}  // namespace

NvbloxNode::NvbloxNode(const rclcpp::NodeOptions & options)
: Node("my_nvblox", options),
  depth_sub_(this, declare_parameter<std::string>("depth_topic", "/camera/aligned_depth_to_color/image_raw")),
  color_sub_(this, declare_parameter<std::string>("color_topic", "/camera/color/image_raw"))
{
  global_frame_ = declare_parameter<std::string>("global_frame", "odom");
  camera_frame_ = declare_parameter<std::string>("camera_frame", "camera_color_optical_frame");
  camera_info_topic_ = declare_parameter<std::string>("camera_info_topic", "/camera/color/camera_info");

  depth_topic_ = get_parameter("depth_topic").as_string();
  color_topic_ = get_parameter("color_topic").as_string();
  camera_info_topic_ = get_parameter("camera_info_topic").as_string();

  voxel_size_ = declare_parameter<double>("voxel_size", 0.05);
  publish_period_ms_ = declare_parameter<int>("publish_period_ms", 500);

  const double esdf_slice_height = declare_parameter<double>("esdf_slice_height", 0.5);
  const double esdf_xy_min = declare_parameter<double>("esdf_xy_min", -5.0);
  const double esdf_xy_max = declare_parameter<double>("esdf_xy_max", 5.0);
  const double esdf_resolution = declare_parameter<double>("esdf_resolution", 0.1);

  tf_buffer_ = std::make_shared<tf2_ros::Buffer>(this->get_clock());
  tf_listener_ = std::make_shared<tf2_ros::TransformListener>(*tf_buffer_);

  mapper_ = std::make_shared<nvblox::Mapper>(
    static_cast<float>(voxel_size_),
    nvblox::BlockMemoryPoolParams(),
    nvblox::ProjectiveLayerType::kTsdf);

  mapper_->esdf_integrator().max_esdf_distance_m(2.0f);

  mesh_publisher_ = std::make_unique<MeshPublisher>(this, "~/mesh", global_frame_);
  esdf_publisher_ = std::make_unique<EsdfPublisher>(
    this, "~/static_esdf_pointcloud", global_frame_,
    static_cast<float>(esdf_slice_height),
    static_cast<float>(esdf_xy_min),
    static_cast<float>(esdf_xy_max),
    static_cast<float>(esdf_resolution));

  camera_info_sub_raw_ = this->create_subscription<CameraInfoMsg>(
    camera_info_topic_,
    rclcpp::SensorDataQoS(),
    std::bind(&NvbloxNode::camera_info_callback, this, std::placeholders::_1));

  sync_ = std::make_shared<ApproxSynchronizer>(
    ApproxSyncPolicy(20), depth_sub_, color_sub_);
  sync_->registerCallback(
    std::bind(&NvbloxNode::synced_callback, this,
    std::placeholders::_1, std::placeholders::_2));

  publish_timer_ = this->create_wall_timer(
    std::chrono::milliseconds(publish_period_ms_),
    std::bind(&NvbloxNode::publish_timer_callback, this));

  RCLCPP_INFO(get_logger(), "my_nvblox node initialized.");
  RCLCPP_INFO(get_logger(), "depth_topic      : %s", depth_topic_.c_str());
  RCLCPP_INFO(get_logger(), "color_topic      : %s", color_topic_.c_str());
  RCLCPP_INFO(get_logger(), "camera_info_topic: %s", camera_info_topic_.c_str());
  RCLCPP_INFO(get_logger(), "global_frame     : %s", global_frame_.c_str());
  RCLCPP_INFO(get_logger(), "camera_frame     : %s", camera_frame_.c_str());
  RCLCPP_INFO(get_logger(), "voxel_size       : %.3f", voxel_size_);
}

void NvbloxNode::camera_info_callback(const CameraInfoMsg::SharedPtr msg)
{
  std::scoped_lock<std::mutex> lock(camera_info_mutex_);
  latest_camera_info_ = msg;
}

void NvbloxNode::synced_callback(
  const ImageMsg::ConstSharedPtr & depth_msg,
  const ImageMsg::ConstSharedPtr & color_msg)
{
  CameraInfoMsg::SharedPtr camera_info_msg;
  {
    std::scoped_lock<std::mutex> lock(camera_info_mutex_);
    camera_info_msg = latest_camera_info_;
  }

  if (!camera_info_msg) {
    RCLCPP_WARN_THROTTLE(
      get_logger(), *get_clock(), 2000,
      "No camera_info received yet.");
    return;
  }

  try {
    const std::string source_frame =
      !color_msg->header.frame_id.empty() ? color_msg->header.frame_id : camera_frame_;

    const auto T_L_C = lookup_pose(
      global_frame_,
      source_frame,
      color_msg->header.stamp);

    const nvblox::Camera camera = make_camera_from_info(*camera_info_msg);

    cv::Mat depth_mat;
    if (depth_msg->encoding == sensor_msgs::image_encodings::TYPE_32FC1) {
      depth_mat = cv_bridge::toCvShare(
        depth_msg, sensor_msgs::image_encodings::TYPE_32FC1)->image;
    } else if (depth_msg->encoding == sensor_msgs::image_encodings::TYPE_16UC1) {
      auto tmp = cv_bridge::toCvCopy(
        depth_msg, sensor_msgs::image_encodings::TYPE_16UC1);
      tmp->image.convertTo(depth_mat, CV_32FC1, 1e-3);
    } else {
      RCLCPP_WARN_THROTTLE(
        get_logger(), *get_clock(), 2000,
        "Unsupported depth encoding: %s", depth_msg->encoding.c_str());
      return;
    }

    cv::Mat color_bgr_mat;
    if (is_bgr8(color_msg->encoding)) {
      color_bgr_mat = cv_bridge::toCvShare(
        color_msg, sensor_msgs::image_encodings::BGR8)->image;
    } else if (is_rgb8(color_msg->encoding)) {
      const auto rgb_cv = cv_bridge::toCvShare(
        color_msg, sensor_msgs::image_encodings::RGB8);
      cv::cvtColor(rgb_cv->image, color_bgr_mat, cv::COLOR_RGB2BGR);
    } else {
      color_bgr_mat = cv_bridge::toCvCopy(
        color_msg, sensor_msgs::image_encodings::BGR8)->image;
    }

    if (depth_mat.rows != color_bgr_mat.rows || depth_mat.cols != color_bgr_mat.cols) {
      RCLCPP_WARN_THROTTLE(
        get_logger(), *get_clock(), 2000,
        "Depth and color size mismatch: depth=%dx%d color=%dx%d",
        depth_mat.cols, depth_mat.rows,
        color_bgr_mat.cols, color_bgr_mat.rows);
      return;
    }

    RCLCPP_INFO_THROTTLE(
      get_logger(), *get_clock(), 3000,
      "synced callback ok: depth=%dx%d (%s), color=%dx%d (%s)",
      depth_msg->width, depth_msg->height, depth_msg->encoding.c_str(),
      color_msg->width, color_msg->height, color_msg->encoding.c_str());

    std::scoped_lock<std::mutex> lock(mapper_mutex_);
    integrate_depth(depth_mat, T_L_C, camera);
    integrate_color(color_bgr_mat, T_L_C, camera);

    ++integrated_frame_count_;
    RCLCPP_INFO_THROTTLE(
      get_logger(), *get_clock(), 5000,
      "Integrated %zu frames successfully.", integrated_frame_count_);

  } catch (const std::exception & e) {
    RCLCPP_WARN_THROTTLE(
      get_logger(), *get_clock(), 1000,
      "Integration skipped: %s", e.what());
  }
}

void NvbloxNode::publish_timer_callback()
{
  std::scoped_lock<std::mutex> lock(mapper_mutex_);

  mapper_->updateColorMesh(nvblox::UpdateFullLayer::kNo);
  mapper_->updateEsdfSlice(nvblox::UpdateFullLayer::kNo);

  mesh_publisher_->publish(*mapper_);
  esdf_publisher_->publish(*mapper_);
}

nvblox::Camera NvbloxNode::make_camera_from_info(const CameraInfoMsg & msg) const
{
  return nvblox::Camera(
    static_cast<float>(msg.k[0]),
    static_cast<float>(msg.k[4]),
    static_cast<float>(msg.k[2]),
    static_cast<float>(msg.k[5]),
    static_cast<int>(msg.width),
    static_cast<int>(msg.height));
}

nvblox::Transform NvbloxNode::lookup_pose(
  const std::string & target_frame,
  const std::string & source_frame,
  const rclcpp::Time & stamp) const
{
  const auto tf = tf_buffer_->lookupTransform(
    target_frame, source_frame, stamp, rclcpp::Duration::from_seconds(0.05));
  const Eigen::Isometry3d T = tf2::transformToEigen(tf);
  return T.cast<float>();
}

void NvbloxNode::integrate_depth(
  const cv::Mat & depth_mat,
  const nvblox::Transform & T_L_C,
  const nvblox::Camera & camera)
{
  const int rows = depth_mat.rows;
  const int cols = depth_mat.cols;

  nvblox::DepthImage depth_device(rows, cols, nvblox::MemoryType::kDevice);

  check_cuda(
    cudaMemcpy(
      depth_device.dataPtr(),
      depth_mat.ptr<float>(),
      static_cast<size_t>(rows * cols) * sizeof(float),
      cudaMemcpyHostToDevice),
    "cudaMemcpy(depth_host_to_device)");

  mapper_->integrateDepth(depth_device, T_L_C, camera);
}

void NvbloxNode::integrate_color(
  const cv::Mat & color_bgr_mat,
  const nvblox::Transform & T_L_C,
  const nvblox::Camera & camera)
{
  const int rows = color_bgr_mat.rows;
  const int cols = color_bgr_mat.cols;

  std::vector<nvblox::Color> host_colors;
  host_colors.resize(static_cast<size_t>(rows * cols));

  for (int v = 0; v < rows; ++v) {
    const auto * row_ptr = color_bgr_mat.ptr<cv::Vec3b>(v);
    for (int u = 0; u < cols; ++u) {
      const auto & px = row_ptr[u];
      host_colors[static_cast<size_t>(v * cols + u)] =
        nvblox::Color(px[2], px[1], px[0]);
    }
  }

  nvblox::ColorImage color_device(rows, cols, nvblox::MemoryType::kDevice);

  check_cuda(
    cudaMemcpy(
      color_device.dataPtr(),
      host_colors.data(),
      host_colors.size() * sizeof(nvblox::Color),
      cudaMemcpyHostToDevice),
    "cudaMemcpy(color_host_to_device)");

  mapper_->integrateColor(color_device, T_L_C, camera);
}

void NvbloxNode::check_cuda(cudaError_t code, const char * expr)
{
  if (code != cudaSuccess) {
    throw std::runtime_error(
      std::string(expr) + " failed: " + cudaGetErrorString(code));
  }
}

}  // namespace my_nvblox

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<my_nvblox::NvbloxNode>());
  rclcpp::shutdown();
  return 0;
}