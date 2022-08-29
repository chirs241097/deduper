#include <algorithm>
#include <cmath>
#include <fstream>
#include <iterator>
#include <limits>
//#include <cstdio>

#include <opencv2/imgcodecs.hpp>

#include "imageutil.hpp"

//libpuzzle uses a contrast-based cropping, and clamps the cropped area to a given percentage.
cv::Range image_util::crop_axis(cv::InputArray s, int axis, double contrast_threshold, double max_crop_ratio)
{
    //axis: 0=x (returns range of columns), 1=y (returns range of rows)
    //input matrix must be continuous
    cv::Mat m = s.getMat();
    cv::Size sz = m.size();
    if (axis == 0)
        sz = cv::Size(m.rows, m.cols);
    int innerstride = axis == 0 ? m.cols : 1;
    int outerstride = axis == 0 ? 1 - m.cols * m.rows : 0;
    std::vector<double> contrs;
    const float *data = m.ptr<float>(0);
    const float *dp = data;
    double total_contr = 0.;
    for (int i = 0; i < sz.height; ++i)
    {
        double accum = 0.;
        float lastv = *data;
        for (int j = 0 ; j < sz.width; ++j)
        {
            data += innerstride;
            //printf("%d %d\n", (data - dp) / m.cols, (data - dp) % m.cols);
            if (data - dp >= sz.height * sz.width)
                break;
            accum += fabsf(*data - lastv);
            lastv = *data;
        }
        //printf("---\n");
        data += outerstride;
        contrs.push_back(accum);
        total_contr += accum;
    }
    //printf("======\n");
    //for (size_t i = 0; i < contrs.size(); ++i) printf("%.4f ",contrs[i]/total_contr);
    //printf("\n%f====\n",total_contr);
    double realth = total_contr * contrast_threshold;
    int l = 0, r = sz.height - 1;
    total_contr = 0;
    for (; l < sz.height; ++l)
    {
        total_contr += contrs[l];
        if (total_contr >= realth) break;
    }
    total_contr = 0;
    for (; r > 0; --r)
    {
        total_contr += contrs[r];
        if (total_contr >= realth) break;
    }
    int crop_max = (int)round(sz.height * max_crop_ratio);
    return cv::Range(std::min(l, crop_max), std::max(r, sz.height - 1 - crop_max) + 1);
}

cv::Mat image_util::crop(cv::InputArray s, double contrast_threshold, double max_crop_ratio)
{
    //input matrix must be continuous
    cv::Range xr = crop_axis(s, 0, contrast_threshold, max_crop_ratio);
    cv::Range yr = crop_axis(s, 1, contrast_threshold, max_crop_ratio);
    //printf("%d,%d %d,%d\n",yr.start,yr.end,xr.start,xr.end);
    return s.getMat()(yr, xr);
}

double image_util::median(std::vector<double> &v)
{
    if (v.empty())
        return std::numeric_limits<double>::quiet_NaN();
    if (v.size() % 2)
    {
        int m = v.size() / 2;
        std::vector<double>::iterator mt = v.begin() + m;
        std::nth_element(v.begin(), mt, v.end());
        return *mt;
    }
    else
    {
        int m = v.size() / 2;
        int n = m - 1;
        std::vector<double>::iterator mt, nt;
        mt = v.begin() + m;
        nt = v.begin() + n;
        std::nth_element(v.begin(), mt, v.end());
        std::nth_element(v.begin(), nt, v.end());
        return (*mt + *nt) / 2.;
    }
}

cv::Mat image_util::blend_white(cv::Mat m)
{
    //input must be a continuous, CV_32FC4 matrix
    cv::Mat ret;
    ret.create(m.size(), CV_32FC3);
    size_t p = m.size().width * m.size().height;
    float *d = m.ptr<float>(0);
    float *o = ret.ptr<float>(0);
    for (size_t i = 0; i < p; ++i)
    {
        float a = d[3];
        o[0] = d[0] * a + (1. - a);
        o[1] = d[1] * a + (1. - a);
        o[2] = d[2] * a + (1. - a);
        d += 4;
        o += 3;
    }
    return ret;
}

cv::Mat image_util::imread_path(const std::filesystem::path &p, int flags)
{
    auto size = std::filesystem::file_size(p);
    std::fstream fst(p, std::ios::binary | std::ios::in);
    std::vector<char> dat;
    dat.resize(size);
    fst.read(dat.data(), size);
    fst.close();
    cv::Mat img = cv::imdecode(dat, flags);
    return img;
}
