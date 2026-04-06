#ifndef MY_NVBLOX_ESDF_BUILDER_HPP_
#define MY_NVBLOX_ESDF_BUILDER_HPP_

#include <Eigen/Geometry>
#include <nvblox/nvblox.h>
#include <memory>

class EsdfBuilder {
public:
    explicit EsdfBuilder(float voxel_size_m);
    ~EsdfBuilder() = default;

    // 核心接口：传入深度图数据、位姿、内参进行建图
    void integrateDepthFrame(const float* depth_data, 
                             int width, int height,
                             const Eigen::Isometry3f& T_W_C, 
                             float fx, float fy, float cx, float cy);

    void updateEsdf();

private:
    std::shared_ptr<nvblox::Mapper> mapper_;
};

#endif
