#include "my_nvblox/esdf_builder.hpp"
#include <cstring>

EsdfBuilder::EsdfBuilder(float voxel_size_m) {
    // 初始化 Mapper：设置体素大小，并指明需要建 TSDF
    mapper_ = std::make_shared<nvblox::Mapper>(
        voxel_size_m, 
        nvblox::MemoryType::kDevice,
        nvblox::ProjectiveLayerType::kTsdf); 
    
    // 设置 ESDF 最大计算距离 (米)
    mapper_->esdf_integrator().max_distance_m(2.0f);
}

void EsdfBuilder::integrateDepthFrame(const float* depth_data, 
                                      int width, int height,
                                      const Eigen::Isometry3f& T_W_C, 
                                      float fx, float fy, float cx, float cy) {
    nvblox::Camera camera(fx, fy, cx, cy, width, height);

    // 将图像从 CPU (Host) 拷贝到 GPU (Device)
    nvblox::DepthImage host_depth_image(width, height, nvblox::MemoryType::kHost);
    std::memcpy(host_depth_image.dataPtr(), depth_data, width * height * sizeof(float));

    nvblox::DepthImage device_depth_image(width, height, nvblox::MemoryType::kDevice);
    device_depth_image.copyFrom(host_depth_image);

    // 融合深度图
    mapper_->integrateDepth(device_depth_image, nvblox::Transform(T_W_C), camera);
}

void EsdfBuilder::updateEsdf() {
    // 生成/更新 ESDF 场
    mapper_->updateEsdf();
}