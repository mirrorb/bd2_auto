#pragma once

#include <opencv2/core/mat.hpp>
#include <opencv2/opencv.hpp>

class PointMatcher {
public:
    PointMatcher(double ransac_reproj_thresh = 5.0, float match_distance_multiplier = 0.5f, float cluster_radius_factor = 1.5f);
    ~PointMatcher() = default;

    std::vector<cv::Point2f> get_points(const cv::Mat &scene_image, const cv::Mat &object_image);

private:
    // 重投影误差阈值。值越小匹配越严格，值越大容忍度越高
    // 建议范围：3.0-8.0，默认5.0。图像失真大时可适当增大
    double ransac_reproj_thresh;

    // 匹配距离倍数阈值。值越大保留的匹配点越多，但可能增加误匹配
    // 建议范围：0.1-2.0，默认0.5。图像质量差时可增加到2.0，质量好时可减小到0.1
    float match_distance_multiplier;

    // 聚类半径因子。值越大聚类范围越大，可能导致误聚类；值越小可能导致过度分散
    // 建议范围：0.8-2.0，默认1.5。目标间距较近时建议减小到1.2左右
    float cluster_radius_factor;

    // Homography至少需要4个点。增大此值可提高匹配可靠性但可能降低检出率
    // 建议范围：4-8，默认4。图像质量好时可设为6
    int min_match_count_per_instance = 4;

    // 采样区域收缩因子（0.0：仅质心；1.0：从质心到AABB边界的完整范围）
    // 建议范围：0.3-0.8，默认0.6。若需要更精确的点击位置，可减小到0.4
    float random_point_shrink_factor = 0.6f;

    // 小epsilon用于浮点比较。一般无需调整
    // 建议范围：1e-6 - 1e-4，除非出现数值精度问题，否则保持默认
    float point_generation_epsilon = 1e-5f;

    // 最大尝试次数。增大此值会增加随机点生成的计算时间，但提高成功率
    // 建议范围：10-50，默认20。若经常生成失败可增加到30
    int random_point_max_tries = 20;

    // 匹配距离绝对阈值。作为保底阈值，防止在最小距离特别小时过度筛选
    // 建议范围：0.02-0.05，默认0.05
    float match_distance_absolute = 0.05f;

    // 计算两点之间的距离
    float point_distance(const cv::Point2f& p1, const cv::Point2f& p2);
    // 在四边形内部生成一个近似中心点或随机点 (用于成功检测的对象)
    cv::Point2f get_point_in_quad(const std::vector<cv::Point2f>& corners, bool truly_random = false);
    // 根据一组点计算一个代表点 (用于匹配不足的聚类)
    cv::Point2f get_representative_point_from_cloud(const std::vector<cv::Point2f>& points, bool truly_random = false);

};
