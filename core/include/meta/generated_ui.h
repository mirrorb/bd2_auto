#pragma once

#include <opencv2/core/types.hpp>

namespace UILayouts {


// 包含固定位置信息的Layout元数据
struct Metadata {
    const char* name;
    const char* filename;
    cv::Rect location;
};


} // namespace UILayouts

namespace UITemplates {


// 仅用于模板搜索的Tempalte元数据
struct Metadata {
    const char* name;
    const char* filename;
};


} // namespace UITemplates
