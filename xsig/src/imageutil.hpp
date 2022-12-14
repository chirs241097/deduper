//Chris Xiong 2022
//License: MPL-2.0
#ifndef IMAGEUTIL_HPP
#define IMAGEUTIL_HPP

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <vector>
#include <opencv2/core.hpp>

#include "compressed_vector.hpp"

#define sqr(x) ((x) * (x))

namespace image_util
{
    cv::Mat crop(cv::InputArray s, double contrast_threshold, double max_crop_ratio);
    cv::Range crop_axis(cv::InputArray s, int axis, double contrast_threshold, double max_crop_ratio);
    double median(std::vector<double> &v);
    cv::Mat blend_white(cv::Mat m);
    cv::Mat imread_path(const std::filesystem::path &p, int flags);

    template<class T, int B>
    static double length(const compressed_vector<T, B> &v, T center)
    {
        double ret = 0;
        for (size_t i = 0; i < v.size(); ++i)
        {
            ret += sqr(1. * v.get(i) - center);
        }
        return sqrt(ret);
    }
    template<class T, int B>
    static double distance(const compressed_vector<T, B> &v1, const compressed_vector<T, B> &v2)
    {
        //assert(v1.size() == v2.size())
        double ret = 0;
        for (size_t i = 0; i < v1.size(); ++i)
        {
            if (abs((int)v1.get(i) - (int)v2.get(i)) == 2 && (v1.get(i) == 2 || v2.get(i) == 2))
            ret += 9;
            else
            ret += sqr(1. * v1.get(i) - v2.get(i));
        }
        return sqrt(ret);
    }
    static double length(const std::vector<uint8_t> &v, uint8_t center)
    {
        double ret = 0;
        for (size_t i = 0; i < v.size(); ++i)
        {
            ret += sqr(1. * v[i] - center);
        }
        return sqrt(ret);
    }
    static double distance(const std::vector<uint8_t> &v1, const std::vector<uint8_t> &v2)
    {
        //assert(v1.size() == v2.size())
        double ret = 0;
        for (size_t i = 0; i < v1.size(); ++i)
        {
            if (abs((int)v1[i] - (int)v2[i]) == 2 && (v1[i] == 2 || v2[i] == 2))
            ret += 9;
            else
            ret += sqr(1. * v1[i] - v2[i]);
        }
        return sqrt(ret);
    }
};

#endif
