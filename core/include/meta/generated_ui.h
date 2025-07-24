#pragma once

#include <opencv2/core/types.hpp>

namespace UILayouts {


// 包含固定位置信息的Layout元数据
struct Metadata {
    const char* name;
    const char* filename;
    cv::Rect location;
};


// DING_YUE
inline const Metadata UI_DING_YUE = {
    "DING_YUE",
    "DING_YUE.png",
    cv::Rect(18, 201, 142, 34)
};

// RI_ZHI
inline const Metadata UI_RI_ZHI = {
    "RI_ZHI",
    "RI_ZHI.png",
    cv::Rect(12, 368, 177, 38)
};

} // namespace UILayouts

namespace UITemplates {


// 仅用于模板搜索的Tempalte元数据
struct Metadata {
    const char* name;
    const char* filename;
};


// MERGE_BUTTOM
inline const Metadata UI_MERGE_BUTTOM = {
    "MERGE_BUTTOM",
    "MERGE_BUTTOM.png"
};

} // namespace UITemplates
