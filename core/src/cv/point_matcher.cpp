#include "cv/point_matcher.h"
#include <random>

#define SHOW_LINES
#define SHOW_POINTS

PointMatcher::PointMatcher(double ransac_reproj_thresh, float match_distance_multiplier, float cluster_radius_factor): 
    ransac_reproj_thresh(ransac_reproj_thresh),
    match_distance_multiplier(match_distance_multiplier),
    cluster_radius_factor(cluster_radius_factor) {}

std::vector<cv::Point2f> PointMatcher::get_points(const cv::Mat &scene_image, const cv::Mat &object_image) {
    std::vector<cv::Point2f> representative_points_all_clusters;
    cv::Mat img_object_gray, img_scene_gray;
    cv::cvtColor(object_image, img_object_gray, cv::COLOR_BGR2GRAY);
    cv::cvtColor(scene_image, img_scene_gray, cv::COLOR_BGR2GRAY);

    // 初始化特征检测器和描述子提取器
    cv::Ptr<cv::Feature2D> detector_descriptor = cv::SIFT::create();
    if (!detector_descriptor) {
        std::cerr << "错误: 无法创建特征检测器!" << std::endl;
        return representative_points_all_clusters;
    }

    // 检测关键点并计算描述子
    std::vector<cv::KeyPoint> keypoints_object, keypoints_scene;
    cv::Mat descriptors_object, descriptors_scene;
    detector_descriptor->detectAndCompute(img_object_gray, cv::noArray(), keypoints_object, descriptors_object);
    detector_descriptor->detectAndCompute(img_scene_gray, cv::noArray(), keypoints_scene, descriptors_scene);

    // 模板图中未找到特征点
    if (keypoints_object.empty() || descriptors_object.empty()) {
        return representative_points_all_clusters;
    }

    // 场景图中未找到特征点
    if (keypoints_scene.empty() || descriptors_scene.empty()) {
        return representative_points_all_clusters;
    }

    // 使用 FLANNBASED 匹配器
    cv::Ptr<cv::DescriptorMatcher> matcher;
    matcher = cv::DescriptorMatcher::create(cv::DescriptorMatcher::FLANNBASED);
    if(!matcher){
        std::cerr << "错误: 无法创建匹配器!" << std::endl;
        return representative_points_all_clusters;
    }

    std::vector<cv::DMatch> k1_matches;
    // 描述子为空
    if (descriptors_object.empty() || descriptors_scene.empty()) {
         return representative_points_all_clusters;
    }
    // 匹配描述子
    try {
        matcher->match(descriptors_object, descriptors_scene, k1_matches);
    } catch (const cv::Exception& e) {
        std::cerr << "匹配时发生OpenCV异常: " << e.what() << std::endl;
        if (descriptors_object.type() != descriptors_scene.type()) {
            std::cerr << "  注意: 对象描述子类型 (" << descriptors_object.type() 
                      << ") 与场景描述子类型 (" << descriptors_scene.type() 
                      << ") 不匹配。" << std::endl;
        }
        return representative_points_all_clusters;
    }

    // 计算最小和最大距离
    double min_dist = std::numeric_limits<double>::max();
    double max_dist = 0;
    for(const auto& match : k1_matches) {
        double dist = match.distance;
        if(dist < min_dist) min_dist = dist;
        if(dist > max_dist) max_dist = dist;
    }

    // 只保留好的匹配
    std::vector<cv::DMatch> good_matches;
    for(const auto& match : k1_matches) {
        if(match.distance <= std::max(this->match_distance_multiplier * min_dist, static_cast<double>(this->match_distance_absolute))) {
            good_matches.push_back(match);
        }
    }

    // 使用good_matches替代后续的k1_matches
    k1_matches = good_matches;
    // 绘制匹配连线
#ifdef SHOW_LINES
    cv::Mat img_matches;
    cv::drawMatches(object_image, keypoints_object, 
                    scene_image, keypoints_scene,
                    good_matches, img_matches, 
                    cv::Scalar::all(-1),    // 匹配线颜色，-1表示随机颜色
                    cv::Scalar::all(-1),    // 单点颜色
                    std::vector<char>(),    // mask
                    cv::DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS |  // 不绘制单个点
                    cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS);     // 绘制关键点的大小和方向
    
    cv::imshow("Keypoint Matches", img_matches);
    cv::waitKey(1);  // 显示1ms，不阻塞
#endif

    // 聚类匹配点并为每个聚类寻找对象
    int detected_objects_count = 0; 
    int processed_clusters_count = 0; 

    float template_diag = std::sqrt(static_cast<float>(img_object_gray.cols * img_object_gray.cols + img_object_gray.rows * img_object_gray.rows));
    const float cluster_distance_threshold = template_diag * this->cluster_radius_factor;

    std::vector<bool> visited_matches(k1_matches.size(), false);

    for (size_t i = 0; i < k1_matches.size(); ++i) {
        if (visited_matches[i]) {
            continue;
        }
        processed_clusters_count++; 

        std::vector<cv::DMatch> current_cluster_matches;
        std::vector<cv::Point2f> current_cluster_scene_points; 
        std::vector<size_t> q;
        q.push_back(i);
        visited_matches[i] = true;
        
        int head = 0;

        // BFS聚类算法
        while(head < q.size()) {
            size_t current_match_idx_val = q[head++];
            current_cluster_matches.push_back(k1_matches[current_match_idx_val]);
            // 检查匹配点的 trainIdx 是否超出场景关键点范围
            if (k1_matches[current_match_idx_val].trainIdx >= static_cast<int>(keypoints_scene.size())) {
                continue;
            }
            current_cluster_scene_points.push_back(keypoints_scene[k1_matches[current_match_idx_val].trainIdx].pt);

            cv::Point2f pt_current_scene = keypoints_scene[k1_matches[current_match_idx_val].trainIdx].pt;

            for (size_t j = 0; j < k1_matches.size(); ++j) {
                if (!visited_matches[j]) {
                    // 检查匹配点的 trainIdx 是否超出场景关键点范围
                    if (k1_matches[j].trainIdx >= static_cast<int>(keypoints_scene.size())) {
                        continue; 
                    }
                    cv::Point2f pt_other_scene = keypoints_scene[k1_matches[j].trainIdx].pt;
                    if (point_distance(pt_current_scene, pt_other_scene) < cluster_distance_threshold) {
                        visited_matches[j] = true;
                        q.push_back(j);
                    }
                }
            }
        }

        if (current_cluster_matches.size() >= this->min_match_count_per_instance) {
            std::vector<cv::Point2f> obj_pts_cluster;
            std::vector<cv::Point2f> scene_pts_cluster_for_homography; 

            for (const auto& match : current_cluster_matches) {
                // 匹配点的 queryIdx 或 trainIdx 超出范围
                if (match.queryIdx >= static_cast<int>(keypoints_object.size()) ||
                    match.trainIdx >= static_cast<int>(keypoints_scene.size())) {
                    continue;
                }
                obj_pts_cluster.push_back(keypoints_object[match.queryIdx].pt);
                scene_pts_cluster_for_homography.push_back(keypoints_scene[match.trainIdx].pt);
            }
            
            // 聚类的匹配点不足
            if (obj_pts_cluster.size() < this->min_match_count_per_instance) {
                if (!current_cluster_scene_points.empty()) { 
                    cv::Point2f representative_point = get_representative_point_from_cloud(current_cluster_scene_points, true);
                     if (representative_point.x >=0 && representative_point.y >=0) {
                        representative_points_all_clusters.push_back(representative_point);
                    }
                }
                continue;
            }

            // 匹配点够的计算单应性矩阵
            cv::Mat H = cv::findHomography(obj_pts_cluster, scene_pts_cluster_for_homography, cv::RANSAC, this->ransac_reproj_thresh);
            if (!H.empty() && H.cols == 3 && H.rows == 3 && H.type() == CV_64F) {
                std::vector<cv::Point2f> obj_corners(4);
                obj_corners[0] = cv::Point2f(0, 0);
                obj_corners[1] = cv::Point2f(static_cast<float>(img_object_gray.cols), 0);
                obj_corners[2] = cv::Point2f(static_cast<float>(img_object_gray.cols), static_cast<float>(img_object_gray.rows));
                obj_corners[3] = cv::Point2f(0, static_cast<float>(img_object_gray.rows));
                
                std::vector<cv::Point2f> scene_corners(4);
                cv::perspectiveTransform(obj_corners, scene_corners, H);

                double area = cv::contourArea(scene_corners);
                double template_area = static_cast<double>(img_object_gray.cols) * img_object_gray.rows;

                // 面积检查略微调整：确保template_area为正数后再使用它作为除数
                bool area_ok = false;
                if (template_area > 1e-5) { // 避免除以零或极小的模板面积
                     area_ok = (area > template_area * 0.05 && area < template_area * 200.0);
                } else { // 如果模板面积极小，只需检查检测到的面积是否合理
                     area_ok = (area > 10.0 && area < 500000.0);
                }
                // 面积检查
                if (area_ok && cv::isContourConvex(scene_corners)) { 
                    // 在四边形内部生成一个近似中心点或随机点 (用于成功检测的对象)
                    cv::Point2f representative_point = get_point_in_quad(scene_corners, true);
                     if (representative_point.x >=0 && representative_point.y >=0) {
                        representative_points_all_clusters.push_back(representative_point);
                    }
                    detected_objects_count++;
                } else {
                     if (!current_cluster_scene_points.empty()) {
                        cv::Point2f representative_point = get_representative_point_from_cloud(current_cluster_scene_points, true);
                        if (representative_point.x >=0 && representative_point.y >=0) {
                            representative_points_all_clusters.push_back(representative_point);
                        }
                    }
                }
            } else {
                // 无法计算有效的单应性矩阵
                if (!current_cluster_scene_points.empty()) {
                    cv::Point2f representative_point = get_representative_point_from_cloud(current_cluster_scene_points, true);
                     if (representative_point.x >=0 && representative_point.y >=0) {
                        representative_points_all_clusters.push_back(representative_point);
                    }
                }
            }
        } else { 
            // 聚类的匹配点不足
             if (!current_cluster_scene_points.empty()) { 
                cv::Point2f representative_point = get_representative_point_from_cloud(current_cluster_scene_points, true); 
                 if (representative_point.x >=0 && representative_point.y >=0) { 
                    representative_points_all_clusters.push_back(representative_point);
                }
             }
        }
    }
#ifdef SHOW_POINTS
    cv::Mat img_scene_bgr_copy = scene_image.clone();
    for (const auto& p : representative_points_all_clusters) {
        cv::circle(img_scene_bgr_copy, p, 5, cv::Scalar(0, 0, 255), -1); // Orange
    }
    cv::imshow("Detected Objects in Scene", img_scene_bgr_copy);
    cv::waitKey(0);
#endif
    return representative_points_all_clusters;
}

// 计算距离
float PointMatcher::point_distance(const cv::Point2f& p1, const cv::Point2f& p2) {
    float dx = p1.x - p2.x;
    float dy = p1.y - p2.y;
    return std::sqrt(dx * dx + dy * dy);
}

// 在四边形内部生成一个近似中心点或随机点 (用于成功检测的对象)
cv::Point2f PointMatcher::get_point_in_quad(const std::vector<cv::Point2f>& corners, bool truly_random) {
    if (corners.size() != 4) {
        return cv::Point2f(-1, -1);
    }
    cv::Point2f center(0, 0);
    for (const auto& p : corners) {
        center.x += p.x;
        center.y += p.y;
    }
    center.x /= 4.0f;
    center.y /= 4.0f;

    if (!truly_random) {
        return center;
    }

    // 四边形的原始AABB
    float q_min_x_orig = corners[0].x, q_max_x_orig = corners[0].x;
    float q_min_y_orig = corners[0].y, q_max_y_orig = corners[0].y;
    for (size_t i = 1; i < 4; ++i) {
        q_min_x_orig = std::min(q_min_x_orig, corners[i].x);
        q_max_x_orig = std::max(q_max_x_orig, corners[i].x);
        q_min_y_orig = std::min(q_min_y_orig, corners[i].y);
        q_max_y_orig = std::max(q_max_y_orig, corners[i].y);
    }

    // 若原始AABB退化，则返回质心
    if (q_max_x_orig <= q_min_x_orig + this->point_generation_epsilon || q_max_y_orig <= q_min_y_orig + this->point_generation_epsilon) {
        return center;
    }

    // 定义一个以质心为中心的较小采样AABB
    float extent_x_neg = center.x - q_min_x_orig;
    float extent_x_pos = q_max_x_orig - center.x;
    float extent_y_neg = center.y - q_min_y_orig;
    float extent_y_pos = q_max_y_orig - center.y;

    // 确保各方向范围为非负值（例如，质心在AABB边缘的情况）
    extent_x_neg = std::max(0.0f, extent_x_neg);
    extent_x_pos = std::max(0.0f, extent_x_pos);
    extent_y_neg = std::max(0.0f, extent_y_neg);
    extent_y_pos = std::max(0.0f, extent_y_pos);
    
    float sample_min_x = center.x - extent_x_neg * this->random_point_shrink_factor;
    float sample_max_x = center.x + extent_x_pos * this->random_point_shrink_factor;
    float sample_min_y = center.y - extent_y_neg * this->random_point_shrink_factor;
    float sample_max_y = center.y + extent_y_pos * this->random_point_shrink_factor;

    // 若计算得到的采样AABB退化，选用该质心
    if (sample_max_x <= sample_min_x + this->point_generation_epsilon || sample_max_y <= sample_min_y + this->point_generation_epsilon) {
        return center;
    }

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> distrib_x(sample_min_x, sample_max_x);
    std::uniform_real_distribution<> distrib_y(sample_min_y, sample_max_y);
    
    cv::Point2f random_pt;
    for(int t = 0; t < this->random_point_max_tries; ++t) {
        random_pt.x = static_cast<float>(distrib_x(gen));
        random_pt.y = static_cast<float>(distrib_y(gen));
        if (cv::pointPolygonTest(corners, random_pt, false) >= 0) {
            return random_pt;
        }
    }
    // 若在最大尝试次数内未找到，则返回质心
    return center;
}

// 根据一组点计算一个代表点 (用于匹配不足的聚类)
cv::Point2f PointMatcher::get_representative_point_from_cloud(const std::vector<cv::Point2f>& points, bool truly_random) {
    if (points.empty()) {
        return cv::Point2f(-1, -1);
    }

    cv::Point2f centroid(0, 0);
    for (const auto& p : points) {
        centroid.x += p.x;
        centroid.y += p.y;
    }
    centroid.x /= points.size();
    centroid.y /= points.size();

    if (!truly_random || points.size() < 2) { 
        return centroid;
    }

    // 点云的AABB
    float p_min_x = points[0].x, p_max_x = points[0].x;
    float p_min_y = points[0].y, p_max_y = points[0].y;
    for (size_t i = 1; i < points.size(); ++i) {
        p_min_x = std::min(p_min_x, points[i].x);
        p_max_x = std::max(p_max_x, points[i].x);
        p_min_y = std::min(p_min_y, points[i].y);
        p_max_y = std::max(p_max_y, points[i].y);
    }

    // 若原始AABB退化（所有点实际上是同一个点）
    if (p_max_x <= p_min_x + this->point_generation_epsilon || p_max_y <= p_min_y + this->point_generation_epsilon) {
        return centroid; 
    }

    // 定义一个向质心缩小的采样范围
    float dist_cx_to_min_x = centroid.x - p_min_x;
    float dist_cx_to_max_x = p_max_x - centroid.x;
    float dist_cy_to_min_y = centroid.y - p_min_y;
    float dist_cy_to_max_y = p_max_y - centroid.y;

    // 确保各方向距离为非负值
    dist_cx_to_min_x = std::max(0.0f, dist_cx_to_min_x);
    dist_cx_to_max_x = std::max(0.0f, dist_cx_to_max_x);
    dist_cy_to_min_y = std::max(0.0f, dist_cy_to_min_y);
    dist_cy_to_max_y = std::max(0.0f, dist_cy_to_max_y);

    float sample_min_x = centroid.x - dist_cx_to_min_x * this->random_point_shrink_factor;
    float sample_max_x = centroid.x + dist_cx_to_max_x * this->random_point_shrink_factor;
    float sample_min_y = centroid.y - dist_cy_to_min_y * this->random_point_shrink_factor;
    float sample_max_y = centroid.y + dist_cy_to_max_y * this->random_point_shrink_factor;

    // 处理可能退化的采样范围
    bool range_x_degen = (sample_max_x <= sample_min_x + this->point_generation_epsilon);
    bool range_y_degen = (sample_max_y <= sample_min_y + this->point_generation_epsilon);

    if (range_x_degen) {
        // 若原始X范围非退化且缩放因子不为0，则提供一个极小的范围
        if (p_max_x - p_min_x > this->point_generation_epsilon && this->random_point_shrink_factor > this->point_generation_epsilon) {
             sample_min_x = centroid.x - this->point_generation_epsilon * 10.0f; 
             sample_max_x = centroid.x + this->point_generation_epsilon * 10.0f;
        } else { // 原始X范围退化，或缩放因子为0，则返回质心
             sample_min_x = centroid.x; sample_max_x = centroid.x;
        }
    }
    if (range_y_degen) {
         if (p_max_y - p_min_y > this->point_generation_epsilon && this->random_point_shrink_factor > this->point_generation_epsilon) {
            sample_min_y = centroid.y - this->point_generation_epsilon * 10.0f;
            sample_max_y = centroid.y + this->point_generation_epsilon * 10.0f;
        } else {
            sample_min_y = centroid.y; sample_max_y = centroid.y;
        }
    }
    // 确保min <= max用于分布，以防之前逻辑有问题
    if (sample_min_x > sample_max_x) { sample_min_x = sample_max_x = centroid.x; }
    if (sample_min_y > sample_max_y) { sample_min_y = sample_max_y = centroid.y; }

    std::random_device rd;
    std::mt19937 gen(rd());

    std::uniform_real_distribution<> distrib_x(sample_min_x, sample_max_x);
    std::uniform_real_distribution<> distrib_y(sample_min_y, sample_max_y);
    
    return cv::Point2f(static_cast<float>(distrib_x(gen)), static_cast<float>(distrib_y(gen)));
}
